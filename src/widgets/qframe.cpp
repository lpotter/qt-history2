/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qframe.cpp#23 $
**
** Implementation of QFrame widget class
**
** Author  : Haavard Nord
** Created : 950201
**
** Copyright (C) 1995 by Troll Tech AS.	 All rights reserved.
**
*****************************************************************************/

#include "qframe.h"
#include "qpainter.h"
#include "qdrawutl.h"
#include "qframe.h"

RCSTAG("$Id: //depot/qt/main/src/widgets/qframe.cpp#23 $")


/*!
  \class QFrame qframe.h
  \brief The QFrame class is the base class of widgets that have an (optional)
  frame.

  \ingroup abstractwidgets
  \ingroup realwidgets

  It draws a label and calls a virtual function, drawContents(), to
  fill in the frame.  QMenuBar, for example, uses this to "raise" the
  menu bar above the surrounding screen:

  \code
    if ( style() == MotifStyle ) {
        setFrameStyle( QFrame::Panel | QFrame::Raised );
        setLineWidth( motifBarFrame );
    }
    else {
        setFrameStyle( QFrame::NoFrame );
    }
  \endcode

  (motifBarFrame is an internal constant, not part of the API.)

  The QFrame class can also be used directly for creating simple frames
  without any contents, for example like this:

  \code
    QFrame * emptyFrame = new QFrame( parentWidget, "empty frame" );
    // if you use a pre-ANSI C++ compiler, check that new did not return 0
    emptyFrame->setFrameStyle( Panel | Sunken );
    emptyFrame->setLineWidth( 2 );
  \endcode

  A frame widget has three attributes: \link setFrameStyle() frame
  style\endlink, a \link setLineWidth() line width\endlink and a \link
  setMidLineWidth() mid-line width\endlink.

  The frame style is specified by a frame shape and a shadow style.
  The frame shapes are \c NoFrame, \c Box, \c Panel, \c WinPanel, \c
  HLine and \c VLine, and the shadow styles are \c Plain, \c Raised
  and \c Sunken.

  The line width is the width of the frame border.

  The mid-line width specifies the width of an extra line in the
  middle of the frame, that uses a third color to obtain a special 3D
  effect.  Notice that a mid-line is only drawn for \c Box, \c HLine
  and \c VLine frames that are raised or sunken.

  This table shows the most useful combinations of style and widths
  (and some rather useless ones):

  <img src=frames.gif height=422 width=520>

  For obvious reasons, \c NoFrame isn't shown.  The gray areas next to
  the \c VLine and \c HLine examples are there because the widgets are
  taller/wider than the natural width of the lines.  frameWidth()
  returns the natural width of the line.

  The labels on the top and right are QLabel objects with frameStyle()
  \c Raised|Panel and lineWidth() 1.

  */


/*!
  Constructs a frame widget with frame style \c NoFrame and 1 pixel frame
  width.

  The \e allowLines argument can be set to FALSE to disallow \c HLine and
  \c VLine shapes.

  The \e parent, \e name and \e f arguments are passed to the QWidget
  constructor.
*/

QFrame::QFrame( QWidget *parent, const char *name, WFlags f,
		bool allowLines )
    : QWidget( parent, name, f )
{
    initMetaObject();
    frect  = QRect( 0, 0, 0, 0 );
    fstyle = NoFrame;
    lwidth = 1;
    mwidth = 0;
    lineok = (short)allowLines;
    updateFrameWidth();
}


/*!
  \fn int QFrame::frameStyle() const
  Returns the frame style.

  The default value is QFrame::NoFrame.
  \sa setFrameStyle(), frameShape(), frameShadow()
*/

/*!
  \fn int QFrame::frameShape() const
  Returns the frame shape value from the frame style.
  \sa frameStyle(), frameShadow()
*/

/*!
  \fn int QFrame::frameShadow() const
  Returns the frame shadow value from the frame style.
  \sa frameStyle(), frameShape()
*/

/*!
  Sets the frame style to \e style.

  The \e style is the bitwise OR between a frame shape and a frame
  shadow style.

  The frame shapes are:
  <ul>
  <li> \c NoFrame draws nothing.
  <li> \c Box draws a rectangular box.
  <li> \c Panel draws a rectangular panel that can be raised or sunken.
  <li> \c WinPanel draws a rectangular panel that can be raised or sunken.
  Specifying this shape sets the line width to 2 pixels.
  <li> \c HLine draws a horizontal line (vertically centered).
  <li> \c VLine draws a vertical line (horizontally centered).
  </ul>

  The shadow styles are:
  <ul>
  <li> \c Plain draws using the palette foreground color.
  <li> \c Raised draws a 3D raised line using the light and dark
  colors of the current color group.
  <li> \c Sunken draws a 3D sunken line using the light and dark
  colors of the current color group.
  </ul>

  If a mid-line width greater than 0 is specified, an additional line
  is drawn for \c Raised or \c Sunken \c Box, \c HLine and \c VLine
  frames.  The mid color of the current color group is used for
  drawing middle lines.

  \warning Attempts to set the frame style to \c HLine or \c VLine
  (with any shadow style) are disregarded unless line shapes are
  allowed.  Line shapes are allowed by default.

  \sa frameStyle(), lineShapesOk(), colorGroup(), QColorGroup */

void QFrame::setFrameStyle( int style )
{
    if ( !lineShapesOk() ) {
	int t = style & QFrame::MShape;
	if ( t == QFrame::HLine || t == QFrame::VLine )
	    return;
    }
    fstyle = (short)style;
    updateFrameWidth();
}

/*!
  \fn bool QFrame::lineShapesOk() const
  Returns TRUE if line shapes (\c HLine or \c VLine) are allowed, or FALSE if
  they are not allowed.

  It is only possible to disallow line shapes in the constructor.
  The default value is TRUE.
*/

/*!
  \fn int QFrame::lineWidth() const
  Returns the line width.  (Note that the \e total line width
  for \c HLine and \c VLine is given by frameWidth(), not
  lineWidth().)

  The default value is 1.
  \sa setLineWidth(), midLineWidth(), frameWidth()
*/

/*!  Sets the line width to \e w.

  \sa frameWidth(), lineWidth(), setMidLineWidth() */

void QFrame::setLineWidth( int w )
{
    lwidth = (short)w;
    updateFrameWidth();
}

/*!
  \fn int QFrame::midLineWidth() const
  Returns the width of the mid-line.

  The default value is 0.
  \sa setMidLineWidth(), lineWidth(), frameWidth()
*/

/*!
  Sets the width of the mid-line to \e w.
  \sa midLineWidth(), setLineWidth()
*/

void QFrame::setMidLineWidth( int w )
{
    mwidth = (short)w;
    updateFrameWidth();
}


void QFrame::updateFrameWidth()
{
    int type  = fstyle & MShape;
    int style = fstyle & MShadow;

    fwidth = -1;

    switch ( type ) {

	case NoFrame:
	    fwidth = 0;
	    break;

	case Box:
	    switch ( style ) {
		case Plain:
		    fwidth = lwidth;
		    break;
		case Raised:
		case Sunken:
		    fwidth = (short)(lwidth*2 + mwidth);
		    break;
	    }
	    break;

	case Panel:
	    switch ( style ) {
		case Plain:
		case Raised:
		case Sunken:
		    fwidth = lwidth;
		    break;
	    }
	    break;

	case WinPanel:
	    switch ( style ) {
		case Plain:
		case Raised:
		case Sunken:
		    fwidth = lwidth = 2;
		    break;
	    }
	    break;

	case HLine:
	case VLine:
	    switch ( style ) {
		case Plain:
		    fwidth = lwidth;
		    break;
		case Raised:
		case Sunken:
		    fwidth = (short)(lwidth*2 + mwidth);
		    break;
	    }
	    break;
    }

    if ( fwidth == -1 ) {			// invalid style?
	fwidth = 0;
#if defined(CHECK_RANGE)
	warning( "QFrame::updateFrameWidth: Internal error" );
#endif
    }
    frameChanged();
}


/*!
  \fn int QFrame::frameWidth() const
  Returns the width of the frame that is drawn.

  Note that the frame width depends on the \link setFrameStyle() frame
  style \endlink, not only the line width and the mid line width.  For
  example, the style \c NoFrame always has a frame width 0, while the
  style \c Panel has a frame width equivalent to the line width.

  \sa lineWidth(), midLineWidth(), frameStyle() */


/*!
  Returns the frame rectangle.

  The default frame rectangle is equivalent to the \link
  QWidget::rect() widget rectangle\endlink.

  \sa setFrameRect()
*/

QRect QFrame::frameRect() const
{
    return frect.isNull() ? rect() : frect;
}


/*!
  Sets the frame rectangle to \e r.

  The frame rectangle is the rectangle the frame is drawn in.  By
  default, this is the entire widget.  Calling setFrameRect() does \e
  not cause a widget update.

  If \e r is a null rectangle (for example
  <code>QRect(0,0,0,0)</code>), then the frame rectangle is equivalent
  to the \link QWidget::rect() widget rectangle\endlink.

  \sa frameRect(), contentsRect() */

void QFrame::setFrameRect( const QRect &r )
{
    frect = r;
}


/*!
  Returns the rectangle inside the frame.
  \sa frameRect(), drawContents()
*/

QRect QFrame::contentsRect() const
{
    QRect r = frameRect();
    int	  w = frameWidth();			// total width
    r.setRect( r.x()+w, r.y()+w, r.width()-w*2, r.height()-w*2 );
    return r;
}


/*!
  Paints the frame.

  Opens the painter on the frame and calls first drawFrame(), then
  drawContents().
*/

void QFrame::paintEvent( QPaintEvent * )
{
    QPainter paint;
    paint.begin( this );
    drawFrame( &paint );
    drawContents( &paint );
    paint.end();
}


/*!
  Adjusts the frame rectangle for the resized widget.
  Nothing is done if the frame rectangle is a
  \link QRect::isNull() null rectangle\endlink.
*/

void QFrame::resizeEvent( QResizeEvent *e )
{
    if ( !frect.isNull() ) {
	QRect r( frect.x(), frect.y(),
		 width()  - (e->oldSize().width()  - frect.width()),
		 height() - (e->oldSize().height() - frect.height()) );
	setFrameRect( r );
    }
}


/*!  Draws the frame using the current frame attributes and color
  group.  The rectangle inside the frame is not affected.

  \sa frameRect() contentsRect() drawContents() frameStyle()
  setPalette() QColorGroup */

void QFrame::drawFrame( QPainter *p )
{
    QPoint	p1, p2;
    QRect	r     = frameRect();
    int		type  = fstyle & MShape;
    int		style = fstyle & MShadow;
    QColorGroup g     = colorGroup();

    switch ( type ) {

	case Box:
	    if ( style == Plain )
		drawPlainRect( p, r, g.foreground(), lwidth );
	    else
		drawShadeRect( p, r, g, style == Sunken, lwidth,
			       mwidth );
	    break;

	case Panel:
	    if ( style == Plain )
		drawPlainRect( p, r, g.foreground(), lwidth );
	    else
		drawShadePanel( p, r, g, style == Sunken, lwidth );
	    break;

	case WinPanel:
	    if ( style == Plain )
		drawPlainRect( p, r, g.foreground(), lwidth );
	    else
		drawWinPanel( p, r, g, style == Sunken );
	    break;

	case HLine:
	case VLine:
	    if ( type == HLine ) {
		p1 = QPoint( r.x(), r.height()/2 );
		p2 = QPoint( r.x()+r.width(), p1.y() );
	    }
	    else {
		p1 = QPoint( r.x()+r.width()/2, 0 );
		p2 = QPoint( p1.x(), r.height() );
	    }
	    if ( style == Plain ) {
		QPen oldPen = p->pen();
		p->setPen( QPen(g.foreground(),lwidth) );
		p->drawLine( p1, p2 );
		p->setPen( oldPen );
	    }
	    else
		drawShadeLine( p, p1, p2, g, style == Sunken,
			       lwidth, mwidth );
	    break;
    }
}


/*!
  Virtual function that draws the contents of the frame.

  This function is reimplemented by subclasses that draw something
  inside the frame.  It should draw only inside contentsRect().
  QFrame does not enable \link QPainter::setClipRect() clipping
  \endlink but you may want to.

  The QPainter is open when you get it, and you must leave it open.

  The default function does nothing.

  \sa contentsRect() QPainter::setClipRect() */

void QFrame::drawContents( QPainter * )
{
}


/*!
  Virtual function that is called when the frame style, line width or
  mid-line width changes.

  This function can be reimplemented by subclasses that need to know
  when the frame attributes change.  */

void QFrame::frameChanged()
{
}
