#include <QtGui>
  
        int main(int argv, char **args)
        {
            QApplication app(argv, args);
  
            QTextEdit *textEdit = new QTextEdit;
            QPushButton *quitButton = new QPushButton("&Quit");
  
           QObject::connect(quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));
 
           QHBoxLayout *layout = new QHBoxLayout;
	    layout->addWidget(quitButton);
           layout->addWidget(textEdit); 
          
 
           QWidget window;
           window.setLayout(layout); // quitbutton,textEdit widgets are now child widgets for 'window'.
 
           window.show();
 
           return app.exec();
       }
