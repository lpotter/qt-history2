TEMPLATE    = subdirs
SUBDIRS     = \
	shared \
	deform \
	gradients \
	pathstroke \
	affine \
        interview \
        mainwindow \
        spreadsheet \
        textedit

CONFIG += ordered

!cross_compile:SUBDIRS += sqlbrowser

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_DATA]/demos
INSTALLS += sources
