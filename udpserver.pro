#-------------------------------------------------
#
# Project created by QtCreator 2015-12-03T16:45:25
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = udpserver
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    udp.cpp \
    udpsocket.cpp

HEADERS  += mainwindow.h \
    udp.h \
    udpsocket.h

FORMS    += mainwindow.ui
