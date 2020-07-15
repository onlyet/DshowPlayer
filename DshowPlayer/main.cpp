#include "DshowPlayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DshowPlayer w;
    w.show();
	//w.showFullScreen();
    return a.exec();
}
