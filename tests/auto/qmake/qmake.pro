load(qttest_p4)
HEADERS += testcompiler.h
SOURCES += tst_qmake.cpp testcompiler.cpp

QT += qt3support

cross_compile: DEFINES += QMAKE_CROSS_COMPILED

DEFINES += QT_USE_USING_NAMESPACE

