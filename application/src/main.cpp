#include "tfp/views/MainWindow.hpp"
#include <iostream>
#include <QApplication>


// ----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    QApplication application(argc, argv);
    tfp::MainWindow mainWindow;
    mainWindow.show();
    return application.exec();
}
