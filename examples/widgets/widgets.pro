TEMPLATE      = subdirs
SUBDIRS       = analogclock \
                calculator \
                charactermap \
                digitalclock \
                groupbox \
                icons \
                imageviewer \
                lineedits \
                movie \
                scribble \
                shapedclock \
                sliders \
                spinboxes \
                styles \
                stylesheet \
                tetrix \
                tooltips \
                wiggly \
                windowflags


# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS widgets.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets
INSTALLS += target sources
