#############################################################################
# Makefile for building box
# Generated by tmake at 11:33, 1997/04/14
#     Project: box
#    Template: e:\tmake\lib\win32-msvc\qtapp.t
#############################################################################

####### Compiler, tools and options

CC	=	cl
CFLAGS	=	-O1 -nologo
INCPATH	=	-I$(QTDIR)\include
LINK	=	link
LFLAGS	=	/SUBSYSTEM:windows /NOLOGO
LIBS	=	wsock32.lib user32.lib gdi32.lib comdlg32.lib \
		$(QTDIR)\lib\qt.lib $(QTDIR)\lib\qgl.lib opengl32.lib
MOC	=	moc

####### Files

HEADERS =	
SOURCES =	box.cpp
OBJECTS =	box.obj
SRCMOC	=	
OBJMOC	=	
TARGET	=	box.exe

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
	-del box.obj
	-del box.exe

####### Compile

box.obj: box.cpp
