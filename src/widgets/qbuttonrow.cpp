/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qbuttonrow.cpp#5 $
**
** Implementation of button row layout widget
**
** Created : 980220
**
** Copyright (C) 1996-1998 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qbuttonrow.h"

#include "qgmanagr.h"
#include "qobjcoll.h"
/*!
  \class QButtonRow qbuttonrow.h
  \brief The QButtonRow widget performs geometry management on its children

  \ingroup geomanagement

  All its children will be placed horizontally and sized
  according to their sizeHint()s. If there is room, all widgets will have the
  same size.

  \sa QGrid and QHBox
*/








/*!
  Constructs an buttonrow widget with parent \a parent and name \a name
 */
QButtonRow::QButtonRow( QWidget *parent, const char *name )
    :QWidget( parent, name )
{
    gm = new QGManager( this );
    ser = gm->newSerChain( QGManager::LeftToRight );
    gm->add( gm->xChain(), ser );
    par = gm->yChain();

    //    lay = new QHBoxLayout( this, parent?0:5, 5, name ); //### border
    first = TRUE;
    prefSize = QSize(0,0);
}

void QButtonRow::recalc()
{
    //    const int border = 5;
    int maxw=0;
    int miny=0;

    debug( "QButtonRow::recalc" );

    QObjectListIt it(*children());
    QObject *o;
    while ( (o=it.current()) ) {
	++it;
	if ( o-> isWidgetType() ) {
	    QWidget *w = (QWidget*) o;
	    maxw = QMAX( maxw, w->minimumSize().width() );
	    miny = QMAX( miny, w->minimumSize().height() );
	}
    }

    if ( maxw == prefSize.width() && miny == prefSize.height() )
	return;
    prefSize = QSize( maxw, miny );
    it.toFirst();
    while ( (o=it.current()) ) {
	++it;
	if ( o->isWidgetType() ) {
	    QWidget *w = (QWidget*) o;
	    w->setMaximumSize( prefSize );
	    debug( "recalc setting %p to (%d,%d)", w, prefSize.width(),
		   prefSize.height() );
	}
    }
}

/*!
  This function is called when a child changes min/max size.
 */
void QButtonRow::layoutEvent( QEvent * )
{
    recalc();
}

/*!
  This function is called when the widget gets a new child or loses an old one.
 */
void QButtonRow::childEvent( QChildEvent *c )
{
    QWidget *w = c->child();

    if ( !c->inserted() ) {
	///recalc();  //############# removed event comes while child is still there!
	return;
    }

    QSize sh = w->sizeHint();
    if ( !sh.isEmpty() )
	w->setMinimumSize( sh );

    if ( first )
	first = FALSE;
    else
	gm->addSpacing( ser, 5 );//### border
    gm->addWidget( ser, w, 1 );
    gm->addWidget( par, w );

    if ( sh.width() > prefSize.width() || sh.height() > prefSize.height() )
	recalc();
    else {
	w->setMaximumSize( prefSize );
	debug( "childEvent setting %p to (%d,%d)", w, prefSize.width(), prefSize.height() );
    }
    if ( isVisible() )
	gm->activate();
}


/*!
  Provides childEvent() while waiting for Qt 2.0.
 */
bool QButtonRow::event( QEvent *e ) {
    switch ( e->type() ) {
    case Event_ChildInserted:
    case Event_ChildRemoved:
	childEvent( (QChildEvent*)e );
	return TRUE;
    case Event_LayoutHint:
	layoutEvent( e );
	return TRUE;
    default:
	return QWidget::event( e );
    }
}


#if 0
void QButtonRow::dump()
{
    QObjectListIt it(*children());
    QObject *o;
    while ( (o=it.current()) ) {
	++it;
	if ( o-> isWidgetType() ) {
	    QWidget *w = (QWidget*) o;
	    debug( "%s/%s, max (%d,%d), min (%d,%d)",
		   w->className(), w->name( "unnamed" ),
		   w->maximumSize().width(), w->maximumSize().height(),
		   w->minimumSize().width(), w->minimumSize().height() );
	}
    }
    QWidget *w = this;
    debug( "%s/%s, max (%d,%d), min (%d,%d)",
	   w->className(), w->name( "unnamed" ),
	   w->maximumSize().width(), w->maximumSize().height(),
	   w->minimumSize().width(), w->minimumSize().height() );

}
#endif


/*!
  Reimplemented for layout reasons.
*/

void QButtonRow::show()
{
        gm->activate();
    QWidget::show();
}
