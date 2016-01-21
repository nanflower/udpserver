#-------------------------------------------------
#
# Project created by QtCreator 2015-12-03T16:45:25
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = udpserver
TEMPLATE = app

QMAKE_CFLAGS += -I/usr/local/include -I$(MFX_HOME)/include
QMAKE_CXXFLAGS += -I/usr/local/include -I$(MFX_HOME)/include
QMAKE_LIBS += -L$(MFX_HOME)/lib/lin_x64 -lmfx -lva -lva-drm -lpthread -lrt -ldl
QMAKE_CXXFLAGS += -Wpointer-arith

INCLUDEPATH += /opt/intel/mediasdk/include
INCLUDEPATH += /usr/local/include
LIBS     += -lavcodec -lavfilter -lavformat -lavutil -lswresample -lswscale
LIBS +=  -L/usr/local/lib/ -lfaac -L/usr/local/


SOURCES += main.cpp\
        mainwindow.cpp \
    udp.cpp \
    udpsocket.cpp \
    tspoolqueue.cpp \
    transcodepool.cpp \
    audio_encode.cpp \
    pipeline_encode.cpp \
    one_process.cpp \
    base_allocator.cpp \
    vaapi_device.cpp \
    vaapi_utils.cpp \
    vaapi_utils_drm.cpp \
    utils.cpp \
    sysmem_allocator.cpp \
    outudppool.cpp \
    pipeline_decode.cpp \
    decodepool.cpp \
    plugin_utils.cpp \
    mfx_buffering.cpp \
    time_def.cpp \
    thread_defs.cpp \
    atomic_defs.cpp \
    thread_defs2.cpp

HEADERS  += mainwindow.h \
    udp.h \
    udpsocket.h \
    tspoolqueue.h \
    transcodepool.h \
    audio_encode.h \
    pipeline_encode.h \
    one_process.h \
    base_allocator.h \
    vaapi_device.h \
    vaapi_utils.h \
    vaapi_utils_drm.h \
    hw_device.h \
    utils.h \
    sysmem_allocator.h \
    global.h \
    values.h \
    outudppool.h \
    sparams.h \
    pipeline_decode.h \
    decodepool.h \
    sample_defs.h \
    atomic_defs.h \
    file_defs.h \
    so_defs.h \
    strings_defs.h \
    thread_defs.h \
    time_defs.h \
    sample_params.h \
    plugin_utils.h \
    sample_types.h \
    avc_structures.h \
    plugin_loader.h \
    mfx_buffering.h

FORMS    += mainwindow.ui
