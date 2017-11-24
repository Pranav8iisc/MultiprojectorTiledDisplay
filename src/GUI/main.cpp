#include <QtGui/QApplication>
#include <QtGui>
#include <QtGui/QWidget>
#include "gui_form.h"
#include "ui_gui_form.h"



int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	gui_form w;
	w.setWindowTitle("Multiprojector tiled display calibration tool");
	//w.show();
	
	w.showMaximized();
	return a.exec();
}
