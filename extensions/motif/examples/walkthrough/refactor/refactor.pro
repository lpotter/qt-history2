######################################################################
# Automatically generated by qmake (1.04a) Mon Nov 11 13:41:34 2002
######################################################################

TEMPLATE = app
DESTDIR = ..
TARGET = walkthrough-refactor
DEPENDPATH += ../Exm ../Xmd
INCLUDEPATH += ..
LIBS += -lXm -lqmotif

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
