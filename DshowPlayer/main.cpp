#include "DshowPlayer.h"
#include "log.h"
#include "Dump.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


	LogInit(a.applicationName(), a.applicationVersion());

	SetUnhandledExceptionFilter(ExceptionFilter);

    DshowPlayer w;
    w.show();
	//w.showFullScreen();
    return a.exec();
}
