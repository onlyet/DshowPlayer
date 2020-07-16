#include "DshowPlayer.h"
#include "log.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


	LogInit(a.applicationName(), a.applicationVersion());


    DshowPlayer w;
    w.show();
	//w.showFullScreen();
    return a.exec();
}
