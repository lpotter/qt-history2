#############################################################################
# Makefile for building t9
# Generated by tmake at 22:41, 1996/12/08
#     Project: t9.pro
#    Template: e:\tmake\lib\win32-msvc\qtapp.t
#############################################################################

####### Compiler, tools and options

CC	=	cl
CFLAGS	=	-O1 -nologo
INCPATH	=	-I$(QTDIR)\include
LINK	=	link
LFLAGS	=	/SUBSYSTEM:windows /NOLOGO
LIBS	=	wsock32.lib user32.lib gdi32.lib comdlg32.lib \
		$(QTDIR)\lib\qt.lib 
MOC	=	moc

####### Files

HEADERS =	cannon.h \
		lcdrange.h
SOURCES =	cannon.cpp \
		lcdrange.cpp \
		main.cpp
OBJECTS =	cannon.obj \
		lcdrange.obj \
		main.obj
SRCMOC	=	moc_cannon.cpp \
		moc_lcdrange.cpp
OBJMOC	=	moc_cannon.obj \
		moc_lcdrange.obj
TARGET	=	t9.exe

####### Implicit rules

.SUFFIXES: .cpp .c

.cpp.obj:
	$(CC) -c $(CFLAGS) $(INCPATH) -Fo$@ $<

.c.obj:
	$(CC) -c $(CFLAGS) $(INCPATH) -Fo$@ $<

####### Make targets

all: $(TARGET) 

$(TARGET): $(OBJECTS) $(OBJMOC)
	$(LINK) $(LFLAGS) /OUT:$(TARGET) @<<
	    $(OBJECTS) $(OBJMOC) $(LIBS)
<<

moc: $(SRCMOC)

clean:
	-del cannon.obj
	-del lcdrange.obj
	-del main.obj
	-del moc_cannon.cpp
	-del moc_lcdrange.cpp
	-del moc_cannon.obj
	-del moc_lcdrange.obj
	-del t9.exe

####### Compile

cannon.obj: cannon.cpp \
		cannon.h \
		$(QTDIR)\include\qwidget.h \
		$(QTDIR)\include\qwindefs.h \
		$(QTDIR)\include\qobjdefs.h \
		$(QTDIR)\include\qglobal.h \
		$(QTDIR)\include\qobject.h \
		$(QTDIR)\include\qstring.h \
		$(QTDIR)\include\qarray.h \
		$(QTDIR)\include\qgarray.h \
		$(QTDIR)\include\qshared.h \
		$(QTDIR)\include\qgeneric.h \
		$(QTDIR)\include\qevent.h \
		$(QTDIR)\include\qrect.h \
		$(QTDIR)\include\qsize.h \
		$(QTDIR)\include\qpoint.h \
		$(QTDIR)\include\qpaintd.h \
		$(QTDIR)\include\qpalette.h \
		$(QTDIR)\include\qcolor.h \
		$(QTDIR)\include\qcursor.h \
		$(QTDIR)\include\qfont.h \
		$(QTDIR)\include\qfontmet.h \
		$(QTDIR)\include\qfontinf.h \
		$(QTDIR)\include\qpainter.h \
		$(QTDIR)\include\qregion.h \
		$(QTDIR)\include\qpen.h \
		$(QTDIR)\include\qbrush.h \
		$(QTDIR)\include\qpntarry.h \
		$(QTDIR)\include\qwmatrix.h

lcdrange.obj: lcdrange.cpp \
		lcdrange.h \
		$(QTDIR)\include\qwidget.h \
		$(QTDIR)\include\qwindefs.h \
		$(QTDIR)\include\qobjdefs.h \
		$(QTDIR)\include\qglobal.h \
		$(QTDIR)\include\qobject.h \
		$(QTDIR)\include\qstring.h \
		$(QTDIR)\include\qarray.h \
		$(QTDIR)\include\qgarray.h \
		$(QTDIR)\include\qshared.h \
		$(QTDIR)\include\qgeneric.h \
		$(QTDIR)\include\qevent.h \
		$(QTDIR)\include\qrect.h \
		$(QTDIR)\include\qsize.h \
		$(QTDIR)\include\qpoint.h \
		$(QTDIR)\include\qpaintd.h \
		$(QTDIR)\include\qpalette.h \
		$(QTDIR)\include\qcolor.h \
		$(QTDIR)\include\qcursor.h \
		$(QTDIR)\include\qfont.h \
		$(QTDIR)\include\qfontmet.h \
		$(QTDIR)\include\qfontinf.h \
		$(QTDIR)\include\qscrbar.h \
		$(QTDIR)\include\qrangect.h \
		$(QTDIR)\include\qlcdnum.h \
		$(QTDIR)\include\qframe.h \
		$(QTDIR)\include\qbitarry.h

main.obj: main.cpp \
		$(QTDIR)\include\qapp.h \
		$(QTDIR)\include\qwidget.h \
		$(QTDIR)\include\qwindefs.h \
		$(QTDIR)\include\qobjdefs.h \
		$(QTDIR)\include\qglobal.h \
		$(QTDIR)\include\qobject.h \
		$(QTDIR)\include\qstring.h \
		$(QTDIR)\include\qarray.h \
		$(QTDIR)\include\qgarray.h \
		$(QTDIR)\include\qshared.h \
		$(QTDIR)\include\qgeneric.h \
		$(QTDIR)\include\qevent.h \
		$(QTDIR)\include\qrect.h \
		$(QTDIR)\include\qsize.h \
		$(QTDIR)\include\qpoint.h \
		$(QTDIR)\include\qpaintd.h \
		$(QTDIR)\include\qpalette.h \
		$(QTDIR)\include\qcolor.h \
		$(QTDIR)\include\qcursor.h \
		$(QTDIR)\include\qfont.h \
		$(QTDIR)\include\qfontmet.h \
		$(QTDIR)\include\qfontinf.h \
		$(QTDIR)\include\qpushbt.h \
		$(QTDIR)\include\qbutton.h \
		$(QTDIR)\include\qscrbar.h \
		$(QTDIR)\include\qrangect.h \
		$(QTDIR)\include\qlcdnum.h \
		$(QTDIR)\include\qframe.h \
		$(QTDIR)\include\qbitarry.h \
		lcdrange.h \
		cannon.h

moc_cannon.obj: moc_cannon.cpp \
		cannon.h \
		$(QTDIR)\include\qwidget.h \
		$(QTDIR)\include\qwindefs.h \
		$(QTDIR)\include\qobjdefs.h \
		$(QTDIR)\include\qglobal.h \
		$(QTDIR)\include\qobject.h \
		$(QTDIR)\include\qstring.h \
		$(QTDIR)\include\qarray.h \
		$(QTDIR)\include\qgarray.h \
		$(QTDIR)\include\qshared.h \
		$(QTDIR)\include\qgeneric.h \
		$(QTDIR)\include\qevent.h \
		$(QTDIR)\include\qrect.h \
		$(QTDIR)\include\qsize.h \
		$(QTDIR)\include\qpoint.h \
		$(QTDIR)\include\qpaintd.h \
		$(QTDIR)\include\qpalette.h \
		$(QTDIR)\include\qcolor.h \
		$(QTDIR)\include\qcursor.h \
		$(QTDIR)\include\qfont.h \
		$(QTDIR)\include\qfontmet.h \
		$(QTDIR)\include\qfontinf.h

moc_lcdrange.obj: moc_lcdrange.cpp \
		lcdrange.h \
		$(QTDIR)\include\qwidget.h \
		$(QTDIR)\include\qwindefs.h \
		$(QTDIR)\include\qobjdefs.h \
		$(QTDIR)\include\qglobal.h \
		$(QTDIR)\include\qobject.h \
		$(QTDIR)\include\qstring.h \
		$(QTDIR)\include\qarray.h \
		$(QTDIR)\include\qgarray.h \
		$(QTDIR)\include\qshared.h \
		$(QTDIR)\include\qgeneric.h \
		$(QTDIR)\include\qevent.h \
		$(QTDIR)\include\qrect.h \
		$(QTDIR)\include\qsize.h \
		$(QTDIR)\include\qpoint.h \
		$(QTDIR)\include\qpaintd.h \
		$(QTDIR)\include\qpalette.h \
		$(QTDIR)\include\qcolor.h \
		$(QTDIR)\include\qcursor.h \
		$(QTDIR)\include\qfont.h \
		$(QTDIR)\include\qfontmet.h \
		$(QTDIR)\include\qfontinf.h

moc_cannon.cpp: cannon.h
	$(MOC) cannon.h -o moc_cannon.cpp

moc_lcdrange.cpp: lcdrange.h
	$(MOC) lcdrange.h -o moc_lcdrange.cpp

