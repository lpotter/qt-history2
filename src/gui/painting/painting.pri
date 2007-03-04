# Qt gui library, paint module

HEADERS += \
        painting/qbezier_p.h \
        painting/qbrush.h \
        painting/qcolor.h \
        painting/qcolor_p.h \
        painting/qcolormap.h \
        painting/qdrawutil.h \
        painting/qpaintdevice.h \
        painting/qpaintengine.h \
        painting/qpainter.h \
        painting/qpainter_p.h \
        painting/qpainterpath.h \
        painting/qpainterpath_p.h \
        painting/qpdf_p.h \
        painting/qpen.h \
        painting/qpolygon.h \
        painting/qpolygonclipper_p.h \
        painting/qprintengine_pdf_p.h \
        painting/qprintengine_ps_p.h \
        painting/qprinter.h \
        painting/qprinter_p.h \
        painting/qprintengine.h \
        painting/qregion.h \
        painting/qstroker_p.h \
        painting/qstylepainter.h \
        painting/qtessellator_p.h \
        painting/qmatrix.h \
        painting/qwmatrix.h \
        painting/qtransform.h \
        painting/qpathclipper_p.h


SOURCES += \
        painting/qbezier.cpp \
        painting/qbrush.cpp \
        painting/qcolor.cpp \
        painting/qcolor_p.cpp \
        painting/qdrawutil.cpp \
        painting/qpaintengine.cpp \
        painting/qpainter.cpp \
        painting/qpainterpath.cpp \
        painting/qpdf.cpp \
        painting/qpen.cpp \
        painting/qpolygon.cpp \
        painting/qprintengine_pdf.cpp \
        painting/qprintengine_ps.cpp \
        painting/qprinter.cpp \
        painting/qstroker.cpp \
        painting/qstylepainter.cpp \
        painting/qtessellator.cpp \
        painting/qregion.cpp \
        painting/qmatrix.cpp \
        painting/qtransform.cpp  \
        painting/qpathclipper.cpp

        DEFINES += QT_RASTER_IMAGEENGINE
        win32:DEFINES += QT_RASTER_PAINTENGINE
        embedded:DEFINES += QT_RASTER_PAINTENGINE
        SOURCES +=                                      \
                painting/qpaintengine_raster.cpp        \
                painting/qdrawhelper.cpp                \
                painting/qblackraster.c                 \
                painting/qgrayraster.c

        HEADERS +=                                      \
                painting/qpaintengine_raster_p.h        \
                painting/qrasterdefs_p.h                \
                painting/qgrayraster_p.h                \
                painting/qblackraster_p.h

win32 {
        HEADERS += painting/qprintengine_win_p.h

        SOURCES += \
                painting/qcolormap_win.cpp \
                painting/qpaintdevice_win.cpp \
                painting/qprintengine_win.cpp \
                painting/qregion_win.cpp
        !win32-borland:LIBS += -lmsimg32
        contains(QT_CONFIG, direct3d) {
                HEADERS += painting/qpaintengine_d3d_p.h
                SOURCES += painting/qpaintengine_d3d.cpp
                RESOURCES += painting/qpaintengine_d3d.qrc
                LIBS += -ld3d9 -ld3dx9 -ldxguid
        }
}

wince-* {
        SOURCES -= painting/qregion_win.cpp
        SOURCES += painting/qregion_wce.cpp
}


unix:x11 {
        HEADERS += \
                painting/qpaintengine_x11_p.h

        SOURCES += \
                painting/qcolormap_x11.cpp \
                painting/qpaintdevice_x11.cpp \
                painting/qpaintengine_x11.cpp
}

!embedded:!x11:mac {
        HEADERS += \
                painting/qpaintengine_mac_p.h \
                painting/qprintengine_mac_p.h

        SOURCES += \
                painting/qcolormap_mac.cpp \
                painting/qpaintdevice_mac.cpp \
                painting/qpaintengine_mac.cpp \
                painting/qprintengine_mac.cpp
}

unix:SOURCES += painting/qregion_unix.cpp

win32|x11|embedded {
        SOURCES += painting/qbackingstore.cpp
}

embedded {
        contains(QT_CONFIG,qtopia) {
                DEFINES += QTOPIA_PRINTENGINE
                HEADERS += painting/qprintengine_qws_p.h
                SOURCES += painting/qprintengine_qws.cpp
        }

        SOURCES += \
                painting/qcolormap_qws.cpp \
                painting/qpaintdevice_qws.cpp
}

x11|embedded {
        contains(QT_CONFIG,qtopia) {
            DEFINES += QT_NO_CUPS QT_NO_LPR
        } else {
            SOURCES += painting/qcups.cpp
            HEADERS += painting/qcups_p.h
        }
} else {
        DEFINES += QT_NO_CUPS QT_NO_LPR
}

mac {

} else:mmx|sse|sse2|iwmmxt {

    X86_HEADERS += painting/qdrawhelper_x86_p.h
    X86_SOURCES += painting/qdrawhelper_x86.cpp

    win32-g++|!win32 {
        x86_compiler.commands = $$QMAKE_CXX -c
        mmx: x86_compiler.commands += -mmmx
        sse:!sse2: x86_compiler.commands += -msse
        sse2: x86_compiler.commands += -msse2
        iwmmxt: x86_compiler.commands += -mcpu=iwmmxt
        x86_compiler.commands += $(CXXFLAGS) $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
        x86_compiler.dependency_type = TYPE_C
        x86_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
        x86_compiler.input = X86_SOURCES
        x86_compiler.variable_out = OBJECTS
        x86_compiler.name = compiling[x86] ${QMAKE_FILE_IN}
        silent:x86_compiler.commands = @echo compiling[x86] ${QMAKE_FILE_IN} && $$x86_compiler.commands
        QMAKE_EXTRA_COMPILERS += x86_compiler
    } else:win32:!win32-msvc {
        HEADERS += $$X86_HEADERS
        SOURCES += $$X86_SOURCES
    }

    sse2: DEFINES += QT_HAVE_SSE2
    sse: DEFINES += QT_HAVE_SSE
    mmx: DEFINES += QT_HAVE_MMX
    iwmmxt: DEFINES += QT_HAVE_IWMMXT
}

win32|x11|embedded {
        HEADERS += painting/qwindowsurface_p.h \
                   painting/qwindowsurface_raster_p.h
        SOURCES += painting/qwindowsurface.cpp \
                   painting/qwindowsurface_raster.cpp

        x11 {
                HEADERS += painting/qwindowsurface_x11_p.h
                SOURCES += painting/qwindowsurface_x11.cpp
        }

        embedded {
                HEADERS += painting/qwindowsurface_qws_p.h
                SOURCES += painting/qwindowsurface_qws.cpp
        }

        win32:contains(QT_CONFIG, direct3d) {
                HEADERS += painting/qwindowsurface_d3d_p.h
                SOURCES += painting/qwindowsurface_d3d.cpp
        }
}

