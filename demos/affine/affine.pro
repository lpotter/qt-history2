SOURCES += main.cpp xform.cpp
HEADERS += xform.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += affine.qrc

# install
target.path = $$[QT_INSTALL_DATA]/demos/affine
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html *.jpg
sources.path = $$[QT_INSTALL_DATA]/demos/affine
INSTALLS += target sources
