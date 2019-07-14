QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = klient
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++11
SOURCES += \
        connection.cpp \
        main.cpp \
        client.cpp
HEADERS += \
        client.h \
        connection.h \
        datastruct.h
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    klient.qrc
