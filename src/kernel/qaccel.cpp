/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qaccel.cpp#17 $
**
** Implementation of QAccel class
**
** Author  : Haavard Nord
** Created : 950419
**
** Copyright (C) 1995 by Troll Tech AS.	 All rights reserved.
**
*****************************************************************************/

#define QAccelList QListM_QAccelItem
#include "qaccel.h"
#include "qapp.h"
#include "qlist.h"
#include "qsignal.h"

RCSTAG("$Id: //depot/qt/main/src/kernel/qaccel.cpp#17 $")


/*!
  \class QAccel qaccel.h
  \brief The QAccel class handles keyboard accelerator keys.

  \ingroup uiclasses

  A QAccel contains a list of accelerator items. Each accelerator item
  consists of an identifier and a keyboard code combined with modifiers
  (\c SHIFT, \c CTRL, \c ALT or \c ASCII_ACCEL).

  For example, <code>CTRL + Key_P</code> could be a shortcut for printing
  a document. The key codes are listed in qkeycode.h.

  When pressed, an accelerator key sends out the signal activated() with a
  number that identifies this particular accelerator item.  Accelerator
  items can also be individually connected, so that two different keys
  will activate two different slots (see connectItem()).

  A QAccel object handles key events to its parent widget and all children
  of this parent widget.

  Example:
  \code
     QAccel *a = new QAccel( myWindow );	// create accels for myWindow
     a->connectItem( a->insertItem(Key_P+CTRL), // adds Ctrl+P accelerator
		     myWindow,			// connected to myWindow's
		     SLOT(printDoc()) );	// printDoc() slot
  \endcode
*/

struct QAccelItem {				// internal accelerator item
    QAccelItem( int k, int i ) { key=k; id=i; enabled=TRUE; signal=0; }
   ~QAccelItem()	       { delete signal; }
    int		id;
    int		key;
    bool	enabled;
    QSignal    *signal;
};

typedef declare(QListM,QAccelItem) QAccelList;	// internal accelerator list


static QAccelItem *find_id( QAccelList *list, int id )
{
    register QAccelItem *item = list->first();
    while ( item && item->id != id )
	item = list->next();
    return item;
}

static QAccelItem *find_key( QAccelList *list, int key, int ascii )
{
    register QAccelItem *item = list->first();
    while ( item ) {
	int k = item->key;
	if ( (k & ASCII_ACCEL) != 0 && (k & 0xff) == ascii ) {
	    break;
	}
	else {
	    if ( k == key )
		break;
	}
	item = list->next();
    }
    return item;
}


/*!
  Creates a QAccel object with a parent widget and a name.
*/

QAccel::QAccel( QWidget *parent, const char *name )
    : QObject( parent, name )
{
    aitems = new QAccelList;
    CHECK_PTR( aitems );
    aitems->setAutoDelete( TRUE );
    enabled = TRUE;
    if ( parent ) {				// install event filter
	QWidget *tlw = parent->topLevelWidget();
	tlw->installEventFilter( this );
    }
#if defined(CHECK_NULL)
    else
	warning( "QAccel: An accelerator must have a parent widget" );
#endif
}

/*!
  Destroys the accelerator object.
*/

QAccel::~QAccel()
{
    if ( parent() )
	parent()->topLevelWidget()->removeEventFilter( this );
    emit destroyed();
    delete aitems;
}


/*!
  Enables the accelerator.  The accelerator is initially enabled.

  Individual keys can also be enabled or disabled.

  \sa disable(), setItemEnabled(), enableItem(), disableItem()
*/

void QAccel::enable()
{
    enabled = TRUE;
}

/*!
  Disables the accelerator.

  Individual keys cannot be enabled in this mode.

  \sa enable(), setItemEnabled(), enableItem(), disableItem()
*/

void QAccel::disable()
{
    enabled = FALSE;
}

/*!
  \fn bool QAccel::isDisabled() const
  Returns TRUE if the accelerator is disabled.
  \sa enable(), disable()
*/


/*!
  Returns the number of accelerator items.
*/

uint QAccel::count() const
{
    return aitems->count();
}


/*!
  Inserts an accelerator item and returns the item's identifier.

  \arg \e key is a key code plus a combination of SHIFT, CTRL and ALT.
  \arg \e id is the accelerator item id.

  If \e id is negative, then the item will be assigned a unique
  identifier.

  \code
    QAccel *a = new QAccel( myWindow );		// create accels for myWindow
    a->insertItem( Key_P + CTRL, 200 );		// Ctrl+P to print document
    a->insertItem( Key_X + ALT , 201 );		// Alt+X  to quit
    a->insertItem( ASCII_ACCEL + 'q', 202 );	// ASCII 'q' to quit
    a->insertItem( Key_D );			// gets id 2
    a->insertItem( Key_P + CTRL + SHIFT );	// gets id 3
  \endcode
*/

int QAccel::insertItem( int key, int id )
{
    if ( id == -1 )
	id = aitems->count();
    aitems->insert( 0, new QAccelItem(key,id) );
    return id;
}

/*!
  Removes the accelerator item with the identifier \e id.
*/

void QAccel::removeItem( int id )
{
    if ( find_id(aitems, id) )
	aitems->remove();
}


/*!
  Removes all accelerator items.
*/

void QAccel::clear()
{
    aitems->clear();
}


/*!
  Returns the key code of the accelerator item with the identifier \e id,
  or zero if the id cannot be found.
*/

int QAccel::key( int id )
{
    QAccelItem *item = find_id(aitems, id);
    return item ? item->key : 0;
}

/*!
  Returns the identifier of the accelerator item with the key code \e key, or
  -1 if the item cannot be found.
*/

int QAccel::findKey( int key ) const
{
    QAccelItem *item = find_key(aitems, key, key & 0xff );
    return item ? item->id : -1;
}


/*!
  Returns TRUE if the accelerator items with the identifier \e id
  is disabled.	Returns FALSE if the item is enabled or cannot
  be found.
  \sa isItemEnabled(), setItemEnabled(), enableItem(), disableItem()
*/

bool QAccel::isItemDisabled( int id ) const
{
    QAccelItem *item = find_id(aitems, id);
    return item ? !item->enabled : FALSE;
}

/*!
  \fn bool QAccel::isItemEnabled( int id ) const
  Returns TRUE if the accelerator items with the identifier \e id is enabled.
  Returns FALSE if the item is disabled or cannot be found.
  \sa isItemDisabled(), setItemEnabled(), enableItem(), disableItem()
*/

/*!
  Enables or disables an accelerator item.
  \arg \e id is the item identifier.
  \arg \e enable specifies whether the item should be enabled or disabled.
  \sa enableItem(), disableItem(), isItemEnabled(), isItemDisabled()
*/

void QAccel::setItemEnabled( int id, bool enable )
{
    QAccelItem *item = find_id(aitems, id);
    if ( item )
	item->enabled = enable;
}

/*!
  \fn void QAccel::enableItem( int id )
  Enables the accelerator item with the identifier \e id.
  \sa disableItem(), setItemEnabled(), isItemEnabled()
*/

/*!
  \fn void QAccel::disableItem( int id )
  Disables the accelerator item with the identifier \e id.
  \sa enableItem(), setItemEnabled(), isItemDisabled()
*/


/*!
  Connects an accelerator item to a slot/signal in another object.

  \arg \e id is the accelerator item id.
  \arg \e receiver is the object to receive a signal.
  \arg \e member is a slot or signal function in the receiver.

  \code
    a->connectItem( 201, mainView, SLOT(quit()) );
  \endcode

  \sa disconnectItem()
*/

bool QAccel::connectItem( int id, const QObject *receiver, const char *member )
{
    QAccelItem *item = find_id(aitems, id);
    if ( item ) {
	if ( !item->signal ) {
	    item->signal = new QSignal;
	    CHECK_PTR( item->signal );
	}
	return item->signal->connect( receiver, member );
    }
    return FALSE;
}

/*!
  Disconnects an accelerator item from a function in another
  object.
  \sa connectItem()
*/

bool QAccel::disconnectItem( int id, const QObject *receiver,
			     const char *member )
{
    QAccelItem *item = find_id(aitems, id);
    if ( item && item->signal )
	return item->signal->disconnect( receiver, member );
    return FALSE;
}


/*!
  Processes keyboard events intended for the top level widget.
  Handles all types of events for the accelerator.  Overrides standard
  widget event dispatching.
*/

bool QAccel::eventFilter( QObject *, QEvent *e )
{
    if ( enabled && e->type() == Event_KeyPress ) {
	QKeyEvent *k = (QKeyEvent *)e;
	int key = k->key();
	if ( k->state() & ShiftButton )
	    key |= SHIFT;
	if ( k->state() & ControlButton )
	    key |= CTRL;
	if ( k->state() & AltButton )
	    key |= ALT;
	QAccelItem *item = find_key(aitems,key,k->ascii());
	if ( item && item->enabled ) {
	    if ( item->signal )
		item->signal->activate();
	    else
		emit activated( item->id );
	    k->ignore();
	    return TRUE;
	}
    }
    return FALSE;
}
