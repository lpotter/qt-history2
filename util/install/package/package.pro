TEMPLATE = app
SOURCES = main.cpp
INCLUDEPATH += ../archive $(QTDIR)/include ../keygen
CONFIG += qt console
TARGET  = package
win32:DESTDIR = ../../../bin
unix:LIBS += -L$(QTDIR)/util/install/archive -larq
win32:LIBS += ../archive/arq.lib

win32:SOURCES += \
	../../../src/3rdparty/zlib/adler32.c \
	../../../src/3rdparty/zlib/compress.c \
	../../../src/3rdparty/zlib/crc32.c \
	../../../src/3rdparty/zlib/deflate.c \
	../../../src/3rdparty/zlib/gzio.c \
	../../../src/3rdparty/zlib/infblock.c \
	../../../src/3rdparty/zlib/infcodes.c \
	../../../src/3rdparty/zlib/inffast.c \
	../../../src/3rdparty/zlib/inflate.c \
	../../../src/3rdparty/zlib/inftrees.c \
	../../../src/3rdparty/zlib/infutil.c \
	../../../src/3rdparty/zlib/trees.c \
	../../../src/3rdparty/zlib/uncompr.c \
	../../../src/3rdparty/zlib/zutil.c
