SOURCES = sqlite.cpp
CONFIG -= qt dylib
mac:CONFIG -= app_bundle
LIBS += -lsqlite3
