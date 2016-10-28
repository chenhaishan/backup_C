TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += -g
LIBS += -pthread
SOURCES += main.c \
    dxyh_lib.c \
    dxyh_thread_lib.c \
    error.c \
    ftpd.c \
    record.c

DISTFILES += \
    ftp_server.pro.user

HEADERS += \
    dxyh_thread.h \
    dxyh.h \
    error.h \
    ftpd.h \
    record.h
