#include "client.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon(":file.png"));
    QCoreApplication::setOrganizationName("NIIP");
    QCoreApplication::setApplicationName("Klient");
    QCoreApplication::setApplicationVersion("0.9.0");
    //QApplication::setQuitOnLastWindowClosed(false);

    Client w;

    w.setMinimumSize(500, 500);
    w.show();

    return a.exec();
}
