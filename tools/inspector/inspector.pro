# Project ID used by some IDEs
GUID 		= {2d461149-0632-4146-96d2-6c814087f7a9}
TEMPLATE	= app
CONFIG		+= qt warn_on release
TARGET		= inspector
HEADERS		= cppparser.h
SOURCES		= main.cpp cppparser.cpp
INTERFACES	= inspector.ui
DESTDIR		= ../../bin
LANGUAGE	= C++
