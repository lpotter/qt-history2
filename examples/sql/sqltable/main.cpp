/****************************************************************************
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qapplication.h>
#include <qsqldatabase.h>
#include <qdatatable.h>
#include <qsqlcursor.h>
#include <qmessagebox.h>

/* Modify the following to match your environment */
#define DRIVER       "QPSQL7"  /* see the Qt SQL documentation for a list of available drivers */
#define DATABASE     "" /* the name of your database */
#define USER         ""   /* user name with appropriate rights */
#define PASSWORD     ""   /* password for USER */
#define HOST         "" /* host on which the database is running */

class SimpleCursor : public QSqlCursor
{
public:
    SimpleCursor () : QSqlCursor( "simpletable" ) {}
protected:
    QSqlRecord* primeInsert()
    {
	/* a real-world application would use sequences, or the like */
	QSqlRecord* buf = QSqlCursor::primeInsert();
	QSqlQuery q( "select max(id)+1 from simpletable;" );
	if ( q.next() )
	       buf->setValue( "id", q.value(0) );
	return buf;
    }
};

int main( int argc, char ** argv )
{
    QApplication a( argc, argv );

    QSqlDatabase * db = QSqlDatabase::addDatabase( DRIVER );
    db->setDatabaseName( DATABASE );
    db->setUserName( USER );
    db->setPassword( PASSWORD );
    db->setHostName( HOST );

    if( !db->open() ){
	QMessageBox::information( 0, "Unable to open database",
				  db->lastError().databaseText() + "\nPlease read the README file in the sqltable directory for more information.");
	return 1;
    }

    SimpleCursor cursor;

    QDataTable table( &cursor ); /* data table uses our cursor */
    table.addColumn( "name", "Name" );
    table.addColumn( "address", "Address" );
    table.setSorting( TRUE );

    a.setMainWidget( &table );
    table.refresh(); /* load data */
    table.show();    /* show widget */

    return a.exec();
}
