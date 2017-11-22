/*
This file will provide camera interface(Canon G7) to the concerned application.
*/



#include<stdio.h>
#include<math.h>
#include"opencv/cv.h"
#include"opencv/highgui.h"
#include<gphoto2/gphoto2-camera.h>
#include <jpeglib.h>
#include <jerror.h>
#include <string.h>
#include<stdlib.h>
#include<fcntl.h>
#include <cstring>
using std::string;

IplImage*camera_preview();
IplImage*camera_capture(char*);
void set_camera_control(const char*control_string,const char* val);



