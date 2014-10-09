/*
TARGET:
To generate chromium configuration and warp mapping(vertex-texture mapping) for multiprojector seamless alignment.

STEPS:
1. Project feature image for each projector.
2. Capture & detect features.(compute camera to projector mapping)
3. Compute global bounding box in camera.(encompassing all projector bounding boxes)
4. Compute local bounding boxes for each projector.
5. Select maximum width and height across all bounding boxes.
6. Normalize coordinates of all features in global bounding box.
7. Compute scale factor for generating chromium configuration coordinates for all projectors.
8. Generate chromium configuration for all projectors.
9. Generate vertex & texture coordinates as inputs to chromium. Texture coordinates:normalized coordinates of original feature points. Vertex coordinates: normalized(-1,1) position of detected features in local bounding boxes(captured in camera).
10. Render
*/


/// Headers
#include"interface.h"
#include"dependencies.h"

#define checkerboard_square_pixels 32
#define checkerboard_X 29
#define checkerboard_Y  21
#define total_features_X (checkerboard_X)
#define total_features_Y (checkerboard_Y)
#define num_features_per_projector (total_features_X*total_features_Y) // checkerboard_X*checkerboard_Y inner detected points, 4 endpoints, 2*(checkerboard_X+checkerboard_Y) points on edges of projection quads

#define max_num_of_projectors 9
#define max_num_of_projectors_in_row 3
#define max_num_of_projectors_in_column 3

#define camera_imagewidth 3648
#define camera_imageheight 2736

#define projector_imagewidth 1024
#define projector_imageheight 768

#define starting_proj_ID 0
#define screen_size_X 2383
#define screen_size_Y 1765

#define round(x) (((x-floor(x))>=0.5f)?(unsigned int)(x+1):(unsigned int)(x))


///Global variables
extern Camera *camera;
extern GPContext *context;
extern CameraWidget *widget; //used for setting camera constrols
extern CameraFile* img_file;
extern int ret;
extern int cam_shutter_speed;
extern int cam_aperture;


float feature_location_camera_space[max_num_of_projectors][num_features_per_projector][2]; //stores detected locations of all features across all projectors
float feature_location_proj_space[num_features_per_projector][2];
float tile_coordinates[max_num_of_projectors][num_features_per_projector][2];

float local_bounding_box[max_num_of_projectors][4]; //0:X, 1:Y, 2:width, 3:height
float global_bounding_box[4];
float normalized_local_bounding_boxes[max_num_of_projectors][4];


float scale_x;
float scale_y;


char filename[500];

unsigned num_active_proj_total;
bool active_projectors[max_num_of_projectors]; // active_projector[i]=true iff ith projector is used.
unsigned effective_proj_id[max_num_of_projectors];



unsigned num_of_projectors_in_row; // number of 'active' projectors along row
unsigned num_of_projectors_in_column; // number of 'active' projectors along column

IplImage*debug_image;


CvPoint2D32f endpoints[4];

IplImage*undist_preview_image,*gray_preview_image;


float vertical_lines[max_num_of_projectors][checkerboard_X][4]; /// arrays holding definition of vertical/horizontal fitted lines(on inner detected corners)
float horizontal_lines[max_num_of_projectors][checkerboard_Y][4];

bool corners_detected=false;
CvPoint2D32f*detected_points=new CvPoint2D32f[4];
static int point_number=0;
CvMat *cshomo; // camera to screen homography

CvMat* cam_intrinsic_mat;
CvMat* cam_dist_vect;



//the only quantity that varies depending on whether computation is in camera or screen
unsigned space_size_X,space_size_Y;
unsigned registration_on_screen;




void callback_get_point(int event,int x,int y,int flags,void* param)
{

    switch(event)
    {
    case CV_EVENT_LBUTTONDOWN:
        detected_points[point_number].x=(float)x;
        detected_points[point_number].y=(float)y;

        break;


    case CV_EVENT_LBUTTONUP:
        ///Do sub-pixel corner detection & draw the detected corner
        cvFindCornerSubPix(gray_preview_image,&detected_points[point_number],1,cvSize(5,5),cvSize(-1,-1),cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,30,0.01));
        cvCircle(undist_preview_image,cvPointFrom32f(detected_points[point_number]),5,cvScalar(255,0,0));
        point_number++;
        if(point_number==4)
            corners_detected=true;
    }





    return;
}


/// Adding code for screen-to-camera homography(to make output perspective-less on screen)
void compute_c_s_homography()
{

    CvFileStorage*fs_camera_intrinsic_matrix=cvOpenFileStorage("Camera_calibration/cam_intrinsic_mat.xml",0,CV_STORAGE_READ);
    cam_intrinsic_mat=(CvMat*)cvReadByName(fs_camera_intrinsic_matrix,0,"cam_intrinsic_mat");
    CvFileStorage*fs_camera_distotion_vect=cvOpenFileStorage("Camera_calibration/cam_distortion_vect.xml",0,CV_STORAGE_READ);
    cam_dist_vect=(CvMat*)cvReadByName(fs_camera_distotion_vect,0,"cam_distortion_vect");



    cvNamedWindow("Click on 4 points:",0);
    cvSetMouseCallback("Click on 4 points:",callback_get_point,NULL);




    printf("\nCapture wall position?");
    fflush(stdout);
    bool alignment_done=false;
    IplImage*preview_image;


    while(alignment_done==false)
    {

        preview_image=camera_preview();
        cvShowImage("Camera_view",preview_image);

        if(cvWaitKey(10)=='y')
            alignment_done=true;
    }


    printf("\nPlease click on 4 points");
    fflush(stdout);

    preview_image=camera_capture("junk.jpg");

    undist_preview_image=cvCreateImage(cvSize(camera_imagewidth,camera_imageheight),IPL_DEPTH_8U,3);
    cvUndistort2(preview_image,undist_preview_image,cam_intrinsic_mat,cam_dist_vect);

    gray_preview_image=cvCreateImage(cvGetSize(preview_image),preview_image->depth,1);

    cvCvtColor(undist_preview_image,gray_preview_image,CV_RGB2GRAY);

    corners_detected=false;

    while(1)
    {

        cvWaitKey(30);
        cvShowImage("Click on 4 points:",undist_preview_image);
        if(corners_detected==true)
            break;
    }



    //Lets copy the image points.
    CvMat*cam_image_points=cvCreateMat(4,3,CV_32FC1);

    for(int i=0; i<4; i++)
    {
        CV_MAT_ELEM(*cam_image_points,float,i,0)=(float)detected_points[i].x;
        CV_MAT_ELEM(*cam_image_points,float,i,1)=(float)detected_points[i].y;
        CV_MAT_ELEM(*cam_image_points,float,i,2)=1.0f;
    }



    ///lets find screen-to-camera homography...
    //define screen points
    CvMat*screen_points=cvCreateMat(4,3,CV_32FC1);
    for(int i=0; i<4; i++)
    {
        CV_MAT_ELEM(*screen_points,float,i,0)=screen_size_X*(i%2);
        CV_MAT_ELEM(*screen_points,float,i,1)=screen_size_Y*floor(i/2);
        CV_MAT_ELEM(*screen_points,float,i,2)=1.0f;
    }

    /*  CV_MAT_ELEM(*screen_points,float,0,0)=0.0;
      CV_MAT_ELEM(*screen_points,float,1,0)=0.0;
      CV_MAT_ELEM(*screen_points,float,2,0)=1.0;

      CV_MAT_ELEM(*screen_points,float,0,1)=2396.0;//2383.0;
      CV_MAT_ELEM(*screen_points,float,1,1)=0.0;
      CV_MAT_ELEM(*screen_points,float,2,1)=1.0;

      CV_MAT_ELEM(*screen_points,float,0,2)=0.0;
      CV_MAT_ELEM(*screen_points,float,1,2)=1799.0;//1769.0;
      CV_MAT_ELEM(*screen_points,float,2,2)=1.0;

      CV_MAT_ELEM(*screen_points,float,0,3)=2390.0;//2383.0;
      CV_MAT_ELEM(*screen_points,float,1,3)=1797.0;//1765.0;
      CV_MAT_ELEM(*screen_points,float,2,3)=1.0;
    */
    cshomo=cvCreateMat(3,3,CV_32FC1);
    cvFindHomography(cam_image_points,screen_points,cshomo,CV_RANSAC);

    // Lets compute pose of screen wrt. camera
    /*
        CvMat*screen_camera_rot_vect=cvCreateMat(3,1,CV_32FC1);
        CvMat*screen_camera_trans_vect=cvCreateMat(3,1,CV_32FC1);
        CvMat*screen_camera_rot_mat=cvCreateMat(3,3,CV_32FC1);
        cvFindExtrinsicCameraParams2(screen_points,cam_image_points,cam_intrinsic_mat,cam_dist_vect,screen_camera_rot_vect,screen_camera_trans_vect);

        cvRodrigues2(screen_camera_rot_vect,screen_camera_rot_mat);
    */
    cvSave("cshomo0.xml",cshomo);

    //cvSave("cs_rot_mat.xml",screen_camera_rot_mat);
    //cvSave("cs_trans_vect.xml",screen_camera_trans_vect);

    cvSaveImage("cshomo_image.bmp",undist_preview_image);


    return;
}



//Projects & captures features which will be used for finding correspondence between camera & projector.
void project_capture_features(unsigned int proj_ID)
{

    CvPoint2D32f*checkerboard_corners=new CvPoint2D32f[checkerboard_X*checkerboard_Y];
    int *corner_count=new int;

    IplImage*gray_cap=cvCreateImage(cvSize(camera_imagewidth,camera_imageheight),IPL_DEPTH_8U,1);
    CvFileStorage*fs_camera_intrinsic_matrix=cvOpenFileStorage("Camera_calibration/cam_intrinsic_mat.xml",0,CV_STORAGE_READ);
    cam_intrinsic_mat=(CvMat*)cvReadByName(fs_camera_intrinsic_matrix,0,"cam_intrinsic_mat");
    CvFileStorage*fs_camera_distotion_vect=cvOpenFileStorage("Camera_calibration/cam_distortion_vect.xml",0,CV_STORAGE_READ);
    cam_dist_vect=(CvMat*)cvReadByName(fs_camera_distotion_vect,0,"cam_distortion_vect");
    IplImage*undist_cap=cvCreateImage(cvSize(camera_imagewidth,camera_imageheight),IPL_DEPTH_8U,3);
    IplImage*gray_image=cvCreateImage(cvSize(camera_imagewidth,camera_imageheight),IPL_DEPTH_8U,1);
    int found=0;

    /// Adding routine to detect endpoints of projected quads which will be used for recovering full projection regions.
    cvNamedWindow("Camera_view",0);
    printf("\nSelect endpoints for projector:%u?",proj_ID);
    fflush(stdout);

    bool cam_alignment_done=false;
    IplImage*preview_image;


    while(cam_alignment_done==false)
    {
        preview_image=camera_preview();
        cvShowImage("Camera_view",preview_image);

        if(cvWaitKey(10)=='y')
            cam_alignment_done=true;
    }


    preview_image=camera_capture("junk.jpg");

    undist_preview_image=cvCreateImage(cvSize(camera_imagewidth,camera_imageheight),IPL_DEPTH_8U,3);
    cvUndistort2(preview_image,undist_preview_image,cam_intrinsic_mat,cam_dist_vect);

    gray_preview_image=cvCreateImage(cvGetSize(preview_image),preview_image->depth,1);

    cvCvtColor(undist_preview_image,gray_preview_image,CV_RGB2GRAY);




    printf("\nStarting internal corner detection for projector:%u...",proj_ID);
    IplImage*cap;

    while(!found)
    {
        cap=camera_capture("junk.jpg");
        cvUndistort2(cap,undist_cap,cam_intrinsic_mat,cam_dist_vect);
        cvCvtColor(undist_cap,gray_cap,CV_RGB2GRAY);

        found=cvFindChessboardCorners(gray_cap,cvSize(checkerboard_X,checkerboard_Y),checkerboard_corners,corner_count,CV_CALIB_CB_ADAPTIVE_THRESH);

        cvWaitKey(30);
    }

    printf("\nInternal corners for projector:%u detected !!",proj_ID);

    /// Check order of detected corners...
    //its a simple but an ugly fix...
    if(checkerboard_corners[0].y>checkerboard_corners[checkerboard_X].y) // means order of detected corners if mirror of expected corners
    {
        printf("\nFault@proj:%d along height!!!!!\n",proj_ID);
        float temp[2];
        //Reverse the order
        for(unsigned int r=0; r<(checkerboard_Y/2); r++)
            for(unsigned int c=0; c<checkerboard_X; c++)
            {
                temp[0]=checkerboard_corners[r*checkerboard_X+c].x;
                temp[1]=checkerboard_corners[r*checkerboard_X+c].y;

                checkerboard_corners[r*checkerboard_X+c].x=checkerboard_corners[(checkerboard_Y-1-r)*checkerboard_X+c].x;
                checkerboard_corners[r*checkerboard_X+c].y=checkerboard_corners[(checkerboard_Y-1-r)*checkerboard_X+c].y;

                checkerboard_corners[(checkerboard_Y-1-r)*checkerboard_X+c].x=temp[0];
                checkerboard_corners[(checkerboard_Y-1-r)*checkerboard_X+c].y=temp[1];
            }
    }


    if(checkerboard_corners[0].x>checkerboard_corners[checkerboard_X-1].x) // reverse along middle element(along columns)
    {
        printf("\nFault@proj:%d along width!!!!!\n",proj_ID);
        float temp[2];


        for(unsigned int r=0; r<checkerboard_Y; r++)
            for(unsigned int c=0; c<(checkerboard_X/2); c++)
            {
                temp[0]=checkerboard_corners[r*checkerboard_X+c].x;
                temp[1]=checkerboard_corners[r*checkerboard_X+c].y;

                checkerboard_corners[r*checkerboard_X+c].x=checkerboard_corners[r*checkerboard_X+checkerboard_X-1-c].x;
                checkerboard_corners[r*checkerboard_X+c].y=checkerboard_corners[r*checkerboard_X+checkerboard_X-1-c].y;

                checkerboard_corners[r*checkerboard_X+checkerboard_X-1-c].x=temp[0];
                checkerboard_corners[r*checkerboard_X+checkerboard_X-1-c].y=temp[1];
            }


    }

    cvFindCornerSubPix(gray_image,checkerboard_corners,*corner_count,cvSize(5,5),cvSize(-1,-1),cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,30,0.01));
    IplImage* temp_image=cvCreateImage(cvGetSize(undist_cap),IPL_DEPTH_8U,3);
    cvCopyImage(undist_cap,temp_image);
    cvDrawChessboardCorners(temp_image,cvSize(checkerboard_X,checkerboard_Y),checkerboard_corners,*corner_count,found);
    cvShowImage("Camera_view",temp_image);
    sprintf(filename,"detected_corners_%d.bmp",proj_ID);
    cvSaveImage(filename,temp_image);




        if(effective_proj_id[proj_ID]==(num_active_proj_total-1))
        {

            ///Lets capture a frame for debugging(visualizing output of individual steps)
            printf("\nPlease put checkerboard image on all projectors for a shot(debug purpose) & hit a key:");
            fflush(stdout);
            cvWaitKey(0);

            //Lets capture it now!!
            cap=camera_capture("junk.jpg");
            debug_image=cvCreateImage(cvGetSize(cap),IPL_DEPTH_8U,3);

            cvUndistort2(cap,debug_image,cam_intrinsic_mat,cam_dist_vect);

            cvSaveImage("debug_image.bmp",debug_image); //all local/global bounding box(s) will be drawn onto it for debugging.
        }



    // Above C & D are swapped so that loop below can store the endpoints in camera space feature array using generic expression.
    unsigned row1=0,col1=0;

    for(unsigned row=0; row<total_features_Y; row++)
        for(unsigned col=0; col<total_features_X; col++)
        {
           // if(row>1 && row<total_features_Y-2 && col>1 && col<total_features_X-2)
           // {

                feature_location_camera_space[proj_ID-starting_proj_ID][row*total_features_X+col][0]=checkerboard_corners[row1*checkerboard_X+col1].x;
                feature_location_camera_space[proj_ID-starting_proj_ID][row*total_features_X+col][1]=checkerboard_corners[row1*checkerboard_X+col1].y;

                if(col1<(checkerboard_X-1))
                    col1++;
                else //its probably time for going to next row
                {
                    row1++; //go to next row
                    col1=0;
                }
           // }

           // else
            //    continue;
        }


    FILE*fd_detected_corners;
    sprintf(filename,"detected_corners_%u_original_camera_space.txt",proj_ID);
    fd_detected_corners=fopen(filename,"w");
    for(unsigned r=0; r<total_features_Y; r++)
        for(unsigned c=0; c<total_features_X; c++)
            //if(r>1 && r<total_features_Y-2 && c>1 && c<total_features_X-2)
                fprintf(fd_detected_corners,"%f %f\n",feature_location_camera_space[proj_ID-starting_proj_ID][r*total_features_X+c][0],feature_location_camera_space[proj_ID-starting_proj_ID][r*total_features_X+c][1]);
    //fprintf(fd_detected_corners,"%f %f\n",CV_MAT_ELEM(*screen_space_feature_coordinates,float,0,r*total_features_X+c),CV_MAT_ELEM(*screen_space_feature_coordinates,float,1,r*total_features_X+c));
    fclose(fd_detected_corners);


    return;
}


/// ADD CODE FOR TRANSFORMING POINTS FROM CAMERA SPACE TO PROJECTOR SPACE...
void transform_camera_to_screen()
{
    CvMat*temp_transformation_array=cvCreateMat(3,total_features_Y*total_features_X,CV_32FC1);

    CvMat* screen_space_feature_coordinates=cvCreateMat(3,total_features_Y*total_features_X,CV_32FC1);

    CvFileStorage*fs_cs_homo=cvOpenFileStorage("cshomo0.xml",0,CV_STORAGE_READ);
    cshomo=(CvMat*)cvReadByName(fs_cs_homo,0,"cshomo0");

    for(unsigned n=0; n<max_num_of_projectors; n++)
        if(active_projectors[n]==true)
        {
            // copy relevant feature coordinates to a temporary array for applying camera to screen transformation.
            for(unsigned r=0; r<total_features_Y; r++)
                for(unsigned c=0; c<total_features_X; c++)
                {
                    for(unsigned i=0; i<2; i++)
                        CV_MAT_ELEM(*temp_transformation_array,float,i,r*total_features_X+c)=feature_location_camera_space[n][r*total_features_X+c][i];

                    CV_MAT_ELEM(*temp_transformation_array,float,2,r*total_features_X+c)=1.0f;
                }

            cvMatMul(cshomo,temp_transformation_array,screen_space_feature_coordinates);

// homogenization...
            for(unsigned r=0; r<total_features_Y; r++)
                for(unsigned c=0; c<total_features_X; c++)
                    for(unsigned i=0; i<2; i++)
                        CV_MAT_ELEM(*screen_space_feature_coordinates,float,i,r*total_features_X+c)/=CV_MAT_ELEM(*screen_space_feature_coordinates,float,2,r*total_features_X+c);


            /// To save efforts lets just copy the points back to 'feature_location_camera_space'
            for(unsigned r=0; r<total_features_Y; r++)
                for(unsigned c=0; c<total_features_X; c++)
                    for(unsigned i=0; i<2; i++)
                        feature_location_camera_space[n][r*total_features_X+c][i]=CV_MAT_ELEM(*screen_space_feature_coordinates,float,i,r*total_features_X+c);


            /// Lets save the detected coordinates in file
            FILE*fd_detected_corners;
            sprintf(filename,"detected_corners_%u_original_screen_space.txt",n);
            fd_detected_corners=fopen(filename,"w");
            for(unsigned r=0; r<total_features_Y; r++)
                for(unsigned c=0; c<total_features_X; c++)
                   // if(r>1 && r<total_features_Y-2 && c>1 && c<total_features_X-2)
                        fprintf(fd_detected_corners,"%f %f\n",feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][0],feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1]);
            //fprintf(fd_detected_corners,"%f %f\n",CV_MAT_ELEM(*screen_space_feature_coordinates,float,0,r*total_features_X+c),CV_MAT_ELEM(*screen_space_feature_coordinates,float,1,r*total_features_X+c));

            fclose(fd_detected_corners);

        }

/// NOTE: FROM NOW ON 'feature_location_camera_space' actually contains points in screen space(its a provisional appraoch will change(making saparate array for screen coordinates if this appraoch works))
    return;
}





void compute_cross_ratio_based_corner(unsigned proj_id,unsigned row,unsigned col)
{

    float x1,y1,x2,y2,x3,y3;
    float CR_proj; // cross ratio
    float AC,BD,AD,BC;
    float A_x,A_y,B_x,B_y,C_x,C_y,D_x,D_y;

    float t;

    if(row<=1 && col>1 && col<total_features_X-2) /// AB
    {
        x1=feature_location_camera_space[proj_id][(total_features_Y/4)*total_features_X+col][0];
        y1=feature_location_camera_space[proj_id][(total_features_Y/4)*total_features_X+col][1];
        x2=feature_location_camera_space[proj_id][(total_features_Y/2)*total_features_X+col][0];
        y2=feature_location_camera_space[proj_id][(total_features_Y/2)*total_features_X+col][1];
        x3=feature_location_camera_space[proj_id][(total_features_Y-3)*total_features_X+col][0];
        y3=feature_location_camera_space[proj_id][(total_features_Y-3)*total_features_X+col][1];

        A_x=(((feature_location_proj_space[row*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        A_y=(((feature_location_proj_space[row*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0f));

        B_x=(((feature_location_proj_space[(total_features_Y/4)*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        B_y=(((feature_location_proj_space[(total_features_Y/4)*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

        C_x=(((feature_location_proj_space[(total_features_Y/2)*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        C_y=(((feature_location_proj_space[(total_features_Y/2)*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

        D_x=(((feature_location_proj_space[(total_features_Y-3)*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        D_y=(((feature_location_proj_space[(total_features_Y-3)*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

    }


    if(row>=total_features_Y-2 && col>1 && col<total_features_X-2) /// CD
    {
        x1=feature_location_camera_space[proj_id][(3*total_features_Y/4)*total_features_X+col][0];
        y1=feature_location_camera_space[proj_id][(3*total_features_Y/4)*total_features_X+col][1];
        x2=feature_location_camera_space[proj_id][(total_features_Y/2)*total_features_X+col][0];
        y2=feature_location_camera_space[proj_id][(total_features_Y/2)*total_features_X+col][1];
        x3=feature_location_camera_space[proj_id][2*total_features_X+col][0];
        y3=feature_location_camera_space[proj_id][2*total_features_X+col][1];

        A_x=(((feature_location_proj_space[row*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        A_y=(((feature_location_proj_space[row*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0f));

        B_x=(((feature_location_proj_space[(3*total_features_Y/4)*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        B_y=(((feature_location_proj_space[(3*total_features_Y/4)*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

        C_x=(((feature_location_proj_space[(total_features_Y/2)*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        C_y=(((feature_location_proj_space[(total_features_Y/2)*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

        D_x=(((feature_location_proj_space[2*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        D_y=(((feature_location_proj_space[2*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0));
    }


    if(row>1 && row<total_features_Y-2 && col<=1) /// AC
    {
        x1=feature_location_camera_space[proj_id][row*total_features_X+total_features_X/4][0];
        y1=feature_location_camera_space[proj_id][row*total_features_X+total_features_X/4][1];
        x2=feature_location_camera_space[proj_id][row*total_features_X+total_features_X/2][0];
        y2=feature_location_camera_space[proj_id][row*total_features_X+total_features_X/2][1];
        x3=feature_location_camera_space[proj_id][row*total_features_X+total_features_X-3][0];
        y3=feature_location_camera_space[proj_id][row*total_features_X+total_features_X-3][1];

       A_x=(((feature_location_proj_space[row*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        A_y=(((feature_location_proj_space[row*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0f));

        B_x=(((feature_location_proj_space[row*total_features_X+total_features_X/4][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        B_y=(((feature_location_proj_space[row*total_features_X+total_features_X/4][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

        C_x=(((feature_location_proj_space[row*total_features_X+total_features_X/2][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        C_y=(((feature_location_proj_space[row*total_features_X+total_features_X/2][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

        D_x=(((feature_location_proj_space[row*total_features_X+total_features_X-3][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        D_y=(((feature_location_proj_space[row*total_features_X+total_features_X-3][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

    }

    if(row>1 && row<total_features_Y-2 && col>=total_features_X-2)  /// BD
    {
        x1=feature_location_camera_space[proj_id][row*total_features_X+(3*total_features_X)/4][0];
        y1=feature_location_camera_space[proj_id][row*total_features_X+(3*total_features_X)/4][1];
        x2=feature_location_camera_space[proj_id][row*total_features_X+total_features_X/2][0];
        y2=feature_location_camera_space[proj_id][row*total_features_X+total_features_X/2][1];
        x3=feature_location_camera_space[proj_id][row*total_features_X+2][0];
        y3=feature_location_camera_space[proj_id][row*total_features_X+2][1];

        A_x=(((feature_location_proj_space[row*total_features_X+col][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        A_y=(((feature_location_proj_space[row*total_features_X+col][1]+1.0f)/2.0f)*(projector_imageheight-1.0f));

        B_x=(((feature_location_proj_space[row*total_features_X+(3*total_features_X)/4][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        B_y=(((feature_location_proj_space[row*total_features_X+(3*total_features_X)/4][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

        C_x=(((feature_location_proj_space[row*total_features_X+total_features_X/2][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        C_y=(((feature_location_proj_space[row*total_features_X+total_features_X/2][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

        D_x=(((feature_location_proj_space[row*total_features_X+2][0]+1.0f)/2.0f)*(projector_imagewidth-1.0f));
        D_y=(((feature_location_proj_space[row*total_features_X+2][1]+1.0f)/2.0f)*(projector_imageheight-1.0));

    }


    AC=sqrt(pow(A_x-C_x,2)+pow(A_y-C_y,2));
    BD=sqrt(pow(B_x-D_x,2)+pow(B_y-D_y,2));

    AD=sqrt(pow(A_x-D_x,2)+pow(A_y-D_y,2));
    BC=sqrt(pow(B_x-C_x,2)+pow(B_y-C_y,2));


    CR_proj=(AC*BD)/(AD*BC);
    // t=((1.0-CR_proj)*(x1*y2+x2*y1)*(x1*y3-x3*y1))/((x3-x1)*((CR_proj-1.0)*x1*y2*y3-CR_proj*x2*y1*y3+x3*y1*y2)+(y3-y1)*((CR_proj-1.0)*x2*x3*y1-CR_proj*x1*x3*y2+x1*x2*y3));

    float bd=sqrt(pow(x1-x3,2)+pow(y1-y3,2));
    float bc=sqrt(pow(x1-x2,2)+pow(y1-y2,2));

    float delta=pow((CR_proj*(bc/bd)),2);

    float b=2.0f*((x3-x1)*(x1-x2)+(y3-y1)*(y1-y2)+pow(bd,2)*delta);
    float a=(1.0f-delta)*pow(bd,2);
    float c=pow(bc,2)-delta*pow(bd,2);


    float t1,t2;

    t1=((-1.0f*b+sqrt(pow(b,2)-4.0*a*c))/(2.0f*a));

    t2=((-1.0f*b-sqrt(pow(b,2)-4.0*a*c))/(2.0f*a));

    /// Checking validity of solution:
    if(t1>0.0f && t2>0.0f)
    {
        printf("No solution");
        printf("\nFor projector:%u at (row,col):(%u,%u)\n",proj_id,row,col);

        printf("t1=%f\tt2=%f",t1,t2);
        return;
    }

    else if(t1<0.0f && t2>0.0f)
        t=t1;


    else if(t1>0.0f && t2<0.0f)
        t=t2;


    else if(t1<0.0f && t2<0.0f)
    {
        printf("Ambigous solution!! exiting...");
        return;
    }

    // Lets do the initialization finally !!!!(So much work for just 2 lines of code :) :)
    feature_location_camera_space[proj_id][row*total_features_X+col][0]=x1+t*(x3-x1);
    feature_location_camera_space[proj_id][row*total_features_X+col][1]=y1+t*(y3-y1);

    return;
}




/// Purpose of this function is to recover additional points using cross ratio followed by refinement using line fitting from detected coordinates
void recover_projector_boundaries(unsigned proj_id)
{
// NOTE: Till this point we only have internal detected coordinates.
/// Lets modify the detected coordinates to ensure collinearity constraint:
    /// Detected corners will be enforced to strictly lie as intersection points of corresponding fitted vertical & horizontal lines.
    float line[4]; //scratch pad


    CvPoint2D32f* vertical_points = new CvPoint2D32f[total_features_Y-4];
    CvMat observed_points_vertical = cvMat(1, total_features_Y-4, CV_32FC2, vertical_points);

    CvPoint2D32f* horizontal_points = new CvPoint2D32f[total_features_X-4];
    CvMat observed_points_horizontal = cvMat(1, total_features_X-4, CV_32FC2, horizontal_points);



    /// fit horizontal lines

    for(unsigned int r=0; r<total_features_Y; r++)
    {

        if(r>1 && r<total_features_Y-2)
        {
            for(unsigned int c=0; c<total_features_X; c++)
            {
                if(c>1 && c<total_features_X-2)
                {
                    horizontal_points[c-2].x=feature_location_camera_space[proj_id][r*total_features_X+c][0];
                    horizontal_points[c-2].y=feature_location_camera_space[proj_id][r*total_features_X+c][1];

                }
            }

           cvFitLine(&observed_points_horizontal,CV_DIST_WELSCH,1,0.001,0.001,line);

            //Lets copy the fitted line parameters...
            for(unsigned int n=0; n<4; n++)
                horizontal_lines[proj_id][r-2][n]=line[n];
        }
    }


    /// fit vertical lines

    for(unsigned int c=0; c<total_features_X; c++)
    {
        if(c>1 && c<total_features_X-2)
        {
            for(unsigned int r=0; r<total_features_Y; r++)
            {
                if(r>1 && r<total_features_Y-2)
                {
                    vertical_points[r-2].x=feature_location_camera_space[proj_id][r*total_features_X+c][0];
                    vertical_points[r-2].y=feature_location_camera_space[proj_id][r*total_features_X+c][1];
                }
                cvFitLine(&observed_points_vertical,CV_DIST_WELSCH,1,0.001,0.001,line);
            }
            //Lets copy the fitted line parameters...
            for(unsigned int n=0; n<4; n++)
                vertical_lines[proj_id][c-2][n]=line[n];
        }
    }

    ///Lets compute the intersection of corresponding vertical & horizontal lines...

    for(unsigned int r=0; r<total_features_Y; r++)
    {

        if(r>1 && r<total_features_Y-2)
        {

            for(unsigned int c=0; c<total_features_X; c++)
            {
                if(c>1 && c<total_features_X-2)
                {
                    float t=(horizontal_lines[proj_id][r-2][0]*(vertical_lines[proj_id][c-2][3]-horizontal_lines[proj_id][r-2][3])-horizontal_lines[proj_id][r-2][1]*(vertical_lines[proj_id][c-2][2]-horizontal_lines[proj_id][r-2][2]))/(vertical_lines[proj_id][c-2][0]*horizontal_lines[proj_id][r-2][1]-horizontal_lines[proj_id][r-2][0]*vertical_lines[proj_id][c-2][1]);

                     feature_location_camera_space[proj_id][r*total_features_X+c][0]=vertical_lines[proj_id][c-2][2]+(t*vertical_lines[proj_id][c-2][0]);
                    feature_location_camera_space[proj_id][r*total_features_X+c][1]=vertical_lines[proj_id][c-2][3]+(t*vertical_lines[proj_id][c-2][1]);


                }
            }
        }
    }


/// compute cross ratio based points along AB,CD,AC,BD

//Lets initialize feature coordinates in projector space...

    for(unsigned int r=0; r<total_features_Y; r++)
        for(unsigned int c=0; c<total_features_X; c++)
        {
            feature_location_proj_space[r*total_features_X+c][0]=2.0f*((c*checkerboard_square_pixels)/((float)projector_imagewidth-1.0f))-1.0f;
            feature_location_proj_space[r*total_features_X+c][1]=(2.0f*(projector_imageheight-1.0f-r*checkerboard_square_pixels)/((float)projector_imageheight-1.0f))-1.0f;

            if(feature_location_proj_space[r*total_features_X+c][0]>1.0f)
                feature_location_proj_space[r*total_features_X+c][0]=1.0f;

            if(feature_location_proj_space[r*total_features_X+c][1]>1.0f)
                feature_location_proj_space[r*total_features_X+c][1]=1.0f;

            if(feature_location_proj_space[r*total_features_X+c][0]<-1.0f)
                feature_location_proj_space[r*total_features_X+c][0]=-1.0f;

            if(feature_location_proj_space[r*total_features_X+c][1]<-1.0f)
                feature_location_proj_space[r*total_features_X+c][1]=-1.0f;

        }


/// Lets add cross ratio based synthetic point generation

    /// Assuming
    ///  A----B
    ///  |    |
    ///  C----D

    for(unsigned row=0; row<total_features_Y; row++)
        for(unsigned col=0; col<total_features_X; col++)
        {
            if(!(col>1 && col<total_features_X-2 && row>1 && row<total_features_Y-2))
            {
                if((row<=1 || row>=total_features_Y-2) && (col<=1 || col>=total_features_X-2))
                    continue;

                // else,initialize the points
                compute_cross_ratio_based_corner(proj_id,row,col);
            }
        }

/// fit lines on estimated points
/// Convention: Let line 1=cross ratio fitted line; line 2=vertical/horizontal line fitted on detected checkerboard corners
    float AB[2][4],AC[2][4],CD[2][4],BD[2][4]; // these arrays will store openCV format line parameters.

    CvPoint2D32f*cross_ratio_based_points_along_X=(CvPoint2D32f*)malloc( (total_features_X-4) * sizeof(cross_ratio_based_points_along_X[0]));
    CvMat cross_ratio_based_points_along_X_mat=cvMat( 1, total_features_X-4, CV_32FC2, cross_ratio_based_points_along_X );



    CvPoint2D32f*cross_ratio_based_points_along_Y=(CvPoint2D32f*)malloc( (total_features_Y-4) * sizeof(cross_ratio_based_points_along_Y[0]));
    CvMat cross_ratio_based_points_along_Y_mat=cvMat( 1, total_features_Y-4, CV_32FC2, cross_ratio_based_points_along_Y );

    CvPoint2D32f*refined_points_along_X=new CvPoint2D32f[total_features_X-4];
    CvPoint2D32f*refined_points_along_Y=new CvPoint2D32f[total_features_Y-4];

// lets copy selected points(camera image) from global feature points array to a local array(used for line fitting)
    float x0,y0,c_x0,c_y0,x1,y1,c_x1,c_y1;
    float t1;
/// AB
    for(unsigned i=0; i<2; i++)
    {
        for(unsigned n=0; n<total_features_X-4; n++)
        {
            cross_ratio_based_points_along_X[n].x=feature_location_camera_space[proj_id][i*total_features_X+(n+2)][0];
            cross_ratio_based_points_along_X[n].y=feature_location_camera_space[proj_id][i*total_features_X+(n+2)][1];
        }
        cvFitLine(&cross_ratio_based_points_along_X_mat,CV_DIST_WELSCH,1,0.001,0.001,AB[i]);

        /// compute intersections with vertical lines
        for(unsigned n=0; n<total_features_X-4; n++)
        {
            x0=AB[i][2];
            y0=AB[i][3];
            c_x0=AB[i][0];
            c_y0=AB[i][1];
            x1=vertical_lines[proj_id][n][2];
            y1=vertical_lines[proj_id][n][3];
            c_x1=vertical_lines[proj_id][n][0];
            c_y1=vertical_lines[proj_id][n][1];

            t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
            refined_points_along_X[n].x=x1+t1*c_x1;
            refined_points_along_X[n].y=y1+t1*c_y1;
        }

        for(unsigned n=0; n<total_features_X-4; n++)
        {
            feature_location_camera_space[proj_id][i*total_features_X+(n+2)][0]=refined_points_along_X[n].x;
            feature_location_camera_space[proj_id][i*total_features_X+(n+2)][1]=refined_points_along_X[n].y;
        }


/// CD
        for(unsigned n=0; n<total_features_X-4; n++)
        {
            cross_ratio_based_points_along_X[n].x=feature_location_camera_space[proj_id][(total_features_Y-(i+1))*total_features_X+(n+2)][0];
            cross_ratio_based_points_along_X[n].y=feature_location_camera_space[proj_id][(total_features_Y-(i+1))*total_features_X+(n+2)][1];
        }

        cvFitLine(&cross_ratio_based_points_along_X_mat,CV_DIST_WELSCH,1,0.001,0.001,CD[i]);

/// compute intersections with vertical lines
        for(unsigned n=0; n<total_features_X-4; n++)
        {

            x0=CD[i][2];
            y0=CD[i][3];
            c_x0=CD[i][0];
            c_y0=CD[i][1];
            x1=vertical_lines[proj_id][n][2];
            y1=vertical_lines[proj_id][n][3];
            c_x1=vertical_lines[proj_id][n][0];
            c_y1=vertical_lines[proj_id][n][1];

            t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
            refined_points_along_X[n].x=x1+t1*c_x1;
            refined_points_along_X[n].y=y1+t1*c_y1;
        }

        // simply copy the points to their respective locations in global 'feature_location' array:
        for(unsigned n=0; n<total_features_X-4; n++)
        {
            feature_location_camera_space[proj_id][(total_features_Y-(i+1))*total_features_X+n+2][0]=refined_points_along_X[n].x;
            feature_location_camera_space[proj_id][(total_features_Y-(i+1))*total_features_X+n+2][1]=refined_points_along_X[n].y;
        }


/// AC
        for(unsigned n=0; n<total_features_Y-4; n++)
        {
            cross_ratio_based_points_along_Y[n].x=feature_location_camera_space[proj_id][(n+2)*total_features_X+i][0];
            cross_ratio_based_points_along_Y[n].y=feature_location_camera_space[proj_id][(n+2)*total_features_X+i][1];
        }

        cvFitLine(&cross_ratio_based_points_along_Y_mat,CV_DIST_WELSCH,1,0.001,0.001,AC[i]);
/// compute intersections with horizontal lines
        for(unsigned n=0; n<total_features_Y-4; n++)
        {
            x0=AC[i][2];
            y0=AC[i][3];
            c_x0=AC[i][0];
            c_y0=AC[i][1];
            x1=horizontal_lines[proj_id][n][2];
            y1=horizontal_lines[proj_id][n][3];
            c_x1=horizontal_lines[proj_id][n][0];
            c_y1=horizontal_lines[proj_id][n][1];

            t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
            refined_points_along_Y[n].x=x1+t1*c_x1;
            refined_points_along_Y[n].y=y1+t1*c_y1;
        }

        // simply copy the points to their respective locations in global 'feature_location' array:
        for(unsigned n=0; n<total_features_Y-4; n++)
        {
            feature_location_camera_space[proj_id][(n+2)*total_features_X+i][0]=refined_points_along_Y[n].x;
            feature_location_camera_space[proj_id][(n+2)*total_features_X+i][1]=refined_points_along_Y[n].y;
        }


/// BD
        for(unsigned n=0; n<total_features_Y-4; n++)
        {
            cross_ratio_based_points_along_Y[n].x=feature_location_camera_space[proj_id][(n+2)*total_features_X+total_features_X-(i+1)][0];
            cross_ratio_based_points_along_Y[n].y=feature_location_camera_space[proj_id][(n+2)*total_features_X+total_features_X-(i+1)][1];
        }

        cvFitLine(&cross_ratio_based_points_along_Y_mat,CV_DIST_WELSCH,1,0.001,0.001,BD[i]);

/// compute intersections with horizontal lines
        for(unsigned n=0; n<total_features_Y-4; n++)
        {
            x0=BD[i][2];
            y0=BD[i][3];
            c_x0=BD[i][0];
            c_y0=BD[i][1];
            x1=horizontal_lines[proj_id][n][2];
            y1=horizontal_lines[proj_id][n][3];
            c_x1=horizontal_lines[proj_id][n][0];
            c_y1=horizontal_lines[proj_id][n][1];

            t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
            refined_points_along_Y[n].x=x1+t1*c_x1;
            refined_points_along_Y[n].y=y1+t1*c_y1;
        }

        // simply copy the points to their respective locations in global 'feature_location' array:
        for(unsigned n=0; n<total_features_Y-4; n++)
        {
            feature_location_camera_space[proj_id][(n+2)*total_features_X+total_features_X-(i+1)][0]=refined_points_along_Y[n].x;
            feature_location_camera_space[proj_id][(n+2)*total_features_X+total_features_X-(i+1)][1]=refined_points_along_Y[n].y;
        }



/// compute intersection of fitted lines AB,CD,AC,BD to compute A,B,C,D.
/// AB vs AC=> A
        x0=AB[i][2];
        y0=AB[i][3];
        c_x0=AB[i][0];
        c_y0=AB[i][1];
        x1=AC[i][2];
        y1=AC[i][3];
        c_x1=AC[i][0];
        c_y1=AC[i][1];

        t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
        feature_location_camera_space[proj_id][i*total_features_X+i][0]=x1+t1*c_x1; // A.x
        feature_location_camera_space[proj_id][i*total_features_X+i][1]=y1+t1*c_y1; // A.y

/// AB vs BD=> B
        x0=AB[i][2];
        y0=AB[i][3];
        c_x0=AB[i][0];
        c_y0=AB[i][1];
        x1=BD[i][2];
        y1=BD[i][3];
        c_x1=BD[i][0];
        c_y1=BD[i][1];

        t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
        feature_location_camera_space[proj_id][i*total_features_X+total_features_X-(i+1)][0]=x1+t1*c_x1; // B.x
        feature_location_camera_space[proj_id][i*total_features_X+total_features_X-(i+1)][1]=y1+t1*c_y1; // B.y


/// CD vs AC=> C
        x0=CD[i][2];
        y0=CD[i][3];
        c_x0=CD[i][0];
        c_y0=CD[i][1];
        x1=AC[i][2];
        y1=AC[i][3];
        c_x1=AC[i][0];
        c_y1=AC[i][1];

        t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
        feature_location_camera_space[proj_id][(total_features_Y-(i+1))*total_features_X+i][0]=x1+t1*c_x1; // C.x
        feature_location_camera_space[proj_id][(total_features_Y-(i+1))*total_features_X+i][1]=y1+t1*c_y1; // C.y



/// CD vs BD=> D
        x0=CD[i][2];
        y0=CD[i][3];
        c_x0=CD[i][0];
        c_y0=CD[i][1];
        x1=BD[i][2];
        y1=BD[i][3];
        c_x1=BD[i][0];
        c_y1=BD[i][1];

        t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
        feature_location_camera_space[proj_id][(total_features_Y-(i+1))*total_features_X+total_features_X-(i+1)][0]=x1+t1*c_x1; // D.x
        feature_location_camera_space[proj_id][(total_features_Y-(i+1))*total_features_X+total_features_X-(i+1)][1]=y1+t1*c_y1; // D.y
    }

/// Now lets find out intersection of:

/// inner AB & outer AC
    x0=AB[1][2];
    y0=AB[1][3];
    c_x0=AB[1][0];
    c_y0=AB[1][1];
    x1=AC[0][2];
    y1=AC[0][3];
    c_x1=AC[0][0];
    c_y1=AC[0][1];

    t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
    feature_location_camera_space[proj_id][1*total_features_X][0]=x1+t1*c_x1;
    feature_location_camera_space[proj_id][1*total_features_X][1]=y1+t1*c_y1;

/// outer AB & inner AC
    x0=AB[0][2];
    y0=AB[0][3];
    c_x0=AB[0][0];
    c_y0=AB[0][1];
    x1=AC[1][2];
    y1=AC[1][3];
    c_x1=AC[1][0];
    c_y1=AC[1][1];

    t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
    feature_location_camera_space[proj_id][1][0]=x1+t1*c_x1;
    feature_location_camera_space[proj_id][1][1]=y1+t1*c_y1;

/// inner AB & outer BD
    x0=AB[1][2];
    y0=AB[1][3];
    c_x0=AB[1][0];
    c_y0=AB[1][1];
    x1=BD[0][2];
    y1=BD[0][3];
    c_x1=BD[0][0];
    c_y1=BD[0][1];

    t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
    feature_location_camera_space[proj_id][1*total_features_X+total_features_X-1][0]=x1+t1*c_x1;
    feature_location_camera_space[proj_id][1*total_features_X+total_features_X-1][1]=y1+t1*c_y1;

/// outer AB & inner BD
    x0=AB[0][2];
    y0=AB[0][3];
    c_x0=AB[0][0];
    c_y0=AB[0][1];
    x1=BD[1][2];
    y1=BD[1][3];
    c_x1=BD[1][0];
    c_y1=BD[1][1];

    t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
    feature_location_camera_space[proj_id][total_features_X-2][0]=x1+t1*c_x1;
    feature_location_camera_space[proj_id][total_features_X-2][1]=y1+t1*c_y1;


/// inner AC & outer CD
    x0=AC[1][2];
    y0=AC[1][3];
    c_x0=AC[1][0];
    c_y0=AC[1][1];
    x1=CD[0][2];
    y1=CD[0][3];
    c_x1=CD[0][0];
    c_y1=CD[0][1];

    t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
    feature_location_camera_space[proj_id][(total_features_Y-1)*total_features_X+1][0]=x1+t1*c_x1;
    feature_location_camera_space[proj_id][(total_features_Y-1)*total_features_X+1][1]=y1+t1*c_y1;




/// outer AC & inner CD
    x0=AC[0][2];
    y0=AC[0][3];
    c_x0=AC[0][0];
    c_y0=AC[0][1];
    x1=CD[1][2];
    y1=CD[1][3];
    c_x1=CD[1][0];
    c_y1=CD[1][1];

    t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
    feature_location_camera_space[proj_id][(total_features_Y-2)*total_features_X][0]=x1+t1*c_x1;
    feature_location_camera_space[proj_id][(total_features_Y-2)*total_features_X][1]=y1+t1*c_y1;


/// inner CD & outer BD
    x0=CD[1][2];
    y0=CD[1][3];
    c_x0=CD[1][0];
    c_y0=CD[1][1];
    x1=BD[0][2];
    y1=BD[0][3];
    c_x1=BD[0][0];
    c_y1=BD[0][1];

    t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
    feature_location_camera_space[proj_id][(total_features_Y-2)*total_features_X+total_features_X-1][0]=x1+t1*c_x1;
    feature_location_camera_space[proj_id][(total_features_Y-2)*total_features_X+total_features_X-1][1]=y1+t1*c_y1;

/// outer CD & inner BD
    x0=CD[0][2];
    y0=CD[0][3];
    c_x0=CD[0][0];
    c_y0=CD[0][1];
    x1=BD[1][2];
    y1=BD[1][3];
    c_x1=BD[1][0];
    c_y1=BD[1][1];

    t1=(c_x0*(y1-y0)-c_y0*(x1-x0))/(c_y0*c_x1-c_y1*c_x0);
    feature_location_camera_space[proj_id][(total_features_Y-1)*total_features_X+total_features_X-2][0]=x1+t1*c_x1;
    feature_location_camera_space[proj_id][(total_features_Y-1)*total_features_X+total_features_X-2][1]=y1+t1*c_y1;


/// Lets save the detected coordinates in file
    FILE*fd_detected_corners;
    sprintf(filename,"detected_corners_%u_recovered.txt",proj_id);
    fd_detected_corners=fopen(filename,"w");
    for(unsigned r=0; r<total_features_Y; r++)
        for(unsigned c=0; c<total_features_X; c++)
            fprintf(fd_detected_corners,"%f %f\n",feature_location_camera_space[proj_id-starting_proj_ID][r*total_features_X+c][0],feature_location_camera_space[proj_id-starting_proj_ID][r*total_features_X+c][1]);

    fclose(fd_detected_corners);


    filename[0]='\0';

    sprintf(filename,"detected_corners_%d.bmp",proj_id);
    debug_image=cvLoadImage(filename);


    for(unsigned row=0; row<total_features_Y; row++)
        for(unsigned col=0; col<total_features_X; col++)
            cvCircle(debug_image,cvPointFrom32f(cvPoint2D32f(feature_location_camera_space[proj_id][row*total_features_X+col][0],camera_imageheight-1.0-feature_location_camera_space[proj_id][row*total_features_X+col][1])),5,cvScalar(255,0,0),-1);


    sprintf(filename,"final_corners_%d.bmp",proj_id);
    cvSaveImage(filename,debug_image);


    return;
}



/// Computes local bounding rectangle for each projector as captured from camera.
void compute_local_bounding_boxes()
{

    float max_x,max_y,min_x,min_y;
    debug_image=cvLoadImage("debug_image.bmp");

    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
    {
        if(active_projectors[n]==true) // iff projector 'n' is used
        {
            max_x=-1.0f; // initializing to defualts.
            max_y=-1.0f;
            min_x=space_size_X;
            min_y=space_size_Y;

            for(unsigned int r=0; r<total_features_Y; r++)
                for(unsigned int c=0; c<total_features_X; c++)
                {

                    if(max_x<feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][0])
                        max_x=feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][0];


                    if(max_y<feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1])
                        max_y=feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1];



                    if(min_x>feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][0])
                        min_x=feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][0];



                    if(min_y>feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1])
                        min_y=feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1];

                }

            local_bounding_box[n-starting_proj_ID][0]=min_x;
            local_bounding_box[n-starting_proj_ID][1]=min_y;
            local_bounding_box[n-starting_proj_ID][2]=max_x-min_x;
            local_bounding_box[n-starting_proj_ID][3]=max_y-min_y;
            printf("\nLocal bounding box for proj:%d\n",n);
            printf("X:%f\t",local_bounding_box[n-starting_proj_ID][0]);
            printf("Y:%f\t",local_bounding_box[n-starting_proj_ID][1]);
            printf("width:%f\t",local_bounding_box[n-starting_proj_ID][2]);
            printf("height:%f\n",local_bounding_box[n-starting_proj_ID][3]);

            /*
                        ///Lets draw this box on debug image

                        unsigned col_num,row_num;
                        col_num=(effective_proj_id[n]%num_of_projectors_in_row);
                        row_num=(effective_proj_id[n]/num_of_projectors_in_row);
                        cvRectangle(debug_image,cvPointFrom32f(cvPoint2D32f(local_bounding_box[n-starting_proj_ID][0],camera_imageheight-1.0f-local_bounding_box[n-starting_proj_ID][1])),cvPointFrom32f(cvPoint2D32f(local_bounding_box[n-starting_proj_ID][0]+local_bounding_box[n-starting_proj_ID][2],(camera_imageheight-1.0f-local_bounding_box[n-starting_proj_ID][1])-local_bounding_box[n-starting_proj_ID][3])),cvScalar(50+row_num*(255/num_of_projectors_in_column),50+col_num*(255/num_of_projectors_in_row),0),5);
              */

        }


    }

    //cvSaveImage("debug_image_with_local_bounding_box.bmp",debug_image);

    return;
}






void compute_min_x_and_width_vert_line(int starting_proj)
{
    float min_x=space_size_X;
    float max_x=-1.0f;

    for(unsigned int n=starting_proj; n<(starting_proj_ID+max_num_of_projectors); n+=max_num_of_projectors_in_row)
    {
        if(active_projectors[n]==true)
        {
            if(min_x>local_bounding_box[n-starting_proj_ID][0])
                min_x=local_bounding_box[n-starting_proj_ID][0];
            if(max_x<(local_bounding_box[n-starting_proj_ID][0]+local_bounding_box[n-starting_proj_ID][2]))
                max_x=local_bounding_box[n-starting_proj_ID][0]+local_bounding_box[n-starting_proj_ID][2];
        }
    }

    for(unsigned int n=starting_proj; n<(starting_proj_ID+max_num_of_projectors); n+=max_num_of_projectors_in_row)
    {
        if(active_projectors[n]==true)
        {
            local_bounding_box[n-starting_proj_ID][0]=min_x;
            local_bounding_box[n-starting_proj_ID][2]=max_x-min_x;
        }
    }

    return;
}

void compute_min_y_and_height_hor_line(int starting_proj)
{
    float min_y=space_size_Y;
    float max_y=-1.0f;

    for(unsigned int n=starting_proj; n<(starting_proj+max_num_of_projectors_in_row); n++)
    {
        if(active_projectors[n]==true)
        {
            if(min_y>local_bounding_box[n-starting_proj_ID][1])
                min_y=local_bounding_box[n-starting_proj_ID][1];

            if(max_y<(local_bounding_box[n-starting_proj_ID][1]+local_bounding_box[n-starting_proj_ID][3]))
                max_y=local_bounding_box[n-starting_proj_ID][1]+local_bounding_box[n-starting_proj_ID][3];
        }
    }


    for(unsigned int n=starting_proj; n<(starting_proj+max_num_of_projectors_in_row); n++)
    {
        if(active_projectors[n]==true)
        {
            local_bounding_box[n-starting_proj_ID][1]=min_y;
            local_bounding_box[n-starting_proj_ID][3]=max_y-min_y;
        }
    }


    return;
}



void compute_global_width_and_height(float * max_width,float * max_height)
{
    *max_width=-1.0f;
    *max_height=-1.0f;

    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
    {
        if(active_projectors[n]==true)
        {
            if(*max_width<local_bounding_box[n-starting_proj_ID][2])
                *max_width=local_bounding_box[n-starting_proj_ID][2];

            if(*max_height<local_bounding_box[n-starting_proj_ID][3])
                *max_height=local_bounding_box[n-starting_proj_ID][3];
        }

    }

    return;
}




void modify_local_bounding_boxes()
{

    for(unsigned int i=starting_proj_ID; i<(starting_proj_ID+max_num_of_projectors_in_row); i++)
        compute_min_x_and_width_vert_line(i);


    unsigned int count=starting_proj_ID;
    for(unsigned int i=starting_proj_ID/num_of_projectors_in_row; i<max_num_of_projectors_in_column; i++)
    {
        compute_min_y_and_height_hor_line(count);
        count+=max_num_of_projectors_in_row;

    }

    /// Set (x,y,width,height) for all projectors
    float *max_width=new float;
    float *max_height=new float;

    compute_global_width_and_height(max_width,max_height);

    printf("\n@1:Width:%f\t@2:Height:%f\n",*max_width,*max_height);
    //Lets modify local bounding box accordingly...
    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
    {
        if(active_projectors[n]==true)
        {
            printf("\nLocal bounding box(after modification):%d\n",n);
            local_bounding_box[n-starting_proj_ID][2]=*max_width;
            local_bounding_box[n-starting_proj_ID][3]=*max_height;

            printf("X:%f\t",local_bounding_box[n-starting_proj_ID][0]);
            printf("Y:%f\t",local_bounding_box[n-starting_proj_ID][1]);
            printf("width:%f\t",local_bounding_box[n-starting_proj_ID][2]);
            printf("height:%f\n",local_bounding_box[n-starting_proj_ID][3]);
        }
    }


    /*
        ///Lets save the modified bounding boxes
        debug_image=cvLoadImage("debug_image.bmp");
        unsigned col_num,row_num;


        for(unsigned n=starting_proj_ID; n<max_num_of_projectors; n++)
        {
            if(active_projectors[n]==true)
            {
                col_num=(n%num_of_projectors_in_row);
                row_num=(n/num_of_projectors_in_row);
                cvRectangle(debug_image,cvPointFrom32f(cvPoint2D32f(local_bounding_box[n-starting_proj_ID][0],camera_imageheight-1.0f-local_bounding_box[n-starting_proj_ID][1])),cvPointFrom32f(cvPoint2D32f(local_bounding_box[n-starting_proj_ID][0]+local_bounding_box[n-starting_proj_ID][2],camera_imageheight-1.0f-local_bounding_box[n-starting_proj_ID][1]-local_bounding_box[n-starting_proj_ID][3])),cvScalar(50+row_num*(255/num_of_projectors_in_column),50+col_num*(255/num_of_projectors_in_row),0),5);
            }
        }

        cvSaveImage("debug_image_with_local_bounding_box(modified).bmp",debug_image);

    */
    return;
}


/// Computes global bounding box from local bounding boxes
void compute_global_bounding_box()
{
    float min_x,max_x,min_y,max_y;
    max_x=-1.0f; // initializing to defualts.
    max_y=-1.0f;
    min_x=space_size_X;
    min_y=space_size_Y;

    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
    {
        if(active_projectors[n]==true)
        {

            if(max_x<(local_bounding_box[n-starting_proj_ID][0]+local_bounding_box[n-starting_proj_ID][2]))
                max_x=local_bounding_box[n-starting_proj_ID][0]+local_bounding_box[n-starting_proj_ID][2];

            if(max_y<(local_bounding_box[n-starting_proj_ID][1]+local_bounding_box[n-starting_proj_ID][3]))
                max_y=local_bounding_box[n-starting_proj_ID][1]+local_bounding_box[n-starting_proj_ID][3];

            if(min_x>local_bounding_box[n-starting_proj_ID][0])
                min_x=local_bounding_box[n-starting_proj_ID][0];

            if(min_y>local_bounding_box[n-starting_proj_ID][1])
                min_y=local_bounding_box[n-starting_proj_ID][1];
        }

        else
            continue;
    }

    global_bounding_box[0]=min_x;
    global_bounding_box[1]=min_y;
    global_bounding_box[2]=max_x-min_x;
    global_bounding_box[3]=max_y-min_y;
    printf("\nglobal bounding box:\n");
    printf("X:%f\t",global_bounding_box[0]);
    printf("Y:%f\t",global_bounding_box[1]);
    printf("width:%f\t",global_bounding_box[2]);
    printf("height:%f\n",global_bounding_box[3]);



    return;
}


///Computes normalized coordinates in global bounding box
void compute_local_normalized_coordinates()
{

    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
    {
        if(active_projectors[n]==true)
        {
            printf("\nNormalized bounding box for projector(after modification):%d",n);
            printf("\nX=%f\t",normalized_local_bounding_boxes[n-starting_proj_ID][0]=(local_bounding_box[n-starting_proj_ID][0]-global_bounding_box[0])/global_bounding_box[2]);
            printf("Y=%f\t",normalized_local_bounding_boxes[n-starting_proj_ID][1]=(local_bounding_box[n-starting_proj_ID][1]-global_bounding_box[1])/global_bounding_box[3]);
            printf("width=%f\t",normalized_local_bounding_boxes[n-starting_proj_ID][2]=local_bounding_box[n-starting_proj_ID][2]/global_bounding_box[2]);
            printf("height=%f\n",normalized_local_bounding_boxes[n-starting_proj_ID][3]=local_bounding_box[n-starting_proj_ID][3]/global_bounding_box[3]);
        }
    }

    return;
}



/// Computes scale factor to be used to map tile in normalized coordinate system for each projector to pixel space for chromium configuration.
void compute_chromium_scale_factor()
{
    float max_width=-1.0f,max_height=-1.0f;

    //First find out larget width & height of any bounding box
    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
    {
        if(active_projectors[n]==true)
        {
            if(max_width<normalized_local_bounding_boxes[n-starting_proj_ID][2])
                max_width=normalized_local_bounding_boxes[n-starting_proj_ID][2];

            if(max_height<normalized_local_bounding_boxes[n-starting_proj_ID][3])
                max_height=normalized_local_bounding_boxes[n-starting_proj_ID][3];
        }
    }

    printf("\nmax_width:%f\n",max_width);
    printf("\nmax_height:%f",max_height);

    printf("\nscale_x:%f\t",scale_x=projector_imagewidth/max_width);
    printf("scale_y:%f\n",scale_y=projector_imageheight/max_height);

    /// Lets save this scale factor to be used for alpha blending
    FILE*fd_xy_scale=fopen("blend_mask_generator/scale_xy.txt","w");
    fprintf(fd_xy_scale,"%f %f",scale_x,scale_y);
    fclose(fd_xy_scale);

    return;
}



/// Uses the scale factor to generate chromium coordinates for each projector.
void generate_chromium_configuration()
{
    char filename[100];
    unsigned int chromium_tile[max_num_of_projectors][4];

    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
    {
        if(active_projectors[n]==true)
        {
            sprintf(filename,"chromium_proj_%d.txt",n);
            FILE*fd_chromium_config=fopen(filename,"w");
            fprintf(fd_chromium_config,"%u\t",chromium_tile[n][0]=round(normalized_local_bounding_boxes[n-starting_proj_ID][0]*scale_x));
            fprintf(fd_chromium_config,"%u\t",chromium_tile[n][1]=round(normalized_local_bounding_boxes[n-starting_proj_ID][1]*scale_y));
            fprintf(fd_chromium_config,"%u\t",chromium_tile[n][2]=round(normalized_local_bounding_boxes[n-starting_proj_ID][2]*scale_x));
            fprintf(fd_chromium_config,"%u\n",chromium_tile[n][3]=round(normalized_local_bounding_boxes[n-starting_proj_ID][3]*scale_y));
            fclose(fd_chromium_config);

            printf("\nTile:%d\n",n);
            printf("X=%f\tY=%f\tWidth=%f\nHeight=%f\n",normalized_local_bounding_boxes[n-starting_proj_ID][0]*scale_x,normalized_local_bounding_boxes[n-starting_proj_ID][1]*scale_y,normalized_local_bounding_boxes[n-starting_proj_ID][2]*scale_x,normalized_local_bounding_boxes[n-starting_proj_ID][3]*scale_y);
        }
    }

    //Lets copy all configuration files to 90.5.2.204 server
    char command[500];
    /* for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+num_of_projectors); n++)
     {
         command[0]='\0';
         sprintf(command,"scp chromium_proj_%d.txt root@90.5.2.204:../global/pranav/chromium_%d.txt",n,n);
         system(command);
     }
    */

    FILE*fd_tiled_config=fopen("TiledDisplay.py","w");
    fprintf(fd_tiled_config,"HorizontalGap = 0\n");
    fprintf(fd_tiled_config,"VerticalGap = 0\n");
    fprintf(fd_tiled_config,"TileWidth = %u\n",projector_imagewidth);
    fprintf(fd_tiled_config,"TileHeight = %u\n",projector_imageheight);
    fprintf(fd_tiled_config,"TileColumns = %u\n",num_of_projectors_in_row);
    fprintf(fd_tiled_config,"TileRows = %u\n",num_of_projectors_in_column);
    fprintf(fd_tiled_config,"FullScreen = 1\n");
    fprintf(fd_tiled_config,"nos=[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]\n");
    fprintf(fd_tiled_config,"tile_info=[]\n");

    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
        if(active_projectors[n]==true)
            fprintf(fd_tiled_config,"tile_info.append([%u,%u,%u,%u])\n",chromium_tile[n][0],chromium_tile[n][1],chromium_tile[n][2],chromium_tile[n][3]);


    /*
        fprintf(fd_tiled_config,"\"\"\"\n");
        fprintf(fd_tiled_config,"#for feature projection script\n");
        unsigned r,c;
        for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
            if(active_projectors[n]==true)
            {
                r=floor(n/max_num_of_projectors_in_column);
                c=n%max_num_of_projectors_in_column;

                sprintf(filename,"tile_info.append([%u,%u,%u,%u])\n",projector_imagewidth*c,r*projector_imageheight,projector_imagewidth,projector_imageheight);
                fprintf(fd_tiled_config,"%s",filename);
            }
        fprintf(fd_tiled_config,"\"\"\"\n");

    */
    fclose(fd_tiled_config);

    if(registration_on_screen==1)
        sprintf(command,"scp TiledDisplay.py demo@90.5.2.204:../../global/tile_specs/screen_space_registration/TiledDisplay.py");

    else
        sprintf(command,"scp TiledDisplay.py demo@90.5.2.204:../../global/tile_specs/camera_space_registration/TiledDisplay.py");

    system(command);


    /// Also saving these points for alpha map generation:
    FILE*fd_detected_corners;
    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
    {
        if(active_projectors[n]==true)
        {
            sprintf(filename,"blend_mask_generator/global_box_detected_corners_%d.txt",n);
            fd_detected_corners=fopen(filename,"w");
            for(unsigned int r=0; r<total_features_Y; r++)
                for(unsigned int c=0; c<total_features_X; c++)
                    fprintf(fd_detected_corners,"%f %f\n",scale_x*((feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][0]-global_bounding_box[0])/global_bounding_box[2]),scale_y*((feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1]-global_bounding_box[1])/global_bounding_box[3]));

            fclose(fd_detected_corners);
        }
    }
    return;
}


/// Defines mapping between tile coordinates for each projector to texture coordinates.
/// Tile coordinates are texture coordinates, Projector coordinates will be vertex coordinates
void create_tile_projector_mapping()
{
    char filename[100];

    // tile coordinates are defined locally for each projector with respect to its local bounding box top leftmost corner as origin
    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
        if(active_projectors[n]==true)
            for(unsigned int r=0; r<total_features_Y; r++)
                for(unsigned int c=0; c<total_features_X; c++)
                {
                    tile_coordinates[n-starting_proj_ID][r*total_features_X+c][0]=(feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][0]-local_bounding_box[n-starting_proj_ID][0])/(local_bounding_box[n-starting_proj_ID][2]);
                    tile_coordinates[n-starting_proj_ID][r*total_features_X+c][1]=(feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1]-local_bounding_box[n-starting_proj_ID][1])/(local_bounding_box[n-starting_proj_ID][3]);
                }


//Lets initialize feature coordinates in projector space...

    for(unsigned int r=0; r<total_features_Y; r++)
        for(unsigned int c=0; c<total_features_X; c++)
        {
            feature_location_proj_space[r*total_features_X+c][0]=2.0f*(((c+2)*checkerboard_square_pixels)/((float)projector_imagewidth-1.0f))-1.0f;
            feature_location_proj_space[r*total_features_X+c][1]=(2.0f*(projector_imageheight-1.0f-(r+2)*checkerboard_square_pixels)/((float)projector_imageheight-1.0f))-1.0f;

            if(feature_location_proj_space[r*total_features_X+c][0]>1.0f)
                feature_location_proj_space[r*total_features_X+c][0]=1.0f;

            if(feature_location_proj_space[r*total_features_X+c][1]>1.0f)
                feature_location_proj_space[r*total_features_X+c][1]=1.0f;

            if(feature_location_proj_space[r*total_features_X+c][0]<-1.0f)
                feature_location_proj_space[r*total_features_X+c][0]=-1.0f;

            if(feature_location_proj_space[r*total_features_X+c][1]<-1.0f)
                feature_location_proj_space[r*total_features_X+c][1]=-1.0f;

        }







    FILE*fd_mapping;
    //create the tile mapping files to be sent to the rendering machine.
    for(unsigned n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
        if(active_projectors[n]==true)
        {
            sprintf(filename,"mapping_tile_proj_%d.txt",n+1);
            fd_mapping=fopen(filename,"w");
            //include alpha map & color correction files.
            fprintf(fd_mapping,"%u %u\n",total_features_Y,total_features_X);
            fprintf(fd_mapping,"/global/tile_specs/texture%u.txt.%uX%u\n",n+1,num_of_projectors_in_column,num_of_projectors_in_row);
            //fprintf(fd_mapping,"/global/tile_specs/texture.txt.test\n");
            fprintf(fd_mapping,"/global/tile_specs/texture.txt.test\n");
            fprintf(fd_mapping,"/global/tile_specs/texture.txt.test\n");
            fprintf(fd_mapping,"/global/tile_specs/texture.txt.test\n");
            //Lets write the actual mapping
            for(int r=(total_features_Y-1); r>=0; r--)
                for(unsigned int c=0; c<total_features_X; c++)
                    fprintf(fd_mapping,"%f\t%f\t%f\t%f\n",feature_location_proj_space[r*total_features_X+c][0],feature_location_proj_space[r*total_features_X+c][1],tile_coordinates[n-starting_proj_ID][r*total_features_X+c][0],tile_coordinates[n-starting_proj_ID][r*total_features_X+c][1]);

            fclose(fd_mapping);
        }

//Lets copy these file to rendering machine.
    char command[500];

    for(unsigned n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
        if(active_projectors[n]==true)
        {
            command[0]='\0';

            if(registration_on_screen==1)
                sprintf(command,"scp mapping_tile_proj_%d.txt demo@90.5.2.204:../../global/tile_specs/screen_space_registration/node%d.info",n+1,n+1);

            else
                sprintf(command,"scp mapping_tile_proj_%d.txt demo@90.5.2.204:../../global/tile_specs/camera_space_registration/node%d.info",n+1,n+1);

            system(command);
        }

    return;
}


void generate_default_chromium_configuration()
{
// This function will generate and send(to rendering machine) the original chromium configuration without any corrections applied.

    FILE*fd_tiled_config=fopen("TiledDisplay.py","w");
    fprintf(fd_tiled_config,"HorizontalGap = 0\n");
    fprintf(fd_tiled_config,"VerticalGap = 0\n");
    fprintf(fd_tiled_config,"TileWidth = %u\n",projector_imagewidth);
    fprintf(fd_tiled_config,"TileHeight = %u\n",projector_imageheight);
    fprintf(fd_tiled_config,"TileColumns = %u\n",num_of_projectors_in_row);
    fprintf(fd_tiled_config,"TileRows = %u\n",num_of_projectors_in_column);
    fprintf(fd_tiled_config,"FullScreen = 1\n");
    fprintf(fd_tiled_config,"nos=[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]\n");
    fprintf(fd_tiled_config,"tile_info=[]\n");

    unsigned r,c;

    unsigned count=0;

    for(unsigned n=0; n<max_num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
        {
            effective_proj_id[n]=count;
            count++;
        }
    }

    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
        if(active_projectors[n]==true)
        {
            r=floor(effective_proj_id[n]/num_of_projectors_in_row);
            c=effective_proj_id[n]%num_of_projectors_in_row;

            sprintf(filename,"tile_info.append([%u,%u,%u,%u])\n",projector_imagewidth*c,r*projector_imageheight,projector_imagewidth,projector_imageheight);
            fprintf(fd_tiled_config,"%s",filename);
        }

    fclose(fd_tiled_config);


    char command[500];

    command[0]='\0';
    sprintf(command,"scp TiledDisplay.py demo@90.5.2.204:../../global/tile_specs/TiledDisplay.py");
    system(command);

    command[0]='\0';
    sprintf(command,"scp TiledDisplay.py demo@90.5.2.204:../../global/tile_specs/original_configuration/TiledDisplay.py");
    system(command);



    // Lets write default mapping configuration for all active projectors
    FILE*fd_mapping;


    //create the tile mapping files to be sent to the rendering machine.
    float vertex_coordinates[4][2],texture_coordinates[4][2];

    vertex_coordinates[0][0]=-1.0;
    vertex_coordinates[0][1]=-1.0;

    vertex_coordinates[1][0]=1.0;
    vertex_coordinates[1][1]=-1.0;

    vertex_coordinates[2][0]=-1.0;
    vertex_coordinates[2][1]=1.0;

    vertex_coordinates[3][0]=1.0;
    vertex_coordinates[3][1]=1.0;


    texture_coordinates[0][0]=0.0;
    texture_coordinates[0][1]=0.0;

    texture_coordinates[1][0]=1.0;
    texture_coordinates[1][1]=0.0;

    texture_coordinates[2][0]=0.0;
    texture_coordinates[2][1]=1.0;

    texture_coordinates[3][0]=1.0;
    texture_coordinates[3][1]=1.0;



    for(unsigned n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
        if(active_projectors[n]==true)
        {
            sprintf(filename,"mapping_tile_proj_%d.txt",n+1);
            fd_mapping=fopen(filename,"w");
            //include alpha map & color correction files.
            fprintf(fd_mapping,"%u %u\n",2,2);
            //fprintf(fd_mapping,"/global/tile_specs/original_configuration/texture%u.txt.%uX%u\n",n+1,num_of_projectors_in_column,num_of_projectors_in_row);
            fprintf(fd_mapping,"/global/tile_specs/texture.txt.test\n");
            fprintf(fd_mapping,"/global/tile_specs/texture.txt.test\n");
            fprintf(fd_mapping,"/global/tile_specs/texture.txt.test\n");
            fprintf(fd_mapping,"/global/tile_specs/texture.txt.test\n");
            //Lets write the actual mapping


            for(unsigned i=0; i<4; i++)
                fprintf(fd_mapping,"%f\t%f\t%f\t%f\n",vertex_coordinates[i][0],vertex_coordinates[i][1],texture_coordinates[i][0],texture_coordinates[i][1]);

            fclose(fd_mapping);
        }

//Lets copy these file to rendering machine.


    for(unsigned n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
        if(active_projectors[n]==true)
        {
            command[0]='\0';
            sprintf(command,"scp mapping_tile_proj_%d.txt demo@90.5.2.204:../../global/tile_specs/original_configuration/node%d.info",n+1,n+1);
            system(command);

            command[0]='\0';
            sprintf(command,"scp mapping_tile_proj_%d.txt demo@90.5.2.204:../../global/tile_specs/node%d.info",n+1,n+1);
            system(command);
        }



    return;
}






///**************************Main procedure*******************************************///
int main()
{

    int load_values;
    unsigned int proj_ID=starting_proj_ID;

    printf("\nPlease enter number of columns:");
    scanf("%u",&num_of_projectors_in_row);

    printf("\nPlease enter number of rows:");
    scanf("%u",&num_of_projectors_in_column);

    num_active_proj_total=num_of_projectors_in_row*num_of_projectors_in_column;

    if(num_active_proj_total>max_num_of_projectors)
    {
        printf("Number of projectors exceed preset maximum number of projectors!!!, exiting...");
        exit(0);
    }


    for(unsigned n=0; n<max_num_of_projectors; n++)
        active_projectors[n]=false; //initializing

    printf("\nPlease enter projector ids of active projector(s):");
    unsigned k;
    for(unsigned n=0; n<num_active_proj_total; n++)
    {
        scanf("%u",&k);
        active_projectors[k]=true;
    }
    char command[200];

    /// Lets save the active map to be used by blend map generator
    printf("\nSending projector masking information to rendering machine...");
    FILE*fd_active_projector_map=fopen("active_projector_map.txt","w");

    for(unsigned n=0; n<max_num_of_projectors; n++)
        fprintf(fd_active_projector_map,"%u ",(active_projectors[n]==true)?1:0);

    fclose(fd_active_projector_map);

    /// also send this to rendering machine for masking unused projectors
    command[0]='\0';
    sprintf(command,"scp active_projector_map.txt demo@90.5.2.204:../../global/pranav/");
    system(command);

    // Generate Hosts_old.py
    FILE*fd_host_list=fopen("Hosts.py","w");
    unsigned row_id,col_id;
    unsigned count=0;
    char host_command1[500],host_command2[100],display_port_command1[500],display_port_command2[100];

    fprintf(fd_host_list,"MOTHERSHIP_HOST = \'gscore4.compunet.barc.in\'\n");

    sprintf(host_command1,"HOSTS = [");

    sprintf(display_port_command1,"DISPLAY_PORT = [");

    for(unsigned n=0; n<max_num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
        {
            row_id=floor(n/max_num_of_projectors_in_row)+1;
            col_id=(n%max_num_of_projectors_in_row);
            host_command2[0]='\0';
            display_port_command2[0]='\0';

            if(count<(num_active_proj_total-1))
            {
                sprintf(host_command2,"\'gscore%u.compunet.barc.in\',",row_id);
                sprintf(display_port_command2,"\'0.%u\',",col_id);
            }

            else
            {
                sprintf(host_command2,"\'gscore%u.compunet.barc.in\'",row_id);
                sprintf(display_port_command2,"\'0.%u\'",col_id);
            }

            strcat(host_command1,host_command2);
            strcat(display_port_command1,display_port_command2);

            count++;
        }
    }

    host_command2[0]='\0';
    display_port_command2[0]='\0';

    sprintf(host_command2,"]");
    sprintf(display_port_command2,"]");

    strcat(host_command1,host_command2);
    strcat(display_port_command1,display_port_command2);


    fprintf(fd_host_list,"%s\n",host_command1);
    fprintf(fd_host_list,"%s",display_port_command1);

    fclose(fd_host_list);


    ///Lets send this active host configuration to rendering machine:

    command[0]='\0';
    sprintf(command,"scp Hosts.py demo@90.5.2.204:../../global/tile_specs/Hosts.py");
    system(command);


    generate_default_chromium_configuration(); // send defualt configuration(without any correction)



    /// Provision for both in-camera & on-screen computations...

    printf("\nRegistration in-camera(0) or on-screen(1)?");
    scanf("%u",&registration_on_screen);

    FILE*use_screen=fopen("use_screen.txt","w");
    fprintf(use_screen,"%u",registration_on_screen);
    fclose(use_screen);



    //set space size
    if(registration_on_screen)
    {
        printf("Hi");
        space_size_X=screen_size_X;
        space_size_Y=screen_size_Y;
    }

    else
    {
        space_size_X=camera_imagewidth;
        space_size_Y=camera_imageheight;
    }


    printf("\nLoad previous detected corner dataset(0:No 1:yes)?");
    scanf("%d",&load_values);

    if(load_values==1) //load previous detected corner information
    {
        IplImage*test_image;
        FILE*fd_detected_corners;

        for(unsigned n=starting_proj_ID; n<(max_num_of_projectors+starting_proj_ID); n++)
        {
            if(active_projectors[n]==true)
            {


                sprintf(filename,"detected_corners_%d.bmp",n);
                test_image=cvLoadImage(filename);

                if(registration_on_screen==true)
                    sprintf(filename,"detected_corners_%u_original_screen_space.txt",n);

                else
                    sprintf(filename,"detected_corners_%u_original_camera_space.txt",n);


                fd_detected_corners=fopen(filename,"r");
                for(unsigned r=0; r<total_features_Y; r++)
                    for(unsigned c=0; c<total_features_X; c++)
                    {
                        //if(r>1 && r<total_features_Y-2 && c>1 && c<total_features_X-2)
                       // {
                            fscanf(fd_detected_corners,"%f %f\n",&feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][0],&feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1]);
                            cvCircle(test_image,cvPointFrom32f(cvPoint2D32f(feature_location_camera_space[n][r*total_features_X+c][0],feature_location_camera_space[n][r*total_features_X+c][1])),5,cvScalar(0,255,0),-1);
                       // }
                    }
                fclose(fd_detected_corners);

                printf("\nDetected corners for projector %u loaded!!!\n",n);


                //sprintf(filename,"test_image_%d.bmp",n);
                //cvSaveImage(filename,test_image);

            }

        }

    }



    else
    {

        /// G7 initialization...
        cvNamedWindow("Camera_view",0);
        cvCreateTrackbar2("Camera_shutter_speed","Camera_view",&cam_shutter_speed,44,NULL);
        cvCreateTrackbar2("Camera_aperture","Camera_view",&cam_aperture,9,NULL);


        ///Lets initialize G7...
        gp_camera_new(&camera);
        context=gp_context_new();
        printf("\nInitializing camera...");

        unsigned int ret=gp_camera_init(camera,context);

        if(ret<GP_OK)
        {
            printf("\nCamera initialization failed, exiting!!!");
            gp_camera_free(camera); //releases camera resources.
            exit(1);
        }

        set_camera_control("capture","1"); //lets open up the camera

        printf("\nSetting image size...");
        fflush(stdout);
        char img_size[]="Large";
        set_camera_control("imagesize",img_size);
        set_camera_control("shootingmode","Manual");
        printf("\nCamera initialized!!");

        fflush(stdout);

        /// G7 initialized.

        compute_c_s_homography();

        while(proj_ID<(starting_proj_ID+max_num_of_projectors))
        {
            if(active_projectors[proj_ID]==true)
            {
                printf("\nNow projecting pattern on projector:%d...",proj_ID);
                fflush(stdout);
                project_capture_features(proj_ID);

            }
            proj_ID++;
        }

        if(registration_on_screen)
            transform_camera_to_screen();

    }



///Just inverting the coordinate system to be inaccordance with OpenGL convention
    for(unsigned int n=starting_proj_ID; n<(starting_proj_ID+max_num_of_projectors); n++)
        if(active_projectors[n]==true) // iff projector 'n' is used
            for(unsigned int r=0; r<total_features_Y; r++)
                for(unsigned int c=0; c<total_features_X; c++)
                    feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1]=space_size_Y-1.0f-feature_location_camera_space[n-starting_proj_ID][r*total_features_X+c][1];

/*
        for(proj_ID=0; proj_ID<(starting_proj_ID+max_num_of_projectors); proj_ID++)
            if(active_projectors[proj_ID]==true)
                recover_projector_boundaries(proj_ID); // avoids loss of projection region in conventional approach.
*/
        compute_local_bounding_boxes();

        modify_local_bounding_boxes();

        compute_global_bounding_box();

        compute_local_normalized_coordinates();

        compute_chromium_scale_factor();

        generate_chromium_configuration();

        create_tile_projector_mapping();





    return 0;

}
