# Qt styles module

HEADERS += \
	styles/qstyle.h \
	styles/qstylefactory.h \
	styles/qstyleinterface_p.h \
	styles/qstyleplugin.h \
	styles/qcommonstyle.h
SOURCES += \
	styles/qstyle.cpp \
	styles/qstylefactory.cpp \
	styles/qstyleplugin.cpp \
	styles/qcommonstyle.cpp

contains( styles, all ) {
	styles += mac cde motifplus sgi platinum compact interlace windows motif
}

x11|embedded|!macx-*:styles -= mac

contains( styles, mac ) {
	HEADERS += \
		styles/qmacstyle_mac.h \
		styles/qmacstyleqd_mac.h 
#		styles/qmacstylecg_mac.h
	SOURCES += \
		styles/qmacstyle_mac.cpp \
		styles/qmacstyleqd_mac.cpp \
		styles/qmacstylecg_mac.cpp
	HEADERS *= styles/qaquastyle_p.h 
	SOURCES *= styles/qaquastyle_p.cpp 

	!contains( styles, windows ) {
		message( mac requires windows )
		styles += windows
	}
}
else:DEFINES += QT_NO_STYLE_MAC

contains( styles, cde ) {
	HEADERS += styles/qcdestyle.h
	SOURCES += styles/qcdestyle.cpp

	!contains( styles, motif ) {
		message( cde requires motif )
		styles += motif
	}
}
else:DEFINES += QT_NO_STYLE_CDE

contains( styles, motifplus ) {
	HEADERS += styles/qmotifplusstyle.h
	SOURCES += styles/qmotifplusstyle.cpp
	!contains( styles, motif ) {
		message( motifplus requires motif )
		styles += motif
	}
}
else:DEFINES += QT_NO_STYLE_MOTIFPLUS

contains( styles, interlace ) {
	HEADERS += styles/qinterlacestyle.h
	SOURCES += styles/qinterlacestyle.cpp
	!contains( styles, windows ) {
		message( interlace requires windows )
		styles += windows
	}
}
else:DEFINES += QT_NO_STYLE_INTERLACE

contains( styles, platinum ) {
	HEADERS += styles/qplatinumstyle.h
	SOURCES += styles/qplatinumstyle.cpp
	!contains( styles, windows ) {
		message( platinum requires windows )
		styles += windows
	}
}
else:DEFINES += QT_NO_STYLE_PLATINUM

contains( styles, windowsxp ) {
	HEADERS += styles/qwindowsxpstyle.h
	SOURCES += styles/qwindowsxpstyle.cpp
	!contains( styles, windowsxp ) {
		message( windowsxp requires windows )
		styles += windows
	}
}
else:DEFINES += QT_NO_STYLE_WINDOWSXP

contains( styles, sgi ) {
	HEADERS += styles/qsgistyle.h
	SOURCES += styles/qsgistyle.cpp
	!contains( styles, motif ) {
		message( sgi requires motif )
		styles += motif
	}
}
else:DEFINES += QT_NO_STYLE_SGI

contains( styles, compact ) {
	HEADERS += styles/qcompactstyle.h
	SOURCES += styles/qcompactstyle.cpp
	!contains( styles, windows ) {
		message( compact requires windows )
		styles += windows
	}
}
else:DEFINES += QT_NO_STYLE_COMPACT

wince-*:styles += pocketpc
contains( styles, pocketpc ) {
	HEADERS += styles/qpocketpcstyle_wce.h
	SOURCES += styles/qpocketpcstyle_wce.cpp

	!contains( styles, windows ) {
		message( pocketpc requires windows )
		styles += windows
	}
}
else:DEFINES += QT_NO_STYLE_POCKETPC
				
contains( styles, windows ) {
	HEADERS += styles/qwindowsstyle.h
	SOURCES += styles/qwindowsstyle.cpp
}
else:DEFINES += QT_NO_STYLE_WINDOWS

contains( styles, motif ) {
	HEADERS += styles/qmotifstyle.h
	SOURCES += styles/qmotifstyle.cpp
}
else:DEFINES += QT_NO_STYLE_MOTIF
