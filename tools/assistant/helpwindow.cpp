/**********************************************************************
** Copyright (C) 2000-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the Qt Assistant.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "helpwindow.h"
#include "mainwindow.h"
#include "tabbedbrowser.h"
#include "helpdialogimpl.h"
#include "config.h"

#include <qurl.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <qfile.h>
#include <qprocess.h>
#include <qpopupmenu.h>
#include <qaction.h>
#include <qfileinfo.h>
#include <qevent.h>
#include <qtextstream.h>
#include <qtextcodec.h>

#if defined(Q_OS_WIN32)
#include <windows.h>
#endif

HelpWindow::HelpWindow( MainWindow *w, QWidget *parent, const char *name )
    : QTextBrowser( parent, name ), mw( w ), shiftPressed( FALSE ), blockScroll( FALSE )
{

}

void HelpWindow::setSource( const QString &name )
{
    if ( name.isEmpty() )
	return;

    if ( shiftPressed ) {
	removeSelection();
	mw->saveSettings();
	mw->saveToolbarSettings();
	MainWindow *nmw = new MainWindow;
	nmw->setup();
	nmw->showLink( name );
	nmw->move( mw->geometry().topLeft() );
	if ( mw->isMaximized() )
	    nmw->showMaximized();
	else
	    nmw->show();
	return;
    }

    if ( name.left( 7 ) == "http://" || name.left( 6 ) == "ftp://" ) {
	QString webbrowser = Config::configuration()->webBrowser();
	if ( webbrowser.isEmpty() ) {
#if defined(Q_OS_WIN32)
	    QT_WA( {
		ShellExecute( winId(), 0, (TCHAR*)name.ucs2(), 0, 0, SW_SHOWNORMAL );
	    } , {
		ShellExecuteA( winId(), 0, name.local8Bit(), 0, 0, SW_SHOWNORMAL );
	    } );
#else
	    int result = QMessageBox::information( mw, tr( "Help" ),
			 tr( "Currently no Web browser is selected.\nPlease use the settings dialog to specify one!\n" ),
			 "Open", "Cancel" );
	    if ( result == 0 ) {
		emit chooseWebBrowser();
		webbrowser = Config::configuration()->webBrowser();
	    }
#endif
	    if ( webbrowser.isEmpty() )
		return;
	}
	QProcess *proc = new QProcess();
	proc->addArgument( webbrowser );
	proc->addArgument( name );
	proc->launch( "" );
	return;
    }

    if ( name.right( 3 ) == "pdf" ) {
	QString pdfbrowser = Config::configuration()->pdfReader();
	if ( pdfbrowser.isEmpty() ) {
	    QMessageBox::information( mw,
				      tr( "Help" ),
				      tr( "No PDF Viewer has been specified\n"
					  "Please use the settings dialog to specify one!\n" ) );
	    return;
	}
	QFileInfo info( pdfbrowser );
	if( !info.exists() ) {
	    QMessageBox::information( mw,
				      tr( "Help" ),
				      tr( "Qt Assistant is unable to start the PDF Viewer\n\n"
					  "%1\n\n"
					  "Please make sure that the executable exists and is located at\n"
					  "the specified location." ).arg( pdfbrowser ) );
	    return;
	}
	QProcess *proc = new QProcess();
	proc->addArgument( pdfbrowser );
	proc->addArgument( name );
	proc->launch( "" );

	return;
    }

    QUrl u( context(), name );
    if ( !u.isLocalFile() ) {
	QMessageBox::information( mw, tr( "Help" ), tr( "Can't load and display non-local file\n"
		    "%1" ).arg( name ) );
	return;
    }

    int i = name.find( '#' );
    QString sect = name;
    if ( i != -1 )
	sect = name.left( i );

    setCharacterEncoding( sect );
    QTextBrowser::setSource( name );

    if( !documentTitle().isEmpty() )
	mw->browsers()->updateTitle( documentTitle() );
    else
	mw->browsers()->updateTitle( tr( "Untitled" ) );
}


void HelpWindow::openLinkInNewWindow()
{
    if ( lastAnchor.isEmpty() )
	return;
    bool oldShiftPressed = shiftPressed;
    shiftPressed = TRUE;
    setSource( lastAnchor );
    shiftPressed = oldShiftPressed;
}

void HelpWindow::openLinkInNewWindow( const QString &link )
{
    lastAnchor = link;
    openLinkInNewWindow();
}

void HelpWindow::openLinkInNewPage()
{
    if( lastAnchor.isEmpty() )
	return;
    mw->browsers()->newTab( lastAnchor );
    lastAnchor = QString::null;
}

void HelpWindow::openLinkInNewPage( const QString &link )
{
    lastAnchor = link;
    openLinkInNewPage();
}

QPopupMenu *HelpWindow::createPopupMenu( const QPoint& pos )
{
    QPopupMenu *m = new QPopupMenu( this );
    lastAnchor = anchorAt( pos );
    if ( !lastAnchor.isEmpty() ) {
	if ( lastAnchor.find( '#' )>=0 ) {
	    QString src = source();
	    int hsh = src.find( '#' );
	    lastAnchor = ( hsh>=0 ? src.left( hsh ) : src ) + lastAnchor;
	}
	m->insertItem( tr("Open Link in New Window\tShift+LMB"),
		       this, SLOT( openLinkInNewWindow() ) );
	m->insertItem( tr("Open Link in New Page"),
		       this, SLOT( openLinkInNewPage() ) );
    }
    mw->actionNewWindow->addTo( m );
    mw->actionOpenPage->addTo( m );
    mw->actionClosePage->addTo( m );
    m->insertSeparator();
    mw->actionGoPrevious->addTo( m );
    mw->actionGoNext->addTo( m );
    mw->actionGoHome->addTo( m );
    m->insertSeparator();
    mw->actionZoomIn->addTo( m );
    mw->actionZoomOut->addTo( m );
    m->insertSeparator();
    mw->actionEditCopy->addTo( m );
    mw->actionEditFind->addTo( m );
    return m;
}

void HelpWindow::keyPressEvent( QKeyEvent *e )
{
    shiftPressed = e->key() == Key_Shift   ? TRUE : shiftPressed;
    QTextBrowser::keyPressEvent( e );
}

void HelpWindow::keyReleaseEvent( QKeyEvent *e )
{
    shiftPressed = e->key() == Key_Shift   ? FALSE : shiftPressed;
    QTextBrowser::keyReleaseEvent( e );
}

void HelpWindow::blockScrolling( bool b )
{
    blockScroll = b;
}

void HelpWindow::ensureCursorVisible()
{
    if ( !blockScroll )
	QTextBrowser::ensureCursorVisible();
}

void HelpWindow::setCharacterEncoding( const QString &name )
{
    QFile file( name );
    if ( !file.open( IO_ReadOnly ) ) {
	qWarning( "can not open file " + name );
	return;
    }

    QTextStream s( &file );
    s.setEncoding( QTextStream::Latin1 );

    QString text;
    while ( !s.atEnd() ) {
	text += s.readLine().lower();
	if ( text.contains( "</head>", FALSE ) )
	    break;
    }
    int i = text.find( "charset=" );
    QString encoding;
    if ( i > -1 ) {
	encoding = text.right( text.length() - (i+8) );
	encoding = encoding.left( encoding.find( "\"" ) );
    }
    QTextCodec *codec = QTextCodec::codecForName( encoding.latin1() );
    if ( !codec )
	encoding = "utf-8";
    else
	encoding = QString( codec->name() );
    QString extension = QString( "text/html; charset=%1" ).arg( encoding );
    mw->browsers()->setMimeExtension( extension );
}
