TEMPLATE 	= app
TARGET		= demo

CONFIG		+= qt warn_off release
unix:LIBS	+= -lm
!iconview: DEFINES  += QT_INTERNAL_ICONVIEW
!workspace: DEFINES += QT_INTERNAL_WORKSPACE
!canvas: DEFINES    += QT_INTERNAL_CANVAS
INCLUDEPATH	+= .
DEPENDPATH	= ../../include

QT += compat
DEFINES += QT_COMPAT_WARNINGS

QTDIR_build:REQUIRES	= "contains(QT_CONFIG, full-config)"

HEADERS		= frame.h \
		  categoryinterface.h \
		  qthumbwheel.h \
                  display.h \
		  textdrawing/textedit.h \
		  textdrawing/helpwindow.h \
		  dnd/dnd.h \
		  dnd/styledbutton.h \
		  dnd/iconview.h \
		  dnd/listview.h \
		  i18n/i18n.h \
		  i18n/wrapper.h \
		  ../aclock/aclock.h
SOURCES		= frame.cpp \
		  qthumbwheel.cpp \
               	  display.cpp \
		  textdrawing/textedit.cpp \
		  textdrawing/helpwindow.cpp \
		  dnd/dnd.cpp \
		  dnd/styledbutton.cpp \
		  dnd/iconview.cpp \
		  dnd/listview.cpp \
		  i18n/i18n.cpp \
		  ../aclock/aclock.cpp \
		  main.cpp

FORMS		= dnd/dndbase.ui

include( ../../src/qt_professional.pri )

canvas {
    HEADERS 	+=graph.h \
		  qasteroids/toplevel.h \
		  qasteroids/view.h \
		  qasteroids/ledmeter.h
    SOURCES 	+=graph.cpp \
		  qasteroids/toplevel.cpp \
		  qasteroids/view.cpp \
		  qasteroids/ledmeter.cpp

    QT     += canvas
}

contains(QT_CONFIG, opengl) {
    HEADERS 	+=opengl/glworkspace.h \
		  opengl/glcontrolwidget.h \
		  opengl/gltexobj.h \
		  opengl/glbox.h \
		  opengl/glgear.h \
		  opengl/gllandscape.h \
		  opengl/fbm.h \
		  opengl/glinfo.h \
		  opengl/glinfotext.h
    SOURCES 	+=opengl/glworkspace.cpp \
	 	  opengl/glcontrolwidget.cpp \
		  opengl/gltexobj.cpp \
		  opengl/glbox.cpp \
		  opengl/glgear.cpp \
		  opengl/gllandscape.cpp \
		  opengl/fbm.c
    win32 {
	SOURCES +=opengl/glinfo_win.cpp
    } mac {
	SOURCES +=opengl/glinfo_mac.cpp
	LIBS 	+=-framework Carbon
    } else:unix {
	SOURCES +=opengl/glinfo_x11.cpp
    }

    FORMS 	+=opengl/printpreview.ui \
		  opengl/gllandscapeviewer.ui

    QT     += opengl
}

sql {
    FORMS 	+=sql/connect.ui \
		  sql/sqlex.ui
    QT	+= sql

    HEADERS	+= sql/sqlsyntaxhighlighter.h
    SOURCES	+= sql/sqlsyntaxhighlighter.cpp
}

table {
    FORMS 	+=widgets/widgetsbase.ui
}

!table {
    FORMS 	+=widgets/widgetsbase_pro.ui
}

TRANSLATIONS	= translations/demo_ar.ts \
		  translations/demo_de.ts \
		  translations/demo_fr.ts \
		  translations/demo_iw.ts

PRECOMPILED_HEADER = demo_pch.h
