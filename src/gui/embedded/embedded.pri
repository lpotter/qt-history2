# Qt/Embedded 

embedded {
	CONFIG -= opengl
	CONFIG -= x11
	LIBS -= -dl
	KERNEL_P        = kernel

	!mac:HEADERS += embedded/qsoundqss_qws.h 
	HEADERS += \
		    embedded/qcopchannel_qws.h \
		    embedded/qdirectpainter_qws.h \
		    embedded/qfontfactorybdf_qws.h \
		    embedded/qfontfactoryttf_qws.h \
		    embedded/qfontmanager_qws.h \
		    embedded/qgfx_qws.h \
		    embedded/qgfxraster_qws.h \
		    embedded/qlock_p.h \
		    embedded/qmemorymanager_qws.h \
		    embedded/qwindowsystem_qws.h \
		    embedded/qwsbeosdecoration_qws.h \
		    embedded/qwscommand_qws.h \
		    embedded/qwscursor_qws.h \
		    embedded/qwsdecoration_qws.h \
		    embedded/qwsdefaultdecoration_qws.h \
		    embedded/qwsdisplay_qws.h \
		    embedded/qwsevent_qws.h \
		    embedded/qwshydrodecoration_qws.h \
		    embedded/qwskde2decoration_qws.h \
		    embedded/qwskdedecoration_qws.h \
		    embedded/qwsmanager_qws.h \
		    embedded/qwsproperty_qws.h \
		    embedded/qwsregionmanager_qws.h \
		    embedded/qwssocket_qws.h \
		    embedded/qwsutils_qws.h \
		    embedded/qwswindowsdecoration_qws.h

	 !mac:SOURCES += embedded/qsoundqss_qws.cpp
         SOURCES +=  \
		    embedded/qcopchannel_qws.cpp \
		    embedded/qdirectpainter_qws.cpp \
		    embedded/qfontfactorybdf_qws.cpp \
		    embedded/qfontfactoryttf_qws.cpp \
		    embedded/qfontmanager_qws.cpp \
		    embedded/qgfx_qws.cpp \
		    embedded/qgfxraster_qws.cpp \
		    embedded/qlock.cpp \
		    embedded/qmemorymanager_qws.cpp \
		    embedded/qwindowsystem_qws.cpp \
		    embedded/qwsbeosdecoration_qws.cpp \
		    embedded/qwscommand_qws.cpp \
		    embedded/qwscursor_qws.cpp \
		    embedded/qwsdecoration_qws.cpp \
		    embedded/qwsdefaultdecoration_qws.cpp \
		    embedded/qwsevent_qws.cpp \
		    embedded/qwshydrodecoration_qws.cpp \
		    embedded/qwskde2decoration_qws.cpp \
		    embedded/qwskdedecoration_qws.cpp \
		    embedded/qwsmanager_qws.cpp \
		    embedded/qwsproperty_qws.cpp \
		    embedded/qwsregionmanager_qws.cpp \
		    embedded/qwssocket_qws.cpp \
		    embedded/qwswindowsdecoration_qws.cpp

	ft:SOURCES += \
		../3rdparty/freetype/builds/unix/ftsystem.c \
		../3rdparty/freetype/src/base/ftdebug.c \
		../3rdparty/freetype/src/base/ftinit.c \
		../3rdparty/freetype/src/base/ftbase.c \
		../3rdparty/freetype/src/base/ftglyph.c \
		../3rdparty/freetype/src/base/ftmm.c \
		../3rdparty/freetype/src/base/ftbbox.c \
		../3rdparty/freetype/src/autohint/autohint.c \
		../3rdparty/freetype/src/cache/ftcache.c \
		../3rdparty/freetype/src/cff/cff.c \
		../3rdparty/freetype/src/cid/type1cid.c \
		../3rdparty/freetype/src/psaux/psaux.c \
		../3rdparty/freetype/src/psnames/psmodule.c \
		../3rdparty/freetype/src/raster/raster.c \
		../3rdparty/freetype/src/sfnt/sfnt.c \
		../3rdparty/freetype/src/smooth/smooth.c \
		../3rdparty/freetype/src/truetype/truetype.c \
		../3rdparty/freetype/src/type1/type1.c \
		../3rdparty/freetype/src/type42/type42.c \
		../3rdparty/freetype/src/pshinter/pshinter.c \
		../3rdparty/freetype/src/pcf/pcf.c \
		../3rdparty/freetype/src/pfr/pfr.c \
		../3rdparty/freetype/src/bdf/bdf.c \
		../3rdparty/freetype/src/gzip/ftgzip.c \
		../3rdparty/freetype/src/winfonts/winfnt.c

	ft:INCLUDEPATH += \
		../3rdparty/freetype/src \
		../3rdparty/freetype/include \
		../3rdparty/freetype/builds/unix

	else:DEFINES += QT_NO_FREETYPE

	ft:DEFINES += FT_CONFIG_OPTION_SYSTEM_ZLIB

	qnx6 { 
		HEADERS += embedded/qwsgfx_qnx6.h
		SOURCES += embedded/qwskeyboard_qnx.cpp \
			   embedded/qwsmouse_qnx.cpp \
			   embedded/qwsgfx_qnx6.cpp
	}
	qnx4 {
	    HEADERS += embedded/qwsmouse_qnx4.h \
			embedded/qwskeyboard_qnx4.h
	}


}


# Qt/Embedded Drivers

embedded {
	HEADERS += embedded/qgfxdriverinterface_p.h \
		    embedded/qgfxdriverplugin_qws.h \
		    embedded/qgfxdriverfactory_qws.h \
		    embedded/qkbd_qws.h \
		    embedded/qkbddriverinterface_p.h \
		    embedded/qkbddriverplugin_qws.h \
		    embedded/qkbddriverfactory_qws.h \
		    embedded/qmouse_qws.h \
		    embedded/qmousedriverinterface_p.h \
		    embedded/qmousedriverplugin_qws.h \
		    embedded/qmousedriverfactory_qws.h

	SOURCES += embedded/qgfxdriverplugin_qws.cpp \
		    embedded/qgfxdriverfactory_qws.cpp \
		    embedded/qkbd_qws.cpp \
		    embedded/qkbddriverplugin_qws.cpp \
		    embedded/qkbddriverfactory_qws.cpp \
		    embedded/qmouse_qws.cpp \
		    embedded/qmousedriverplugin_qws.cpp \
		    embedded/qmousedriverfactory_qws.cpp

#
# Graphics drivers
#
        linux-* {
	        HEADERS += embedded/qgfxlinuxfb_qws.h 
		SOURCES += embedded/qgfxlinuxfb_qws.cpp 
	} 
	else:DEFINES += QT_NO_QWS_LINUXFB

	contains( gfx-drivers, qvfb ) {
		HEADERS += embedded/qgfxvfb_qws.h
		SOURCES += embedded/qgfxvfb_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_VFB

	contains( gfx-drivers, vnc ) {
		HEADERS += embedded/qgfxvnc_qws.h
		SOURCES += embedded/qgfxvnc_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_VNC

	!contains( DEFINES, QT_NO_QWS_LINUXFB):contains( gfx-drivers, vga16 ) {
		HEADERS += embedded/qgfxvga16_qws.h
		SOURCES += embedded/qgfxvga16_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_VGA16

	contains( gfx-drivers, transformed ) {
		HEADERS += embedded/qgfxtransformed_qws.h
		SOURCES += embedded/qgfxtransformed_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_TRANSFORMED

	contains( gfx-drivers, mach64 ) {
		HEADERS += embedded/qgfxmach64_qws.h \
			   embedded/qgfxmach64defs_qws.h
		SOURCES += embedded/qgfxmach64_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_MACH64

	contains( gfx-drivers, voodoo ) {
		HEADERS += embedded/qgfxvoodoo_qws.h \
			   embedded/qgfxvoodoodefs_qws.h
		SOURCES += embedded/qgfxvoodoo_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_VOODOO3

	contains( gfx-drivers, matrox ) {
		HEADERS += embedded/qgfxmatrox_qws.h \
			   embedded/qgfxmatroxdefs_qws.h
		SOURCES += embedded/qgfxmatrox_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_MATROX

	contains( gfx-drivers, shadowfb ) {
		HEADERS += embedded/qgfxshadow_qws.h
		SOURCES += embedded/qgfxshadow_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_SHADOWFB

	contains( gfx-drivers, repeater ) {
		HEADERS += embedded/qgfxrepeater_qws.h
		SOURCES += embedded/qgfxrepeater_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_REPEATER

#
# Keyboard drivers
#

	contains( kbd-drivers, sl5000 ) {
		HEADERS +=embedded/qkbdsl5000_qws.h
		SOURCES +=embedded/qkbdsl5000_qws.cpp
		!contains( kbd-drivers, tty ) {
		    kbd-drivers += tty
		}
	}
	else:DEFINES += QT_NO_QWS_KBD_SL5000

	contains( kbd-drivers, tty ) {
		HEADERS +=embedded/qkbdtty_qws.h
		SOURCES +=embedded/qkbdtty_qws.cpp
		!contains( kbd-drivers, pc101 ) {
		    kbd-drivers += pc101
		}
	}
	else:DEFINES += QT_NO_QWS_KBD_TTY

	contains( kbd-drivers, usb ) {
		HEADERS +=embedded/qkbdusb_qws.h
		SOURCES +=embedded/qkbdusb_qws.cpp
		!contains( kbd-drivers, pc101 ) {
		    kbd-drivers += pc101
		}
	}
	else:DEFINES += QT_NO_QWS_KBD_USB

	contains( kbd-drivers, pc101 ) {
		HEADERS +=embedded/qkbdpc101_qws.h
		SOURCES +=embedded/qkbdpc101_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_KBD_PC101

	contains( kbd-drivers, yopy ) {
		HEADERS +=embedded/qkbdyopy_qws.h
		SOURCES +=embedded/qkbdyopy_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_KBD_YOPY

	contains( kbd-drivers, vr41xx ) {
		HEADERS +=embedded/qkbdvr41xx_qws.h
		SOURCES +=embedded/qkbdvr41xx_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_KBD_VR41

#
# Mouse drivers
#

	contains( mouse-drivers, pc ) {
		HEADERS +=embedded/qmousepc_qws.h
		SOURCES +=embedded/qmousepc_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_MOUSE_PC

	contains( mouse-drivers, bus ) {
		HEADERS +=embedded/qmousebus_qws.h
		SOURCES +=embedded/qmousebus_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_MOUSE_BUS

	contains( mouse-drivers, linuxtp ) {
		HEADERS +=embedded/qmouselinuxtp_qws.h
		SOURCES +=embedded/qmouselinuxtp_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_MOUSE_LINUXTP

	contains( mouse-drivers, vr41xx ) {
		HEADERS +=embedded/qmousevr41xx_qws.h
		SOURCES +=embedded/qmousevr41xx_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_MOUSE_VR41

	contains( mouse-drivers, yopy ) {
		HEADERS +=embedded/qmouseyopy_qws.h
		SOURCES +=embedded/qmouseyopy_qws.cpp
	}
	else:DEFINES += QT_NO_QWS_MOUSE_YOPY
}

