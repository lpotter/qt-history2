GUID 	 = {2d01fa2e-5564-489d-9b3e-eb2444f71b8e}
TEMPLATE = lib
TARGET	 = qsqlodbc

CONFIG  += qt plugin
DESTDIR	 = ../../../sqldrivers

HEADERS		= ../../../../src/sql/drivers/odbc/qsql_odbc.h
SOURCES		= main.cpp \
		  ../../../../src/sql/drivers/odbc/qsql_odbc.cpp

mac {
        !contains( LIBS, .*odbc.* ) {
            LIBS        *= -liodbc
        }
}

unix {
	OBJECTS_DIR	= .obj
	!contains( LIBS, .*odbc.* ) {
	    LIBS 	*= -lodbc
	}
}

win32 {
	OBJECTS_DIR		= obj
	!win32-borland:LIBS	*= odbc32.lib
	win32-borland:LIBS	*= $(BCB)/lib/PSDK/odbc32.lib
}

QTDIR_build:REQUIRES	= sql

target.path += $$plugins.path/sqldrivers
INSTALLS += target
