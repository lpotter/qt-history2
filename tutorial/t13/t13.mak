#############################################################################
# Makefile for building t13
# Generated by tmake at 15:41, 1997/04/07
#     Project: t13.pro
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
		gamebrd.h \
		lcdrange.h
SOURCES =	cannon.cpp \
		gamebrd.cpp \
		lcdrange.cpp \
		main.cpp
OBJECTS =	cannon.obj \
		gamebrd.obj \
		lcdrange.obj \
		main.obj
SRCMOC	=	moc_cannon.cpp \
		moc_gamebrd.cpp \
		moc_lcdrange.cpp
OBJMOC	=	moc_cannon.obj \
		moc_gamebrd.obj \
		moc_lcdrange.obj
TARGET	=	t13.exe

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
	-del gamebrd.obj
	-del lcdrange.obj
	-del main.obj
	-del moc_cannon.cpp
	-del moc_gamebrd.cpp
	-del moc_lcdrange.cpp
	-del moc_cannon.obj
	-del moc_gamebrd.obj
	-del moc_lcdrange.obj
	-del t13.exe

####### Compile

cannon.obj: cannon.cpp

gamebrd.obj: gamebrd.cpp

lcdrange.obj: lcdrange.cpp

main.obj: main.cpp

moc_cannon.obj: moc_cannon.cpp \
		cannon.h

moc_gamebrd.obj: moc_gamebrd.cpp \
		gamebrd.h

moc_lcdrange.obj: moc_lcdrange.cpp \
		lcdrange.h

moc_cannon.cpp: cannon.h
	$(MOC) cannon.h -o moc_cannon.cpp

moc_gamebrd.cpp: gamebrd.h
	$(MOC) gamebrd.h -o moc_gamebrd.cpp

moc_lcdrange.cpp: lcdrange.h
	$(MOC) lcdrange.h -o moc_lcdrange.cpp

