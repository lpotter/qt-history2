/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qheader.cpp#81 $
**
** Implementation of QHeader widget class (table header)
**
** Created : 961105
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

#include "qheader.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qbitmap.h"
#include "qbitarray.h"

static const int GRIPMARGIN  = 4;		//half the size of the resize area
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
    : QWidget( parent, name )
{
    orient = Horizontal;
    init( 0 );
}

/*!
  Constructs a horizontal header with \a n sections.

  The \e parent and \e name arguments are sent to the QWidget constructor.

*/

QHeader::QHeader( int n,  QWidget *parent, const char *name )
    : QWidget( parent, name )
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
  Returns the position in pixels of section \a i of the header. The
  position is measured from the start of the first header. \a i is the
  actual index.
*/

int QHeader::cellPos( int i ) const
{
    int r = cachedPos;

    if ( i >=  cachedIdx  ) {
	for ( int j = cachedIdx; j < i; j++ )
	    r += pSize( j );
    } else { //###
	for ( int j = cachedIdx-1; j >= i; j-- )
	    r -= pSize( j );
    }
    if ( i != cachedIdx ) {
	QHeader *This = (QHeader*)this;	//mutable
	This->cachedIdx = i;
	This->cachedPos = r;
    }
    return r;
}


/*!
  Returns the number of sections in the header.
*/

int QHeader::count() const
{
    return data->labels.size() - 1;
}



/*!
  \fn Orientation QHeader::orientation() const

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
    offs = 0;
    cachedIdx = 0;
    cachedPos = 0;
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
	data->labels[i] = 0;
    }
    data->clicks.fill( TRUE );
    data->resize.fill( TRUE );
    data->move = TRUE;
    /*
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
    */
    handleIdx = 0;
    //################
    data->sizes[n] = 0;
    data->a2l[n] = 0;
    data->l2a[n] = 0;
    //#############
    setMouseTracking( TRUE );
    trackingIsOn = FALSE;
    setBackgroundMode( PaletteButton );
}

/*!
  Sets the header orientation.  The \e orientation must be
  QHeader::Vertical or QHeader::Horizontal.

  When adding labels without the size parameter, setOrientation
  should be called first, otherwise labels will be sized incorrectly.
  \sa orientation()
*/

void QHeader::setOrientation( Orientation orientation )
{
    if (orient==orientation) return;
    orient = orientation;
    /*
    int n = count();
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
    updateTableSize();
    */
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
  Returns the actual index of the section at widget position \a c, or -1 if outside.
 */
int QHeader::cellAt( int c ) const
{
    int pos = c + offset();
    int i = cachedIdx;
    if ( pos >= cachedPos ) {
	while ( i < count() && pos >= cellPos( i+1 ) )
	    i++;
    } else {
	while ( i >= 0 && pos < cellPos( i ) )
	    i--;
    }
    return i >= count() ? -1 : i;
}


/*!
  Tries to find a line that is not a neighbor of  \c handleIdx.
 */
int QHeader::findLine( int c )
{
    int i = cellAt( c );
    if ( i == -1 )
	return handleIdx; //####### frustrating, but safe behavior.
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

void QHeader::moveCell( int fromIdx, int toIdx )
{
    if ( fromIdx == toIdx ||
	 fromIdx < 0 || fromIdx > count() ||
	 toIdx < 0 || toIdx > count() )
	return;
    int i;
    if ( (fromIdx < cachedIdx) != (toIdx < cachedIdx ) ) {
	if ( fromIdx < cachedIdx ) {
	    //lose one section
	    cachedIdx--;
	    cachedPos -= pSize( fromIdx );
	} else {
	    //gain one section
	    cachedIdx++;
	    cachedPos += pSize( fromIdx );
	}
    }
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

void QHeader::mousePressEvent( QMouseEvent *m )
{
    if ( m->button() != LeftButton )
	return;
    handleIdx = 0;
    int c = orient == Horizontal ? m->pos().x() : m->pos().y();

    int i = cellAt( c );
    int p = pPos( i );

    if (  i != 0 && c < p + GRIPMARGIN || c > p + pSize( i ) - GRIPMARGIN ) {
	if ( c < p + GRIPMARGIN )
	    handleIdx = i;
	else
	    handleIdx = i+1;
	oldHIdxSize = cellSize( handleIdx - 1 );
	if ( data->resize.testBit(mapToLogical(handleIdx - 1)) )
	    state = Sliding;
	else
	    state = Blocked;
    } else if ( i >= 0 ) {
	handleIdx = i;
	moveToIdx = -1;
	if ( data->clicks.testBit(mapToLogical(i)) )
	    state = Pressed;
	else
	    state = Blocked;
	clickPos = c;
	repaint(sRect( handleIdx ));
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
	    moveCell( handleIdx, moveToIdx );
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
	int i = cellAt( s );
	int p = pPos( i );
	bool hit = FALSE;

	if (  s < p + GRIPMARGIN || s > p + pSize( i ) - GRIPMARGIN ) {
	    if ( s < p + GRIPMARGIN )
		i--;
	    if ( i >= 0 && data->resize.testBit(mapToLogical(i)) ) {
		hit = TRUE;
		if ( orient == Horizontal )
		    setCursor( splitHCursor );
		else
		    setCursor( splitVCursor );
	    }
	}
	if ( !hit )
	    setCursor( arrowCursor );
    } else {
	switch ( state ) {
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
    int lim = pPos(index-1) + 2*GRIPMARGIN;
    if ( s == lim ) return;
    if ( s < lim ) s = lim;
    int oldPos = pPos( index );
    int delta = s - oldPos;
    int lIdx = mapToLogical(index - 1);
    int oldSize = data->sizes[lIdx];
    int newSize = oldSize + delta;
    setCellSize( lIdx, newSize );
    int repaintPos = QMIN( oldPos, s );
    if ( orient == Horizontal )
        repaint(repaintPos-2, 0, width(), height());
    else
        repaint(0, repaintPos-oldSize+2, width(), height());
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
    if ( i < 0 || i >= count() )
	return rect(); // ### eeeeevil
    else if ( orient == Horizontal )
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
    if ( i < 0 || i >= count() )
	return;
    if ( i >= 0 && i < count() ) {
	if ( data->labels[i] )
	    delete data->labels[i];
	data->labels[i] = new QString( s );
	if ( size >= 0 ) {
	    setCellSize( i, size );
	}
    }
    repaint();
}


/*!
  Returns the text set on logical section \a i.
*/
QString QHeader::label( int i )
{
    if ( i < 0 || i >= count() )
	return QString::null;
    if ( data->labels[i] )
	return *(data->labels[i]);
    else
	return QString::null;
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
	const QFontMetrics & fm = fontMetrics();
        if ( orient == Horizontal )
            size = -fm.minLeftBearing()
                   +fm.width( s )
                   -fm.minRightBearing() + QH_MARGIN*2;
        else
            size = fm.lineSpacing() + 6; // Use same size as horizontal QHeader
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
#if 0
    //    recalc();
    if ( orient == Horizontal )
	setNumCols( n );
    else
	setNumRows( n );
#endif
    repaint(); //####
    return n - 1;
}


/*!
  Returns the recommended size of the QHeader.
*/
QSize QHeader::sizeHint() const
{ // #### seriously buggy if there are lots of columns
    QFontMetrics fm( font() );
    if ( orient == Horizontal )
	return QSize( count() > 0
		      ? cellSize( count()-1 ) + cellPos( count()-1 )
		      : -1,
		      fm.lineSpacing() + 6 );
    else {
        int width = fm.width( ' ' );
	for ( int i=0 ; i<count() ; i++ )
	    if ( data->labels[i] )
		width = QMAX( width , fm.width( *data->labels[i] ) );
	return QSize( width + 2*QH_MARGIN,
		      count() > 0
		      ? cellSize( count()-1 ) + cellPos( count()-1 )
		      : -1 );
    }
}

/*!
  A horizontal header may stretch horizontally, but is fixed
  vertically.
*/

QSizePolicy QHeader::sizePolicy() const
{
    if ( orient == Horizontal )
	return QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
    else
	return QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
}


/*!
  Scrolls the header such that \a x becomes the leftmost (or uppermost
  for vertical headers) visible pixel.
*/

void QHeader::setOffset( int x )
{
    int oldOff = offs;
    offs = x;
    if ( orient == Horizontal )
	scroll( oldOff-offs, 0 );
    else
	scroll( 0, oldOff-offs);
}



/*!
  Returns the position of actual division line \a i. May return a position
  outside the widget.

  Note that the last division line is numbered count(). (There are one more lines than
  sections).
 */
int QHeader::pPos( int i ) const
{
    if ( i < 0 || i > count() )
	return 0;

    return cellPos( i ) - offset();
}


/*!
  Returns the size of actual section \a i.
 */
int QHeader::pSize( int i ) const
{
    if ( i < 0 || i >= count() )
	return 0;

    return data->sizes[mapToLogical(i)];
}



/*!
  int QHeader::offset() const
  Returns the leftmost (or uppermost for vertical headers) visible pixel.
 */


int QHeader::offset() const
{
    return offs;
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
    if ( mapToActual(i) < cachedIdx )
	cachedPos += s - data->sizes[i];
    data->sizes[i] = s;
}


/*!
  Enable user resizing of logical column \a i if \a enable is TRUE, disable otherwise.
  If \a i is negative, resizing is enabled/disabled for all columns.

  \sa setMovingEnabled(), setClickEnabled()
*/

void QHeader::setResizeEnabled( bool enable, int i )
{
    if ( i < 0 )
	data->resize.fill( enable );
    else if ( i < count() )
	data->resize[i] = enable;
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
  Enable clicking in logical column \a i if \a enable is TRUE, disable otherwise.
  If \a i is negative, clicking is enabled/disabled for all columns.

  If enabled, the sectionClicked() signal is emitted when the user clicks.

  \sa setMovingEnabled(), setResizeEnabled()
*/

void QHeader::setClickEnabled( bool enable, int i )
{
    if ( i < 0 )
	data->clicks.fill( enable );
    else if ( i < count() )
	data->clicks[i] = enable;
}




/*!
  paints section \a id of the header, inside rectangle \a fr in widget coordinates.
*/

void QHeader::paintSection( QPainter *p, int id, QRect fr )
{
    bool down = (id==handleIdx) && ( state == Pressed || state == Moving );
    p->setBrushOrigin( fr.topLeft() );
    style().drawBevelButton(p, fr.x(), fr.y(), fr.width(), fr.height(), colorGroup(), down);

    int logIdx = mapToLogical(id);
    if ( logIdx < 0 )
	return;

    QString s;
    if ( data->labels[logIdx] )
	s = *(data->labels[logIdx]);
    else if ( orient == Horizontal )
	s.sprintf( "Col %d", logIdx );
    else
	s.sprintf( "Row %d", logIdx );

    int d = 0;
    if ( style() == WindowsStyle  &&
	 id==handleIdx && ( state == Pressed || state == Moving ) )
	d = 1;

    QRect r( fr.x() + QH_MARGIN+d, fr.y() + 2+d, fr.width() - 6, fr.height() - 4 );

    p->drawText ( r, AlignLeft| AlignVCenter|SingleLine, s );
}


/*!
  Paints the header.
*/

void QHeader::paintEvent( QPaintEvent *e )
{
    QPainter p( this );
    p.setPen( colorGroup().buttonText() );
    int pos = orient == Horizontal
		     ? e->rect().left()
		     : e->rect().top();
    int id = cellAt( pos );
    if ( id < 0 )
	if ( pos > 0 )
	    return;
	else
	    id = 0;
    for ( int i = id; i < count(); i++ ) {
	QRect r = sRect( i );
	paintSection( &p, i, r );
	if ( orient == Horizontal && r. right() >= e->rect().right() ||
	     orient == Vertical && r. bottom() >= e->rect().bottom() )
	    return;
    }

}


//#### what about lastSectionCoversAll?
