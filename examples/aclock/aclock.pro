TEMPLATE	= app
TARGET		= aclock

CONFIG		+= qt warn_on
DEPENDPATH	= ../../include

QTDIR_build:REQUIRES	= "contains(QT_CONFIG, full-config)"

HEADERS		= aclock.h
SOURCES		= aclock.cpp \
		  main.cpp
QT		+= compat
