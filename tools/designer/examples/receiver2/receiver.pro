# Project ID used by some IDEs
GUID 	 = {b91341bf-59f0-43af-9f6b-553be109ebed}
TEMPLATE = app
LANGUAGE = C++
TARGET   = receiver

CONFIG	+= qt warn_on release
INCLUDEPATH	+= $$QT_SOURCE_TREE/tools/designer/uilib
LIBS	+= -lqui
SOURCES	+= main.cpp  receiver.cpp
HEADERS	+= receiver.h
FORMS	= mainform.ui
DBFILE	= receiver.db
IMAGEFILE	= images.cpp
