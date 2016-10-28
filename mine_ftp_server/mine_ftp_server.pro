TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    error_handle.c \
    socket_rw_wrapper.c \
    thread_lib.c \
    record.c \
    ftpd.c

HEADERS += \
    error_handle.h \
    socket_rw_wrapper.h \
    thread_lib.h \
    record.h \
    ftpd.h
