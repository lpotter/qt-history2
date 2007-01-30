TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

HEADERS += colorbutton.h \
           window.h \
           preview.h \
           styledbutton.h \
           fontspage.h \
           printerpage.h \
           interfacepage.h \
           listwidget.h \
           appearancepage.h 
SOURCES += colorbutton.cpp \
           main.cpp \
           window.cpp \
           preview.cpp \
           styledbutton.cpp \
           fontspage.cpp \
           printerpage.cpp \
           interfacepage.cpp \
           listwidget.cpp \
           appearancepage.cpp 
FORMS += window.ui \
         preview.ui \
         fontspage.ui \
         printerpage.ui \
         interfacepage.ui \
         appearancepage.ui
RESOURCES += qtconfig.qrc
