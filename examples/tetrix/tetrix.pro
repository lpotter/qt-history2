# Project ID used by some IDEs
GUID 		= {89f911f6-6f75-4c74-9947-d5d18ff4977f}
TEMPLATE	= app
TARGET		= tetrix

CONFIG		+= qt warn_on release
DEPENDPATH	= ../../include

QTDIR_build:REQUIRES	= small-config

HEADERS		= gtetrix.h \
		  qdragapp.h \
		  qtetrix.h \
		  qtetrixb.h \
		  tpiece.h
SOURCES		= gtetrix.cpp \
		  qdragapp.cpp \
		  qtetrix.cpp \
		  qtetrixb.cpp \
		  tetrix.cpp \
		  tpiece.cpp
