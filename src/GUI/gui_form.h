#ifndef TEST1_H
#define TEST1_H

#include <QtGui/QMainWindow>
#include "ui_gui_form.h"

#include <QRadioButton>
#include <QMessageBox>
#include <QPixmap>
#include <QtGui/QGroupBox>
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>
#include <QLabel>
#include <QPrinter>
#include <QMenu>
#include <QMenuBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QString>
#include <QSlider>
//#include "ui_test.h"
#include <QPushButton>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QImage>
#include <QMouseEvent>
#include <QPoint>
#include <QFile>
#include <QTextStream>
#include <fstream>
#include <QTimer>
#include <QGridLayout>
#include <QWidget>
#include <opencv/cv.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv/highgui.h>
#include <iostream>

#include <math.h>
#include <string>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>


using namespace std;
using namespace cv;

class gui_form : public QMainWindow
{
	Q_OBJECT

public:
	gui_form(QWidget *parent = 0, Qt::WindowFlags flags=0);
	~gui_form();

	

private:
	Ui::gui_formClass ui;
	//void openImage();
	//void createActions();
    //void createMenus();
	QSlider *slider;
	//void sliderValueChanged(int);
	QPushButton *but1, *but2;
	QLabel *imageLabel, *label;

	

	QMenu *fileMenu;
	QMenu *viewMenu;
	QMenu *helpMenu;
	QMenu *filterMenu;

    QScrollArea *scrollArea, *scrollArea1;
	QAction *openAct;
	QAction *printAct;
	QAction *zoomInAct;
	QAction *zoomOutAct;
	QAction *exitAct;
	QAction *fitToWindowAct;
	QAction *aboutAct;
	QAction *aboutQtAct;
	QAction *gaussianAct;
	QAction *lineAct;
    QPixmap input;
	
	QString fileName;

	double scaleFactor;
	Mat image_Lit;
	QImage Mat2QImage(cv::Mat const&);
	
	QImage image, image1;
	Mat image_test;
	cv::VideoCapture cap;
	QTimer *timer;

private slots:
	void DisableButton();
	void UpdatePicture();
	void processFrameAndUpdateGUI();
	virtual void doNextFrame()
	{
		repaint();
	}
signals:
	void mousePressed( const QPoint& );
	void mouseClickEvent();
	
};

#endif // TEST1_H
