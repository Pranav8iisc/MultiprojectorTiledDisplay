#include "gui_form.h"
#include <QtGui>
#include <QApplication>
#include <QPushButton>
#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>
#include <QPainter>
#include <QPen>
#include <QFileDialog>
#include <QTimer>

#include <QWidget>
#include <opencv/cv.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv/highgui.h>
#include <opencv2/video/video.hpp>

#include <iostream>
#include <stdio.h>
#include <string>

#include <QDataStream>
#include "ui_gui_form.h"

using namespace cv;
using namespace std;

//using cv::Mat;
using std::string;


gui_form::gui_form(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
	     
{
	ui.setupUi(this);
	
	QGroupBox *echoGroup = new QGroupBox(this);
	echoGroup->resize(400,300);
	echoGroup->move(700,100);
	echoGroup->setFont(QFont("Italic",10,QFont::Black));
	echoGroup->setTitle("Input Parameters Box");

	QGroupBox *echoGroup1 = new QGroupBox(this);
	echoGroup1->resize(500,500);
	echoGroup1->move(100,100);
	echoGroup1->setFont(QFont("Italic",10,QFont::Bold));
	echoGroup1->setTitle("Select Four Corner Points");
//	connect(); //TODO	

	QGroupBox *echoGroup2 = new QGroupBox(this);
	echoGroup2->resize(500,70);
	echoGroup2->move(100,20);
	echoGroup2->setFont(QFont("Italic",10,QFont::Bold));
	echoGroup2->setTitle("Adjust Camera Shutter Speed");
//	connect(); //TODO


	QGridLayout *echoLayout = new QGridLayout(this);
	QGridLayout *echoLayout1 = new QGridLayout(this);
	QGridLayout *echoLayout2 = new QGridLayout(this);

	imageLabel = new QLabel(this);
	imageLabel->setBackgroundRole(QPalette::Dark);
	imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	imageLabel->setScaledContents(true);
	imageLabel->setAutoFillBackground(true);

	imageLabel->update();
		
	echoLayout1->addWidget(imageLabel,0,0);
    
	QPushButton *but1 = new QPushButton("Feature Projection", this);
	but1->setFont(QFont("Italic",10,QFont::Bold));
	QPalette pal=but1->palette();
	pal.setColor(QPalette::Button,QColor(Qt::blue));
	but1->setAutoFillBackground(true);
	but1->setPalette(pal);
	but1->update();
//	connect(); //TODO
	//connect(slider, SIGNAL(valueChanged(int)),this,SLOT(display(int)));


	QPushButton *but2 = new QPushButton("Corner Detection", this);
	but2->setFont(QFont("Italic",10,QFont::Bold));
	pal.setColor(QPalette::Button,QColor(Qt::blue));
	but2->setAutoFillBackground(true);
	but2->setPalette(pal);
	but2->update();
//	connect(); //TODO


	QPushButton *but3 = new QPushButton("Blend Mask Generator", this);
	but3->setFont(QFont("Italic",10,QFont::Bold));
	pal.setColor(QPalette::Button,QColor(Qt::blue));
	but3->setAutoFillBackground(true);
	but3->setPalette(pal);
	but3->update();
//	connect(); //TODO


	QPushButton *but4 = new QPushButton("Apply Settings", this);
	but4->setFont(QFont("Italic",10,QFont::Bold));
	pal.setColor(QPalette::Button,QColor(Qt::blue));
	but4->setAutoFillBackground(true);
	but4->setPalette(pal);
	but4->update();
//	connect(); //TODO


	QRadioButton *radiobutton1 = new QRadioButton("method1", this);
	QRadioButton *radiobutton2 = new QRadioButton("method2", this);
	QRadioButton *radiobutton3 = new QRadioButton("method3", this);

	radiobutton1->setFont(QFont("Times",10,QFont::Bold));
	radiobutton2->setFont(QFont("Times",10,QFont::Bold));
	radiobutton3->setFont(QFont("Times",10,QFont::Bold));
//	connect(); //TODO
//	connect(); //TODO
//	connect(); //TODO
	

	QSlider *slider = new QSlider(Qt::Horizontal,0);
	slider->setRange(0,100);
	slider->setValue(0);
	QLabel *label = new QLabel("No Data");
	
    //echoLayout->addWidget(slider,0,1);
	echoLayout2->addWidget(slider,0,0);
	echoLayout->addWidget(but1,1,0);
	echoLayout->addWidget(but2,1,1);
	echoLayout->addWidget(but3,2,0);
	echoLayout->addWidget(radiobutton1,2,1);
	echoLayout->addWidget(radiobutton2,3,1);
	echoLayout->addWidget(radiobutton3,4,1);
	echoLayout->addWidget(but4,5,0);
	

	connect(but1, SIGNAL(clicked()), this, SLOT(DisableButton()));
	connect(but2, SIGNAL(clicked()), this, SLOT(UpdatePicture()));

	echoGroup->setLayout(echoLayout);
	echoGroup1->setLayout(echoLayout1);
	echoGroup2->setLayout(echoLayout2);
}

gui_form::~gui_form()
{
	
}

void gui_form::DisableButton()
{
	
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open File"), QDir::currentPath());
	
	//Mat image_Lit = cv::imread(fileName.toAscii().data());
	if(!fileName.isEmpty())
	{
		QImage image(fileName);
		if(image.isNull())
		{
			QMessageBox::information(this, tr("Image Viewer"),
									 tr("Cannot load it%l.").arg(fileName));
			return;
		}
		imageLabel->setAlignment(Qt::AlignCenter);//AlignTop);
		imageLabel->setPixmap(QPixmap::fromImage(image.scaled(imageLabel->size(),
			Qt::KeepAspectRatio,Qt::FastTransformation)));
	}
   
}

void gui_form::UpdatePicture()
{

	
	//VideoCapture cap("E:/PM.avi");
	
	VideoCapture cap(CV_CAP_DSHOW + 0);
	
	
	
	waitKey(10);
	if(!cap.isOpened())
		cout<<"Cannot open camera"<<endl;
	
	double fps = cap.get(CV_CAP_PROP_FPS);
	double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);

	timer = new QTimer(this);
	//connect(timer, SIGNAL(timeout()), this, SLOT(processFrameAndUpdateGUI()));
	connect(timer, SIGNAL(timeout()), this, SLOT(doNextFrame()));
	timer->start(20);

	cv::namedWindow("The Output image",CV_WINDOW_AUTOSIZE);
	Mat dest;
	while(true)
	{
		 
		bool bSuccess = cap.read(dest);
		
		if (!bSuccess)
		{
			cout << "Cannot read frame from video file" << endl;
			break;
		}
		
		imshow("The Output image",dest);
		if(waitKey(30)>=0)
		{
		break;
		}
		
		imageLabel->setAlignment(Qt::AlignCenter);
		imageLabel->setPixmap(QPixmap::fromImage(Mat2QImage(dest).scaled(imageLabel->size(),
													Qt::KeepAspectRatio,Qt::FastTransformation)));
		
		waitKey(0);
	}
	

	
}

void gui_form::processFrameAndUpdateGUI()
{
	cv::namedWindow("The Output image",CV_WINDOW_AUTOSIZE);
	
	while(true)
	{
		Mat dest; 
		bool bSuccess = cap.read(dest);
		
		if (!bSuccess)
		{
			cout << "Cannot read frame from video file" << endl;
			break;
		}
		
		imshow("The Output image",dest);
		if(waitKey(30)>=0)
		{
		break;
		}
	}
	
	

	
	/*
	cvtColor(dest,dest,CV_BGR2RGB);
	
	QImage image1 = QImage((uchar*) dest.data, dest.cols, dest.rows, dest.step, QImage::Format_RGB888);
	imageLabel->setAlignment(Qt::AlignCenter);//AlignTop);
		imageLabel->setPixmap(QPixmap::fromImage(image1.scaled(imageLabel->size(),
			Qt::KeepAspectRatio,Qt::FastTransformation)));
	*/
	

	
	


}


QImage gui_form::Mat2QImage(cv::Mat const& src)
{
	QImage dest(src.cols, src.rows, QImage::Format_ARGB32);
	const float scale = 255;
	if (src.depth() == CV_8U)
	{
		if (src.channels() == 1)
		{
			for (int i = 0; i < src.rows; ++i)
			{
				for (int j =0; j < src.cols; ++j)
				{
					int level = src.at<quint8>(i,j);
					dest.setPixel(j,i,qRgb(level,level,level));
				}
			}
		}
	
		else if (src.channels() == 3)
		{
			for (int i = 0; i < src.rows; ++i)
			{
				for (int j = 0; j < src.cols; ++j)
				{
					cv::Vec3b bgr = src.at<cv::Vec3b>(i,j);
					dest.setPixel(j, i, qRgb(bgr[2], bgr[1], bgr[0]));
				}
			}
		}
	}
	else if (src.depth() == CV_32F)
	{
		if (src.channels() == 1)
		{
			for (int i = 0; i < src.rows; ++i)
			{
				for (int j = 0; j < src.cols; ++j)
				{
					int level = scale * src.at<float>(i,j);
					dest.setPixel(j,i,qRgb(level,level,level));
				}
			}
		}
		else if (src.channels() == 3)
		{
			for (int i = 0; i < src.rows; ++i)
			{
				for (int j = 0; j < src.cols; ++j)
				{
					cv::Vec3f bgr = scale * src.at<cv::Vec3f>(i,j);
					dest.setPixel(j,i,qRgb(bgr[2],bgr[1],bgr[0]));
				}
			}
		}
	}
	return dest;
}

