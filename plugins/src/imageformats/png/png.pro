TEMPLATE = lib
CONFIG += qt plugin
TARGET += qpng
VERSION = 1.0.0
REQUIRES = !no-png !png
DESTDIR = ../../../imageformats

SOURCES += main.cpp

system-png {
        unix:LIBS  += -lpng
        win32:LIBS += libpng.lib
}
!system-png {
	INCLUDEPATH += $(QTDIR)/src/3rdparty/libpng
	INCLUDEPATH += $(QTDIR)/src/3rdparty/zlib
	SOURCES += \
    	    ../../../../src/3rdparty/libpng/png.c \
	    ../../../../src/3rdparty/libpng/pngerror.c \
	    ../../../../src/3rdparty/libpng/pngget.c \
	    ../../../../src/3rdparty/libpng/pngmem.c \
	    ../../../../src/3rdparty/libpng/pngpread.c \
	    ../../../../src/3rdparty/libpng/pngread.c \
	    ../../../../src/3rdparty/libpng/pngrio.c \
	    ../../../../src/3rdparty/libpng/pngrtran.c \
	    ../../../../src/3rdparty/libpng/pngrutil.c \
	    ../../../../src/3rdparty/libpng/pngset.c \
	    ../../../../src/3rdparty/libpng/pngtrans.c \
	    ../../../../src/3rdparty/libpng/pngwio.c \
	    ../../../../src/3rdparty/libpng/pngwrite.c \
	    ../../../../src/3rdparty/libpng/pngwtran.c \
	    ../../../../src/3rdparty/libpng/pngwutil.c
}

!system-zlib:!win32:SOURCES += \
	../../../../src/3rdparty/zlib/adler32.c \
	../../../../src/3rdparty/zlib/compress.c \
	../../../../src/3rdparty/zlib/crc32.c \
	../../../../src/3rdparty/zlib/deflate.c \
	../../../../src/3rdparty/zlib/gzio.c \
	../../../../src/3rdparty/zlib/infblock.c \
	../../../../src/3rdparty/zlib/infcodes.c \
	../../../../src/3rdparty/zlib/inffast.c \
	../../../../src/3rdparty/zlib/inflate.c \
	../../../../src/3rdparty/zlib/inftrees.c \
	../../../../src/3rdparty/zlib/infutil.c \
	../../../../src/3rdparty/zlib/trees.c \
	../../../../src/3rdparty/zlib/uncompr.c \
	../../../../src/3rdparty/zlib/zutil.c

system-zlib:unix:LIBS += -lz
system-zlib:mac:LIBS += -lz
