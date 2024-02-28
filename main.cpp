#include "server.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");
    QCoreApplication a(argc, argv);
    Server serv;
    return a.exec();
}
