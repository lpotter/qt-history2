######################################################################
# Automatically generated by qmake (1.04a) Mon Nov 11 13:41:34 2002
######################################################################
GUID 	 = {68c88e3b-2f1c-4d20-b638-9712e202fcc2}
TEMPLATE = app
TARGET   = walkthrough-mainwindow

DESTDIR  = ..
LIBS    += -lXm -lqmotif

DEPENDPATH  += ../Exm ../Xmd
INCLUDEPATH += ..

# Input
FORMS    = ../pageeditdialog.ui \
           mainwindow.ui
HEADERS += page.h \
           ../Exm/CommandB.h \
           ../Exm/CommandBP.h \
           ../Exm/ExmString.h \
           ../Exm/ExmStringP.h \
           ../Exm/Simple.h \
           ../Exm/SimpleP.h \
           ../Exm/TabB.h \
           ../Exm/TabBP.h \
           ../Xmd/Menus.h \
           ../Xmd/Print.h \
           ../Xmd/PrintP.h
SOURCES += actions.cpp \
           io.cpp \
           todo.cpp \
           ../Exm/CommandB.c \
           ../Exm/ExmString.c \
           ../Exm/Simple.c \
           ../Exm/TabB.c \
           ../Xmd/Menus.c \
           ../Xmd/Print.c
