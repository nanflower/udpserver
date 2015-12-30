#-------------------------------------------------
#
# Project created by QtCreator 2015-12-03T16:45:25
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = udpserver
TEMPLATE = app
INCLUDEPATH += /opt/intel/mediasdk/include

LIBS     += -lavcodec -lavfilter -lavformat -lavutil -lswresample -lswscale
LIBS +=  -L/usr/local/lib/ -lfaac -L/usr/local/

SOURCES += main.cpp\
        mainwindow.cpp \
    udp.cpp \
    udpsocket.cpp \
    tspoolqueue.cpp \
    transcodepool.cpp \
    audio_encode.cpp \
    pipeline_encode.cpp

HEADERS  += mainwindow.h \
    udp.h \
    udpsocket.h \
    tspoolqueue.h \
    transcodepool.h \
    audio_encode.h \
    pipeline_encode.h

FORMS    += mainwindow.ui
