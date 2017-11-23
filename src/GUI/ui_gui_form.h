/********************************************************************************
** Form generated from reading UI file 'gui_form.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GUI_FORM_H
#define UI_GUI_FORM_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_gui_formClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *gui_formClass)
    {
        if (gui_formClass->objectName().isEmpty())
            gui_formClass->setObjectName(QString::fromUtf8("gui_formClass"));
        gui_formClass->resize(600, 400);
        menuBar = new QMenuBar(gui_formClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        gui_formClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(gui_formClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        gui_formClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(gui_formClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        gui_formClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(gui_formClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        gui_formClass->setStatusBar(statusBar);

        retranslateUi(gui_formClass);

        QMetaObject::connectSlotsByName(gui_formClass);
    } // setupUi

    void retranslateUi(QMainWindow *gui_formClass)
    {
        gui_formClass->setWindowTitle(QApplication::translate("gui_formClass", "gui_form", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class gui_formClass: public Ui_gui_formClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GUI_FORM_H
