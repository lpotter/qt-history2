/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qapplication.h>
#include <qsqldatabase.h>
#include "book.h"
#include "bookform.h"

bool create_connections();

int main( int argc, char *argv[] ) 
{
    QApplication app( argc, argv );

    if ( ! create_connections() ) 
	return 1;

    BookForm bookForm;
    app.setMainWidget( &bookForm );
    bookForm.show();

    return app.exec();
}


bool create_connections()
{
    // create the default database connection
    QSqlDatabase *defaultDB = QSqlDatabase::addDatabase( "QPSQL6" );
    defaultDB->setDatabaseName( "testdb" );
    defaultDB->setUserName( "db" );
    defaultDB->setPassword( "db" );
    defaultDB->setHostName( "silverfish" );
    if ( ! defaultDB->open() ) { 
	qWarning( "Failed to open books database: " + 
		  defaultDB->lastError().driverText() );
	qWarning( defaultDB->lastError().databaseText() );
	return false;
    }

    return true;
}


