REQUIRES	= sql
TEMPLATE	= lib
CONFIG+= qt warn_on release plugin

HEADERS		= qsql_psql.h 

SOURCES		= main.cpp \
		  qsql_psql.cpp 

unix:OBJECTS_DIR	= .obj
win32:OBJECTS_DIR	= obj
win32:LIBS	+= libpqdll.lib
isEmpty(LIBS) {
	message( "No PostgreSQL libraries specified for linking.  See the Qt SQL Module documentation for information on building SQL driver plugins." )
}

TARGET		= qsqlpsql
DESTDIR		= ../../../../plugins/sqldrivers

target.path=$$plugins.path/sqldrivers
isEmpty(target.path):target.path=$$QT_PREFIX/plugins/sqldrivers
INSTALLS += target
