/*
TARGET: OpenGL application which will project features based on key event.
*/
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdlib.h>
#include<math.h>




#define num_of_proj 9
#define num_of_proj_in_row 3
#define num_of_proj_in_col 3
#define proj_imagewidth 1024
#define proj_imageheight 768

#define margin_X 32
#define margin_Y 32



static unsigned int proj_id=0;
static GLubyte checkImage[proj_imageheight][proj_imagewidth][4];
static GLuint texName;

void makeCheckImage(void)
{
    int i, j, c;

    for (i = 0; i < proj_imageheight; i++)
    {
        for (j = 0; j < proj_imagewidth; j++)
        {
            if((i<margin_Y) || (i>=(proj_imageheight-margin_Y)) || (j<margin_X) || (j>=(proj_imagewidth-margin_X)))
            {
                checkImage[i][j][0]=(GLubyte)255;
                checkImage[i][j][1]=(GLubyte)255;
                checkImage[i][j][2]=(GLubyte)255;
                checkImage[i][j][3]=(GLubyte)255;
                continue;
            }

            c = (((i&0b100000)==0)^((j&0b100000)==0))*255;
            checkImage[i][j][0] = (GLubyte) c;//(GLubyte)((unsigned char)checkerboard_image->imageData[i*checkerboard_image->widthStep+j]);//(GLubyte) c;
            checkImage[i][j][1] = (GLubyte) c;//(GLubyte)((unsigned char)checkerboard_image->imageData[i*checkerboard_image->widthStep+j]);//(GLubyte) c;
            checkImage[i][j][2] = (GLubyte) c;//(GLubyte)((unsigned char)checkerboard_image->imageData[i*checkerboard_image->widthStep+j]);//(GLubyte) c;
            checkImage[i][j][3] = (GLubyte) 255;
        }
    }

    return;
}




void init(void)
{
    // Setting screen background
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);

    makeCheckImage();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_2D, texName);

    //Setting OGL texture paramters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);

    // Coupling loaded image to 2D texture OGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, proj_imagewidth,
                 proj_imageheight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 checkImage);
return;
}


void display()
{
    float vertex_coordinates[4][2];
    float origin_x=0.0f,origin_y=0.0f;

//Compute vertex coordinates

        origin_x=-1.0f+(2.0f/num_of_proj_in_row)*(proj_id%num_of_proj_in_row);
        origin_y=-1.0f+(2.0f/num_of_proj_in_col)*(floor(proj_id/num_of_proj_in_row));
        unsigned int i;
        for(unsigned int vertex_id=0,i=0; vertex_id<4; vertex_id++)
        {
            if(vertex_id==2)
            i++;
            vertex_coordinates[vertex_id][0]=(2.0f/num_of_proj_in_row)*floor(vertex_id/2)+origin_x;
            vertex_coordinates[vertex_id][1]=(2.0f/num_of_proj_in_col)*(i%2)+origin_y;
            i++;

        }


        //Lets map the texture & vertex coordinates...
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBindTexture(GL_TEXTURE_2D, texName);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); //0,0
    glVertex2f(vertex_coordinates[0][0],vertex_coordinates[0][1]);
    glTexCoord2f(0.0, 1.0); // 0,1
    glVertex2f(vertex_coordinates[1][0],vertex_coordinates[1][1]);
    glTexCoord2f(1.0,1.0); // 1,1
    glVertex2f(vertex_coordinates[2][0],vertex_coordinates[2][1]);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(vertex_coordinates[3][0],vertex_coordinates[3][1]);
    glEnd();
    glFlush();
    glDisable(GL_TEXTURE_2D);

return;

}

void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);

}


void keyboard (unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27:
        exit(0);
        break;

    case 'n': // start projecting feature image on next projector
      proj_id++;
      glutPostRedisplay();
      break;

    default:
        break;
    }
}


int main(int argc, char** argv)
{
    // Generate texture
    // Generate window(Proj_imagewidth*n,Proj_imageheight*n)

    // Register key callback
    // Each key event will ask to project feature pattern on next projector
    //For a key event:
    // Generate vertex coordinates
    // Generate texture coordinates
    // Map the texture to the polygon
//Update vertex coordinates for next key event

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(proj_imagewidth*num_of_proj_in_row, proj_imageheight*num_of_proj_in_col);
    glutInitWindowSize(1024,768);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(argv[0]);

    init();
    glutDisplayFunc(display);
    glutFullScreen();
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();

    return 0;
}

