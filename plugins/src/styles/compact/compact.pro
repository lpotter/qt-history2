# Project ID used by some IDEs
GUID 	 = {a9cf91dd-ae5d-498c-82ed-1e467ce433ae}
TEMPLATE = lib
TARGET	 = qcompactstyle

CONFIG  += qt warn_off release plugin
DESTDIR	 = ../../../styles

HEADERS		= ../../../../include/qcompactstyle.h

SOURCES		= main.cpp \
		  ../../../../src/styles/qcompactstyle.cpp

!contains(styles, windows) {
	HEADERS += ../../../../include/qwindowsstyle.h
	SOURCES += ../../../../src/styles/qwindowsstyle.cpp
}

unix:OBJECTS_DIR	= .obj
win32:OBJECTS_DIR	= obj

target.path += $$plugins.path/styles
INSTALLS += target
