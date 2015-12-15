#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "udp.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    udp udpsocket;
    udpsocket.Init();
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}
