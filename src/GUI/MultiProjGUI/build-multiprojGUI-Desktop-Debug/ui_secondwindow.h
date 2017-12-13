/********************************************************************************
** Form generated from reading UI file 'secondwindow.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SECONDWINDOW_H
#define UI_SECONDWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QStatusBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_cshomoWindow
{
public:
    QWidget *centralwidget;
    QFrame *frame;
    QLabel *label;
    QSlider *horizontalSlider;
    QLabel *label_2;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QMenuBar *menubar;
    QMenu *menuCompute_camera_screen_homography;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *cshomoWindow)
    {
        if (cshomoWindow->objectName().isEmpty())
            cshomoWindow->setObjectName(QString::fromUtf8("cshomoWindow"));
        cshomoWindow->resize(800, 600);
        centralwidget = new QWidget(cshomoWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        frame = new QFrame(centralwidget);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setGeometry(QRect(70, 60, 531, 271));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(80, 40, 121, 17));
        horizontalSlider = new QSlider(centralwidget);
        horizontalSlider->setObjectName(QString::fromUtf8("horizontalSlider"));
        horizontalSlider->setGeometry(QRect(150, 370, 311, 29));
        horizontalSlider->setOrientation(Qt::Horizontal);
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(150, 350, 201, 17));
        pushButton = new QPushButton(centralwidget);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(220, 430, 99, 27));
        pushButton_2 = new QPushButton(centralwidget);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(350, 430, 99, 27));
        cshomoWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(cshomoWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 25));
        menuCompute_camera_screen_homography = new QMenu(menubar);
        menuCompute_camera_screen_homography->setObjectName(QString::fromUtf8("menuCompute_camera_screen_homography"));
        cshomoWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(cshomoWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        cshomoWindow->setStatusBar(statusbar);

        menubar->addAction(menuCompute_camera_screen_homography->menuAction());

        retranslateUi(cshomoWindow);

        QMetaObject::connectSlotsByName(cshomoWindow);
    } // setupUi

    void retranslateUi(QMainWindow *cshomoWindow)
    {
        cshomoWindow->setWindowTitle(QApplication::translate("cshomoWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("cshomoWindow", "Camera live view", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("cshomoWindow", "Adjust Camera Shutter Speed", 0, QApplication::UnicodeUTF8));
        pushButton->setText(QApplication::translate("cshomoWindow", "Capture", 0, QApplication::UnicodeUTF8));
        pushButton_2->setText(QApplication::translate("cshomoWindow", "Next", 0, QApplication::UnicodeUTF8));
        menuCompute_camera_screen_homography->setTitle(QApplication::translate("cshomoWindow", "Compute camera-screen homography", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class cshomoWindow: public Ui_cshomoWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SECONDWINDOW_H
