/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qmainwindow.cpp#80 $
**
** Implementation of QMainWindow class
**
** Created : 980312
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "qmainwindow.h"

#include "qlist.h"
#include "qtimer.h"
#include "qlayout.h"
#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qapplication.h"

#include "qpainter.h"

#include "qmenubar.h"
#include "qtoolbar.h"
#include "qstatusbar.h"

#include "qtooltip.h"
#include "qwhatsthis.h"

#include "qlayoutengine.h"

#ifdef QT_BUILDER
#include "qdom.h"
#endif

/*! \class QMainWindow qmainwindow.h

  \brief The QMainWindow class provides a typical application window,
  with a menu bar, some tool bars and a status bar.

  \ingroup realwidgets
  \ingroup application

  In addition, you need the large central widget, which you supply and
  tell QMainWindow about using setCentralWidget(), and perhaps a few
  tool bars, which you can add using addToolBar().

  The central widget is not touched by QMainWindow.  QMainWindow
  manages its geometry, and that is all.  For example, the
  application/application.cpp example (an editor) sets a QMultiLineEdit
  to be the central widget.

  QMainWindow automatically detects the creation of a menu bar or
  status bar if you specify the QMainWindow as parent, or you can use
  the provided menuBar() and statusBar() functions.  menuBar() and
  statusBar() create a suitable widget if one doesn't exist, and
  updates the window's layout to make space.

  QMainWindow also provides a QToolTipGroup connected to the status
  bar.  toolTipGroup() provides access to the QToolTipGroup, but there
  is no way to set the tool tip group.

  By default, QMainWindow only allows toolbars above the central
  widget.  You can use setDockEnabled() to allow toolbars in other
  docks (a \e dock is a place where toolbars can stay).  Currently,
  only \c Top, \c Left, \c Right and \c Bottom are meaningful.

  Several functions let you change the appearance of a QMainWindow
  globally: setRightJustification() determines whether QMainWindow
  should ensure that the toolbars fill the available space,
  setUsesBigPixmaps() determines whether QToolButton (and other
  classes) should draw small or large pixmaps (see QIconSet for more
  about that).

  The current release of QMainWindow does not provide draggable
  toolbars.  This feature is planned for inclusion in one of the next
  releases.

  <img src=qmainwindow-m.png> <img src=qmainwindow-w.png>

  \sa QToolBar QStatusBar QMenuBar QToolTipGroup QDialog
*/

/*!
  \enum QMainWindow::ToolBarDock

  Each toolbar can be in one of the following positions:
  <ul>
    <li>\c Top - above the central widget, below the menubar.
    <li>\c Bottom - below the central widget, above the status bar.
    <li>\c Left - to the left of the central widget.
    <li>\c Right - to the left of the central widget.
  </ul>

  Other values are also defined for future expansion.
*/

class QMainWindowPrivate {
public:
    struct ToolBar {
	ToolBar() : t(0), nl(FALSE) {}
	ToolBar( QToolBar * tb, bool n=FALSE )
	    : t(tb), nl(n) {}
	QToolBar * t;
	bool nl;
    };
	
    typedef QList<ToolBar> ToolBarDock;

    QMainWindowPrivate()
	: top(0), left(0), right(0), bottom(0), tornOff(0), unmanaged(0),
	  mb(0), sb(0), ttg(0), mc(0), timer(0), tll(0), ubp( FALSE ),
	  justify( FALSE ), moving( 0 )
    {
	// nothing
    }

    ~QMainWindowPrivate()
    {
	if ( top ) {
	    top->setAutoDelete( TRUE );
	    delete top;
	}
	if ( left ) {
	    left->setAutoDelete( TRUE );
	    delete left;
	}
	if ( right ) {
	    right->setAutoDelete( TRUE );
	    delete right;
	}
	if ( bottom ) {
	    bottom->setAutoDelete( TRUE );
	    delete bottom;
	}
	if ( tornOff ) {
	    tornOff->setAutoDelete( TRUE );
	    delete tornOff;
	}
	if ( unmanaged ) {
	    unmanaged->setAutoDelete( TRUE );
	    delete unmanaged;
	}
    }

    ToolBarDock * top, * left, * right, * bottom, * tornOff, * unmanaged;

    QMenuBar * mb;
    QStatusBar * sb;
    QToolTipGroup * ttg;

    QWidget * mc;

    QTimer * timer;

    QBoxLayout * tll;

    bool ubp;
    bool justify;

    QToolBar * moving;
    QPoint pos;
    QPoint offset;
};



class QToolLayout : public QLayout
{
public:
    QToolLayout( QWidget *parent, int border=0, int space=-1,
		const char *name=0 )
	: QLayout( parent, border, space, name ) { init(); }
    QToolLayout( QLayout* parent, int space=-1, const char *name=0 )
	: QLayout( parent, space, name )  { init(); }
    QToolLayout( int space=-1, const char *name=0 )
	: QLayout( space, name )  { init(); }
    ~QToolLayout();

    void addItem( QLayoutItem *item);
    bool hasHeightForWidth() const { return TRUE; }
    int heightForWidth( int ) const;
    QSize sizeHint() const { return minimumSize(); }
    QSize minimumSize() const;
    QLayoutIterator iterator();
    QSizePolicy::ExpandData expanding() const { return QSizePolicy::NoDirection; }
    void setDirection( QBoxLayout::Direction d ) { dir = d; }
    QBoxLayout::Direction direction() const { return dir; }
    void newLine();
    void setRightJustified( bool on ) { fill = on; }
    bool rightJustified() const { return fill; }
    void invalidate();
protected:
    void setGeometry( const QRect& );
private:
    void init();
    int layoutItems( const QRect&, bool testonly = FALSE );
    QList<QLayoutItem> list;
    QArray<QLayoutStruct> *array;
    QBoxLayout::Direction dir;
    bool fill;
    int cached_width;
    int cached_hfw;
};


void QToolLayout::init()
{
    list.setAutoDelete( TRUE );
    array = 0;
    dir = QBoxLayout::Down;
    fill = FALSE;
    cached_width = 0;
}

QToolLayout::~QToolLayout()
{
    delete array;
}

QSize QToolLayout::minimumSize() const
{
    QSize s;
    QListIterator<QLayoutItem> it(list);
    QLayoutItem *o;
    while ( (o=it.current()) != 0 ) {
	++it;
	if ( o->isEmpty() )
	    continue;
	s = s.expandedTo( o->minimumSize() );
    }
    return s;
}

void QToolLayout::invalidate()
{
    cached_width = 0;
    delete array;
    array=0;
}

int QToolLayout::layoutItems( const QRect &r, bool testonly )
{
    int n = list.count();
    if ( !testonly ) {
	if ( !array ) {
	    array = new QArray<QLayoutStruct>(n);
	    for (int i = 0; i < n; i++ ) {
		QLayoutStruct &a = (*array)[i];
		QLayoutItem *o = list.at(i);
		a.init();
		a.empty = FALSE;
		a.sizeHint = o->sizeHint().width();
		a.expansive = o->expanding() & QSizePolicy::Horizontal;
	    }
	}
	ASSERT( array->size() == list.count() );
    }
    bool up = dir == QBoxLayout::Up;
    int y = r.y();
    int lineh = 0;		//height of this line so far.
    int linew = 0;

    int start = 0;
    int idx = 0;

    QLayoutItem *next = list.first();
    QSize nsh = next->sizeHint();
    while ( idx < n ) {
	QSize sh = nsh;
	idx++;
	if ( idx < n ) {
	    next = list.at(idx);
	    nsh =  next->sizeHint();
	} else {
	    next = 0;
	    nsh = QSize(0,0);
	}
	linew = linew + sh.width() + spacing();
	lineh = QMAX( lineh, sh.height() );
	if ( !next || idx > start && linew + nsh.width() > r.width() ) {
	    //linebreak
	    if ( !testonly ) {
		int right = fill ? r.right() : linew - spacing();
		qGeomCalc( *array, start, idx-start, r.left(), right,
			   spacing() );
		for ( int i = start; i < idx; i++ ) {
		    QRect g( (*array)[i].pos,
			     up ? r.y() + r.bottom() - y - lineh : y,
			     (*array)[i].size, lineh );
		    list.at(i)->setGeometry( g );
		}
	    }
	    y = y + lineh + spacing();
	    linew = 0;
	    lineh = 0;
	    start = idx;
	}
		
    }
    return y - r.y() - spacing();
}


int QToolLayout::heightForWidth( int w ) const
{
    if ( cached_width != w ) {
	//Not all C++ compilers support "mutable" yet:
	QToolLayout * mthis = (QToolLayout*)this;
	int h = mthis->layoutItems( QRect(0,0,w,0), TRUE );
	mthis->cached_hfw = h;
	return h;
    }
    return cached_hfw;
}

void QToolLayout::addItem( QLayoutItem *item )
{
    list.append( item );
}


class QToolLayoutIterator :public QGLayoutIterator
{
public:
    QToolLayoutIterator( QList<QLayoutItem> *l ) :idx(0), list(l)  {}
    uint count() const { return list->count(); }
    QLayoutItem *current() { return idx < (int)count() ? list->at(idx) : 0;  }
    QLayoutItem *next() { idx++; return current(); }
    QLayoutItem *takeCurrent() { return list->take( idx ); }
private:
    int idx;
    QList<QLayoutItem> *list;
};

QLayoutIterator QToolLayout::iterator()
{
    return QLayoutIterator( new QToolLayoutIterator( &list ) );
}

void QToolLayout::setGeometry( const QRect &r )
{
    QLayout::setGeometry( r );
    layoutItems( r );
}

/*!  Constructs an empty main window. */

QMainWindow::QMainWindow( QWidget * parent, const char * name, WFlags f )
    : QWidget( parent, name, f )
{
    d = new QMainWindowPrivate;
    d->timer = new QTimer( this );
    connect( d->timer, SIGNAL(timeout()),
	     this, SLOT(setUpLayout()) );
}


/*! Destroys the object and frees any allocated resources.

*/

QMainWindow::~QMainWindow()
{
    delete d;
}


/*!  Sets this main window to use the menu bar \a newMenuBar.

  The old menu bar, if there was any, is deleted along with its
  contents.

  \sa menuBar()
*/

void QMainWindow::setMenuBar( QMenuBar * newMenuBar )
{
    if ( !newMenuBar )
	return;
    if ( d->mb )
	delete d->mb;
    d->mb = newMenuBar;
    d->mb->installEventFilter( this );
    triggerLayout();
}


/*!  Returns the menu bar for this window.  If there isn't any,
  menuBar() creates an empty menu bar on the fly.

  \sa statusBar()
*/

QMenuBar * QMainWindow::menuBar() const
{
    if ( d->mb )
	return d->mb;

    QObjectList * l
	= ((QObject*)this)->queryList( "QMenuBar", 0, FALSE, FALSE );
    QMenuBar * b;
    if ( l && l->count() )
	b = (QMenuBar *)l->first();
    else
	b = new QMenuBar( (QMainWindow *)this, "automatic menu bar" );
    delete l;
    d->mb = b;
    d->mb->installEventFilter( this );
    ((QMainWindow *)this)->triggerLayout();
    return b;
}


/*!  Sets this main window to use the status bar \a newStatusBar.

  The old status bar, if there was any, is deleted along with its
  contents.

  \sa setMenuBar() statusBar()
*/

void QMainWindow::setStatusBar( QStatusBar * newStatusBar )
{
    if ( !newStatusBar || newStatusBar == d->sb )
	return;
    if ( d->sb )
	delete d->sb;
    d->sb = newStatusBar;
    // ### this code can cause unnecessary creation of a tool tip group
    connect( toolTipGroup(), SIGNAL(showTip(const QString&)),
	     d->sb, SLOT(message(const QString&)) );
    connect( toolTipGroup(), SIGNAL(removeTip()),
	     d->sb, SLOT(clear()) );
    d->sb->installEventFilter( this );
    triggerLayout();
}


/*!  Returns the status bar for this window.  If there isn't any,
  statusBar() creates an empty status bar on the fly, and if necessary
  a tool tip group too.

  \sa  menuBar() toolTipGroup()
*/

QStatusBar * QMainWindow::statusBar() const
{
    if ( d->sb )
	return d->sb;

    QObjectList * l
	= ((QObject*)this)->queryList( "QStatusBar", 0, FALSE, FALSE );
    QStatusBar * s;
    if ( l && l->count() )
	s = (QStatusBar *)l->first();
    else
	s = new QStatusBar( (QMainWindow *)this, "automatic status bar" );
    delete l;
    ((QMainWindow *)this)->setStatusBar( s );
    return s;
}


/*!  Sets this main window to use the tool tip group \a newToolTipGroup.

  The old tool tip group, if there was any, is deleted along with its
  contents.  All the tool tips connected to it lose the ability to
  display the group texts.

  \sa menuBar() toolTipGroup()
*/

void QMainWindow::setToolTipGroup( QToolTipGroup * newToolTipGroup )
{
    if ( !newToolTipGroup || newToolTipGroup == d->ttg )
	return;
    if ( d->ttg )
	delete d->ttg;
    d->ttg = newToolTipGroup;

    connect( toolTipGroup(), SIGNAL(showTip(const QString&)),
	     statusBar(), SLOT(message(const QString&)) );
    connect( toolTipGroup(), SIGNAL(removeTip()),
	     statusBar(), SLOT(clear()) );
    triggerLayout();
}


/*!  Returns the tool tip group for this window.  If there isn't any,
  menuBar() creates an empty tool tip group on the fly.

  \sa menuBar() statusBar()
*/

QToolTipGroup * QMainWindow::toolTipGroup() const
{
    if ( d->ttg )
	return d->ttg;

    QToolTipGroup * t = new QToolTipGroup( (QMainWindow*)this,
					   "automatic tool tip group" );
    ((QMainWindowPrivate*)d)->ttg = t;
    ((QMainWindow *)this)->triggerLayout();
    return t;
}


/*!  Sets \a dock to be available if \a enable is TRUE, and not
  available if \a enable is FALSE.

  The user can drag a toolbar to any enabled dock.
*/

void QMainWindow::setDockEnabled( ToolBarDock dock, bool enable )
{
    if ( enable ) {
	switch ( dock ) {
	case Top:
	    if ( !d->top )
		d->top = new QMainWindowPrivate::ToolBarDock();
	    break;
	case Left:
	    if ( !d->left )
		d->left = new QMainWindowPrivate::ToolBarDock();
	    break;
	case Right:
	    if ( !d->right )
		d->right = new QMainWindowPrivate::ToolBarDock();
	    break;
	case Bottom:
	    if ( !d->bottom )
		d->bottom = new QMainWindowPrivate::ToolBarDock();
	    break;
	case TornOff:
	    if ( !d->tornOff )
		d->tornOff = new QMainWindowPrivate::ToolBarDock();
	    break;
	case Unmanaged:
	    if ( !d->unmanaged )
		d->unmanaged = new QMainWindowPrivate::ToolBarDock();
	    break;
	}
    } else {
	qWarning( "oops! unimplemented, untested, and not quite thought out." );
    }
}


/*!  Returns TRUE if \a dock is enabled, or FALSE if it is not.

  \sa setDockEnabled()
*/

bool QMainWindow::isDockEnabled( ToolBarDock dock ) const
{
    switch ( dock ) {
    case Top:
	return d->top != 0;
    case Left:
	return d->left != 0;
    case Right:
	return d->right != 0;
    case Bottom:
	return d->bottom != 0;
    case TornOff:
	return d->tornOff != 0;
    case Unmanaged:
	return d->unmanaged != 0;
    }
    return FALSE; // for illegal values of dock
}




/*!  Adds \a toolBar to this the end of \a edge and makes it start a
new line of tool bars if \a nl is TRUE.

If \a toolBar is already managed by some main window, it is first
removed from that window.
*/

void QMainWindow::addToolBar( QToolBar * toolBar,
			      ToolBarDock edge, bool newLine )
{
    if ( !toolBar )
	return;

    if ( toolBar->mw ) {
	toolBar->mw->removeToolBar( toolBar );
	toolBar->mw = this;
    }

    setDockEnabled( edge, TRUE );

    QMainWindowPrivate::ToolBarDock * dl = 0;
    if ( edge == Top ) {
	dl = d->top;
	toolBar->setOrientation( QToolBar::Horizontal );
	toolBar->installEventFilter( this );
    } else if ( edge == Left ) {
	dl = d->left;
	toolBar->setOrientation( QToolBar::Vertical );
	toolBar->installEventFilter( this );
    } else if ( edge == Bottom ) {
	dl = d->bottom;
	toolBar->setOrientation( QToolBar::Horizontal );
	toolBar->installEventFilter( this );
    } else if ( edge == Right ) {
	dl = d->right;
	toolBar->setOrientation( QToolBar::Vertical );
	toolBar->installEventFilter( this );
    } else if ( edge == TornOff ) {
	dl = d->tornOff;
    } else if ( edge == Unmanaged ) {
	dl = d->unmanaged;
    }

    if ( !dl )
	return;

    dl->append( new QMainWindowPrivate::ToolBar( toolBar, newLine ) );
    triggerLayout();
}


/*!  Adds \a toolBar to this the end of \a edge, labelling it \a label
and makes it start a new line of tool bars if \a newLine is TRUE.

If \a toolBar is already managed by some main window, it is first
removed from that window.
*/

void QMainWindow::addToolBar( QToolBar * toolBar, const QString &label,
			      ToolBarDock edge, bool newLine )
{
    if ( toolBar ) {
	toolBar->setLabel( label );
	addToolBar( toolBar, edge, newLine );
    }
}


static QMainWindowPrivate::ToolBar * takeToolBarFromDock( QToolBar * t,
							  QMainWindowPrivate::ToolBarDock *l )
{
    if ( !l || !l->count() )
	return 0;
    QMainWindowPrivate::ToolBar * ct = l->first();
    do {
	if ( ct->t == t ) {
	    l->take();
	    return ct;
	}
    } while( (ct=l->next()) != 0 );
    return 0;
}


/*!  Moves \a toolBar to this the end of \a edge.

If \a toolBar is already managed by some main window, it is moved from
that window to this and set to \e not start a new line.
*/

void QMainWindow::moveToolBar( QToolBar * toolBar, ToolBarDock edge )
{
    if ( !toolBar )
	return;
    if ( toolBar->mw )
	toolBar->mw->removeToolBar( toolBar );

    QMainWindowPrivate::ToolBar * ct;
    ct = takeToolBarFromDock( toolBar, d->top );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->left );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->right );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->bottom );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->tornOff );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->unmanaged );
    if ( ct ) {
	setDockEnabled( edge, TRUE );

	QMainWindowPrivate::ToolBarDock * dl = 0;
	if ( edge == Top ) {
	    dl = d->top;
	    toolBar->setOrientation( QToolBar::Horizontal );
	    toolBar->installEventFilter( this );
	} else if ( edge == Left ) {
	    dl = d->left;
	    toolBar->setOrientation( QToolBar::Vertical );
	    toolBar->installEventFilter( this );
	} else if ( edge == Bottom ) {
	    dl = d->bottom;
	    toolBar->setOrientation( QToolBar::Horizontal );
	    toolBar->installEventFilter( this );
	} else if ( edge == Right ) {
	    dl = d->right;
	    toolBar->setOrientation( QToolBar::Vertical );
	    toolBar->installEventFilter( this );
	} else if ( edge == TornOff ) {
	    dl = d->tornOff;
	} else if ( edge == Unmanaged ) {
	    dl = d->unmanaged;
	}

	if ( !dl ) {
	    delete ct;
	    return;
	}

	dl->append( ct );
    } else {
	addToolBar( toolBar, edge );
    }

    triggerLayout();
}


/*!  Removes \a toolBar from this main window, if \a toolBar is
  non-null and known by this main window.
*/

void QMainWindow::removeToolBar( QToolBar * toolBar )
{
    if ( !toolBar )
	return;
    QMainWindowPrivate::ToolBar * ct;
    ct = takeToolBarFromDock( toolBar, d->top );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->left );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->right );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->bottom );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->tornOff );
    if ( !ct )
	ct = takeToolBarFromDock( toolBar, d->unmanaged );
    if ( ct ) {
	toolBar->mw = 0;
	delete ct;
	triggerLayout();
    }
}



class QToolBoxLayout : public QBoxLayout
{
public:
    QToolBoxLayout( Direction d ) :QBoxLayout(d) {}
    QSize minimumSize() const;
};

QSize QToolBoxLayout::minimumSize() const
{
    QSize s(0,0);
    QLayoutIterator it = ((QToolBoxLayout*)this)->iterator();
    QLayoutItem *o;
    while ( (o = it.current()) ) {
	++it;
	s = s.expandedTo( o->minimumSize() );
    }
    s.setHeight(1);
    return s;
}



static void addToolBarToLayout( QMainWindowPrivate::ToolBarDock * dock,
				QBoxLayout * tl,
				QBoxLayout::Direction direction,
				QBoxLayout::Direction dockDirection,
				bool /*mayNeedDockLayout*/,
				bool justify,
				Qt::GUIStyle style )
{
    if ( !dock || dock->isEmpty() )
	return;

    QBoxLayout * dockLayout = tl;

    if ( direction == QBoxLayout::RightToLeft || direction == QBoxLayout::LeftToRight ){
	QToolLayout *layout = 0;
	QMainWindowPrivate::ToolBar * t = dock->first();
	while ( t ) {
	    if ( !layout || t->nl ) {
		layout = new QToolLayout( tl );
		layout->setDirection( dockDirection );
		layout->setRightJustified( justify );
	    }
	    layout->add(t->t);
	    t = dock->next();
	}
    } else {
	QBoxLayout * toolBarRowLayout = 0;
	QMainWindowPrivate::ToolBar * t = dock->first();
	bool anyToolBars;
	do {
	    bool nl = t->nl;
	    if ( !toolBarRowLayout || nl ) {
		if ( toolBarRowLayout && !justify )
		    toolBarRowLayout->addStretch( 1 );
		toolBarRowLayout = new QToolBoxLayout( direction );
		dockLayout->addLayout( toolBarRowLayout, 0 );
	    }
	    toolBarRowLayout->addWidget( t->t, 0 );
	    anyToolBars = TRUE;//####
	} while ( (t=dock->next()) != 0 );

	if ( anyToolBars && style == Qt::MotifStyle )
	    dockLayout->addSpacing( 2 );

	if ( toolBarRowLayout && (!justify || !anyToolBars) )
	    toolBarRowLayout->addStretch( 1 );
    }
}


/*!  Sets up the \link QBoxLayout geometry management \endlink of this
  window.  Called automatically when needed, so you should never need
  to call this.
*/

void QMainWindow::setUpLayout()
{
    if ( !d->mb ) {
	// slightly evil hack here.  reconsider this after 1.40.
	QObjectList * l
	    = ((QObject*)this)->queryList( "QMenuBar", 0, FALSE, FALSE );
	if ( l && l->count() )
	    d->mb = menuBar();
	delete l;
    }
    if ( !d->sb ) {
	// as above.
	QObjectList * l
	    = ((QObject*)this)->queryList( "QStatusBar", 0, FALSE, FALSE );
	if ( l && l->count() )
	    d->sb = statusBar();
	delete l;
    }

    d->timer->stop();
    delete d->tll;
    d->tll = new QBoxLayout( this, QBoxLayout::Down );
    //    d->tll->setResizeMode( QLayout::FreeResize );//############# we need floating layout with height for width here!!!!

    if ( d->mb && !d->mb->testWState(Qt::WState_ForceHide) )
	d->tll->setMenuBar( d->mb );
    if ( style() == WindowsStyle )
	d->tll->addSpacing( 1 );

    addToolBarToLayout( d->top, d->tll,
			QBoxLayout::LeftToRight, QBoxLayout::Down, FALSE,
			d->justify, style() );
    QBoxLayout * mwl = new QBoxLayout( QBoxLayout::LeftToRight );
    d->tll->addLayout( mwl, 1 );
    addToolBarToLayout( d->left, mwl,
			QBoxLayout::Down, QBoxLayout::LeftToRight, FALSE,
			d->justify, style() );
    if ( centralWidget() && !centralWidget()->testWState(Qt::WState_ForceHide) )
	mwl->addWidget( centralWidget(), 1 );
    else
	mwl->addStretch( 1 );
    addToolBarToLayout( d->right, mwl,
			QBoxLayout::Down, QBoxLayout::LeftToRight, FALSE,
			d->justify, style() );
    addToolBarToLayout( d->bottom, d->tll,
			QBoxLayout::LeftToRight, QBoxLayout::Up, TRUE,
			d->justify, style() );

    if ( d->sb && !d->sb->testWState(Qt::WState_ForceHide) )
	d->tll->addWidget( d->sb, 0 );
    //debug( "act %d, %d", x(), y() );
    d->tll->activate();

}


/*!  \reimp */
void QMainWindow::show()
{
    setUpLayout();
    QWidget::show();
}


/*!  \reimp */
QSize QMainWindow::sizeHint() const
{
    if ( !d->tll ) {
	QMainWindow* that = (QMainWindow*) this;
	that->setUpLayout();
    }
    return d->tll->totalSizeHint();
}

/*!  \reimp */
QSize QMainWindow::minimumSizeHint() const
{
    if ( !d->tll ) {
	QMainWindow* that = (QMainWindow*) this;
	that->setUpLayout();
    }
    return d->tll->totalMinimumSize();
}


/*!  Sets the central widget for this window to \a w.  The central
  widget is the one around which the toolbars etc. are arranged.
*/

void QMainWindow::setCentralWidget( QWidget * w )
{
    if ( d->mc )
	d->mc->removeEventFilter( this );
    d->mc = w;
    d->mc->installEventFilter( this );
    triggerLayout();
}


/*!  Returns a pointer to the main child of this main widget.  The
  main child is the big widget around which the tool bars are
  arranged.

  \sa setCentralWidget()
*/

QWidget * QMainWindow::centralWidget() const
{
    return d->mc;
}


/*! \reimp */

void QMainWindow::paintEvent( QPaintEvent * )
{
#if 0
    // this code should only be used if there's a menu bar, in Windows
    // Style, and there's a tool bar immediately below it.  or
    // something like that.  I'll have to figure it out again.
    if ( style() == WindowsStyle && d->mb ) {
	QPainter p( this );
	int y = d->mb->height();
	p.setPen( colorGroup().dark() );
	p.drawLine( 0, y, width()-1, y );
    }
#endif
}


/*!
  \reimp
*/

bool QMainWindow::eventFilter( QObject* o, QEvent *e )
{
    if ( ( e->type() == QEvent::MouseButtonPress ||
		  e->type() == QEvent::MouseMove ||
		  e->type() == QEvent::MouseButtonRelease ) &&
		0 && // 1.4x
		o && ( d->moving || o->inherits( "QToolBar" ) ) ) {
	moveToolBar( d->moving ? d->moving : (QToolBar *)o, (QMouseEvent *)e );
	return TRUE;
    }
    return QWidget::eventFilter( o, e );
}


/*!
  Monitors events to ensure layout is updated.
*/
void QMainWindow::resizeEvent( QResizeEvent* )
{
    //    setUpLayout(); // #####
}

/*!
  Monitors events to ensure layout is updated.
*/
void QMainWindow::childEvent( QChildEvent* e)
{
    if ( e->type() == QEvent::ChildRemoved ) {
	if ( e->child() == 0 ||
	     !e->child()->isWidgetType() ||
	     ((QWidget*)e->child())->testWFlags( WType_TopLevel ) ) {
	    // nothing
	} else if ( e->child() == d->sb ) {
	    d->sb = 0;
	    triggerLayout();
	} else if ( e->child() == d->mb ) {
	    d->mb = 0;
	    triggerLayout();
	} else if ( e->child() == d->mc ) {
	    d->mc = 0;
	    triggerLayout();
	} else if ( e->child()->isWidgetType() ) {
	    removeToolBar( (QToolBar *)(e->child()) );
	    triggerLayout();
	}
    }
}

/*!
  Monitors events to ensure layout is updated.
*/

bool QMainWindow::event( QEvent * e )
{
    if ( e->type() == QEvent::LayoutHint )
	triggerLayout();
    return QWidget::event( e );
}


/*!  Returns the state last set by setUsesBigPixmaps().  The initial
  state is FALSE.
  \sa setUsesBigPixmaps();
*/

bool QMainWindow::usesBigPixmaps() const
{
    return d->ubp;
}


/*!  Sets tool buttons in this main windows to use big pixmaps if \a
  enable is TRUE, and small pixmaps if \a enable is FALSE.

  The default is FALSE.

  Tool buttons and other interested widgets are responsible for
  reading the correct state on startup, and for connecting to this
  widget's pixmapSizeChanged() signal.

  \sa QToolButton::setUsesBigPixmap()
*/

void QMainWindow::setUsesBigPixmaps( bool enable )
{
    if ( d->ubp == enable )
	return;

    d->ubp = enable;
    emit pixmapSizeChanged( enable );
}


/*! \fn void QMainWindow::pixmapSizeChanged( bool )

  This signal is called whenever the setUsesBigPixmaps() is called
  with a value which is different from the current setting.  All
  relevant widgets must connect to this signal.
*/


/*!  Sets this main window to expand its toolbars to fill all
  available space if \a enable is TRUE, and to give the toolbars just
  the space they need if \a enable is FALSE.

  The default is FALSE.

  \sa rightJustification();
*/

void QMainWindow::setRightJustification( bool enable )
{
    if ( enable == d->justify )
	return;
    d->justify = enable;
    triggerLayout();
}


/*!  Returns TRUE if this main windows right-justifies its toolbars, and
  FALSE if it uses a ragged right edge.

  The default is to use a ragged right edge.

  ("Right edge" sometimes means "bottom edge".)

  \sa setRightJustification()
*/

bool QMainWindow::rightJustification() const
{
    return d->justify;
}


void QMainWindow::triggerLayout()
{
    delete d->tll;
    d->tll = 0;
    d->timer->start( 0, TRUE );
}


/*!  Handles mouse event \e e on behalf of tool bar \a t and does all
  the funky docking.
*/

void QMainWindow::moveToolBar( QToolBar* /*t*/ , QMouseEvent * /*e*/ )
{
#if 0
    if ( e->type() == QEvent::MouseButtonPress ) {
	//debug( "saw press" );
	d->moving = 0;
	d->offset = e->pos();
	d->pos = QCursor::pos();
	return;
    } else if ( e->type() == QEvent::MouseButtonRelease ) {
	//debug( "saw release" );
	if ( d->moving ) {
	    qApp->removeEventFilter( this );
	    d->moving = 0;
	}
	return;
    }

    //debug( "saw move" );

    // with that out of the way, let's concentrate on the moves...

    // first, the threshold

    QPoint p( QCursor::pos() );
    if ( !d->moving &&
	 QABS( p.x() - d->pos.x() ) < 3 &&
	 QABS( p.y() - d->pos.y() ) < 3 )
	return;

    // okay.  it's a real move.

    //    debug( "move event to %d, %d", p.x(), p.y() );

    if ( !d->moving ) {
	d->moving = t;
	qApp->installEventFilter( this );
    }

    QPoint lp( mapFromGlobal( p ) );
    QMainWindowPrivate::ToolBarDock * dock = 0;
    // five possible cases: in each of the docs, and outside.
    if ( centralWidget()->geometry().contains( lp ) ||
	 !rect().contains( lp ) ) {
	// not a dock
	if ( t->parentWidget() ) {
	    t->reparent( 0, 0,
			 QPoint( p.x() - d->offset.x(),
				 p.y() - d->offset.y() ),
			 TRUE );
	    QApplication::syncX();
	    dock = d->tornOff;
	} else {
	    t->move( p.x() - d->offset.x(),
		     p.y() - d->offset.y() );
	}
    } else if ( lp.y() < centralWidget()->y() ) {
	//top dock
	dock = d->top;
    } else if ( lp.y() >= centralWidget()->y() + centralWidget()->height() ) {
	// bottom dock
	dock = d->bottom;
    } else if ( lp.x() < centralWidget()->x() ) {
	// bottom dock
	dock = d->left;
    } else if ( lp.x() >= centralWidget()->x() + centralWidget()->width() ) {
	// right dock
	dock = d->right;
    } else {
	qFatal( "never to happen" );
    }

    if ( !dock )
	return;

    qDebug( "1" );
    // at this point dock points to the new dock
    QMainWindowPrivate::ToolBar * ct;
    ct = takeToolBarFromDock( t, d->top );
    if ( !ct )
	ct = takeToolBarFromDock( t, d->left );
    if ( !ct )
	ct = takeToolBarFromDock( t, d->right );
    if ( !ct )
	ct = takeToolBarFromDock( t, d->bottom );
    if ( dock != d->tornOff && !ct )
	ct = takeToolBarFromDock( t, d->tornOff );
    if ( dock == d->tornOff || ct == 0 )
	return;

    qDebug( "2" );
    QMainWindowPrivate::ToolBar * c = dock->first();
    QRect inLine;
    QRect betweenLines;
    int linestart = 0;
    while( c && ct ) {
	qDebug( "3 %p %p", c, ct );
	if ( c->nl ) {
	    if ( dock == d->top ) {
		betweenLines.setRect( 0, 0, width(),
				      c->t->y() + c->t->height()/4 );
	    } else if ( dock == d->bottom ) {
		betweenLines.setRect( 0, c->t->y() + c->t->height()/4,
				      width(), c->t->height()/2 );
	    } else if ( dock == d->left ) {
		betweenLines.setRect( 0, 0, c->t->x() + c->t->width()/4,
				      height() );
	    } else {
		betweenLines.setRect( c->t->x() + 3*c->t->width()/4, 0,
				      c->t->width()/2, height() );
	    }
	    linestart = dock->at();
	}
	if ( dock == d->top || dock == d->bottom ) {
	    inLine.setRect( c->t->x()-c->t->height()/4, c->t->y(),
			    c->t->height()/2, c->t->height() );
	} else {
	    inLine.setRect( c->t->x(), c->t->y() - c->t->width()/4,
			    c->t->width(), c->t->width()/2 );
	}
	if ( inLine.contains( lp ) ) {
	    // ct goes in just before c, and takes over nl
	    dock->insert( dock->at(), ct );
	    if ( t->parentWidget() != this )
		t->reparent( this, 0, QPoint( 0, -t->height() ), TRUE );
	    t->setOrientation( (dock == d->top || dock == d->bottom )
			       ? QToolBar::Horizontal : QToolBar::Vertical );
	    ct->nl = c->nl;
	    c->nl = FALSE;
	    ct = 0;
	    triggerLayout();
	} else {
	    QMainWindowPrivate::ToolBar * c2 = dock->next();
	    if ( c2 == 0 || c2->nl ) {
		// about to do the next line, so check whether c
		// should go in above this line
		if ( betweenLines.contains( lp ) ) {
		    dock->insert( linestart, ct );
		    if ( t->parentWidget() != this )
			t->reparent( this, 0, QPoint( 0, -t->height() ),
				     TRUE );
		    t->setOrientation( (dock == d->top || dock == d->bottom )
				       ? QToolBar::Horizontal
				       : QToolBar::Vertical );
		    ct->nl = TRUE;
		    ct = 0;
			triggerLayout();
		} else {
		    // perhaps at the end of this line?  let's see
		    if ( dock == d->top || dock == d->bottom )
			inLine.setRect( c->t->x() + c->t->width(),
					c->t->y(),
					width() - c->t->x() - c->t->width(),
					c->t->height() );
		    else
			inLine.setRect( c->t->x(),
					c->t->y() + c->t->height(),
					c->t->width(),
					height() - c->t->y() - c->t->height());
		    if ( inLine.contains( lp ) ) {
			dock->insert( dock->at(), ct );
			if ( t->parentWidget() != this )
			    t->reparent( this, 0, QPoint( 0, -t->height() ),
					 TRUE );
			t->setOrientation( (dock == d->top ||
					    dock == d->bottom )
					   ? QToolBar::Horizontal
					   : QToolBar::Vertical );
			ct->nl = FALSE;
			ct = 0;
			triggerLayout();
		    }
		}
	    }
	    c = c2;
	}
    }
    qDebug( "4" );
    // okay, is it at the very end?
    if ( ct ) {
	qDebug( "4a" );
	dock->append( ct );
	if ( t->parentWidget() != this )
	    t->reparent( this, 0, QPoint( 0, -t->height() ), TRUE );
	t->setOrientation( (dock == d->top || dock == d->bottom )
			   ? QToolBar::Horizontal : QToolBar::Vertical );
	ct->nl = TRUE;
	triggerLayout();
    }
#endif
}

/*!
    Enters What's This? question mode and returns immediately.

    This is the same as QWhatsThis::enterWhatsThisMode(), but as a slot of of a
    main window object. This way it can be easily used for popup menus
    as in the code fragment:

  \code
    QPopupMenu * help = new QPopupMenu( this );
    help->insertItem( "What's &This", this , SLOT(whatsThis()), SHIFT+Key_F1);
  \endcode

  \sa QWhatsThis::enterWhatsThisMode()

 */
void QMainWindow::whatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}


/*!
\reimp
*/

void QMainWindow::styleChange( QStyle& old )
{
    setUpLayout();
    QWidget::styleChange( old );
}

#ifdef QT_BUILDER
bool QMainWindow::setConfiguration( const QDomElement& element )
{
    QDomElement r = element.firstChild().toElement();
    for( ; !r.isNull(); r = r.nextSibling().toElement() )
    {
	if ( r.tagName() == "ToolBar" )
	{
	    if ( !r.firstChild().toElement().toWidget( this ) )
		return FALSE;
	}
	else if ( r.tagName() == "MenuBar" )
        {
	    if ( !r.firstChild().toElement().toWidget( this ) )
		return FALSE;
	}
	else if ( r.tagName() == "CentralWidget" )
	{
	    QDomElement ch = r.firstChild().toElement();
	    if ( ch.tagName() == "Widget" )
	    {
		QWidget* w = ch.firstChild().toElement().toWidget( this );
		if ( w == 0 )
		    return FALSE;
		setCentralWidget( w );
	    }
	    else if ( ch.tagName() == "Layout" )
	    {
		QWidget* w = new QWidget( this );
		QLayout* l = ch.firstChild().toElement().toLayout( w );
		if ( l == 0 )
		    return FALSE;
		setCentralWidget( w );
	    }
	    else
		return FALSE;
	}
    }

    // Dont call QWidget configure since we do not accept layouts or
    // or direct child widget except for bars and the central widget
    if ( !QObject::setConfiguration( element ) )
	return FALSE;

    return TRUE;
}
#endif
