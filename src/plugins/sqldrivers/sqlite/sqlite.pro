TARGET	 = qsqlite

HEADERS		= ../../../sql/drivers/sqlite/qsql_sqlite.h
SOURCES		= smain.cpp \
		  ../../../sql/drivers/sqlite/qsql_sqlite.cpp

!system-sqlite:!contains( LIBS, .*sqlite.* ) {
    CONFIG(release, debug|release):DEFINES *= NDEBUG
    INCLUDEPATH += ../../../3rdparty/sqlite

    SOURCES += ../../../3rdparty/sqlite/alter.c \
               ../../../3rdparty/sqlite/analyze.c \
               ../../../3rdparty/sqlite/attach.c \
               ../../../3rdparty/sqlite/auth.c \
               ../../../3rdparty/sqlite/btree.c \
               ../../../3rdparty/sqlite/build.c \
               ../../../3rdparty/sqlite/callback.c \
	       ../../../3rdparty/sqlite/date.c \
               ../../../3rdparty/sqlite/delete.c \
               ../../../3rdparty/sqlite/expr.c \
               ../../../3rdparty/sqlite/func.c \
               ../../../3rdparty/sqlite/hash.c \
               ../../../3rdparty/sqlite/insert.c \
               ../../../3rdparty/sqlite/legacy.c \
               ../../../3rdparty/sqlite/main.c \
               ../../../3rdparty/sqlite/opcodes.c \
               ../../../3rdparty/sqlite/pager.c \
               ../../../3rdparty/sqlite/parse.c \
               ../../../3rdparty/sqlite/pragma.c \
               ../../../3rdparty/sqlite/prepare.c \
               ../../../3rdparty/sqlite/printf.c \
               ../../../3rdparty/sqlite/random.c \
               ../../../3rdparty/sqlite/select.c \
               ../../../3rdparty/sqlite/table.c \
               ../../../3rdparty/sqlite/tokenize.c \
               ../../../3rdparty/sqlite/trigger.c \
               ../../../3rdparty/sqlite/update.c \
               ../../../3rdparty/sqlite/utf.c \
               ../../../3rdparty/sqlite/util.c \
               ../../../3rdparty/sqlite/vacuum.c \
               ../../../3rdparty/sqlite/vdbeapi.c \
	       ../../../3rdparty/sqlite/vdbeaux.c \
               ../../../3rdparty/sqlite/vdbe.c \
               ../../../3rdparty/sqlite/vdbefifo.c \
               ../../../3rdparty/sqlite/vdbemem.c \
               ../../../3rdparty/sqlite/where.c

               unix:SOURCES += ../../../3rdparty/sqlite/os_unix.c
               win32:SOURCES += ../../../3rdparty/sqlite/os_win.c
} else {
    LIBS *= $$QT_LFLAGS_SQLITE
    QMAKE_CXXFLAGS *= $$QT_CFLAGS_SQLITE
}

include(../qsqldriverbase.pri)
