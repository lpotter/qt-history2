/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qlineedit.cpp#60 $
**
** Implementation of QLineEdit widget class
**
** Created : 941011
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
***********************************************************************/

#include "qlined.h"
#include "qpainter.h"
#include "qdrawutl.h"
#include "qfontmet.h"
#include "qpixmap.h"
#include "qkeycode.h"
#include "qclipbrd.h"
#include "qapp.h"

#include <ctype.h>

RCSTAG("$Id: //depot/qt/main/src/widgets/qlineedit.cpp#60 $");


/*!
  \class QLineEdit qlined.h

  \brief The QLineEdit widget is a simple line editor for inputting text.

  \ingroup realwidgets

  The default key bindings are described in keyPressEvent(); they cannot
  be customized except by inheriting the class.
*/


/*!
  \fn void QLineEdit::textChanged( const char * )
  This signal is emitted every time the text has changed.
  The argument is the new text.
*/


static const int blinkTime  = 500;		// text cursor blink time
static const int scrollTime = 100;		// mark text scroll time

#define LEFT_MARGIN 4
#define RIGHT_MARGIN 4
#define TOP_MARGIN 4
#define BOTTOM_MARGIN 4


static int xPosToCursorPos( char *s, const QFontMetrics &fm,
			    int xPos, int width )
{
    char *tmp;
    int	  dist;

    if ( xPos > width )
	xPos = width;
    if ( xPos <= 0 )
	return 0;
    dist = xPos;
    tmp	 = s;
    while ( *tmp && dist > 0 )
	dist -= fm.width( tmp++, 1 );
    if ( dist < 0 && ( xPos - dist > width || fm.width( tmp - 1, 1)/2 < -dist))
	tmp--;
    return tmp - s;
}

static int showLastPartOffset( char *s, const QFontMetrics &fm, int width )
{
    if ( !s || s[0] == '\0' )
	return 0;
    char *tmp = &s[strlen( s ) - 1];
    do {
	width -= fm.width( tmp--, 1 );
    } while ( tmp >=s && width >=0 );
    return width < 0 ? tmp - s + 2 : 0;
}


/*!
  Constructs a line editor with an empty edit buffer.

  The cursor position is set to the start of the line, the maximum buffer
  size to 32767 characters, and the buffer contents to "".

  The \e parent and \e name arguments are sent to the QWidget constructor.
*/

QLineEdit::QLineEdit( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
    initMetaObject();
    pm		  = 0;
    cursorPos	  = 0;
    offset	  = 0;
    maxLen	  = 32767;
    cursorOn	  = TRUE;
    markAnchor	  = 0;
    markDrag	  = 0;
    dragScrolling = FALSE;
    scrollingLeft = FALSE;
    tbuf	  = "";
    setAcceptFocus( TRUE );
    setCursor( ibeamCursor );
}

/*!
  Destroys the line editor.
*/

QLineEdit::~QLineEdit()
{
    delete pm;
}

/*!
  Sets the line editor text to \e text.

  If necessary the text is truncated to fit maxLength().

  \sa text()
*/

void QLineEdit::setText( const char *text )
{
    if ( tbuf == text )				// no change
	return;
    tbuf = text ? text : "";
    if ( (int)tbuf.length() > maxLen )
	tbuf.resize( maxLen+1 );
    cursorPos = 0;
    offset    = 0;
    repaint( !hasFocus() );
    emit textChanged( tbuf.data() );
}


/*!
  Selects all text (i.e. marks it) and does an "end" operation. Useful
  when a default value has been inserted. If the user types before
  clicking on the widget the selected text will be erased.
*/

void QLineEdit::selectAll()
{
    markAnchor = 0;
    markDrag   = 0;
    cursorPos  = 0;
    end( TRUE );
}



/*!
  Deselects all text (i.e. removes marking) and leaves the cursor at the
  current position.
*/

void QLineEdit::deselect()
{
    markAnchor = cursorPos;
    markDrag   = cursorPos;
    repaint( !hasFocus() );
}


/*!
  Returns a pointer to the text currently in the line.

  If you need to store the text, you should make a copy of it. This can
  conveniently be done with a QString object:
  \code
    QString s = lineEd->text();	 // makes a copy and stores it in s
  \endcode

  \sa setText()
*/

const char *QLineEdit::text() const
{
    return tbuf.data();
}

/*!
  Returns TRUE if part of the text has been marked by the user (e.g. by
  clicking and dragging).
*/

bool QLineEdit::hasMarkedText() const
{
    return markAnchor != markDrag;
}

/*!
  Returns the text marked by the user (e.g. by clicking and
  dragging), or 0 if no text is marked.
  \sa hasMarkedText()
*/

QString QLineEdit::markedText() const
{
    if ( markAnchor != markDrag ) {
	return tbuf.mid( minMark(), maxMark() - minMark() );
    } else {
	return 0;
    }
}

/*!
  Returns the current maximum length of the text in the editor.
  \sa setMaxLength()
*/

int QLineEdit::maxLength() const
{
    return maxLen;
}

/*!
  Set the maximum length of the text in the editor.  If the text is
  currently too long, it is chopped off at the limit. Any marked text will
  be unmarked.	The cursor position is set to 0 and the first part of the
  string is shown. \sa maxLength().
*/

void QLineEdit::setMaxLength( int m )
{
    maxLen = m;
    markAnchor = 0;
    markDrag   = 0;
    if ( (int)tbuf.length() > maxLen )
	tbuf.resize( maxLen + 1 );		// include \0
    offset    = 0;
    cursorPos = 0;
    repaint( !hasFocus() );
}

/*!
  \fn void  QLineEdit::returnPressed()
  This signal is emitted when the return or enter key is pressed.
*/

/*!
  The key press event handler converts a key press to some line editor
  action.

  If return or enter is pressed, the signal returnPressed will be emitted.

  Here are the default key bindings:
  <ul>
  <li><i> Left Arrow </i> Move the cursor one character leftwards
  <li><i> Right Arrow </i> Move the cursor one character rightwards
  <li><i> Backspace </i> Delete the character to the left of the cursor
  <li><i> Home </i> Move the cursor to the beginning of the line
  <li><i> End </i>	 Move the cursor to the end of the line
  <li><i> Delete </i> Delete the character to the right of the cursor
  <li><i> Shift - Left Arrow </i> Mark text one character leftwards
  <li><i> Shift - Right Arrow </i> Mark text one character rightwards
  <li><i> Control-A </i> Move the cursor to the beginning of the line
  <li><i> Control-B </i> Move the cursor one character leftwards
  <li><i> Control-C </i> Copy the marked text to the clipboard.
  <li><i> Control-D </i> Delete the character to the right of the cursor
  <li><i> Control-E </i> Move the cursor to the end of the line
  <li><i> Control-F </i> Move the cursor one character rightwards
  <li><i> Control-H </i> Delete the character to the left of the cursor
  <li><i> Control-V </i> Paste the clipboard text into line edit.
  <li><i> Control-X </i> Cut the marked text, copy to clipboard.
  </ul>

  <strong><a href=mailto:qt-bugs@troll.no>Comments solicited</a></strong>

  All other keys with valid ASCII codes insert themselves into the line.
*/

void QLineEdit::keyPressEvent( QKeyEvent *e )
{
    if ( e->key() == Key_Enter || e->key() == Key_Return ) {
	emit returnPressed();
	return;
    }
     if ( e->ascii() >= 32 && e->key() != Key_Delete ) {
	if ( (int)tbuf.length() < maxLen ) {
	    if ( hasMarkedText() ) {
		tbuf.remove( minMark(), maxMark() - minMark() );
		cursorPos = minMark();
		if ( cursorPos < offset )
		    offset = cursorPos;
	    }
	    tbuf.insert( cursorPos, e->ascii() );
	    cursorRight( FALSE );			// will repaint
	    emit textChanged( tbuf.data() );
	}
	return;
    }
    int unknown = 0;
    if ( e->state() & ControlButton ) {
	switch ( e->key() ) {
	    case Key_A:
	    case Key_Left:
		home( e->state() & ShiftButton );
		break;
	    case Key_B:
		cursorLeft( e->state() & ShiftButton );
		break;
	    case Key_C:
		if ( hasMarkedText() ) {
		    copyText();
		}
		break;
	    case Key_D:
		del();
		break;
	    case Key_E:
	    case Key_Right:
		end( e->state() & ShiftButton );
		break;
	    case Key_F:
		cursorRight( e->state() & ShiftButton );
		break;
	    case Key_H:
		backspace();
		break;
	    case Key_V: {			// paste
		QString t = QApplication::clipboard()->text();
		if ( !t.isEmpty() ) {
		    int i = t.find( '\n' );	// no multiline text
		    if ( i >= 0 )
			t.truncate( i );
		    uchar *p = (uchar *) t.data();
		    while ( *p ) {		// unprintable becomes space
			if ( *p < 32 )
			    *p = 32;
			p++;
		    }
		    if ( hasMarkedText() ) {
			tbuf.remove( minMark(), maxMark() - minMark() );
			cursorPos = minMark();
			if ( cursorPos < offset )
			    offset = cursorPos;
		    }
		    int tlen = t.length();
		    int blen = tbuf.length();
		    if ( tlen+blen >= maxLen ) {
			if ( blen >= maxLen )
			    break;
			t.truncate( maxLen-tlen );
		    }
		    tbuf.insert( cursorPos, t );
		    cursorRight( FALSE, tlen );
		    emit textChanged( tbuf.data() );
		}
		}
	    case Key_X:
		if ( hasMarkedText() ) {
		    copyText();
		    del();
		}
		break;
	    default:
		unknown++;
	}
    } else {
	switch ( e->key() ) {
	    case Key_Left:
		cursorLeft( e->state() & ShiftButton );
		break;
	    case Key_Right:
		cursorRight( e->state() & ShiftButton );
		break;
	    case Key_Backspace:
		backspace();
		break;
	    case Key_Home:
		home( e->state() & ShiftButton );
		break;
	    case Key_End:
		end( e->state() & ShiftButton );
		break;
	    case Key_Delete:
		del();
		break;
	    default:
		unknown++;
	}
    }

    if ( unknown ) {				// unknown key
	e->ignore();
	return;
    }
}


/*!
  Handles the cursor blinking.
*/

void QLineEdit::focusInEvent( QFocusEvent * )
{
    pm = new QPixmap( width(), height() );   // used for flicker-free update
    CHECK_PTR( pm );
    startTimer( blinkTime );
    cursorOn = TRUE;
    repaint( !hasFocus() );
}

/*!
  Handles the cursor blinking.
*/

void QLineEdit::focusOutEvent( QFocusEvent * )
{
    killTimers();
    delete pm;
    pm		  = 0;
    cursorOn	  = FALSE;
    dragScrolling = FALSE;
    repaint( !hasFocus() );
}


/*!
  Handles paint events for the line editor.
*/  

void QLineEdit::paintEvent( QPaintEvent * )
{
    paint( TRUE );
}


/*!
  This event is used to implement the blinking text cursor
  and scrolling when marking text.
*/

void QLineEdit::timerEvent( QTimerEvent * )
{
    if ( hasFocus() ) {
	if ( dragScrolling ) {
	    if ( scrollingLeft )
		cursorLeft( TRUE );	// mark left
	    else
		cursorRight( TRUE );	// mark right
	} else {
	    cursorOn = !cursorOn;
	    repaint( !hasFocus() );
	}
    }
}


/*!
  Handles resize events for this widget.
*/

void QLineEdit::resizeEvent( QResizeEvent *e )
{
    if ( hasFocus() ) {
	delete pm;
	pm = new QPixmap( e->size().width(), e->size().height() );
    }
    int max = lastCharVisible();
    if ( cursorPos > max ) {
	QFontMetrics fm = fontMetrics();
	int w = width() - LEFT_MARGIN - RIGHT_MARGIN;
	int i = cursorPos;
	while ( w > 0 && i > 0 ) {
	    i--;
	    w -= fm.width( tbuf.at(i) );
	}
	if ( w < 0 && i != cursorPos )
	    i++;
	offset = i;
    }
    repaint( !hasFocus() );
}

/*!
  Handles mouse press events for this widget.
*/

void QLineEdit::mousePressEvent( QMouseEvent *e )
{
    killTimers();
    cursorPos = offset + xPosToCursorPos( &tbuf[(int)offset], fontMetrics(),
				       e->pos().x() - LEFT_MARGIN,
				       width() - LEFT_MARGIN - RIGHT_MARGIN );
    if ( e->button() == MidButton ) {		// paste
	QKeyEvent k( Event_KeyPress, Key_V, 0, ControlButton );
	keyPressEvent( &k );
	return;
    }
    markAnchor = cursorPos;
    newMark( markAnchor, FALSE );
    cursorOn	  = TRUE;
    dragScrolling = FALSE;
    startTimer( blinkTime );
    repaint( !hasFocus() );
}

/*!
  Handles mouse move events for the line editor, primarily for
  marking text.
*/

void QLineEdit::mouseMoveEvent( QMouseEvent *e )
{
    if ( e->pos().x() < LEFT_MARGIN || e->pos().x() > width() - RIGHT_MARGIN) {
	scrollingLeft =	 ( e->pos().x() < LEFT_MARGIN );
	if ( !dragScrolling ) {
	    if ( scrollingLeft )
		newMark( offset, FALSE );
	    else
		newMark( lastCharVisible(), FALSE );
	    killTimers();
	    dragScrolling = TRUE;
	    cursorOn	  = TRUE;
	    startTimer( scrollTime );
	    repaint( !hasFocus() );
	} else {
	    if ( scrollingLeft ) {
		int steps = -(e->pos().x() + LEFT_MARGIN) / 15 + 2;
		cursorLeft( TRUE, steps );
	    } else {
		int steps = (e->pos().x() - width() +  RIGHT_MARGIN) / 15 + 2;
		cursorRight( TRUE, steps );
	    }
	}
    } else {
	dragScrolling = FALSE;
	int mousePos  = offset +
	    xPosToCursorPos( &tbuf[(int)offset], fontMetrics(),
			     e->pos().x() - LEFT_MARGIN,
			     width() - LEFT_MARGIN - RIGHT_MARGIN );
	newMark( mousePos, FALSE );
	cursorOn  = TRUE;
	killTimers();
	startTimer( blinkTime );
	repaint( !hasFocus() );
    }
}

/*!
  Handles mouse release events for this widget.
*/

void QLineEdit::mouseReleaseEvent( QMouseEvent * )
{
    copyText();
    if ( dragScrolling ) {
	dragScrolling = FALSE;
	killTimers();
	startTimer( blinkTime );
    }
}

/*!
  Handles mouse double click events for this widget.
*/

void QLineEdit::mouseDoubleClickEvent( QMouseEvent * )
{
    if ( dragScrolling ) {
	dragScrolling = FALSE;
	killTimers();
	startTimer( blinkTime );
    }
    markWord( cursorPos );
    repaint( !hasFocus() );
}

/*!
  \internal
  Repaints the line editor as needed. If the line editor is in
  focus, the line is painted using a pixmap buffer. If not, a faster
  but flickering drawing method is used.
*/

void QLineEdit::paint( bool frame )
{
    if ( hasFocus() ) {
	pixmapPaint();
    } else {
	QPainter p;
	p.begin( this );
	paintText( &p, size(), frame );
	p.end();
    }
}

/*!
  \internal
  Paints the line editor in a pixmap and then blts the pixmap onto the screen.
*/

void QLineEdit::pixmapPaint()
{
    if ( pm == 0 ) // be paranoid
	pm = new QPixmap( width(), height() );
    QPainter p;
    p.begin( pm );
    p.setFont( font() );
    paintText( &p, pm->size() , TRUE );
    p.end();
    bitBlt( this, 0, 0, pm, 0, 0, width(), height() );
}


/*!
  \internal
  Paints the line editor.
*/

void QLineEdit::paintText( QPainter *p, const QSize &s, bool frame )
{
    QColorGroup	 g    = colorGroup();
    QColor	 bg   = isEnabled() ? g.base() : g.background();
    QFontMetrics fm   = fontMetrics();
    char *displayText = &tbuf[(int)offset];
    int markBegin     = minMark();
    int markEnd	      = maxMark();

    if ( frame ) {
	QBrush fill( bg );
	qDrawWinPanel( p, 0, 0, s.width(), s.height(), g, TRUE, &fill );
    } else {
	p->fillRect( LEFT_MARGIN, TOP_MARGIN,
		     width()  - LEFT_MARGIN - RIGHT_MARGIN  + 1,
		     height() - TOP_MARGIN  - BOTTOM_MARGIN + 1, bg );
    }

    p->setClipRect( LEFT_MARGIN, TOP_MARGIN,
		    s.width()  - LEFT_MARGIN - RIGHT_MARGIN,
		    s.height() - TOP_MARGIN - BOTTOM_MARGIN );

    int charsVisible = lastCharVisible() - offset;
    if ( displayText[ charsVisible ] != '\0' )
	charsVisible++;

    int ypos = s.height() - BOTTOM_MARGIN - fm.descent() - 1 -
	       (s.height() - BOTTOM_MARGIN - TOP_MARGIN - fm.height())/2;
    int mark1,mark2; // start and end of inverted text, 0 = leftmost char

    if ( markBegin > offset ) {
	if ( markBegin <  offset + charsVisible )
	    mark1 = markBegin - offset;
	else
	    mark1 = charsVisible;
    } else {
	mark1 = 0;
    }

    if ( markEnd > offset ) {
	if ( markEnd <	offset + charsVisible )
	    mark2 = markEnd - offset;
	else
	    mark2 = charsVisible;
    } else {
	mark2 = 0;
    }
    if ( mark1 != mark2 ) {
	int xpos1 =  LEFT_MARGIN + fm.width( displayText, mark1 );
	int xpos2 =  xpos1 + fm.width( displayText + mark1, mark2 - mark1 ) -1;
	p->fillRect( xpos1, ypos - fm.ascent(),
		     xpos2 - xpos1, fm.height(), g.text() );
	p->setPen( g.base() );
	p->drawText( xpos1, ypos, displayText + mark1, mark2 - mark1 );
    }
    p->setPen( g.text() );
    if ( mark1 != 0 )
	p->drawText( LEFT_MARGIN, ypos, displayText, mark1 );
    if ( mark2 != charsVisible )
	p->drawText( LEFT_MARGIN + fm.width( displayText, mark2 ), ypos,
		     displayText + mark2, charsVisible - mark2 );

    p->setPen( g.foreground() );

    p->setClipping( FALSE );
    if ( cursorOn ) {
	int curXPos   = LEFT_MARGIN +
			fm.width( displayText, cursorPos - offset ) - 1;
	int curYPos   = ypos - fm.ascent();
	if ( hasFocus() ) {
	    p->drawLine( curXPos - 2, curYPos,
			 curXPos + 2, curYPos );
	    p->drawLine( curXPos    , curYPos,
			 curXPos    , curYPos + fm.height() - 1);
	    p->drawLine( curXPos - 2, curYPos + fm.height() - 1,
			 curXPos + 2, curYPos + fm.height() - 1);
	}
    }
}


/*!
  Moves the cursor leftwards one or more characters.
  \sa cursorRight()
*/

void QLineEdit::cursorLeft( bool mark, int steps )
{
    if ( steps < 0 ) {
	cursorRight( mark, -steps );
	return;
    }
    if ( cursorPos != 0 ) {
	killTimers();
	cursorOn = TRUE;
	cursorPos -= steps;
	if ( cursorPos < 0 )
	    cursorPos = 0;
	if ( mark ) {
	    newMark( cursorPos );
	} else {
	    markAnchor = cursorPos;
	    markDrag   = markAnchor;
	}
	if ( cursorPos < offset )
	    offset = cursorPos;
	startTimer( dragScrolling ? scrollTime : blinkTime );
	repaint( !hasFocus() );
    }
}

/*!
  Moves the cursor rightwards one or more characters.
  \sa cursorLeft()
*/

void QLineEdit::cursorRight( bool mark, int steps )
{
    if ( steps < 0 ) {
	cursorLeft( mark, -steps );
	return;
    }
    int len = (int)strlen( tbuf );
    if ( len > cursorPos ) {
	QFontMetrics fm = fontMetrics();
	killTimers();
	cursorOn   = TRUE;
	cursorPos += steps;
	if ( cursorPos > len )
	    cursorPos = len;
	if ( mark ) {
	    newMark( cursorPos );
	} else {
	    markAnchor = cursorPos;
	    markDrag   = markAnchor;
	}
	int surplusWidth = width() - LEFT_MARGIN - RIGHT_MARGIN
			   - fm.width( &tbuf[ offset ], cursorPos - offset);
	if ( surplusWidth < 0 ) {
	    while ( surplusWidth < 0 && offset < cursorPos - 1 ) {
		surplusWidth += fm.width( &tbuf[ offset ], 1 );
		offset++;
	    }
	}
	startTimer( dragScrolling ? scrollTime : blinkTime );
	repaint( !hasFocus() );
    }
}

/*!
  Deletes the character on the left side of the text cursor and moves the
  cursor one position to the left. If a text has been marked by the user
  (e.g. by clicking and dragging) the cursor will be put at the beginning
  of the marked text and the marked text will be removed.  \sa del()
*/

void QLineEdit::backspace()
{
    if ( hasMarkedText() ) {
	del();
    } else {
	if ( cursorPos > 0 ) {
	    cursorLeft( FALSE );
	    del();
	}
    }
}

/*!
  Deletes the character on the right side of the text cursor. If a text
  has been marked by the user (e.g. by clicking and dragging) the cursor
  will be put at the beginning of the marked text and the marked text will
  be removed.  \sa backspace()
*/

void QLineEdit::del()
{
    if ( hasMarkedText() ) {
	tbuf.remove( minMark(), maxMark() - minMark() );
	cursorPos  = minMark();
	markAnchor = cursorPos;
	markDrag   = cursorPos;
	if ( cursorPos < offset )
	    offset = cursorPos;
	repaint( !hasFocus() );
	emit textChanged( tbuf.data() );
    } else {
	if ( cursorPos != (int)strlen(tbuf) ) {
	    tbuf.remove( cursorPos, 1 );
	    repaint( !hasFocus() );
	    emit textChanged( tbuf.data() );
	}
    }
}

/*!
  Moves the text cursor to the left end of the line. If mark is TRUE text
  will be marked towards the first position, if not any marked text will
  be unmarked if the cursor is moved.  \sa end()
*/

void QLineEdit::home( bool mark )
{
    if ( cursorPos != 0 ) {
	killTimers();
	cursorPos = 0;
	if ( mark ) {
	    newMark( cursorPos );
	} else {
	    markAnchor = 0;
	    markDrag   = markAnchor;
	}
	offset	 = 0;
	cursorOn = TRUE;
	startTimer( dragScrolling ? scrollTime : blinkTime );
	repaint( !hasFocus() );
    }
}

/*!
  Moves the text cursor to the right end of the line. If mark is TRUE text
  will be marked towards the last position, if not any marked text will
  be unmarked if the cursor is moved.
  \sa home()
*/

void QLineEdit::end( bool mark )
{
    int tlen = strlen( tbuf );
    if ( cursorPos != tlen ) {
	killTimers();
	offset += showLastPartOffset( &tbuf[offset], fontMetrics(),
				      width() - LEFT_MARGIN - RIGHT_MARGIN );
	cursorPos = tlen;
	if ( mark ) {
	    newMark( cursorPos );
	} else {
	    markAnchor = cursorPos;
	    markDrag   = markAnchor;
	}
	cursorOn  = TRUE;
	startTimer( dragScrolling ? scrollTime : blinkTime );
	repaint( !hasFocus() );
    }
}


/*!
  Sets a new marked text limit, does not repaint the widget.
*/

void QLineEdit::newMark( int pos, bool copy )
{
    markDrag  = pos;
    cursorPos = pos;
    if ( copy )
	copyText();
}


void QLineEdit::markWord( int pos )
{
    int i = pos;
    while ( i >= 0 && isprint(tbuf.at(i)) && !isspace(tbuf.at(i)) )
	i--;
    if ( i != pos )
	i++;
    markAnchor = i;

    int lim = tbuf.length();
    i = pos;
    while ( i < lim && isprint(tbuf.at(i)) && !isspace(tbuf.at(i)) )
	i++;
    markDrag = i;

    int maxVis	  = lastCharVisible();
    int markBegin = minMark();
    int markEnd	  = maxMark();
    if ( markBegin < offset || markBegin > maxVis ) {
	if ( markEnd >= offset && markEnd <= maxVis ) {
	    cursorPos = markEnd;
	} else {
	    offset    = markBegin;
	    cursorPos = markBegin;
	}
    } else {
	cursorPos = markBegin;
    }
    copyText();
}


/*!
  Copies the marked text to the clipboard.
*/

void QLineEdit::copyText()
{
    QString t = markedText();
    if ( !t.isEmpty() ) {
#if defined(_WS_X11_)
	disconnect( QApplication::clipboard(), SIGNAL(dataChanged()), this, 0);
#endif
	QApplication::clipboard()->setText( t );
#if defined(_WS_X11_)
	connect( QApplication::clipboard(), SIGNAL(dataChanged()),
		 this, SLOT(clipboardChanged()) );
#endif
    }
}


/*!
  This private slot is activated when this line edit owns the clipboard and
  some other widget/application takes over the clipboard. (X11 only)
*/

void QLineEdit::clipboardChanged()
{
#if defined(_WS_X11_)
    disconnect( QApplication::clipboard(), SIGNAL(dataChanged()),
		this, SLOT(clipboardChanged()) );
    markDrag = markAnchor = cursorPos;
    repaint( !hasFocus() );
#endif
}



int QLineEdit::lastCharVisible() const
{
    int tDispWidth = width() - LEFT_MARGIN - RIGHT_MARGIN;
    return offset + xPosToCursorPos( &tbuf[(int)offset], fontMetrics(),
			       tDispWidth, tDispWidth );
}

int QLineEdit::minMark() const
{
    return markAnchor < markDrag ? markAnchor : markDrag;
}

int QLineEdit::maxMark() const
{
    return markAnchor > markDrag ? markAnchor : markDrag;
}

