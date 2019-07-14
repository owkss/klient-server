QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = Server
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += c++11
SOURCES += \
        main.cpp \
        server.cpp
HEADERS += \
        datastruct.h \
        server.h

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    server.qrc
