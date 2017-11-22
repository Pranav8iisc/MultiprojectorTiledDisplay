/*
TARGET:
Reads chromium configuration, computes blend mask for all projectors
*/

#include<stdio.h>
#include"opencv/cv.h"
#include"opencv/highgui.h"



#define num_of_projectors 9
#define num_of_proj_along_row 3
#define num_of_proj_along_column 3
#define proj_imagewidth 1024
#define proj_imageheight 768
#define total_pixels (proj_imageheight*proj_imagewidth)

#define feature_X 29
#define feature_Y 21

#define total_feature_X (feature_X+4)
#define total_feature_Y (feature_Y+4)

#define total_features (total_feature_X*total_feature_Y)

#define BLACK_OUT 0
#define AVERAGE 1
#define DIST_TRANSFORM 2


float m[num_of_projectors][4],c[num_of_projectors][4]; // to store (m,c) for all 4 boundaries of all projectors(used in distance transform)
bool **region[num_of_projectors];

bool active_projectors[num_of_projectors];



bool no_one_else_in_the_region(unsigned int x, unsigned int y,unsigned int proj_id) // this function checks whether there are multiple projection regions at pixel (x,y)
{
    for(unsigned int n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
        {
            if((region[n][x][y]==true) && (n!=proj_id))
            {
                return false;
            }
            else
                continue;
        }
    }

    return true; // since if control has come at this line it means there was no other projector than proj_id itself at pixel location (x,y) in global chromium texture space.
}

float blending_function(unsigned int method, unsigned int proj_id,unsigned proj_bounding_box_x,unsigned proj_bounding_box_y,unsigned proj_bounding_box_width,unsigned proj_bounding_box_height,unsigned int col,unsigned int row,unsigned tile_imagewidth,unsigned tile_imageheight)
{
//this function simply sets alpha value for pixel(c,r) for projector n(depending upon blending method).
    float blend_value=0;
    unsigned count=0;
    float min_dist;

    switch(method)
    {
    case BLACK_OUT:
        static bool **pixel_occupied[1]; // a fix since as of now i don't know how to allocate a[x][y] {x,y are variables}
        static bool occupancy_initialized=false;
        if(occupancy_initialized==false)
        {
            pixel_occupied[0]=new bool*[tile_imagewidth];
            for(unsigned i=0; i<tile_imagewidth; i++)
                pixel_occupied[0][i]=new bool[tile_imageheight];

            for(unsigned r_1=0; r_1<tile_imageheight; r_1++)
                for(unsigned c_1=0; c_1<tile_imagewidth; c_1++)
                    pixel_occupied[0][c_1][r_1]=false;

            occupancy_initialized=true;

        }

        if(pixel_occupied[0][col][row]==true)
        {

            blend_value=0;
        }
        else
        {
            blend_value=255;
            pixel_occupied[0][col][row]=true;

        }
        break;

    case AVERAGE:
        for(unsigned int n=0; n<num_of_projectors; n++)
        {
            if(active_projectors[n]==true)
                if(region[n][col][row]==true)
                    count++;
        }
        blend_value=255/count;
        break;

    case DIST_TRANSFORM:
        //find the nearest boundary of projector i for pixel(c,r)
        /// A----B
        /// |    |
        /// |    |
        /// C----D
        float distance[4];
        /*unsigned choice;
        //Distance from A-B
        printf("\n1. Distance from bounding box boundaries\n");
        printf("\n2. Distance from nearest edge of projector quad(along one axis)\n");
        printf("\n3. Perpendicular distance from");
        scanf("%u",&choice);

        if(choice==1)
        {
            distance[0]=row-proj_bounding_box_y;

            // Distance from A-C
            distance[1]=col-proj_bounding_box_x;

            // Distance from C-D
            distance[2]=proj_bounding_box_y+proj_bounding_box_height-row;

            // Distance from B-D
            distance[3]=proj_bounding_box_x+proj_bounding_box_width-col;
        }

        */
        // if(choice==3)
        //  {
        for(unsigned i=0; i<4; i++)
            distance[i]=abs(m[proj_id][i]*col-row+c[proj_id][i])/sqrt(powf(m[proj_id][i],2.0)+1.0f);
        //  }

        min_dist=(proj_bounding_box_width>proj_bounding_box_height)?proj_bounding_box_width+1.0f:proj_bounding_box_height+1.0f;

        for(unsigned i=0; i<4; i++)
            if(distance[i]<min_dist)
                min_dist=distance[i];

        // assign alpha=d1/(sum(di)) where i:overlapping projectors at that point
        blend_value=min_dist;
        break;

    default:
        printf("\nInvalid blending function specified!! exiting...");
        exit(0);

    }

    return blend_value;
}

// TODO
void configure_nvidia_blend()
{
}



int main()
{
    FILE*fd_active_projector_map=fopen("../active_projector_map.txt","r");


    unsigned k;

    for(unsigned n=0; n<num_of_projectors; n++)
    {
        fscanf(fd_active_projector_map,"%u ",&k);
        active_projectors[n]=(k==1)?true:false;
    }

    char filename[500];
    unsigned int proj_bounding_box[num_of_projectors][4]; // it will store chromium tile configuration for each projector
    unsigned char (*alpha)[proj_imagewidth][proj_imageheight];
    alpha=new unsigned char[num_of_projectors][proj_imagewidth][proj_imageheight];
    float (*alpha_weight)[proj_imagewidth][proj_imageheight];
    alpha_weight=new float[num_of_projectors][proj_imagewidth][proj_imageheight];

    // read chromium configuration
    for(unsigned int n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
        {
            sprintf(filename,"../chromium_proj_%d.txt",n);
            FILE*fd_chromium_config=fopen(filename,"r");
            fscanf(fd_chromium_config,"%u  %u  %u  %u\n",&proj_bounding_box[n][0],&proj_bounding_box[n][1],&proj_bounding_box[n][2],&proj_bounding_box[n][3]);
            printf("\nProj_%d:%u\t%u\t%u\t%u\n",n,proj_bounding_box[n][0],proj_bounding_box[n][1],proj_bounding_box[n][2],proj_bounding_box[n][3]);

            fclose(fd_chromium_config);
        }
    }

//determine rowXcol configuration of projectors
    unsigned active_num_of_rows=0,active_num_of_col=0;
    unsigned row_count[num_of_proj_along_column]; //maintains the number of rows for each column
    unsigned col_count[num_of_proj_along_row]; //maintains the number of columns for each row

    unsigned row_id,col_id;

    for(unsigned i=0; i<num_of_proj_along_column; i++)
        row_count[i]=0;

    for(unsigned i=0; i<num_of_proj_along_row; i++)
        col_count[i]=0;

    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
        {
            row_id=n/num_of_proj_along_column;
            col_id=n%num_of_proj_along_row;
            row_count[col_id]++;
            col_count[row_id]++;
        }

    }


    for(unsigned i=0; i<num_of_proj_along_column; i++)
    {
        if(active_num_of_rows<row_count[i])
            active_num_of_rows=row_count[i];
    }

    for(unsigned i=0; i<num_of_proj_along_row; i++)
    {
        if(active_num_of_col<col_count[i])
            active_num_of_col=col_count[i];
    }

    printf("Number of rows:%u\t Number of columns:%u",active_num_of_rows,active_num_of_col);

    unsigned i=0;

    while(active_projectors[i]==0 && i<num_of_projectors)
    i++;


    unsigned tile_imagewidth=proj_bounding_box[i+active_num_of_col-1][0]+proj_bounding_box[i+active_num_of_col-1][2]-proj_bounding_box[i][0];
    unsigned tile_imageheight=proj_bounding_box[i+(active_num_of_rows-1)*num_of_proj_along_column][1]+proj_bounding_box[i+(active_num_of_rows-1)*num_of_proj_along_column][3]-proj_bounding_box[i][1];

    printf("\nTilewidth:%u\tTileheight:%u\n",tile_imagewidth,tile_imageheight);
    // Apply mask on later projector if there is an overlap with preceding projector.
    /// NOTE : Perchannel blend mask has 8-bit width
    unsigned choice;
    printf("\nApply blending?");
    scanf("%u",&choice);
    unsigned int method; //blending method
    float detected_corners[num_of_projectors][total_features][2];
    if(choice==1)
    {
        //1. Create image with resolution tightly encompassing all projector tiles.
        unsigned boundary[num_of_projectors];

        for(unsigned i=0; i<num_of_projectors; i++)
        {
            if(active_projectors[i]==true)
            {
                region[i]=new bool*[tile_imagewidth];

                for(unsigned j=0; j<tile_imagewidth; j++)
                    region[i][j]=new bool[tile_imageheight];
            }
        }



        //2. Map all detected corners for each projector to chromium global texture space.
        // Read all detected corners & Multiply coordinates with scale factor to get coordinates in chromium global texture space.


        //4. Create boundary array such that boundary[k]=1 iff boundary of kth-projector quad has been visited along current row.
        //Create region_array[i][j][k]=1 if pixel(i,j) is in projection region of projector k

        //inititalize arrays
        for(unsigned int n=0; n<num_of_projectors; n++)
        {
            if(active_projectors[n==true])
            {
                boundary[n]=0;
                for(unsigned int r=0; r<tile_imageheight; r++)
                    for(unsigned int c=0; c<tile_imagewidth; c++)
                        region[n][c][r]=false;
            }
        }



        /// NOTE: Detected corners are in common(global bounding box) coordinate system in camera image.
        for(unsigned int n=0; n<num_of_projectors; n++)
        {
            if(active_projectors[n]==true)
            {
                sprintf(filename,"global_box_detected_corners_%d.txt",n);
                FILE* fd_detected_corners=fopen(filename,"r");
                for(unsigned int i=0; i<total_features; i++)
                    fscanf(fd_detected_corners,"%f %f\n",&detected_corners[n][i][0],&detected_corners[n][i][1]);



                /// Instead, compute equation of straight line & mark all points along increasing x
                /// A----B (Assumed order of detected corners)
                /// |    |
                /// |    |
                /// C----D


                //Lets draw the boundaries of individual projection regions in 'region' array.

                int x_compute,y_compute;


                /// A-B
                // from smaller x to larger x
                unsigned int smaller_x,larger_x;
                smaller_x=detected_corners[n][0][0];
                larger_x=detected_corners[n][total_feature_X-1][0];
                if(smaller_x>larger_x)
                {
                    printf("Error@1!!!");
                    fflush(stdout);
                    exit(0);
                }


                if(smaller_x<proj_bounding_box[n][0])
                    smaller_x=proj_bounding_box[n][0];

                if(larger_x>=(proj_bounding_box[n][0]+proj_bounding_box[n][2]))
                    larger_x=proj_bounding_box[n][0]+proj_bounding_box[n][2]-1;




                for(unsigned int x=smaller_x; x<larger_x; x++)
                {
                    // increase x by 1, compute corresponding y
                    m[n][0]=(detected_corners[n][total_feature_X-1][1]-detected_corners[n][0][1])/(detected_corners[n][total_feature_X-1][0]-detected_corners[n][0][0]);
                    c[n][0]=detected_corners[n][total_feature_X-1][1]-m[n][0]*detected_corners[n][total_feature_X-1][0];// c=y1-mx1 or c=y2-mx2
                    y_compute=m[n][0]*x+c[n][0];

                    if(y_compute<proj_bounding_box[n][1])
                    {
                        y_compute=proj_bounding_box[n][1];
                    }

                    else if(y_compute>=(proj_bounding_box[n][1]+proj_bounding_box[n][3]))
                    {
                        y_compute=proj_bounding_box[n][1]+proj_bounding_box[n][3]-1;
                    }

                    region[n][x][y_compute]=true;
                }


                /// C-D
                smaller_x=detected_corners[n][total_feature_X*(total_feature_Y-1)][0];
                larger_x=detected_corners[n][total_feature_X*total_feature_Y-1][0];
                if(smaller_x>larger_x)
                {
                    printf("Error@2!!!");
                    fflush(stdout);
                    exit(0);
                }


                if(smaller_x<proj_bounding_box[n][0])
                    smaller_x=proj_bounding_box[n][0];

                if(larger_x>=(proj_bounding_box[n][0]+proj_bounding_box[n][2]))
                    larger_x=proj_bounding_box[n][0]+proj_bounding_box[n][2]-1;


                for(unsigned int x=smaller_x; x<larger_x; x++)
                {
                    // increase x by 1, compute corresponding y
                    m[n][1]=(detected_corners[n][total_feature_X*total_feature_Y-1][1]-detected_corners[n][total_feature_X*(total_feature_Y-1)][1])/(detected_corners[n][total_feature_X*total_feature_Y-1][0]-detected_corners[n][total_feature_X*(total_feature_Y-1)][0]);
                    c[n][1]=detected_corners[n][total_feature_X*total_feature_Y-1][1]-m[n][1]*detected_corners[n][total_feature_X*total_feature_Y-1][0];// c=y1-mx1 or c=y2-mx2
                    y_compute=m[n][1]*x+c[n][1];

                    if(y_compute<proj_bounding_box[n][1])
                    {
                        y_compute=proj_bounding_box[n][1];
                    }

                    else if(y_compute>=(proj_bounding_box[n][1]+proj_bounding_box[n][3]))
                    {
                        y_compute=proj_bounding_box[n][1]+proj_bounding_box[n][3]-1;
                    }

                    region[n][x][y_compute]=true;
                }


                /// A-C
                unsigned int smaller_y=detected_corners[n][total_feature_X*(total_feature_Y-1)][1];
                unsigned int larger_y=detected_corners[n][0][1];
                if(smaller_y>larger_y)
                {
                    printf("Error@3!!!");
                    fflush(stdout);
                    exit(0);
                }

                if(smaller_y<proj_bounding_box[n][1])
                    smaller_y=proj_bounding_box[n][1];

                if(larger_y>=(proj_bounding_box[n][1]+proj_bounding_box[n][3]))
                    larger_y=proj_bounding_box[n][1]+proj_bounding_box[n][3]-1;




                for(unsigned int y=smaller_y; y<larger_y; y++)
                {
                    // increase y by 1, compute corresponding x
                    m[n][2]=(detected_corners[n][total_feature_X*(total_feature_Y-1)][1]-detected_corners[n][0][1])/(detected_corners[n][total_feature_X*(total_feature_Y-1)][0]-detected_corners[n][0][0]);
                    c[n][2]=detected_corners[n][total_feature_X*(total_feature_Y-1)][1]-m[n][2]*detected_corners[n][total_feature_X*(total_feature_Y-1)][0];// c=y1-mx1 or c=y2-mx2
                    x_compute=(y-c[n][2])/m[n][2];
                    if(x_compute<proj_bounding_box[n][0])
                        x_compute=proj_bounding_box[n][0];

                    else if(x_compute>=(proj_bounding_box[n][0]+proj_bounding_box[n][2]))
                        x_compute=proj_bounding_box[n][0]+proj_bounding_box[n][2]-1;

                    region[n][x_compute][y]=true;

                }


                /// B-D
                smaller_y=detected_corners[n][total_feature_X*total_feature_Y-1][1];
                larger_y=detected_corners[n][total_feature_X-1][1];

                if(smaller_y>larger_y)
                {
                    printf("Error@4!!!");
                    fflush(stdout);
                    exit(0);
                }

                if(smaller_y<proj_bounding_box[n][1])
                    smaller_y=proj_bounding_box[n][1];

                if(larger_y>=(proj_bounding_box[n][1]+proj_bounding_box[n][3]))
                    larger_y=proj_bounding_box[n][1]+proj_bounding_box[n][3]-1;


                for(unsigned int y=smaller_y; y<larger_y; y++)
                {
                    // increase x by 1, compute corresponding y
                    m[n][3]=(detected_corners[n][total_feature_X*total_feature_Y-1][1]-detected_corners[n][total_feature_X-1][1])/(detected_corners[n][total_feature_X*total_feature_Y-1][0]-detected_corners[n][total_feature_X-1][0]);
                    c[n][3]=detected_corners[n][total_feature_X*total_feature_Y-1][1]-m[n][3]*detected_corners[n][total_feature_X*total_feature_Y-1][0];// c=y1-mx1 or c=y2-mx2
                    x_compute=(y-c[n][3])/m[n][3];
                    if(x_compute<proj_bounding_box[n][0])
                        x_compute=proj_bounding_box[n][0];

                    else if(x_compute>=(proj_bounding_box[n][0]+proj_bounding_box[n][2]))
                        x_compute=proj_bounding_box[n][0]+proj_bounding_box[n][2]-1;

                    region[n][x_compute][y]=true;
                }
            }
        }


        IplImage*proj_bound_image=cvCreateImage(cvSize(tile_imagewidth,tile_imageheight),IPL_DEPTH_8U,1);
        proj_bound_image->origin=1;
        for(unsigned n=0; n<num_of_projectors; n++)
        {
            if(active_projectors[n]==true)
            {
                cvZero(proj_bound_image);
                for(unsigned r=0; r<tile_imageheight; r++)
                    for(unsigned c=0; c<tile_imagewidth; c++)
                        proj_bound_image->imageData[r*proj_bound_image->widthStep+c]=(region[n][c][r]==true)?255:0;

                sprintf(filename,"projection_boundary_%u.bmp",n);
                cvSaveImage(filename,proj_bound_image);

            }
        }



        //5. Scan the complete chromium global texture image row wise and fill the arrays(after every row scan set boundary[k]=0 for all projectors k)
        /// Filling the marked polygons.
        unsigned int set_x[num_of_projectors];
        for(unsigned int r=0; r<tile_imageheight; r++)
        {
            for(unsigned int n=0; n<num_of_projectors; n++)
                if(active_projectors[n]==true)
                    set_x[n]=tile_imagewidth; //invalid value

            for(unsigned int c=0; c<tile_imagewidth; c++)
            {
                for(unsigned int n=0; n<num_of_projectors; n++)
                {
                    if(active_projectors[n]==true)
                    {
                        if(boundary[n]==0) // means we have not seen proj'n quad along this row
                        {
                            if(region[n][c][r]==true) // if it is starting of quad(for the 1st time)
                            {
                                boundary[n]=1; // we have entered in nth projector boundary.
                            }
                            else
                                continue; //proj n does not have projection region here.
                        }
                        else // if boundary == true
                        {
                            if(boundary[n]==1) // already inside the projection region
                            {
                                if(region[n][c][r]==true)
                                {
                                    boundary[n]=2; //this marks the end of boundary
                                    set_x[n]=c;
                                }

                                else //region=false
                                {
                                    region[n][c][r]=true;
                                }
                                continue;
                            }


                            if(boundary[n]==2) //already past one boundary set along this row
                            {

                                if(region[n][c][r]==true)
                                {
                                    //lets see if we can find corresponding valid boundary end

                                    unsigned int d=c;
                                    while(d>set_x[n])
                                    {
                                        region[n][d][r]=true;
                                        d--;
                                    }

                                    set_x[n]=c;
                                }


                                continue;
                            }

                            if(boundary[n]==3) //projector discarded from further boundary searching considerations for this row
                                continue;
                        }
                    }
                }
            }
            for(unsigned proj_id=0; proj_id<num_of_projectors; proj_id++)
                if(active_projectors[proj_id]==true)
                    boundary[proj_id]=0;
        }



        /// Lets save region array as image for verifying accuracy of computed alpha maps:
        IplImage*proj_image=cvCreateImage(cvSize(tile_imagewidth,tile_imageheight),IPL_DEPTH_8U,1);
        proj_image->origin=1;
        for(unsigned n=0; n<num_of_projectors; n++)
        {
            if(active_projectors[n]==true)
            {
                cvZero(proj_image);
                for(unsigned r=0; r<tile_imageheight; r++)
                    for(unsigned c=0; c<tile_imagewidth; c++)
                        proj_image->imageData[r*proj_image->widthStep+c]=(region[n][c][r]==true)?255:0;

                sprintf(filename,"projection_region_%u.bmp",n);
                cvSaveImage(filename,proj_image);
            }
        }


        //6. Now for each projector iterate in its 1024X768 space:
        // Set alpha[k][c][r]=1 for pixels with region_array[i][j][k]=1 only for a 'k'{single projector region}
        // Modify alpha[k][c][r]=h{depending upon blending function} for pixels where more than 1 projectors overlap.

        printf("\nPlease enter(numeric) blending method to be used:\n");
        printf("0:Black out\n1:Average weight\n2:Distance transform\n");
        scanf("%u",&method);


        for(unsigned int n=0; n<num_of_projectors; n++)
        {
            if(active_projectors[n]==true)
            {
                for(unsigned int r=0; r<proj_imageheight; r++)
                    for(unsigned int c=0; c<proj_imagewidth; c++)
                    {
                        alpha[n][c][r]=0;
                        alpha_weight[n][c][r]=0.0;
                    }
                for(unsigned int r=0; r<proj_imageheight; r++)
                    for(unsigned int c=0; c<proj_imagewidth; c++)
                    {
                        if((region[n][c+proj_bounding_box[n][0]][r+proj_bounding_box[n][1]]==true) && (no_one_else_in_the_region(c+proj_bounding_box[n][0],r+proj_bounding_box[n][1],n))) // true if no projector other than 'n' is at pixel (c,r) in global texture
                        {
                            alpha_weight[n][c][r]=255.0f; /// NOTE: proj[n].x,proj[n].y represent the starting (x,y) for projector 'n' tile.
                        }

                        else
                        {
                            if(region[n][c+proj_bounding_box[n][0]][r+proj_bounding_box[n][1]]==false)
                                continue;
                            else
                                alpha_weight[n][c][r]=blending_function(method,n,proj_bounding_box[n][0],proj_bounding_box[n][1],proj_bounding_box[n][2],proj_bounding_box[n][3],c+proj_bounding_box[n][0],r+proj_bounding_box[n][1],tile_imagewidth,tile_imageheight); // blending value for projector n at pixel (c,r)
                        }

                    }
            }
        }
    }

    else // no blending
    {
        for(unsigned int n=0; n<num_of_projectors; n++)
            if(active_projectors[n]==true)
                for(unsigned int r=0; r<proj_imageheight; r++)
                    for(unsigned int c=0; c<proj_imagewidth; c++)
                        alpha[n][c][r]=(unsigned char)255;
    }


///Lets revise alpha map for blending methods requiring mutual contributions from neighbouring projectors
    if(method==DIST_TRANSFORM)
    {
        //we simply need to calculate denominator of blending weight
        float alpha_denominator;
        unsigned x,y;

        // denominator calculation

        for(unsigned i=0; i<tile_imageheight; i++)
            for(unsigned j=0; j<tile_imagewidth; j++)
            {
                alpha_denominator=0.0f;
                for(unsigned n=0; n<num_of_projectors; n++)
                {
                    if(active_projectors[n]==true)
                        if(region[n][j][i]==true)
                        {
                            x=j-proj_bounding_box[n][0];
                            y=i-proj_bounding_box[n][1];
                            alpha_denominator+=alpha_weight[n][x][y];
                        }
                }
                // alpha modification
                if(alpha_denominator!=0.0f)
                {

                    for(unsigned n=0; n<num_of_projectors; n++)
                    {
                        if(active_projectors[n]==true)
                        {
                            if(region[n][j][i]==true)
                            {
                                if(no_one_else_in_the_region(j,i,n))
                                    alpha[n][j-proj_bounding_box[n][0]][i-proj_bounding_box[n][1]]=(unsigned char)255;

                                else
                                {
                                    alpha_weight[n][j-proj_bounding_box[n][0]][i-proj_bounding_box[n][1]]/=alpha_denominator;
                                    if(alpha_weight[n][j-proj_bounding_box[n][0]][i-proj_bounding_box[n][1]]>1.0f)
                                        printf("PROBLEM!!");
                                    alpha[n][j-proj_bounding_box[n][0]][i-proj_bounding_box[n][1]]=(unsigned char)(pow(alpha_weight[n][j-proj_bounding_box[n][0]][i-proj_bounding_box[n][1]],1.0/2.6f)*255.0f);
                                }
                            }
                        }
                    }
                }
            }
    }

    if(method==BLACK_OUT || method==AVERAGE)
    {
        for(unsigned int n=0; n<num_of_projectors; n++)
            if(active_projectors[n]==true)
                for(unsigned int r=0; r<proj_imageheight; r++)
                    for(unsigned int c=0; c<proj_imagewidth; c++)
                        alpha[n][c][r]=(unsigned char)alpha_weight[n][c][r];
    }





/// Lets do one provision to clip the region from the overall projection region so that overall output is rectangular.

    unsigned AB[num_of_projectors],CD[num_of_projectors],AC[num_of_projectors],BD[num_of_projectors];

    for(unsigned i=0; i<num_of_projectors; i++)
    {
        AB[i]=0;
        CD[i]=0;
        AC[i]=0;
        BD[i]=0;
    }

    unsigned id;
    for(unsigned i=0; i<num_of_projectors; i++)
    {
        if(active_projectors[i]==true) //just search for the 1st active projector.
        {
            /// AB
            id=i+(active_num_of_rows-1)*num_of_proj_along_column;
            for(; id<(i+(active_num_of_rows-1)*num_of_proj_along_column+active_num_of_col); id++)
                AB[id]=1;

            /// CD
            id=i;
            for(; id<i+active_num_of_col; id++)
                CD[id]=1;

            /// AC
            id=i;
            for(; id<=(i+(active_num_of_rows-1)*num_of_proj_along_column); id+=num_of_proj_along_column)
                AC[id]=1;

            /// BD
            id=i+active_num_of_col-1;
            for(; id<(i+(active_num_of_rows-1)*num_of_proj_along_column+active_num_of_col); id+=num_of_proj_along_column)
                BD[id]=1;

            break; //job done
        }
    }


    unsigned min_y;
    min_y=tile_imageheight;
/// Clipping along AB
    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
            if(AB[n]==true)
            {
                printf("\nProj_id:%u is along AB\n",n);
                if(min_y>detected_corners[n][0][1])
                    min_y=detected_corners[n][0][1];

                if(min_y>detected_corners[n][total_feature_X-1][1])
                    min_y=detected_corners[n][total_feature_X-1][1];
            }

    }

//lets remove all the region with y>min_y for all AB[n]==true.
    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
            if(AB[n]==true)
            {
                for(unsigned r=0; r<proj_imageheight; r++)
                    for(unsigned c=0; c<proj_imagewidth; c++)
                        if((proj_bounding_box[n][1]+r)>min_y)
                            alpha[n][c][r]=0;
            }

    }



    /// Clipping along CD
    unsigned max_y=0;
    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
            if(CD[n]==true)
            {
                printf("\nProj_id:%u is along CD\n",n);
                if(max_y<detected_corners[n][(total_feature_Y-1)*total_feature_X][1])
                    max_y=detected_corners[n][(total_feature_Y-1)*total_feature_X][1];

                if(max_y<detected_corners[n][(total_feature_Y-1)*total_feature_X+total_feature_X-1][1])
                    max_y=detected_corners[n][(total_feature_Y-1)*total_feature_X+total_feature_X-1][1];
            }

    }

    //lets remove all the region with y>min_y for all AB[n]==true.
    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
            if(CD[n]==true)
            {
                for(unsigned r=0; r<proj_imageheight; r++)
                    for(unsigned c=0; c<proj_imagewidth; c++)
                        if((proj_bounding_box[n][1]+r)<max_y)
                            alpha[n][c][r]=0;
            }

    }

/// Clipping along AC
    unsigned max_x=0;
    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
            if(AC[n]==true)
            {
                printf("\nProj_id:%u is along AC\n",n);
                if(max_x<detected_corners[n][0][0])
                    max_x=detected_corners[n][0][0];

                if(max_x<detected_corners[n][(total_feature_Y-1)*total_feature_X][0])
                    max_x=detected_corners[n][(total_feature_Y-1)*total_feature_X][0];
            }

    }

    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
            if(AC[n]==true)
            {
                for(unsigned r=0; r<proj_imageheight; r++)
                    for(unsigned c=0; c<proj_imagewidth; c++)
                        if((proj_bounding_box[n][0]+c)<max_x)
                            alpha[n][c][r]=0;
            }

    }


/// Clipping along BD
    unsigned min_x=tile_imagewidth;
    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
            if(BD[n]==true)
            {
                printf("\nProj_id:%u is along BD\n",n);
                fflush(stdout);
                if(min_x>detected_corners[n][total_feature_X-1][0])
                    min_x=detected_corners[n][total_feature_X-1][0];

                if(min_x>detected_corners[n][(total_feature_Y-1)*total_feature_X+total_feature_X-1][0])
                    min_x=detected_corners[n][(total_feature_Y-1)*total_feature_X+total_feature_X-1][0];
            }

    }

    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
            if(BD[n]==true)
            {
                for(unsigned r=0; r<proj_imageheight; r++)
                    for(unsigned c=0; c<proj_imagewidth; c++)
                        if((proj_bounding_box[n][0]+c)>min_x)
                            alpha[n][c][r]=0;
            }

    }

//Lets write the alpha map:
    for(unsigned int n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
        {
            sprintf(filename,"alpha_map_%d.txt",n);
            FILE*fd_alpha_map=fopen(filename,"w");
            fprintf(fd_alpha_map,"%u %u\n",proj_imagewidth,proj_imageheight);
            for(unsigned int r=0; r<proj_imageheight; r++)
            {
                for(unsigned int c=0; c<proj_imagewidth; c++)
                    fprintf(fd_alpha_map,"%u ",alpha[n][c][r]);

            }

            fclose(fd_alpha_map);
        }
    }


    /// Lets save region array as image for verifying accuracy of computed alpha maps:
    IplImage*alpha_image=cvCreateImage(cvSize(proj_imagewidth,proj_imageheight),IPL_DEPTH_8U,1);
    alpha_image->origin=1;
    for(unsigned n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
        {
            cvZero(alpha_image);
            for(unsigned r=0; r<proj_imageheight; r++)
                for(unsigned c=0; c<proj_imagewidth; c++)
                    alpha_image->imageData[r*alpha_image->widthStep+c]=(unsigned char)alpha[n][c][r];

            sprintf(filename,"alpha_map_%u.jpg",n);
            cvSaveImage(filename,alpha_image);

        }
    }





    //Lets send these mask to remote shared folder
    char command[500];

    FILE*fd_use_screen=fopen("../use_screen.txt","r");
    unsigned use_screen;
    fscanf(fd_use_screen,"%u",&use_screen);
    fclose(fd_use_screen);







    for(unsigned int n=0; n<num_of_projectors; n++)
    {
        if(active_projectors[n]==true)
        {
            command[0]='\0';
            if(use_screen==1)
            sprintf(command,"scp alpha_map_%u.txt demo@90.5.2.204:../../global/tile_specs/screen_space_registration/texture%u.txt.%uX%u",n,n+1,num_of_proj_along_column,num_of_proj_along_row);

            else
            sprintf(command,"scp alpha_map_%u.txt demo@90.5.2.204:../../global/tile_specs/camera_space_registration/texture%u.txt.%uX%u",n,n+1,num_of_proj_along_column,num_of_proj_along_row);

            system(command);


        }
    }


    // TODO: add function for NVIDIA Blend mask binding	
    configure_nvidia_blend();


    return 0;
}
