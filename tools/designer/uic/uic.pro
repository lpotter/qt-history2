TEMPLATE	= app
CONFIG		+= qt console warn_on release
HEADERS	= uic.h \
		  ../shared/widgetdatabase.h \
		  ../shared/domtool.h \
		  ../integration/kdevelop/kdewidgets.h \
		  ../shared/widgetinterface.h \
		  ../shared/widgetplugin.h
SOURCES	= uic.cpp  \
		  ../shared/widgetdatabase.cpp  \
		  ../shared/domtool.cpp \
		   ../integration/kdevelop/kdewidgets.cpp \
		  ../shared/widgetplugin.cpp

TARGET		= uic
INCLUDEPATH	= ../shared ../util ../../../src/3rdparty/zlib/
!zlib:LIBS      += -lz

unix:LIBS	+= -lqutil -L../lib
win32:LIBS	+= $(QTDIR)/lib/qutil.lib
DEFINES 	+= UIC
DESTDIR		= $(QTDIR)/bin
