/****************************************************************************
** $Id: $
**
** Copyright ( C ) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qapplication.h>
#include <qtranslator.h>
#include <qfileinfo.h>
#include <qmessagebox.h>

#include "ab_mainwindow.h"

int main( int argc, char ** argv )
{
    QApplication a( argc, argv );

    QString lang;
    if ( argc != 2 ) {
        int i = QMessageBox::information( 0, "Language?", "Which language?",
                                          "Deutsch", "English" );
        switch ( i ) {
        case 0: lang = "de"; break;
        case 1: lang = "en"; break;
        }
    } else {
        lang = argv[1];
    }

    QString lfile;
    if ( lang != "en" )
        lfile = lang + ".qm";
    else 
        lfile = "orig.qm";
    
    QFileInfo fi( lfile );
    if ( !fi.exists() ) {
    	QMessageBox::warning( 0, QObject::tr( "File error" ),
                              QString( QObject::tr( "Cannot find translation for language: " )+ lang +
                                       QObject::tr( "\n(try 'de' or 'en')" ) ) );
        return 0;
    }

    QTranslator translator( 0 );
    translator.load( lfile, "." );
    a.installTranslator( &translator );

    ABMainWindow *mw = new ABMainWindow();
    mw->setCaption( "Addressbook 1" );
    a.setMainWidget( mw );
    mw->show();

    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
    int result = a.exec();
    delete mw;
    return result;
}
