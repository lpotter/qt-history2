/****************************************************************************
** $Id: //depot/qt/main/examples/life/main.cpp#2 $
**
** Copyright (C) 1992-2000 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <stdlib.h>
#include "lifedlg.h"
#include <qapplication.h>
 
void usage()
{
    qWarning( "Usage: life [-scale scale]" );
}

int main( int argc, char **argv )
{
    QApplication a( argc, argv );

    int scale = 10;

    for ( int i = 1; i < argc; i++ ){
        QString arg = argv[i];
	if ( arg == "-scale" )
	    scale = atoi( argv[++i] );
	else {
	    usage();
	    exit(1);
	}
    }

    if ( scale < 2 )
	scale = 2;

    LifeDialog *life = new LifeDialog( scale );
    a.setMainWidget( life );
    life->show();

    return a.exec();
}
