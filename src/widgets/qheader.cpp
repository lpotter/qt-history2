/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qheader.cpp#56 $
**
** Implementation of QHeader widget class (table header)
**
** Created : 961105
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#include "qheader.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qbitmap.h"
#include "qbitarray.h"

static const int MINSIZE  = 8;
static const int MARKSIZE = 32;
static const int QH_MARGIN = 4;


struct QHeaderData
{
    QArray<QCOORD>	sizes;
    QArray<QString*>	labels;
    QArray<int>	        a2l;
    QArray<int>	        l2a;

    QBitArray           clicks;
    QBitArray           resize;
    bool		move;

};


/*!
  \class QHeader qheader.h
  \brief The QHeader class provides a table header.
  \ingroup realwidgets

  This is a table heading of the type used in a list view. It gives
  the user the opportunity to resize and move the columns (or rows for
  vertical headings).

  This class can be used without a table view, if you need to control
  table-like structures.

  <img src=qheader-m.gif> <img src=qheader-w.gif>

  \sa QListView QTableView
  <a href="http://www.microsoft.com/win32dev/uiguide/uigui181.htm">Microsoft Style Guide</a>
 */



/*!
  Constructs a horizontal header.

  The \e parent and \e name arguments are sent to the QWidget constructor.
*/

QHeader::QHeader( QWidget *parent, const char *name )
    : QTableView( parent, name )
{
    orient = Horizontal;
    init( 0 );
}

/*!
  Constructs a horizontal header with \a n sections.

  The \e parent and \e name arguments are sent to the QWidget constructor.

*/

QHeader::QHeader( int n,  QWidget *parent, const char *name )
    : QTableView( parent, name )
{
    orient = Horizontal;
    init( n );
}

/*!
  Destroys the header.
 */
QHeader::~QHeader()
{
    delete data;
}

/*!
  \fn void QHeader::sectionClicked (int logical)

  This signal is emitted when a part of the header is clicked. In a
  list view, this signal would typically be connected to a slot which sorts
  the specified column.
*/

/*!
  \fn void QHeader::sizeChange( int section, int oldSize, int newSize )

  This signal is emitted when the user has changed the size of some
  of the parts of the header. This signal is typically connected to a slot
  that repaints the table. \a section is the logical section resized.
*/

/*!
  \fn void QHeader::moved (int from, int to)

  This signal is emitted when the user has moved column \a from to
  position \a to.
  */

/*!
  Returns the size in pixels of section \a i of the header. \a i is the
  actual index.
  */

int QHeader::cellSize( int i ) const
{
    int s = pSize( i );
    return s;
}



/*!
  Returns the position in pixels of section \a i of the header.  \a i is the
  actual index.
*/

int QHeader::cellPos( int i ) const
{
    /* cvs conflict here */

    int r = pPos( i );
    return r + offset();
}


/*!
  Returns the number of sections in the header.
*/

int QHeader::count() const
{
    return data->labels.size() - 1;
}



/*!
  \fn QHeader::Orientation QHeader::orientation() const

  Returns \c Horizontal if the header is horizontal, \c Vertical if
  the header is vertical.

  */

/*!
  \fn void QHeader::setTracking( bool enable )

  Sets tracking if \a enable is TRUE, otherwise turns off tracking.
  If tracking is on, the sizeChange() signal is emitted continuously
  while the mouse is moved, otherwise it is only emitted when the
  mouse button is released.

  \sa tracking()
  */

/*!
  \fn bool QHeader::tracking() const

  Returns TRUE if tracking is on, FALSE otherwise.

  \sa setTracking()
  */

/*!
  What do you think it does?
 */
void QHeader::init( int n )
{
    state = Idle;

    data = new QHeaderData;

    data->sizes.resize(n+1);
    data->labels.resize(n+1);
    data->a2l.resize(n+1);
    data->l2a.resize(n+1);
    data->clicks.resize(n+1);
    data->resize.resize(n+1);
    for ( int i = 0; i < n ; i ++ ) {
	data->sizes[i] = 88;
	data->a2l[i] = i;
	data->l2a[i] = i;
    }
    data->clicks.fill( TRUE );
    data->resize.fill( TRUE );
    data->move = TRUE;

    setFrameStyle( QFrame::NoFrame );

    if ( orient == Horizontal ) {
	setCellWidth( 0 );
	setCellHeight( height() );
	setNumCols( n );
	setNumRows( 1 );
    } else {
	setCellWidth( width() );
	setCellHeight( 0 );
	setNumCols( 1 );
	setNumRows( n );
    }
    handleIdx = 0;
    //################
    data->sizes[n] = 0;
    data->a2l[n] = 0;
    data->l2a[n] = 0;
    //#############
    setMouseTracking( TRUE );
    trackingIsOn = FALSE;
}

/*!
  Sets the header orientation.  The \e orientation must be
  QQHeader::Vertical or QHeader::Horizontal.
  \sa orientation()
*/

void QHeader::setOrientation( Orientation orientation )
{
    orient = orientation;
    repaint();
}


/*!
  Paints a rectangle starting at \a p, with length \s.
  */
void QHeader::paintRect( int p, int s )
{
    QPainter paint( this );
    paint.setPen( QPen( black, 1, DotLine ) );
    if ( orient == Horizontal )
	paint.drawRect( p, 3, s, height() - 5 );
    else	
	paint.drawRect( 3, p, height() - 5, s );
}

/*!
  Marks the division line at \a idx.
  */
void QHeader::markLine( int idx )
{
    QPainter paint( this );
    paint.setPen( QPen( black, 1, DotLine ) );
    int p = pPos( idx );
#if 0
    paint.drawLine(  p, 0, p, height() );
    paint.drawLine(  p-3, 1, p+4, 1 );
    paint.drawLine(  p-3, height()-3, p+4, height()-3 );
#else
    int x = p - MARKSIZE/2;
    int y = 2;
    int x2 = p + MARKSIZE/2;
    int y2 = height() - 3;
    if ( orient == Vertical ) {
	int t = x; x = y; y = t;
	t = x2; x2 = y2; y2 = t;
    }

    paint.drawLine( x, y, x2, y );
    paint.drawLine( x, y+1, x2, y+1 );

    paint.drawLine( x, y2, x2, y2 );
    paint.drawLine( x, y2-1, x2, y2-1 );

    paint.drawLine( x, y, x, y2 );
    paint.drawLine( x+1, y, x+1, y2 );

    paint.drawLine( x2, y, x2, y2 );
    paint.drawLine( x2-1, y, x2-1, y2 );
#endif
}

/*!
  Removes the mark at the division line at \a idx.
  */
void QHeader::unMarkLine( int idx )
{
    if ( idx < 0 )
	return;
    int p = pPos( idx );
    int x = p - MARKSIZE/2;
    int y = 2;
    int x2 = p + MARKSIZE/2;
    int y2 = height() - 3;
    if ( orient == Vertical ) {
	int t = x; x = y; y = t;
	t = x2; x2 = y2; y2 = t;
    }
    repaint( x, y, x2-x+1, y2-y+1);
}

/*!
  Returns the actual index of the section at position \a c, or -1 if outside.
 */
int QHeader::cellAt( int c ) const
{
    int i = ( orient == Horizontal ) ?  findCol( c ) :  findRow( c );
    return i >= count() ? -1 : i;
}


/*!
  Tries to find a line that is not a neighbour of  \c handleIdx.
 */
int QHeader::findLine( int c )
{
    int i = cellAt( c );
    if ( i == -1 )
	return handleIdx; //####### frustrating, but safe behaviour.
    if ( i == handleIdx )
	return i;
    if ( i == handleIdx - 1 &&  pPos( handleIdx ) - c > MARKSIZE/2 )
	return i;
    if ( i == handleIdx + 1 && c - pPos( i ) > MARKSIZE/2 )
	return i + 1;
    if ( c - pPos( i ) > pSize( i ) / 2 )
	return i + 1;
    else
	return i;
}

void QHeader::moveAround( int fromIdx, int toIdx )
{
    if ( fromIdx == toIdx )
	return;
    int i;

    int idx = data->a2l[fromIdx];
    if ( fromIdx < toIdx ) {
	for ( i = fromIdx; i < toIdx - 1; i++ ) {
	    int t;
	    data->a2l[i] = t = data->a2l[i+1];
	    data->l2a[t] = i;
	}
	data->a2l[toIdx-1] = idx;
	data->l2a[idx] = toIdx-1;
    } else {
	for ( i = fromIdx; i > toIdx ; i-- ) {
	    int t;
	    data->a2l[i] = t = data->a2l[i-1];
	    data->l2a[t] = i;
	}
	data->a2l[toIdx] = idx;
	data->l2a[idx] = toIdx;
    }
}

/*!
  sets up the painter
*/

void QHeader::setupPainter( QPainter *p )
{
    p->setPen( colorGroup().text() );
    p->setFont( font() );
}


/*!
  paints a section of the header
*/

void QHeader::paintCell( QPainter *p, int row, int col )
{
    int i = ( orient == Horizontal ) ? col : row;
    int size = pSize( i );
    bool down = (i==handleIdx) && ( state == Pressed || state == Moving );

    QRect fr( 0, 0, orient == Horizontal ?  size : width(),
	      orient == Horizontal ?  height() : size );

    if ( style() == WindowsStyle )
	qDrawWinButton( p, fr, colorGroup(), down );
    else
	qDrawShadePanel( p, fr, colorGroup(), down );

    int logIdx = mapToLogical(i);

    QString s;
    if ( data->labels[logIdx] )
	s = *(data->labels[logIdx]);
    else if ( orient == Horizontal )
	s.sprintf( "Col %d", logIdx );
    else
	s.sprintf( "Row %d", logIdx );

    int d = 0;
    if ( style() == WindowsStyle  &&
	 i==handleIdx && ( state == Pressed || state == Moving ) )
	d = 1;
    QRect r( QH_MARGIN+d, 2+d, size - 6, height() - 4 ); //#####

    if ( 0 && orient == Vertical ) { // we can't do that...
	QWMatrix m;
	m.rotate( 90 );
	m.translate( 0, -width() ); //###########
	p->setWorldMatrix( m );
    }

    p->drawText ( r, AlignLeft| AlignVCenter|SingleLine, s );
}


void QHeader::mousePressEvent( QMouseEvent *m )
{
    if ( m->button() != LeftButton )
	return;
    handleIdx = 0;
    int c = orient == Horizontal ? m->pos().x() : m->pos().y();
    int i = 0;
    while ( i < (int) count() ) {
	if ( pPos(i+1) - MINSIZE/2 < c &&
	     c < pPos(i+1) + MINSIZE/2 ) {
		handleIdx = i+1;
		oldHIdxSize = cellSize( i );
	    if ( data->resize.testBit(i) )
		state = Sliding;
	    else
		state = Blocked;
	    break;
	} else if (  pPos(i)  < c && c < pPos( i+1 ) ) {
	    handleIdx = i;
	    moveToIdx = -1;
	    if ( data->clicks.testBit(i) )
		state = Pressed;
	    else
		state = Blocked;
	    clickPos = c;
	    repaint(sRect( handleIdx ));
	    break;
	}
	i++;
    }
}

void QHeader::mouseReleaseEvent( QMouseEvent *m )
{
    if ( m->button() != LeftButton )
	return;
    State oldState = state;
    state = Idle;
    switch ( oldState ) {
    case Pressed:
	repaint(sRect( handleIdx ));
	if ( sRect( handleIdx ).contains( m->pos() ) )
	    emit sectionClicked( handleIdx );
	break;
    case Sliding: {
	int s = orient == Horizontal ? m->pos().x() : m->pos().y();
	// setCursor( arrowCursor ); // We're probably still there...
	handleColumnResize( handleIdx, s, TRUE );
	} break;
    case Moving: {
	setCursor( arrowCursor );
	if ( handleIdx != moveToIdx && moveToIdx != -1 ) {
	    moveAround( handleIdx, moveToIdx );
	    emit moved( handleIdx, moveToIdx );
	    repaint();
	} else {
	    if ( sRect( handleIdx).contains( m->pos() ) )
		emit sectionClicked( handleIdx );
	    repaint(sRect( handleIdx ));
	}
	break;
    }
    case Blocked:
	//nothing
	break;
    default:
	// empty, probably.  Idle, at any rate.
	break;
    }
}

void QHeader::mouseMoveEvent( QMouseEvent *m )
{
    int s = orient == Horizontal ? m->pos().x() : m->pos().y();
    if ( state == Idle ) {
	bool hit = FALSE;
	int i = 0;
	while ( i <= (int) count() ) {
	    if ( i && pPos(i) - MINSIZE/2 < s && s < pPos(i) + MINSIZE/2 &&
		 data->resize.testBit(i-1) ) {
		hit = TRUE;
		if ( orient == Horizontal )
		    setCursor( splitHCursor );
		else
		    setCursor( splitVCursor );
		break;
	    }
	    i++;
	}
	if ( !hit )
	    setCursor( arrowCursor );
    } else {
	switch ( state ) {
	case Idle:
	    debug( "QHeader::mouseMoveEvent() (%s) Idle state",
		   name( "unnamed" ) );
	    break;
	case Pressed:
	case Blocked:
	    if ( QABS( s - clickPos ) > 4 && data->move ) {
		state = Moving;
		moveToIdx = -1;
		if ( orient == Horizontal )
		    setCursor( sizeHorCursor );
		else
		    setCursor( sizeVerCursor );
	    }
	    break;
	case Sliding:
	    handleColumnResize( handleIdx, s, FALSE );
	    break;
	case Moving: {
	    int newPos = findLine( s );
	    if ( newPos != moveToIdx ) {
		if ( moveToIdx == handleIdx || moveToIdx == handleIdx + 1 )
		    repaint( sRect(handleIdx) );
		else
		    unMarkLine( moveToIdx );
		moveToIdx = newPos;
		if ( moveToIdx == handleIdx || moveToIdx == handleIdx + 1 )
		    paintRect( pPos( handleIdx ), pSize( handleIdx ) );
		else
		    markLine( moveToIdx );
	    }
	    break;
	}
	default:
	    warning( "QHeader::mouseMoveEvent: (%s) unknown state",
		     name( "unnamed" ) );
	    break;
	}
    }
}

void QHeader::handleColumnResize( int index, int s, bool final )
{
    int lim = pPos(index-1) + MINSIZE;
    if ( s == lim ) return;
    if ( s < lim ) s = lim;
    int oldPos = pPos( index );
    int delta = s - oldPos;
    int lIdx = mapToLogical(index - 1);
    int oldSize = data->sizes[lIdx];
    int newSize = data->sizes[lIdx] = oldSize + delta;
    int repaintPos = QMIN( oldPos, s );
    repaint(repaintPos-2, 0, width(), height());
    if ( tracking() && oldSize != newSize )
	emit sizeChange( lIdx, oldSize, newSize );
    else if ( final && oldHIdxSize != newSize )
	emit sizeChange( lIdx, oldHIdxSize, newSize );
}

/*!
  Returns the rectangle covered by actual section \a i.
*/

QRect QHeader::sRect( int i )
{
    if ( orient == Horizontal )
	return QRect( pPos( i ), 0, pSize( i ), height() );
    else
	return QRect( 0, pPos( i ), width(), pSize( i ) );
}

/*!
  Sets the text on logical section \a i to \a s. If the section does not exist,
  nothing happens.
  If \a size is non-negative, the section width is set to \a size.
*/

void QHeader::setLabel( int i, const QString &s, int size )
{
    if ( i >= 0 && i < count() ) {
	if ( data->labels[i] )
	    delete data->labels[i];
	data->labels[i] = new QString( s );
	if ( size >= 0 )
	    data->sizes[i] = size;
    }
    repaint();
}


/*!
  Returns the text set on logical section \a i.
*/
QString QHeader::label( int i )
{
    if ( data->labels[i] )
	return *(data->labels[i]);
    else
	return ""; // ### or 0?
}

/*!
  Adds a new section, with label text \a s. Returns the index.
  If \a size is non-negative, the section width is set to \a size,
  otherwise a size currently sufficient for the label text is used.
*/

int QHeader::addLabel( const QString &s, int size )
{
    int n = count() + 1; //###########
    data->labels.resize( n + 1 );
    data->labels[n-1] = new QString( s );
    data->sizes.resize( n + 1 );
    if ( size < 0 ) {
	QFontMetrics fm = fontMetrics();
        size = -fm.minLeftBearing()
            +fm.width( s )
            -fm.minRightBearing() + QH_MARGIN*2;
    }
    data->sizes[n-1] = size;
    data->a2l.resize( n + 1 );
    data->l2a.resize( n + 1 );
    data->a2l[n-1] = n-1;
    data->l2a[n-1] = n-1;
    data->clicks.resize(n+1);
    data->resize.resize(n+1);
    data->clicks.setBit(n-1);
    data->resize.setBit(n-1);

    //    recalc();
    if ( orient == Horizontal )
	setNumCols( n );
    else
	setNumRows( n );
    repaint(); //####
    return n - 1;
}


/*!
  Handles resize events.
*/

void QHeader::resizeEvent( QResizeEvent * )
{
    setCellHeight( height() ); //######
}

/*!
  Returns the recommended size of the QHeader.
*/
QSize QHeader::sizeHint() const
{
    QFontMetrics fm( font() );
    if ( orient == Horizontal )
	return QSize( count() > 0
		      ? cellSize( count()-1 ) + cellPos( count()-1 )
		      : -1,
		      fm.lineSpacing() + 6 );
    else
	return QSize( fm.lineSpacing() + 6,
		      count() > 0
		      ? cellSize( count()-1 ) + cellPos( count()-1 )
		      : -1 );
}


/*!
  Scrolls the header such that \a x becomes the leftmost (or uppermost
  for vertical headers) visible pixel.
*/

void QHeader::setOffset( int x )
{
    if ( orient == Horizontal )
	setXOffset( x );
    else
	setYOffset( x );
}



/*!
  Returns the position of actual division line \a i. May return a position
  outside the widget.
 */
int QHeader::pPos( int i ) const
{
    int r = 0;
    bool ok;
    if ( orient == Horizontal )
	ok = colXPos( i, &r );
    else
	ok = rowYPos( i, &r );
    if ( !ok ) {
	r = 0;
	for ( int j = 0; j < i; j++ )
	    r += pSize( j );
	r -= offset();
    }
    return r;
}


/*!
  Returns the size of actual section \a i.
 */
int QHeader::pSize( int i ) const
{
    return data->sizes[mapToLogical(i)];
}



/*!
  int QHeader::offset() const
  Returns the leftmost (or uppermost for vertical headers) visible pixel.
 */


int QHeader::offset() const
{
     if ( orient == Horizontal )
	return xOffset();
    else
	return yOffset();
}


/*! \reimp */

int QHeader::cellHeight( int row )
{
    if ( orient == Vertical )
	return pSize( row );
    else
	return QTableView::cellHeight();
}


/*! \reimp */

int QHeader::cellWidth( int col )
{
    if ( orient == Horizontal )
	return pSize( col );
    else
	return QTableView::cellWidth();
}


/*!
  Translates from actual index \a a to logical index.  Returns -1 if
  \a a is outside the legal range.
*/

int QHeader::mapToLogical( int a ) const
{
    return ( a >= 0 && a < count() ) ? data->a2l[ a ] : -1;
}


/*!
  Translates from logical index \a l to actual index.  Returns -1 if
  \a l is outside the legal range.
*/

int QHeader::mapToActual( int l ) const
{
    return ( l >= 0 && l < count() ) ? data->l2a[ l ] : -1;
}


/*!
  Sets the size of logical cell \a i to \a s pixels.

  \warning does not repaint or send out signals at present.
*/

void QHeader::setCellSize( int i, int s )
{
    data->sizes[i] = s;
}


/*!
  Enable user resizing of column \a i if \a enable is TRUE, disable otherwise.
  If \a i is negative, resizing is enabled/disabled for all columns.

  \sa setMovingEnabled(), setClickEnabled()
*/

void QHeader::setResizeEnabled( bool enable, int i )
{
    if ( i < 0 ) {
	data->resize.fill( enable );
    } else {
	data->resize[i] = enable;
    }
}


/*!
    Enable the user to exchange  columns if \a enable is TRUE,
    disable otherwise.

  \sa setClickEnabled(), setResizeEnabled()
*/

void QHeader::setMovingEnabled( bool enable )
{
    data->move = enable;
}


/*!
  Enable clicking in column \a i if \a enable is TRUE, disable otherwise.
  If \a i is negative, clicking is enabled/disabled for all columns.

  If enabled, the sectionClicked() signal is emitted when the user clicks.

  \sa setMovingEnabled(), setResizeEnabled()
*/

void QHeader::setClickEnabled( bool enable, int i )
{
    if ( i < 0 ) {
	data->clicks.fill( enable );
    } else {
	data->clicks[i] = enable;
    }
}
