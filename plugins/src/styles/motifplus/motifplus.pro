GUID 	 = {d730474b-dba1-4d12-a989-72cf1012c68e}
TEMPLATE = lib
TARGET	 = qmotifplusstyle

CONFIG  += qt warn_off release plugin
DESTDIR	 = ../../../styles

HEADERS		= ../../../../include/qmotifplusstyle.h
SOURCES		= main.cpp \
		  ../../../../src/styles/qmotifplusstyle.cpp

!contains(styles, motif) {
	HEADERS += ../../../../include/qmotifstyle.h
	SOURCES += ../../../../src/styles/qmotifstyle.cpp
}

unix:OBJECTS_DIR	= .obj
win32:OBJECTS_DIR	= obj

target.path += $$plugins.path/styles
INSTALLS += target
