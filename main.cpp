#include "mainwindow.h"
#include <QApplication>

#include "outudppool.h"

outudppool *g_pLoopListBuffer[16] = {0};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
