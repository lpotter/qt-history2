# Project ID used by some IDEs
GUID 	 = {0adfd13c-4563-474a-85d2-29d69e7f753f}
TEMPLATE = app
CONFIG	+= qt warn_on
LANGUAGE = C++

SOURCES	+= colorbutton.cpp main.cpp previewframe.cpp previewwidget.cpp mainwindow.cpp paletteeditoradvanced.cpp
HEADERS	+= colorbutton.h previewframe.h previewwidget.h mainwindow.h paletteeditoradvanced.h
FORMS	= mainwindowbase.ui paletteeditoradvancedbase.ui previewwidgetbase.ui
IMAGES	= images/appicon.png

PROJECTNAME	= Qt Configuration
TARGET		= qtconfig
DESTDIR		= ../../bin

target.path=$$bins.path
INSTALLS	+= target
INCLUDEPATH	+= .
DBFILE		 = qtconfig.db
