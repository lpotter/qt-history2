TEMPLATE = lib
CONFIG += qt
CONFIG += staticlib
QT += xml

TARGET = signalsloteditor
DEFINES += QT_SIGNALSLOTEDITOR_LIBRARY

DESTDIR = ../../../lib

INCLUDEPATH += ../../lib/sdk \
    ../../shared \
    ../../uilib \
    ../../lib/extension

HEADERS += default_membersheet.h signalsloteditor.h membersheet.h signalsloteditor_global.h

SOURCES += default_membersheet.cpp signalsloteditor.cpp

MOCABLE += signalsloteditor.cpp

include(../../sharedcomponents.pri)
