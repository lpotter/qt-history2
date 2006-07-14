# -*- Mode: makefile -*-
COMMON_FOLDER = $$PWD/../common
include(../arthurtester.pri)
TEMPLATE = app
TARGET = htmlgenerator
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = ../bin

CONFIG += console

QT += svg opengl xml

# Input
HEADERS += htmlgenerator.h
SOURCES += main.cpp htmlgenerator.cpp
