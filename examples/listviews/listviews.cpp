/****************************************************************************
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "listviews.h"

#include <qlabel.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpopupmenu.h>
#include <qheader.h>
#include <qregexp.h>

// -----------------------------------------------------------------

MessageHeader::MessageHeader( const MessageHeader &mh )
{
    msender = mh.msender;
    msubject = mh.msubject;
    mdatetime = mh.mdatetime;
}

MessageHeader &MessageHeader::operator=( const MessageHeader &mh )
{
    msender = mh.msender;
    msubject = mh.msubject;
    mdatetime = mh.mdatetime;

    return *this;
}

// -----------------------------------------------------------------

Folder::Folder( Folder *parent, const QString &name )
    : QObject( parent, name ), fName( name )
{
    lstMessages.setAutoDelete( TRUE );
}

// -----------------------------------------------------------------

FolderListItem::FolderListItem( QListView *parent, Folder *f )
    : QListViewItem( parent )
{
    myFolder = f;
    setText( 0, f->folderName() );

    if ( !myFolder->children().isEmpty() )
	insertSubFolders( myFolder->children() );
}

FolderListItem::FolderListItem( FolderListItem *parent, Folder *f )
    : QListViewItem( parent )
{
    myFolder = f;

    setText( 0, f->folderName() );

    if ( !myFolder->children().isEmpty() )
	insertSubFolders( myFolder->children() );
}

void FolderListItem::insertSubFolders( const QObjectList &lst )
{
    for (int i = 0; i < lst.size(); ++i) {
	Folder *f = static_cast<Folder *>(lst.at(i));
	(void)new FolderListItem( this, f );
    }
}

// -----------------------------------------------------------------

MessageListItem::MessageListItem( QListView *parent, Message *m )
    : QListViewItem( parent )
{
    myMessage = m;
    setText( 0, myMessage->header().sender() );
    setText( 1, myMessage->header().subject() );
    setText( 2, myMessage->header().datetime().toString() );
}

void MessageListItem::paintCell( QPainter *p, const QPalette &pal,
				 int column, int width, int alignment )
{
    QPalette _pal( pal );
    QColor c = _pal.text();

    if ( myMessage->state() == Message::Unread )
	_pal.setColor( QPalette::Text, Qt::red );

    QListViewItem::paintCell( p, _pal, column, width, alignment );

    _pal.setColor( QPalette::Text, c );
}

// -----------------------------------------------------------------

ListViews::ListViews( QWidget *parent, const char *name )
    : QSplitter( Qt::Horizontal, parent, name )
{
    lstFolders.setAutoDelete( TRUE );

    folders = new QListView( this );
    folders->header()->setClickEnabled( FALSE );
    folders->addColumn( "Folder" );

    initFolders();
    setupFolders();

    folders->setRootIsDecorated( TRUE );
    setResizeMode( folders, QSplitter::KeepSize );

    QSplitter *vsplitter = new QSplitter( Qt::Vertical, this );

    messages = new QListView( vsplitter );
    messages->addColumn( "Sender" );
    messages->addColumn( "Subject" );
    messages->addColumn( "Date" );
    messages->setColumnAlignment( 1, Qt::AlignRight );
    messages->setAllColumnsShowFocus( TRUE );
    messages->setShowSortIndicator( TRUE );
    menu = new QPopupMenu( messages );
    for( int i = 1; i <= 10; i++ )
	menu->insertItem( QString( "Context Item %1" ).arg( i ) );
    connect(messages, SIGNAL( contextMenuRequested( QListViewItem *, const QPoint& , int ) ),
	    this, SLOT( slotRMB( QListViewItem *, const QPoint &, int ) ) );
    vsplitter->setResizeMode( messages, QSplitter::KeepSize );

    message = new QLabel( vsplitter );
    message->setAlignment( Qt::AlignTop );
    message->setBackgroundRole( QPalette::Base );

    connect( folders, SIGNAL( selectionChanged( QListViewItem* ) ),
	     this, SLOT( slotFolderChanged( QListViewItem* ) ) );
    connect( messages, SIGNAL( selectionChanged() ),
	     this, SLOT( slotMessageChanged() ) );
    connect( messages, SIGNAL( currentChanged( QListViewItem * ) ),
	     this, SLOT( slotMessageChanged() ) );

    messages->setSelectionMode( QListView::Extended );
    // some preparations
    folders->firstChild()->setOpen( TRUE );
    folders->firstChild()->firstChild()->setOpen( TRUE );
    folders->setCurrentItem( folders->firstChild()->firstChild()->firstChild() );
    folders->setSelected( folders->firstChild()->firstChild()->firstChild(), TRUE );

    messages->setSelected( messages->firstChild(), TRUE );
    messages->setCurrentItem( messages->firstChild() );
    message->setMargin( 5 );

    QList<int> lst;
    lst.append( 170 );
    setSizes( lst );
}

void ListViews::initFolders()
{
    unsigned int mcount = 1;

    for ( unsigned int i = 1; i < 20; i++ ) {
	QString str;
	str = QString( "Folder %1" ).arg( i );
	Folder *f = new Folder( 0, str );
	for ( unsigned int j = 1; j < 5; j++ ) {
	    QString str2;
	    str2 = QString( "Sub Folder %1" ).arg( j );
	    Folder *f2 = new Folder( f, str2 );
	    for ( unsigned int k = 1; k < 3; k++ ) {
		QString str3;
		str3 = QString( "Sub Sub Folder %1" ).arg( k );
		Folder *f3 = new Folder( f2, str3 );
		initFolder( f3, mcount );
	    }
	}
	lstFolders.append( f );
    }
}

void ListViews::initFolder( Folder *folder, unsigned int &count )
{
    for ( unsigned int i = 0; i < 3; i++, count++ ) {
	QString str;
	str = QString( "Message %1  " ).arg( count );
	QDateTime dt = QDateTime::currentDateTime();
	dt = dt.addSecs( 60 * count );
	MessageHeader mh( "Trolltech <info@trolltech.com>  ", str, dt );

	QString body;
	body = QString( "This is the message number %1 of this application, \n"
			"which shows how to use QListViews, QListViewItems, \n"
			"QSplitters and so on. The code should show how easy\n"
			"this can be done in Qt." ).arg( count );
	Message *msg = new Message( mh, body );
	folder->addMessage( msg );
    }
}

void ListViews::setupFolders()
{
    folders->clear();
    for(QList<Folder*>::Iterator it = lstFolders.begin(); it != lstFolders.end(); ++it)
	(void)new FolderListItem( folders, (*it) );
}

void ListViews::slotRMB( QListViewItem* Item, const QPoint & point, int )
{
    if( Item )
	menu->popup( point );
}


void ListViews::slotFolderChanged( QListViewItem *i )
{
    if ( !i )
	return;
    messages->clear();
    message->setText( "" );

    FolderListItem *item = ( FolderListItem* )i;

    Folder *folder = item->folder();
    for ( Message* msg = folder->firstMessage(); msg; msg = folder->nextMessage() ) 
	(void)new MessageListItem( messages, msg );
}

void ListViews::slotMessageChanged()
{
    QListViewItem *i = messages->currentItem();
    if ( !i )
	return;

    if ( !i->isSelected() ) {
	message->setText( "" );
	return;
    }

    MessageListItem *item = ( MessageListItem* )i;
    Message *msg = item->message();

    QString text;
    QString tmp = msg->header().sender();
    tmp = tmp.replace( "<", "&lt;" );
    tmp = tmp.replace( ">", "&gt;" );
    text = QString( "<b><i>From:</i></b> <a href=\"mailto:info@trolltech.com\">%1</a><br>"
		    "<b><i>Subject:</i></b> <big><big><b>%2</b></big></big><br>"
		    "<b><i>Date:</i></b> %3<br><br>"
		    "%4" ).
	   arg( tmp ).arg( msg->header().subject() ).
	   arg( msg->header().datetime().toString() ).arg( msg->body() );

    message->setText( text );

    msg->setState( Message::Read );
}
