load(qttest_p4)

SOURCES  += tst_qhostinfo.cpp

QT = core network core
win32:LIBS += -lws2_32

DEFINES += QT_USE_USING_NAMESPACE

