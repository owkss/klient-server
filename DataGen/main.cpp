#include "datastruct.h"

#include <QFile>
#include <QDebug>
#include <QDataStream>
#include <QRandomGenerator>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QFile file("data.dat");
    file.open(QIODevice::Append);

    SData temp;
    QDataStream out(&file);

    QRandomGenerator rg;
    for (int i(0); i < 10000; ++i)
    {
        temp.literal = rg.bounded('A', 'Z');
        temp.x = rg.bounded(0, 65535);
        temp.y = rg.bounded(0, 65535);
        temp.length = rg.bounded(-999999, 999999);
        out << temp;
    }

    file.flush();
    file.close();

    qDebug() << "Complete!";
    return a.exec();
}
