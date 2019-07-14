#include "server.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Server serverWindow;

    serverWindow.resize(200, 400);
    serverWindow.show();

    return a.exec();
}
