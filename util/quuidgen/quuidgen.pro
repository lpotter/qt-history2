GUID		= {6c4242b7-f85c-4ad8-8ba5-0b296d594d08}
TEMPLATE	= app
CONFIG		+= qt warn_on release
HEADERS		= quuidgen.h
SOURCES		= main.cpp quuidgen.cpp
INTERFACES	= quuidbase.ui
TARGET		= quuidgen
unix:LIBS	+= -L/lib -luuid
DESTDIR		= ../../bin
