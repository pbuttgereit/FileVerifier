////////////////////////////////////////////////////
//                                                //
// FileVerifier - Compare File Collections        //
//                                                //
// Authors:                                       //
//  Peter Buttgereit (Version 0.1)                //
//  https://github.com/pbuttgereit/FileVerifier   //
//                                                //
//  Bjoern Martensen (Version 0.2)                //
//  https://github.com/bmartensen/FileVerifier    //
//                                                //
// License: GNU LGPL Version 3 (see License.txt)  //
// Application icon: https://icons8.de/license    //
//                                                //
////////////////////////////////////////////////////

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("FileVerifier");

    MainWindow w;
    w.show();

    return a.exec();
}
