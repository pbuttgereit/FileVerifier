#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("FileVerifier");
    a.setApplicationVersion("0.1");
    a.setApplicationDisplayName("FileVerifier");
    a.setOrganizationName("FileVerifier");
    MainWindow w;
    w.show();

    return a.exec();
}
