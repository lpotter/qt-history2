/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qspinbox.cpp#21 $
**
** Implementation of QSpinBox widget class
**
** Created : yes
**
** Copyright (C) 1997 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qspinbox.h"
#include "qpushbt.h"
#include "qstrlist.h"
#include "qpainter.h"
#include "qkeycode.h"
#include "qbitmap.h"
#include "qlined.h"
#include "qvalidator.h"

static double multipliers[9] = { 1.0, 0.1, 0.01,
				 0.001, 0.0001, 0.00001,
				 0.000001, 0.0000001, 0.00000001 };

/*! \class QSpinBox qspinbox.h

  \brief The QSpinBox class provides a spin box, sometimes called
  up-down widget, little arrows widget or spin button.

  The default spin box accepts numeric input; either integer or
  floating-point numbers.  The API only provides floating-point
  numbers.

  setRange() or a convenience constructor can be used to set the legal
  range; current() and setCurrent() provide access to the current
  value.  QSpinBox emits the selected signal() whenever the current
  value of the spin box changes.

  Most spin boxes are directional, but some are circular: If the range
  is 00-99 and the current value is 99, clicking Up can give 00.
  QSpinBox provides this functionality using setWrapping() and
  wrapping().

  QSpinBox can easily be subclassed to provide other functionality; to
  reimplement it you'll problably need to set your own validator

  The spin box deviates from Motif look a bit.

  <img src=qspinbox-m.gif> <img src=qspinbox-w.gif>
*/


struct QSpinBoxData {
    QDoubleValidator * v;
    double previousSelectedValue;
};


/*!  Creates an empty, non-wrapping spin box with TabFocus \link
  setFocusPolicy() focus policy. \endlink.
*/

QSpinBox::QSpinBox( QWidget * parent , const char * name )
    : QFrame( parent, name )
{
    d = 0; // not used
    wrap = FALSE;

    up = new QPushButton( this, "up" );
    up->setFocusPolicy( QWidget::NoFocus );
    up->setAutoRepeat( TRUE );

    down = new QPushButton( this, "down" );
    down->setFocusPolicy( QWidget::NoFocus );
    down->setAutoRepeat( TRUE );

    vi = new QLineEdit( this, "this is not /usr/bin/vi" );
    vi->setFrame( FALSE );
    setFocusProxy( vi );
    setFocusPolicy( StrongFocus );

    if ( style() == WindowsStyle )
	setFrameStyle( WinPanel | Sunken );
    else
	setFrameStyle( Panel | Sunken );
    setLineWidth( 2 );

    connect( up, SIGNAL(pressed()), SLOT(next()) );
    connect( down, SIGNAL(pressed()), SLOT(prev()) );
    connect( vi, SIGNAL(textChanged(const char *)),
	     SLOT(textChanged()) );

    vi->installEventFilter( this );

}


/*!  Deletes the spin box, freeing all memory and other resoures.
*/

QSpinBox::~QSpinBox()
{
    delete d;
}


/*!  Sets the legal range of the spin box to \a bottom-top inclusive,
  with \a decimals decimal places.  \a decimals must be at most 8.
  Here are some examples:

  \code
    s->setRange( 42, 69 ); // integers from 42-69 inclusive
    s->setRange( 0, 1, 3 ); // 0.000, 0.001, 0.002, ..., 0.999, 1.000
    s->setRange( 0.1, 360, 1 ); // 0.1, ... 359.8, 359.9, 360.0
    s->setRange( -32768, 32767 ); // the integers -32768 to 32767 inclusive
  \endcode

  If you don't set a valid value using setCurrent(), QSpinBox picks
  one in show() - normally the lowest valid value.
*/

void QSpinBox::setRange( double bottom, double top, int decimals )
{
    if ( !d ) {
	d = new QSpinBoxData;
	d->v = new QDoubleValidator( bottom, top, decimals > 8 ? 8 : decimals,
				     this, "default spin-box validator" );
    }
    if ( vi && d->v != vi->validator() )
	vi->setValidator( d->v );
}


/*!  Makes the spin box wrap around from the last to the first item of
  \a w is TRUE, or not if not.

  \sa wrapping() setCurrent()
*/

void QSpinBox::setWrapping( bool w )
{
    wrap = w;
    newValue();
}

/*!  \fn bool QSpinBox::wrapping() const

  Returns the current setWrapping() value.
*/


/*!  Returns the current value of the spin box, or 0 is the current
  value is unparsable.
*/

double QSpinBox::current() const
{
    bool ok = FALSE;
    double result = QString( text() ).toDouble( &ok );
    return ok ? result : 0;
}


/*!  Returns the last valid text of the spin box, or "" if the spin box
  never has contained any valid text.
*/

const char * QSpinBox::text() const
{ //### should return 0?
    return pv.isNull() ? "" : (const char *)pv;
}

/*!  Moves the spin box to the next value.  This is the same as
  clicking on the pointing-up button, and can be used for e.g.
  keyboard accelerators.

  \sa prev(), setCurrent(), current()
*/

void QSpinBox::next()
{
    if ( !d || !d->v )
	return;

    bool ok;
    double c = QString(text()).toDouble( &ok );

    c += multipliers[QMIN(d->v->decimals(),8)];

    QString s;
    if ( d->v->decimals() )
	s.sprintf( "%.*f", d->v->decimals(), c );
    else
	s.sprintf( "%d", (int)c );

    if ( s.toDouble( &ok ) > d->v->top() )
	c = wrapping() ? d->v->bottom() : d->v->top();

    setCurrent( c );
}


/*!  Moves the spin box to the previous value.  This is the same as
  clicking on the pointing-down button, and can be used for e.g.
  keyboard accelerators.

  \sa next(), setCurrent(), current()
*/

void QSpinBox::prev()
{
    if ( !d )
	return;

    bool ok;
    double c = QString(text()).toDouble( &ok );

    c -= multipliers[QMIN(d->v->decimals(),8)];

    QString s;
    if ( d->v->decimals() )
	s.sprintf( "%.*f", d->v->decimals(), c );
    else
	s.sprintf( "%d", (int)c );

    if ( s.toDouble( &ok ) < d->v->bottom() )
	c = wrapping() ? d->v->top(): d->v->bottom();

    setCurrent( c );
}


/*!  Sets the current value of the spin box to \a value.  \a value is
  forced into the legal range.
*/

void QSpinBox::setCurrent( double value )
{
    if ( d && value > d->v->top() )
	value = d->v->top();
    else if ( d && value < d->v->bottom() )
	value = d->v->bottom();
    QString s;
    if ( d->v->decimals() )
	s.sprintf( "%.*f", d->v->decimals(), value );
    else
	s.sprintf( "%d", (int)value );
    vi->setText( s );
}


/*! \fn void QSpinBox::selected( double )

  This signal is emitted every time the value of the spin box changes
  (by setCurrent(), by a keyboard accelerator, by mouse clicks, or by
  telepathy).

  Note that it is emitted \e every time, not just for the \"final\"
  step - if the user clicks 'up' three times, this signal is emitted
  three times.
*/


/*!  Returns a good-looking size for the spin box.
*/

QSize QSpinBox::sizeHint() const
{ // maybe write this around QLineEdit::sizeHint()
    QFontMetrics fm = fontMetrics();
    int h = fm.height();
    if ( h < 22 ) // enough space for the button pixmaps
	h = 22;
    int w = 40; // never less than 40 pixels for the value

    QString s( "999.99" );
    if ( d ) {
	double m = QMAX( QABS(d->v->top()), QABS(d->v->bottom()) );
	if ( d->v->decimals() )
	    s.sprintf( "%.*f", d->v->decimals(), m );
	else
	    s.sprintf( "%d", (int)m );
    }
    w = fm.width( s );
    
    return QSize( frameWidth() * 2 // right/left frame
		  + (8*h)/5 // buttons - approximate golden ratio
		  + 6 // right/left margins
		  + w, // longest value
		  frameWidth() * 2 // top/bottom frame
		  + 4 // top/bottom margins
		  + h // font height
		  );
}


/*!  Interprets the up and down keys coming to the embedded QLineEdit.
*/

bool QSpinBox::eventFilter( QObject * o, QEvent * e )
{
    if ( o != vi )
	return FALSE;

    if ( e->type() == Event_FocusOut &&
	 !pv.isNull() /*ARNTS&&
	 vi->validator()->isValid( vi->text() ) != QValidator::Acceptable*/ )
	// return to last valid choice on focus change
	vi->setText( pv );
    else if ( e->type() == Event_KeyPress ) {
	QKeyEvent * k = (QKeyEvent *)e;
	if ( k->key() == Key_Up ) {
	    next();
	    k->accept();
	    return TRUE;
	} else if ( k->key() == Key_Down ) {
	    prev();
	    k->accept();
	    return TRUE;
	}
    }
    return FALSE;
}


/*!  Handles resize events for the spin box.  
*/

void QSpinBox::resizeEvent( QResizeEvent * e )
{
    if ( !up || !down ) // happens if the application has a pointer error
	return;

    QSize bs; // no, it's short for 'button size'
    bs.setHeight( e->size().height()/2 - frameWidth() );
    if ( bs.height() < 9 )
	bs.setHeight( 9 );
    bs.setWidth( bs.height() * 8 / 5 );

    if ( up->size() != bs ) {
	up->resize( bs );
	QBitmap bm( (bs.height() - 6) * 2 - 1, bs.height() - 6 );
	QPointArray a;
	a.setPoints( 3,
		     bm.height()-1, 0,
		     0, bm.height()-1,
		     bm.width()-1, bm.height()-1 );
	QPainter p;
	p.begin( &bm );
	p.eraseRect( 0, 0, bm.width(), bm.height() );
	p.setBrush( color1 );
	p.drawPolygon( a );
	p.end();
	up->setPixmap( bm );
    }

    if ( down->size() != bs ) {
	down->resize( bs );
	QBitmap bm( (bs.height() - 6) * 2 - 1, bs.height() - 6 );
	QPointArray a;
	a.setPoints( 3,
		     bm.height()-1, bm.height()-1,
		     0, 0,
		     bm.width()-1, 0 );
	QPainter p;
	p.begin( &bm );
	p.eraseRect( 0, 0, bm.width(), bm.height() );
	p.setBrush( color1 );
	p.drawPolygon( a );
	p.end();
	down->setPixmap( bm );
    }

    int x = e->size().width() - frameWidth() - bs.width();

    up->move( x, frameWidth() );
    down->move( x, height() - frameWidth() - up->height() );

    vi->setGeometry( frameWidth(), frameWidth(),
		     x - frameWidth(), height() - 2*frameWidth() );
}


/*!  This slot calls newValue() whenever the spinbox contains a new
  and acceptable value.
*/

void QSpinBox::textChanged()
{
    if ( vi->validator() /*ARNTS&&
	 vi->validator()->isValid( vi->text() ) != QValidator::Acceptable*/ )
	return;

    QString s = vi->text();
    if ( pv.isNull() || s != pv ) {
	pv = s;
	newValue();
    }
};


/*! This virtual function is called whenever the spinbox contains a
  new acceptable value.  This implementation simply emits the
  selected() signal; most reimplementation can probably be as simple.
*/


void QSpinBox::newValue()
{    
    double value = current();
    // the documentation simplifies - it's possible that two different
    // strings turn into the same double, and we guard against that.
    if ( !d || d->previousSelectedValue == value )
	return;

    d->previousSelectedValue = value;
    up->setEnabled( wrapping() || value < d->v->top() );
    down->setEnabled( wrapping() || value > d->v->bottom() );
    emit selected( value );
}


/*!  Sets the validator for user input to \a v.  The default is to use
  a suitable QDoubleValidator.  Note that next(), prev(), current(),
  setCurrent() and setRange() all depend on the default validator, so
  if you reimplement this function, you probably need to reimplement
  at least next() and prev() too.
*/

void QSpinBox::setValidator( QValidator * v )
{
    if ( vi )
	vi->setValidator( v );
}


/*!  Returns a pointer to the 'up' button, which may be needed by some
  subclasses.
*/

QPushButton * QSpinBox::upButton()
{
    return up;
}


/*!  Returns a pointer to the 'down' button, which may be needed by some
  subclasses.
*/

QPushButton * QSpinBox::downButton()
{
    return down;
}


/*!  Returns a pointer to the embedded QLineEdit, which may be needed
  by some subclasses.
*/

QLineEdit * QSpinBox::editor()
{
    return vi;
}
