TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -pthread

SOURCES += main.c \
    myconio_lib.c \
    record.c \
    shell.c \
    ftp.c \
    login.c \
    dxyh_lib.c \
    error.c

HEADERS += \
    myconio.h \
    record.h \
    shell.h \
    ftp.h \
    login.h \
    dxyh.h \
    error.h
