TEMPLATE	= lib
CONFIG+= qt warn_on debug plugin

HEADERS		= ../../../../include/qaquastyle.h

SOURCES		= main.cpp \
		  ../../../../src/styles/qaquastyle.cpp

!contains(styles, windows) {
	HEADERS += ../../../../include/qwindowsstyle.h
	SOURCES += ../../../../src/styles/qwindowsstyle.cpp
}

unix:OBJECTS_DIR	= .obj
win32:OBJECTS_DIR	= obj

TARGET		= qaquastyle
DESTDIR		= ../../../styles

target.path=$$plugins.path/styles
isEmpty(target.path):target.path=$$QT_PREFIX/plugins/styles
INSTALLS += target
