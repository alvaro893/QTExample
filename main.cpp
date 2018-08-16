#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // for settings
    QCoreApplication::setOrganizationName("AlvaroInc");
    QCoreApplication::setOrganizationDomain("alvaroweb.fi");
    QCoreApplication::setApplicationName("Awesome App");

    MainWindow w;
    w.show();

    return a.exec();
}
