TEMPLATE    =	qtlib
HEADERS     =	dialogs/qfiledlg.h \
		dialogs/qmsgbox.h \
		dialogs/qtabdlg.h \
		kernel/qaccel.h \
		kernel/qapp.h \
		kernel/qbitmap.h \
		kernel/qbrush.h \
		kernel/qclipbrd.h \
		kernel/qcolor.h \
		kernel/qconnect.h \
		kernel/qcursor.h \
		kernel/qdialog.h \
		kernel/qdrawutl.h \
		kernel/qevent.h \
		kernel/qfont.h \
		kernel/qfontdta.h \
		kernel/qfontinf.h \
		kernel/qfontmet.h \
		kernel/qgmanagr.h \
		kernel/qimage.h \
		kernel/qkeycode.h \
		kernel/qlayout.h \
		kernel/qmetaobj.h \
		kernel/qobjcoll.h \
		kernel/qobjdefs.h \
		kernel/qobject.h \
		kernel/qpaintd.h \
		kernel/qpaintdc.h \
		kernel/qpainter.h \
		kernel/qpalette.h \
		kernel/qpdevmet.h \
		kernel/qpen.h \
		kernel/qpicture.h \
		kernel/qpixmap.h \
		kernel/qpmcache.h \
		kernel/qpntarry.h \
		kernel/qpoint.h \
		kernel/qprinter.h \
		kernel/qrect.h \
		kernel/qregion.h \
		kernel/qsignal.h \
		kernel/qsize.h \
		kernel/qsocknot.h \
		kernel/qtimer.h \
		kernel/qwidcoll.h \
		kernel/qwidget.h \
		kernel/qwindefs.h \
		kernel/qwindow.h \
		kernel/qwmatrix.h \
		tools/qarray.h \
		tools/qbitarry.h \
		tools/qbuffer.h \
		tools/qcache.h \
		tools/qcollect.h \
		tools/qdatetm.h \
		tools/qdict.h \
		tools/qdir.h \
		tools/qdstream.h \
		tools/qfile.h \
		tools/qfiledef.h \
		tools/qfileinf.h \
		tools/qgarray.h \
		tools/qgcache.h \
		tools/qgdict.h \
		tools/qgeneric.h \
		tools/qglist.h \
		tools/qglobal.h \
		tools/qgvector.h \
		tools/qintcach.h \
		tools/qintdict.h \
		tools/qiodev.h \
		tools/qlist.h \
		tools/qqueue.h \
		tools/qregexp.h \
		tools/qshared.h \
		tools/qstack.h \
		tools/qstring.h \
		tools/qstrlist.h \
		tools/qstrvec.h \
		tools/qtstream.h \
		tools/qvector.h \
		widgets/qbttngrp.h \
		widgets/qbutton.h \
		widgets/qchkbox.h \
		widgets/qcombo.h \
		widgets/qframe.h \
		widgets/qgrpbox.h \
		widgets/qlabel.h \
		widgets/qlcdnum.h \
		widgets/qlined.h \
		widgets/qlistbox.h \
		widgets/qmenubar.h \
		widgets/qmenudta.h \
		widgets/qmlined.h \
		widgets/qpopmenu.h \
		widgets/qpushbt.h \
		widgets/qradiobt.h \
		widgets/qrangect.h \
		widgets/qscrbar.h \
		widgets/qslider.h \
		widgets/qtabbar.h \
		widgets/qtablevw.h \
		widgets/qtooltip.h
SOURCES     =	dialogs/qfiledlg.cpp \
		dialogs/qmsgbox.cpp \
		dialogs/qtabdlg.cpp \
		kernel/qaccel.cpp \
		kernel/qapp.cpp \
		kernel/qapp_win.cpp \
		kernel/qbitmap.cpp \
		kernel/qclb_win.cpp \
		kernel/qclipbrd.cpp \
		kernel/qcol_win.cpp \
		kernel/qcolor.cpp \
		kernel/qconnect.cpp \
		kernel/qcur_win.cpp \
		kernel/qcursor.cpp \
		kernel/qdialog.cpp \
		kernel/qdrawutl.cpp \
		kernel/qevent.cpp \
		kernel/qfnt_win.cpp \
		kernel/qfont.cpp \
		kernel/qgmanagr.cpp \
		kernel/qimage.cpp \
		kernel/qlayout.cpp \
		kernel/qmetaobj.cpp \
		kernel/qobject.cpp \
		kernel/qpainter.cpp \
		kernel/qpalette.cpp \
		kernel/qpdevmet.cpp \
		kernel/qpic_win.cpp \
		kernel/qpicture.cpp \
		kernel/qpixmap.cpp \
		kernel/qpm_win.cpp \
		kernel/qpmcache.cpp \
		kernel/qpntarry.cpp \
		kernel/qpoint.cpp \
		kernel/qprinter.cpp \
		kernel/qprn_win.cpp \
		kernel/qptd_win.cpp \
		kernel/qptr_win.cpp \
		kernel/qrect.cpp \
		kernel/qregion.cpp \
		kernel/qrgn_win.cpp \
		kernel/qsignal.cpp \
		kernel/qsize.cpp \
		kernel/qsocknot.cpp \
		kernel/qtimer.cpp \
		kernel/qwid_win.cpp \
		kernel/qwidget.cpp \
		kernel/qwindow.cpp \
		kernel/qwmatrix.cpp \
		tools/qbitarry.cpp \
		tools/qbuffer.cpp \
		tools/qcollect.cpp \
		tools/qdatetm.cpp \
		tools/qdir.cpp \
		tools/qdstream.cpp \
		tools/qfile.cpp \
		tools/qfileinf.cpp \
		tools/qgarray.cpp \
		tools/qgcache.cpp \
		tools/qgdict.cpp \
		tools/qglist.cpp \
		tools/qglobal.cpp \
		tools/qgvector.cpp \
		tools/qiodev.cpp \
		tools/qregexp.cpp \
		tools/qstring.cpp \
		tools/qtstream.cpp \
		widgets/qbttngrp.cpp \
		widgets/qbutton.cpp \
		widgets/qchkbox.cpp \
		widgets/qcombo.cpp \
		widgets/qframe.cpp \
		widgets/qgrpbox.cpp \
		widgets/qlabel.cpp \
		widgets/qlcdnum.cpp \
		widgets/qlined.cpp \
		widgets/qlistbox.cpp \
		widgets/qmenubar.cpp \
		widgets/qmenudta.cpp \
		widgets/qmlined.cpp \
		widgets/qpopmenu.cpp \
		widgets/qpushbt.cpp \
		widgets/qradiobt.cpp \
		widgets/qrangect.cpp \
		widgets/qscrbar.cpp \
		widgets/qslider.cpp \
		widgets/qtabbar.cpp \
		widgets/qtablevw.cpp \
		widgets/qtooltip.cpp
TARGET      =	qt
VERSION     =	1.2
