/****************************************************************************
** $Id: $
**
** Implementation of Motif-like style class
**
** Created : 981231
**
** Copyright (C) 1998-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
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
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qsgistyle.h"

#ifndef QT_NO_STYLE_SGI

#include "qpopupmenu.h"
#include "qapplication.h"
#include "qbutton.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qpixmap.h"
#include "qpalette.h"
#include "qwidget.h"
#include "qpushbutton.h"
#include "qscrollbar.h"
#include "qcombobox.h"
#include "qslider.h"
#include <limits.h>


static bool sliderMoving                = FALSE;
static bool sliderHandleActive          = FALSE;
static bool repaintByMouseMove          = FALSE;

static int activeScrollBarElement       = 0;
static QWidget *hotWidget		= 0;
static QPoint mousePos			= QPoint(-1,-1);

/*!
  \class QSGIStyle qsgistyle.h
  \brief The QSGIStyle class provides SGI/Irix look and feel.
  \ingroup appearance

  This class implements the SGI look and feel. It resembles the
  SGI/Irix Motif GUI style as closely as QStyle allows.
*/

/*!
  Constructs a QSGIStyle.

  If \a useHighlightCols is FALSE (default value), the style will
  polish the application's color palette to emulate the Motif way of
  highlighting, which is a simple inversion between the base and the
  text color.

  \sa QMotifStyle::useHighlightColors()
*/
QSGIStyle::QSGIStyle( bool useHighlightCols ) : QMotifStyle( useHighlightCols ), isApplicationStyle( 0 )
{
}

/*!
  Destroys the style.
*/
QSGIStyle::~QSGIStyle()
{
}

/*!
  \reimp

  Changes some application-wide settings to be
  SGI-like, e.g., sets bold/italic font for
  the menu system.
*/
void
QSGIStyle::polish( QApplication* app)
{
    isApplicationStyle = 1;
    QMotifStyle::polish( app );

    QFont f = QApplication::font();
    f.setBold( TRUE );
    f.setItalic( TRUE );
    QApplication::setFont( f, TRUE, "QPopupMenu" );
    QApplication::setFont( f, TRUE, "QMenuBar" );
    QApplication::setFont( f, TRUE, "QComboBox" );

    QPalette pal = QApplication::palette();
    // check this on SGI-Boxes
    //pal.setColor( QColorGroup::Background, pal.active().midlight() );
    if (pal.active().button() == pal.active().background())
        pal.setColor( QColorGroup::Button, pal.active().button().dark(120) );
    // darker basecolor in list-widgets
    pal.setColor( QColorGroup::Base, pal.active().base().dark(130) );
    if (! useHighlightColors() ) {
        pal.setColor( QPalette::Active, QColorGroup::Highlight, pal.active().text() );
        pal.setColor( QPalette::Active, QColorGroup::HighlightedText, pal.active().base() );
        pal.setColor( QPalette::Inactive, QColorGroup::Highlight, pal.inactive().text() );
        pal.setColor( QPalette::Inactive, QColorGroup::HighlightedText, pal.inactive().base() );
        pal.setColor( QPalette::Disabled, QColorGroup::Highlight, pal.disabled().text() );
        pal.setColor( QPalette::Disabled, QColorGroup::HighlightedText, pal.disabled().base() );
    }
    QApplication::setPalette( pal, TRUE );

    // different basecolor and highlighting in Q(Multi)LineEdit
    pal.setColor( QColorGroup::Base, QColor(211,181,181) );
    pal.setColor( QPalette::Active, QColorGroup::Highlight, pal.active().midlight() );
    pal.setColor( QPalette::Active, QColorGroup::HighlightedText, pal.active().text() );
    pal.setColor( QPalette::Inactive, QColorGroup::Highlight, pal.inactive().midlight() );
    pal.setColor( QPalette::Inactive, QColorGroup::HighlightedText, pal.inactive().text() );
    pal.setColor( QPalette::Disabled, QColorGroup::Highlight, pal.disabled().midlight() );
    pal.setColor( QPalette::Disabled, QColorGroup::HighlightedText, pal.disabled().text() );

    QApplication::setPalette( pal, TRUE, "QLineEdit" );
    QApplication::setPalette( pal, TRUE, "QMultiLineEdit" );

    pal = QApplication::palette();
    pal.setColor( QColorGroup::Button, pal.active().background() );
    QApplication::setPalette( pal, TRUE, "QMenuBar" );
    QApplication::setPalette( pal, TRUE, "QToolBar" );

}

/*! \reimp
*/
void
QSGIStyle::unPolish( QApplication* /* app */ )
{
    QFont f = QApplication::font();
    QApplication::setFont( f, TRUE, "QPopupMenu" );
    QApplication::setFont( f, TRUE, "QMenuBar" );
    QApplication::setFont( f, TRUE, "QComboBox" );
}

/*! \reimp

  Installs an event filter for several widget classes
  to enable hovering.
*/
void
QSGIStyle::polish( QWidget* w )
{
    QMotifStyle::polish(w);

    if ( !isApplicationStyle ) {
        QPalette sgiPal = QApplication::palette();

        sgiPal.setColor( QColorGroup::Background, sgiPal.active().midlight() );
        if (sgiPal.active().button() == sgiPal.active().background())
            sgiPal.setColor( QColorGroup::Button, sgiPal.active().button().dark(110) );
        sgiPal.setColor( QColorGroup::Base, sgiPal.active().base().dark(130) );
        if (! useHighlightColors() ) {
            sgiPal.setColor( QPalette::Active, QColorGroup::Highlight, sgiPal.active().text() );
            sgiPal.setColor( QPalette::Active, QColorGroup::HighlightedText, sgiPal.active().base() );
            sgiPal.setColor( QPalette::Inactive, QColorGroup::Highlight, sgiPal.inactive().text() );
            sgiPal.setColor( QPalette::Inactive, QColorGroup::HighlightedText, sgiPal.inactive().base() );
            sgiPal.setColor( QPalette::Disabled, QColorGroup::Highlight, sgiPal.disabled().text() );
            sgiPal.setColor( QPalette::Disabled, QColorGroup::HighlightedText, sgiPal.disabled().base() );
        }

        if ( w->inherits("QLineEdit") || w->inherits("QMultiLineEdit") ) {
            // different basecolor and highlighting in Q(Multi)LineEdit
            sgiPal.setColor( QColorGroup::Base, QColor(211,181,181) );
            sgiPal.setColor( QPalette::Active, QColorGroup::Highlight, sgiPal.active().midlight() );
            sgiPal.setColor( QPalette::Active, QColorGroup::HighlightedText, sgiPal.active().text() );
            sgiPal.setColor( QPalette::Inactive, QColorGroup::Highlight, sgiPal.inactive().midlight() );
            sgiPal.setColor( QPalette::Inactive, QColorGroup::HighlightedText, sgiPal.inactive().text() );
            sgiPal.setColor( QPalette::Disabled, QColorGroup::Highlight, sgiPal.disabled().midlight() );
            sgiPal.setColor( QPalette::Disabled, QColorGroup::HighlightedText, sgiPal.disabled().text() );

        } else if ( w->inherits("QMenuBar") || w->inherits("QToolBar") ) {
            sgiPal.setColor( QColorGroup::Button, sgiPal.active().midlight() );
        }

        w->setPalette( sgiPal );
    }

    if ( w->inherits("QButton") || w->inherits("QSlider") || w->inherits("QScrollBar") ) {
        w->installEventFilter( this );
        w->setMouseTracking( TRUE );
        if ( w->inherits("QToolButton") )
            w->setBackgroundMode( QWidget::PaletteBackground );
        if ( w->inherits("QScrollBar") )
            w->setBackgroundMode( QWidget::NoBackground );
    } else if ( w->inherits("QMenuBar") ) {
        ((QFrame*) w)->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        w->setBackgroundMode( QWidget::PaletteBackground );
    } else if ( w->inherits("QPopupMenu") ) {
        ((QFrame*) w)->setLineWidth( pixelMetric( PM_DefaultFrameWidth ) + 1 );
    } else if ( w->inherits("QToolBar") ) {
        w->setBackgroundMode( QWidget::PaletteBackground );
    } else if ( w->inherits("QToolBarSeparator") ) {
        w->setBackgroundMode( QWidget::PaletteBackground );
    }
}

/*! \reimp */
void
QSGIStyle::unPolish( QWidget* w )
{
    w->unsetPalette( );

    if ( w->inherits("QButton") || w->inherits("QSlider") || w->inherits("QScrollBar") )
        w->removeEventFilter( this );
}

/*! \reimp */
bool QSGIStyle::eventFilter( QObject* o, QEvent* e )
{
    if ( !o->isWidgetType() || e->type() == QEvent::Paint )
	return QMotifStyle::eventFilter( o, e );

    QWidget *widget = (QWidget*)o;

    switch ( e->type() ) {
    case QEvent::MouseButtonPress:
        {
            if ( widget->inherits("QSlider") ) {
/*
                sliderLastPosition.pos = QRect( 0, -1, 0, -1 );
                sliderLastPosition.slider = (QWidget*)o;
                sliderMoving = TRUE;
*/
            }
        }
        break;
    case QEvent::MouseButtonRelease:
        {
            if ( widget->inherits("QSlider") )
            {
/*
                sliderLastPosition.pos = QRect( 0, -1, 0, -1 );
                sliderLastPosition.slider = 0;
                sliderMoving = FALSE;
                ((QWidget*) o)->repaint( FALSE );
*/
            }
        }
        break;
    case QEvent::MouseMove:
        {
	    hotWidget = widget;
            QMouseEvent* me = (QMouseEvent*) e;
            mousePos = me->pos();
            if ( widget->inherits("QScrollBar") || widget->inherits("QSlider") ) {
                repaintByMouseMove = me->button() == NoButton;
                widget->repaint( FALSE );
                repaintByMouseMove = FALSE;
            }
        }
        break;
    case QEvent::Enter:
        hotWidget = widget;
        widget->repaint( FALSE );
        break;
    case QEvent::Leave:
        if ( widget == hotWidget) {
            hotWidget = 0;
            widget->repaint( FALSE );
        }
        break;
    default:
        break;
    }
    return QMotifStyle::eventFilter( o, e );
}

static const int sgiItemFrame           = 2;    // menu item frame width
static const int sgiSepHeight           = 1;    // separator item height
static const int sgiItemHMargin         = 3;    // menu item hor text margin
static const int sgiItemVMargin         = 2;    // menu item ver text margin
static const int sgiArrowHMargin        = 6;    // arrow horizontal margin
// static const int sgiTabSpacing               = 12;   // space between text and tab ### not used?!?
// static const int sgiCheckMarkHMargin = 2;    // horiz. margins of check mark ### not used?!?
static const int sgiCheckMarkSpace      = 20;

/*! \reimp */
int QSGIStyle::pixelMetric( PixelMetric metric, const QWidget *widget ) const
{
    switch ( metric ) {
    case PM_DefaultFrameWidth:
	return 2;

    case PM_ButtonDefaultIndicator:
	return 4;

    case PM_ScrollBarExtent:
	return 21;

    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
	return 20;

    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
	return 12;

    case PM_SplitterWidth:
	return QMAX( 10, QApplication::globalStrut().width() );

    default:
	break;
    }
    return QMotifStyle::pixelMetric( metric, widget );
}

static void drawPanel( QPainter *p, int x, int y, int w, int h,
		const QColorGroup &g, bool sunken,
		int lineWidth, const QBrush* fill)
{
    if ( w == 0 || h == 0 )
	return;
#if defined(CHECK_RANGE)
    ASSERT( w > 0 && h > 0 && lineWidth >= 0 );
#endif
    QPen oldPen = p->pen();			// save pen
    QPointArray a( 4*lineWidth );
    if ( sunken )
	p->setPen( g.dark() );
    else
	p->setPen( g.light() );
    int x1, y1, x2, y2;
    int i;
    int n = 0;
    x1 = x;
    y1 = y2 = y;
    x2 = x+w-2;
    for ( i=0; i<lineWidth; i++ ) {		// top shadow
	a.setPoint( n++, x1, y1++ );
	a.setPoint( n++, x2--, y2++ );
    }
    x2 = x1;
    y1 = y+h-2;
    for ( i=0; i<lineWidth; i++ ) {		// left shadow
	a.setPoint( n++, x1++, y1 );
	a.setPoint( n++, x2++, y2-- );
    }
    p->drawLineSegments( a );
    n = 0;
    if ( sunken )
	p->setPen( g.light() );
    else
	p->setPen( g.dark() );
    x1 = x;
    y1 = y2 = y+h-1;
    x2 = x+w-1;
    for ( i=0; i<lineWidth; i++ ) {		// bottom shadow
	a.setPoint( n++, x1++, y1-- );
	a.setPoint( n++, x2, y2-- );
    }
    x1 = x2;
    y1 = y;
    y2 = y+h-lineWidth-1;
    for ( i=0; i<lineWidth; i++ ) {		// right shadow
	a.setPoint( n++, x1--, y1++ );
	a.setPoint( n++, x2--, y2 );
    }
    p->drawLineSegments( a );
    if ( fill ) {				// fill with fill color
	QBrush oldBrush = p->brush();
	p->setPen( Qt::NoPen );
	p->setBrush( *fill );
	p->drawRect( x+lineWidth, y+lineWidth, w-lineWidth*2, h-lineWidth*2 );
	p->setBrush( oldBrush );
    }
    p->setPen( oldPen );			// restore pen
}

static void drawSeparator( QPainter *p, int x1, int y1, int x2, int y2,
			  const QColorGroup &g, bool sunken,
			  int /*lineWidth*/, int /*midLineWidth*/ )
{
    QPen oldPen = p->pen();

    p->setPen( g.midlight() );
    p->drawLine( x1, y1, x2, y2 );
    if (sunken) {
	p->setPen( g.shadow() );
	if ( y2-y1 < x2-x1 )
	    p->drawLine( x1, y1+1, x2, y2+1 );
	else
	    p->drawLine( x1+1, y1, x2+1, y2 );
    }

    p->setPen( oldPen );
}

static int get_combo_extra_width( int h, int *return_awh=0 )
{
    int awh;
    if ( h < 8 ) {
        awh = 6;
    } else if ( h < 14 ) {
        awh = h - 2;
    } else {
        awh = h/2;
    }
    if ( return_awh )
        *return_awh = awh;
    return awh*3/2;
}

static void get_combo_parameters( const QRect &r,
                                  int &ew, int &awh, int &ax,
                                  int &ay, int &sh, int &dh,
                                  int &sy )
{
    ew = get_combo_extra_width( r.height(), &awh );

    sh = (awh+3)/4;
    if ( sh < 3 )
        sh = 3;
    dh = sh/2 + 1;

    ay = r.y() + (r.height()-awh-sh-dh)/2;
    if ( ay < 0 ) {
        //panic mode
        ay = 0;
        sy = r.height();
    } else {
        sy = ay+awh+dh;
    }
    if( QApplication::reverseLayout() )
        ax = r.x();
    else
        ax = r.x() + r.width() - ew;
    ax  += (ew-awh)/2;
}

/*! \reimp */
void QSGIStyle::drawPrimitive( PrimitiveElement pe,
		    QPainter *p,
		    const QRect &r,
		    const QColorGroup &cg,
		    SFlags flags,
		    void **data ) const
{
    const int x = r.x();
    const int y = r.y();
    const int w = r.width();
    const int h = r.height();
    const bool sunken = flags & ( Style_Sunken | Style_Down | Style_On );
    const int defaultFrameWidth = pixelMetric( PM_DefaultFrameWidth );

    switch ( pe ) {
    case PE_ButtonCommand:
	drawPanel( p, x, y, w, h, cg, sunken, pixelMetric( PM_DefaultFrameWidth ),
		   ( sunken ? &cg.brush( QColorGroup::Mid ) :
			      &cg.brush( QColorGroup::Button ) ));
	break;

    case PE_ButtonBevel:
	{
	    drawPrimitive( PE_ButtonCommand, p, QRect( x+1, y+1, w-2, h-2 ), cg, flags, data );

	    QPen oldPen = p->pen();
	    QPointArray a;

	    // draw twocolored rectangle
	    p->setPen( sunken ? cg.light() : cg.dark().dark(200) );
	    a.setPoints( 3, x, y+h-1, x+w-1, y+h-1, x+w-1, y );
	    p->drawPolyline( a );
	    p->setPen( cg.dark() );
	    a.setPoints( 3, x, y+h-2, x, y, x+w-2, y );
	    p->drawPolyline( a );

	    p->setPen( oldPen );
	}
	break;

    case PE_ArrowUp:
    case PE_ArrowDown:
    case PE_ArrowLeft:
    case PE_ArrowRight:
	{
	    QPointArray a;				// arrow polygon
	    switch ( pe ) {
	    case PE_ArrowUp:
		a.setPoints( 3, 0,-5, -5,4, 4,4 );
		break;
	    case PE_ArrowDown:
		a.setPoints( 3, 0,4, -4,-4, 4,-4 );
		break;
	    case PE_ArrowLeft:
		a.setPoints( 3, -4,0, 4,-5, 4,4 );
		break;
	    case PE_ArrowRight:
		a.setPoints( 3, 4,0, -4,-5, -4,4 );
		break;
	    default:
		return;
	    }

	    QPen savePen = p->pen();			// save current pen
	    p->setPen( Qt::NoPen );
	    a.translate( x+w/2, y+h/2 );
	    p->setBrush( flags & Style_Enabled ? cg.dark() : cg.light() );
	    p->drawPolygon( a );			// draw arrow
	    p->setPen( savePen );			// restore pen
	}
	break;

    case PE_Indicator:
	{
	    QPen oldPen = p->pen();

	    p->fillRect( x, y, w, h, cg.brush( QColorGroup::Background ) );

	    drawPrimitive( PE_ButtonBevel, p, r, cg, flags, data );

	    if ( !(flags & QStyle::Style_Off) )
		drawPrimitive( PE_CheckMark, p, r, cg, flags, data );

	    p->setPen( oldPen );
	}
	break;

    case PE_IndicatorMask:
	{
	    QPen oldPen = p->pen();
	    QBrush oldBrush = p->brush();

	    p->setPen( Qt::color1 );
	    p->setBrush( Qt::color1 );
	    p->fillRect( x, y, w, h, QBrush( Qt::color0 ) );
	    p->fillRect( x+2, y+3, w-7, h-7, QBrush( Qt::color1 ) );

	    if ( !(flags & QStyle::Style_Off) ) {
		static QCOORD check_mark[] = {
			14,0,  10,0,  11,1,  8,1,  9,2,	 7,2,  8,3,  6,3,
			7,4,  1,4,  6,5,  1,5,	6,6,  3,6,  5,7,  4,7,
			5,8,  5,8,  4,3,  2,3,	3,2,  3,2 };

		QPointArray amark;
		amark = QPointArray( sizeof(check_mark)/(sizeof(QCOORD)*2), check_mark );
		amark.translate( x+w-18, y+h-14 );
		p->drawLineSegments( amark );
		amark.translate( +1, +1 );
		p->drawLineSegments( amark );
	    }

	    p->setBrush( oldBrush );
	    p->setPen( oldPen );
	}
	break;

    case PE_CheckMark:
	{
	    static QCOORD check_mark[] = {
		14,0,  10,0,  11,1,  8,1,  9,2,	 7,2,  8,3,  6,3,
		7,4,  1,4,  6,5,  1,5,	6,6,  3,6,  5,7,  4,7,
		5,8,  5,8,  4,3,  2,3,	3,2,  3,2 };

	    QPen oldPen = p->pen();

	    QPointArray amark;
	    amark = QPointArray( sizeof(check_mark)/(sizeof(QCOORD)*2), check_mark );
	    amark.translate( x+1, y+1 );

	    if ( flags & Style_On ) {
		p->setPen( flags & Style_Enabled ? cg.shadow() : cg.dark() );
		p->drawLineSegments( amark );
		amark.translate( -1, -1 );
		p->setPen( flags & Style_Enabled ? QColor(255,0,0) : cg.dark() );
		p->drawLineSegments( amark );
		p->setPen( oldPen );
	    } else {
		p->setPen( flags & Style_Enabled ? cg.dark() : cg.mid() );
		p->drawLineSegments( amark );
		amark.translate( -1, -1 );
		p->setPen( flags & Style_Enabled ? QColor(230,120,120) : cg.dark() );
		p->drawLineSegments( amark );
		p->setPen( oldPen );
	    }
	}
	break;

    case PE_ExclusiveIndicator:
	{
	    p->save();
	    p->eraseRect( x, y, w, h );
	    p->translate( x, y );

	    p->setPen( cg.button() );
	    p->setBrush( cg.button() );
	    QPointArray a;
	    a.setPoints( 4, 5,0, 11,6, 6,11, 0,5);
	    p->drawPolygon( a );

	    p->setPen( cg.dark() );
	    p->drawLine( 0,5, 5,0 );
	    p->drawLine( 6,0, 11,5 );
	    p->setPen( flags & Style_Down ? cg.light() : cg.dark().dark(200) );
	    p->drawLine( 11,6, 6,11 );
	    p->drawLine( 5,11, 0,6 );
	    p->drawLine( 2,7, 5,10 );
	    p->drawLine( 6,10, 9,7 );
	    p->setPen( cg.light() );
	    p->drawLine( 2,5, 5,2 );

	    if ( flags & Style_On ) {
		p->setPen( flags & Style_Enabled ? Qt::blue : Qt::darkGray );
		p->setBrush( flags & Style_Enabled ? Qt::blue : Qt::darkGray  );
		a.setPoints(3, 6,2, 8,4, 6,6 );
		p->drawPolygon( a );
		p->setBrush( Qt::NoBrush );

		p->setPen( cg.shadow() );
		p->drawLine( 7,7, 9,5 );
	    } else {
		p->drawLine( 6,2, 9,5 );
	    }
	    p->restore();
	}
	break;

    case PE_ExclusiveIndicatorMask:
	{
	    p->save();
	    QPen oldPen = p->pen();
	    QBrush oldBrush = p->brush();

	    p->setPen( Qt::color1 );
	    p->setBrush( Qt::color1 );
	    QPointArray a;
	    a.setPoints( 8, 0,5, 5,0, 6,0, 11,5, 11,6, 6,11, 5,11, 0,6 );
	    a.translate( x, y );
	    p->drawPolygon( a );

	    p->setBrush( oldBrush );
	    p->setPen( oldPen );
	    p->restore();
	}
	break;

    case PE_Panel:
    case PE_PanelPopup:
	{
	    const int lineWidth = data ? *(int*)data[0] : defaultFrameWidth;
	    drawPanel( p, x, y, w, h, cg, flags & (Style_Sunken | Style_Down | Style_On), lineWidth, 0 );
	    if ( lineWidth <= 1 )
		return;

	    // draw extra shadinglines
	    QPen oldPen = p->pen();
	    p->setPen( cg.midlight() );
	    p->drawLine( x+1, y+h-3, x+1, y+1 );
	    p->drawLine( x+1, y+1, x+w-3, y+1 );
	    p->setPen( cg.mid() );
	    p->drawLine( x+1, y+h-2, x+w-2, y+h-2 );
	    p->drawLine( x+w-2, y+h-2, x+w-2, y+1 );
	    p->setPen(oldPen);
	}
	break;

    case PE_ScrollBarSubLine:
	drawPrimitive(((flags & Style_Horizontal) ? PE_ArrowLeft : PE_ArrowUp),
		      p, r, cg, Style_Enabled | flags);
	break;

    case PE_ScrollBarAddLine:
	drawPrimitive(((flags & Style_Horizontal) ? PE_ArrowRight : PE_ArrowDown),
		      p, r, cg, Style_Enabled | flags);
	break;

    case PE_ScrollBarSubPage:
    case PE_ScrollBarAddPage:
	p->fillRect(r, cg.brush(QColorGroup::Mid));
	break;

    case PE_ScrollBarSlider:
	drawPrimitive(PE_ButtonBevel, p, r, cg, Style_Enabled | Style_Raised);
	break;

    default:
	QMotifStyle::drawPrimitive( pe, p, r, cg, flags, data );
	break;
    }
}

/*! \reimp */
void QSGIStyle::drawControl( ControlElement element,
		  QPainter *p,
		  const QWidget *widget,
		  const QRect &r,
		  const QColorGroup &cg,
		  SFlags flags,
		  void **data ) const
{
    switch ( element ) {
    case CE_PushButton:
	{
	    const QPushButton *btn = (QPushButton*)widget;
	    int x1, y1, x2, y2;
	    r.coords( &x1, &y1, &x2, &y2 );

	    p->setPen( cg.foreground() );
	    p->setBrush( QBrush( cg.button(),Qt::NoBrush ) );

	    int diw = pixelMetric( QStyle::PM_ButtonDefaultIndicator );
	    if ( btn->isDefault() || btn->autoDefault() ) {
		x1 += diw;
		y1 += diw;
		x2 -= diw;
		y2 -= diw;
	    }

	    QPointArray a;
	    if ( btn->isDefault() ) {
		if ( diw == 0 ) {
		    a.setPoints( 9,
				 x1, y1, x2, y1, x2, y2, x1, y2, x1, y1+1,
				 x2-1, y1+1, x2-1, y2-1, x1+1, y2-1, x1+1, y1+1 );
		    p->setPen( cg.shadow() );
		    p->drawPolyline( a );
		    x1 += 2;
		    y1 += 2;
		    x2 -= 2;
		    y2 -= 2;
		} else {
		    qDrawShadePanel( p, btn->rect(), cg, TRUE );
		}
	    }

	    QBrush fill = cg.brush( QColorGroup::Button );
	    if ( !btn->isFlat() || btn->isOn() || btn->isDown() )
		drawPrimitive( PE_ButtonBevel, p, QRect( x1, y1, x2-x1+1, y2-y1+1 ), cg, flags, data );

	    if ( p->brush().style() != Qt::NoBrush )
		p->setBrush( Qt::NoBrush );
	}
    break;

    default:
	QMotifStyle::drawControl( element, p, widget, r, cg, flags, data );
	break;
    }    
}

/*! \reimp */
void QSGIStyle::drawComplexControl( ComplexControl control,
			 QPainter *p,
			 const QWidget* widget,
			 const QRect& r,
			 const QColorGroup& cg,
			 SFlags flags,
			 SCFlags sub,
			 SCFlags subActive,
			 void **data ) const
{
    switch ( control ) {
    case CC_Slider:
	{
	    const QSlider * slider = (const QSlider *) widget;

	    QRect groove = querySubControlMetrics(CC_Slider, widget, SC_SliderGroove,
						  data),
		  handle = querySubControlMetrics(CC_Slider, widget, SC_SliderHandle,
						  data);

	    if ((sub & SC_SliderGroove) && groove.isValid()) {
		qDrawShadePanel( p, groove, cg, TRUE, 2,
				 &cg.brush( QColorGroup::Mid ) );

		if ( slider->hasFocus() ) {
		    QRect fr = subRect( SR_SliderFocusRect, widget );
		    drawPrimitive( PE_FocusRect, p, fr, cg );
		}
	    }

	    if (( sub & SC_SliderHandle ) && handle.isValid()) {
		drawPrimitive( PE_ButtonBevel, p, handle, cg );

		if ( slider->orientation() == Horizontal ) {
		    QCOORD mid = handle.x() + handle.width() / 2;
		    qDrawShadeLine( p, mid, handle.y(), mid,
				    handle.y() + handle.height() - 2,
				    cg, TRUE, 1);
		} else {
		    QCOORD mid = handle.y() + handle.height() / 2;
		    qDrawShadeLine( p, handle.x(), mid,
				    handle.x() + handle.width() - 2, mid,
				    cg, TRUE, 1);
		}
	    }

	    if ( sub & SC_SliderTickmarks )
		QMotifStyle::drawComplexControl( control, p, widget, r, cg, flags,
						  SC_SliderTickmarks, subActive,
						  data );

	    break;
	}
    case CC_ComboBox:
	if ( sub & SC_ComboBoxArrow ) {
	    int awh, ax, ay, sh, sy, dh, ew;
	    get_combo_parameters( widget->rect(), ew, awh, ax, ay, sh, dh, sy );

//	    drawBevelButton( p, x, y, w, h, g, FALSE, &fill );

	    QBrush arrow = cg.brush( QColorGroup::Dark );
	    drawPrimitive( PE_ArrowDown, p, QRect( ax, ay, awh, awh ), cg, flags, data );

	    p->fillRect( ax, sy, awh, sh, arrow );
	}
	if ( sub & SC_ComboBoxEditField ) {
	    const QComboBox * cb = (QComboBox *) widget;
	    if ( cb->editable() ) {
		QRect er = QStyle::visualRect( querySubControlMetrics( CC_ComboBox, cb,
								       SC_ComboBoxEditField ), cb );
		er.addCoords( -1, -1, 1, 1);
		qDrawShadePanel( p, QRect( er.x()-1, er.y()-1, er.width()+2, er.height()+2 ),
				 cg, TRUE, 1, &cg.brush( QColorGroup::Button ) );
	    }
	}
	break;

    case CC_ScrollBar:
	{
	    if (sub == (SC_ScrollBarAddLine | SC_ScrollBarSubLine | SC_ScrollBarAddPage |
			SC_ScrollBarSubPage | SC_ScrollBarFirst | SC_ScrollBarLast |
			SC_ScrollBarSlider))
		qDrawShadePanel(p, widget->rect(), cg, TRUE,
				pixelMetric(PM_DefaultFrameWidth, widget),
				&cg.brush(QColorGroup::Mid));
	    QMotifStyle::drawComplexControl(control, p, widget, r, cg, flags, sub,
					     subActive, data);
	}
	break;

    default:
	QMotifStyle::drawComplexControl( control, p, widget, r, cg, flags, sub, subActive, data );
	break;
    }
}

/*! \reimp */
QRect QSGIStyle::subRect( SubRect r, const QWidget *widget ) const
{
    QRect rect;

    switch ( r ) {
    case SR_ComboBoxFocusRect:
	{
	    int awh, ax, ay, sh, sy, dh, ew;
	    int fw = pixelMetric( PM_DefaultFrameWidth, widget );
	    QRect tr = widget->rect();

	    tr.addCoords( fw, fw, -fw, -fw );
	    get_combo_parameters( tr, ew, awh, ax, ay, sh, dh, sy );
	    rect.setRect(ax-2, ay-2, awh+4, awh+sh+dh+4);
	}
	break;
    default:
	return QMotifStyle::subRect( r, widget );
    }
    
    return rect;
}

/*! \reimp */
QRect QSGIStyle::querySubControlMetrics( ComplexControl control,
					   const QWidget *widget,
					   SubControl sub,
					   void **data ) const
{
    QRect rect;

    switch ( control ) {
    case CC_ComboBox:
	switch ( sub ) {
	case SC_ComboBoxArrow:
	    {
		int ew, awh, sh, dh, ax, ay, sy;
		int fw = pixelMetric( PM_DefaultFrameWidth, widget );
		QRect cr = widget->rect();
		cr.addCoords( fw, fw, -fw, -fw );
		get_combo_parameters( cr, ew, awh, ax, ay, sh, dh, sy );
		rect.setRect( ax, ay, awh, awh );
	    }
	    break;

	case SC_ComboBoxEditField:
	    {
		int fw = pixelMetric( PM_DefaultFrameWidth, widget );
		rect = widget->rect();
		rect.addCoords( fw, fw, -fw, -fw );
		int ew = get_combo_extra_width( rect.height() );

 		rect.addCoords( 1, 1, -1-ew, -1 );
	    }
	    break;

	default:
	    break;
	}
	break;

    default:
	return QMotifStyle::querySubControlMetrics( control, widget, sub, data );
    }

    return rect;
}

/*!
    Draws a raised shape used as a combobox.
 *
void
QSGIStyle::drawComboButton( QPainter *p, int x, int y, int w, int h,
                            const QColorGroup &g,
                            bool sunken,
                            bool editable,
                            bool enabled,
                            const QBrush *fb )
{
    QBrush fill = fb ? *fb : g.brush( QColorGroup::Button );

    int awh, ax, ay, sh, sy, dh, ew;
    get_combo_parameters( buttonRect(x,y,w,h), ew, awh, ax, ay, sh, dh, sy );

    drawBevelButton( p, x, y, w, h, g, FALSE, &fill );

    QBrush arrow = g.brush( QColorGroup::Dark );
    drawArrow( p, DownArrow, FALSE, ax, ay, awh, awh, g, TRUE );

    p->fillRect( ax, sy, awh, sh, arrow );

    if ( editable ) {
        QRect r( comboButtonRect( x, y, w, h ) );
        qDrawShadePanel( p, QRect( r.x()-1, r.y()-1, r.width()+2, r.height()+2 ),
                         g, TRUE, 1, &fill );
    }
}


#define HORIZONTAL      (sb->orientation() == QScrollBar::Horizontal)
#define VERTICAL        !HORIZONTAL
#define SGI_BORDER      1
#define SLIDER_MIN      9 //### motif says 6 but that's too small
#define ADD_LINE_ACTIVE ( activeControl == AddLine )
#define SUB_LINE_ACTIVE ( activeControl == SubLine )

/*!\reimp
 *
void QSGIStyle::scrollBarMetrics( const QScrollBar* sb,
                                  int &sliderMin, int &sliderMax,
                                  int &sliderLength, int&buttonDim ) const
{
    int maxLength;
    int b = SGI_BORDER;
    int length = HORIZONTAL ? sb->width()  : sb->height();
    int extent = HORIZONTAL ? sb->height() : sb->width();

    if ( length > ( extent - b*2 - 1 )*2 + b*2 )
        buttonDim = extent - b*2;
    else
        buttonDim = ( length - b*2 )/2 - 1;

    sliderMin = b + buttonDim;
    maxLength  = length - b*2 - buttonDim*2;

    if ( sb->maxValue() == sb->minValue() ) {
        sliderLength = maxLength;
    } else {
        sliderLength = (sb->pageStep()*maxLength)/
                       (sb->maxValue()-sb->minValue()+sb->pageStep());
        uint range = sb->maxValue()-sb->minValue();
        if ( sliderLength < SLIDER_MIN || range > INT_MAX/2 )
            sliderLength = SLIDER_MIN;
        if ( sliderLength > maxLength )
            sliderLength = maxLength;
    }
    sliderMax = sliderMin + maxLength - sliderLength;
}

static QRect scrollerStartOldPos(1, -1, 1, -1);
static bool scrollerMoving = FALSE;

/*!
    Draws scroll bar controls in SGI-like style.
*
void
QSGIStyle::drawScrollBarControls( QPainter* p, const QScrollBar* sb,
                                  int sliderStart, uint controls, uint activeControl )
{
    ( (QScrollBar*)sb )->setBackgroundMode( QWidget::PaletteButton );
    QColorGroup g = sb->colorGroup();

    QColor lazyButton;
    lazyButton = QApplication::palette().active().button();

    int sliderMin, sliderMax, sliderLength, buttonDim;
    scrollBarMetrics( sb, sliderMin, sliderMax, sliderLength, buttonDim );

    if ( sliderStart > sliderMax )
        sliderStart = sliderMax;

    int b = SGI_BORDER;
    int dimB = buttonDim;
    QRect addB;
    QRect subB;
    QRect addPageR;
    QRect subPageR;
    QRect sliderR;
    int addX, addY, subX, subY, sliderM;
    int length = HORIZONTAL ? sb->width() : sb->height();
    int extent = HORIZONTAL ? sb->height() : sb->width();

    if ( HORIZONTAL ) {
        subY = addY = ( extent - dimB ) / 2;
        subX = b;
        addX = length - dimB - b;
    } else {
        subX = addX = ( extent - dimB ) / 2;
        subY = b;
        addY = length - dimB - b;
    }

    subB.setRect( subX,subY,dimB,dimB );
    addB.setRect( addX,addY,dimB,dimB );

    int sliderEnd = sliderStart + sliderLength;
    int sliderW = extent - b*2;
    if ( HORIZONTAL ) {
        subPageR.setRect( subB.right() + 1, b,
                          sliderStart - subB.right() - 1 , sliderW );
        addPageR.setRect( sliderEnd, b, addX - sliderEnd, sliderW );
        sliderR .setRect( sliderStart, b, sliderLength, sliderW );
    } else {
        subPageR.setRect( b, subB.bottom() + 1, sliderW,
                          sliderStart - subB.bottom() - 1 );
        addPageR.setRect( b, sliderEnd, sliderW, addY - sliderEnd );
        sliderR .setRect( b, sliderStart, sliderW, sliderLength );
    }
    sliderM = sliderStart + sliderLength / 2;

    bool isScrollBarUpToDate = FALSE;

    if ( repaintByMouseMove ) {
        if ( addB.contains( mousePos ) ) {
            isScrollBarUpToDate = ( activeScrollBarElement == AddLine );
            activeScrollBarElement = AddLine;
        } else if ( subB.contains( mousePos )) {
            isScrollBarUpToDate = ( activeScrollBarElement == SubLine );
            activeScrollBarElement = SubLine;
        } else if ( sliderR.contains( mousePos )) {
            isScrollBarUpToDate = ( activeScrollBarElement == Slider );
            activeScrollBarElement = Slider;
        } else {
            activeScrollBarElement = 0;
        }
    } else
    {
        activeScrollBarElement = 0;
    }

    if ( !isScrollBarUpToDate )
    {
        QBrush fill( lazyButton );
        if ( controls & AddLine && addB.isValid() ) {
            drawButton( p, addB.x(), addB.y(),
                        addB.width(), addB.height(), g, FALSE,
                        (deviceUnderMouse == p->device()
                        && addB.contains(mousePos)
                        && !ADD_LINE_ACTIVE ) ?
                            &g.brush( QColorGroup::Midlight ) : &fill );
            drawArrow( p, VERTICAL ? DownArrow : RightArrow,
                       ADD_LINE_ACTIVE, addB.x()+2, addB.y()+2,
                       addB.width()-4, addB.height()-4, g, TRUE );
        }
        if ( controls & SubLine && subB.isValid() ) {
            drawButton( p, subB.x(), subB.y(),
                        subB.width(), subB.height(), g, FALSE,
                        (deviceUnderMouse == p->device()
                        && subB.contains(mousePos)
                        && !SUB_LINE_ACTIVE ) ?
                            &g.brush( QColorGroup::Midlight ) : &fill );
            drawArrow( p, VERTICAL ? UpArrow : LeftArrow,
                       SUB_LINE_ACTIVE, subB.x()+2, subB.y()+2,
                       subB.width()-4, subB.height()-4, g, TRUE );
        }

        if (sb->backgroundPixmap() )
            fill = QBrush( g.mid(), *sb->backgroundPixmap() );

        if ( scrollerMoving )
            p->setClipRegion( QRegion(subPageR) - QRegion(scrollerStartOldPos) - QRegion(sliderR) );

        if ( controls & SubPage && subPageR.isValid() )
            qDrawShadePanel( p, subPageR, g, FALSE, 1, &fill );

        if ( scrollerMoving )
            p->setClipRegion( QRegion(addPageR) - QRegion(scrollerStartOldPos) - QRegion(sliderR) );

        if ( controls & AddPage && addPageR.isValid() )
            qDrawShadePanel( p, addPageR, g, FALSE, 1, &fill );

        if ( activeControl & Slider) {
            if ( scrollerMoving) {
                p->setClipRegion( QRegion(scrollerStartOldPos) - QRegion(sliderR) );
                qDrawShadePanel( p, scrollerStartOldPos, g, TRUE, 2, &g.brush( QColorGroup::Dark) );
            } else {
                scrollerStartOldPos = sliderR;
                scrollerMoving = TRUE;
            }
        }

        if ( controls & Slider && sliderR.isValid() ) {
            if ( scrollerMoving && activeControl != Slider ) {
                p->setClipping( FALSE );
                scrollerMoving = FALSE;
                qDrawShadePanel( p, subPageR, g, FALSE, 1, &fill );
                qDrawShadePanel( p, addPageR, g, FALSE, 1, &fill );
            }

            QRegion lineRegion( sliderR );
            if ( sliderLength >= 20 ) {
                if ( HORIZONTAL ) {
                    lineRegion -= QRegion( sliderM-4, sliderR.y()+2, 0, sliderR.height()-5 );
                    lineRegion -= QRegion( sliderM, sliderR.y()+2, 0, sliderR.height()-5 );
                    lineRegion -= QRegion( sliderM+4, sliderR.y()+2, 0, sliderR.height()-5 );
                } else {
                    lineRegion -= QRegion( sliderR.x()+2, sliderM-4, sliderR.width()-5, 0 );
                    lineRegion -= QRegion( sliderR.x()+2, sliderM, sliderR.width()-5, 0 );
                    lineRegion -= QRegion( sliderR.x()+2, sliderM+4, sliderR.width()-5, 0 );
                }
            }

            p->setClipRegion( lineRegion );

            QPoint bo = p->brushOrigin();
            p->setBrushOrigin(sliderR.topLeft());
            if ( sliderR.isValid() ) {
                if ( deviceUnderMouse == p->device() && sliderR.contains(mousePos))
                    drawBevelButton( p, sliderR.x(), sliderR.y(),
                                 sliderR.width(), sliderR.height(), g,
                                 FALSE, &g.brush( QColorGroup::Midlight ) );
                else
                    drawBevelButton( p, sliderR.x(), sliderR.y(),
                                 sliderR.width(), sliderR.height(), g );

                if (sliderLength >= 20 ) {
                    p->setClipping( FALSE );

                    if ( HORIZONTAL ) {
                        drawSeparator( p, sliderM-5, sliderR.y()+2, sliderM-5, sliderR.y()+sliderR.height()-3, g );
                        drawSeparator( p, sliderM-1, sliderR.y()+2, sliderM-1, sliderR.y()+sliderR.height()-3, g );
                        drawSeparator( p, sliderM+3, sliderR.y()+2, sliderM+3, sliderR.y()+sliderR.height()-3, g );
                    } else {
                        drawSeparator( p, sliderR.x()+2, sliderM-5, sliderR.x()+sliderR.width()-3, sliderM-5, g );
                        drawSeparator( p, sliderR.x()+2, sliderM-1, sliderR.x()+sliderR.width()-3, sliderM-1, g );
                        drawSeparator( p, sliderR.x()+2, sliderM+3, sliderR.x()+sliderR.width()-3, sliderM+3, g );
                    }
                }
            }
            p->setBrushOrigin(bo);
        }
    }

    p->setClipping( FALSE );
}

/*!
    Draws the sliding element of a slider-widget.
*
void
QSGIStyle::drawSlider( QPainter* p, int x, int y, int w, int h, const QColorGroup& g,
                 Orientation orient, bool tickAbove, bool tickBelow )
{
    QRect sliderR( x, y, w-1, h-1 );
    if ( sliderMoving ) {
        if ( sliderLastPosition.slider == (QWidget*)p->device() ) {
            if ( !sliderLastPosition.pos.isValid() ) {
                sliderLastPosition.pos= sliderR;
            } else {
                p->setClipRegion( QRegion(sliderLastPosition.pos ) - QRegion(sliderR) );
                qDrawShadePanel( p, sliderLastPosition.pos, g, TRUE, 2, &g.brush( QColorGroup::Dark) );
            }
        }
    } else
        sliderLastPosition.pos = QRect( 1, -1, 1, -1 );

    if ( repaintByMouseMove) {
        bool aboutToBeActive = sliderR.contains( mousePos );
        if ( sliderHandleActive == aboutToBeActive )
            return;

        sliderHandleActive = aboutToBeActive;
    }

    p->setClipping( FALSE );
    if ( deviceUnderMouse == p->device() && sliderR.contains( mousePos ) )
        drawBevelButton( p, x, y, w-1, h-1, g, FALSE, &g.brush( QColorGroup::Midlight ) );
    else
        drawBevelButton( p, x, y, w-1, h-1, g, FALSE );

    if ( orient == Horizontal ) {
        QCOORD mid = x + w / 2 - 2;
        drawSeparator( p, mid,  y+2 , mid,  y + h - 4, g );
    } else {
        QCOORD mid = y +h / 2;
        drawSeparator( p, x+2, mid,  x + w - 4, mid, g );
    }
}


/*!
    Draws the groove of a slider-widget.
*
void
QSGIStyle::drawSliderGroove( QPainter* p, int x, int y, int w, int h,
                 const QColorGroup& g, QCOORD, Orientation )
{
    if ( repaintByMouseMove)
        return;

    if ( sliderLastPosition.slider == p->device() ) {
        if ( sliderMoving && sliderLastPosition.pos.isValid() )
            p->setClipRegion( QRegion( x,y,w,h )
                - QRegion( sliderLastPosition.pos ) );
    }

    qDrawShadePanel( p, x, y, w, h, g, TRUE, 1 );
    drawButton( p, x+1, y+1, w-2, h-2, g );

    p->setClipping( FALSE );
}

/*! \reimp
*
void
QSGIStyle::drawSplitter( QPainter *p, int x, int y, int w, int h,
                               const QColorGroup& g, Orientation orient )
{
    const int motifOffset = 10;
    int sw = splitterWidth();
    if ( orient == Horizontal ) {
        int xPos = x + w/2;
        int kPos = motifOffset;
        int kSize = sw - 2;

        qDrawShadeLine( p, xPos, kPos + kSize - 1 ,
                xPos, h, g );
        drawBevelButton( p, xPos-sw/2+1, kPos,
                kSize, kSize+2, g,  FALSE, &g.brush( QColorGroup::Button ));
        qDrawShadeLine( p, xPos+2, 0, xPos, kPos, g );
    } else {
        int yPos = y + h/2;
        int kPos = w - motifOffset - sw;
        int kSize = sw - 2;

        qDrawShadeLine( p, 0, yPos, kPos, yPos, g );
        drawBevelButton( p, kPos, yPos-sw/2+1,
                kSize+2, kSize, g, FALSE, &g.brush( QColorGroup::Button ));
        qDrawShadeLine( p, kPos + kSize+1, yPos, w, yPos, g );
    }
}

/*! \reimp
*
int QSGIStyle::popupMenuItemHeight( bool checkable, QMenuItem* mi,
                                    const QFontMetrics& fm ) const
{
    int h = 0;
    if ( mi->isSeparator() ) {
        h = sgiSepHeight;
    } else {
        if ( mi->pixmap() ) {
            h = mi->pixmap()->height() + 2*sgiItemFrame;
        } else {
            h = fm.height() + 2*sgiItemVMargin + 2*sgiItemFrame;
        }
    }

    if ( !mi->isSeparator() && mi->iconSet() != 0 ) {
        h = QMAX( h, mi->iconSet()->pixmap( QIconSet::Small, QIconSet::Normal ).height() + 2*sgiItemFrame );
        h += 2;
    }
    if ( mi->custom() )
        h = QMAX( h, mi->custom()->sizeHint().height() + 2*sgiItemVMargin + 2*sgiItemFrame );

    return h;
}

/*! \reimp
*
void
QSGIStyle::drawPopupPanel( QPainter *p, int x, int y, int w, int h,
                               const QColorGroup &g, int lineWidth,
                               const QBrush *fill )
{
    if (lineWidth == 3 && w > 3 && h > 3 )
        drawBevelButton( p, x, y,  w, h, g, FALSE, fill );
    else
        QMotifStyle::drawPopupPanel( p, x, y, w, h, g, lineWidth, fill );
}


static void drawSGIPrefix( QPainter *p, int x, int y, QString* miText )
{
    if ( miText && (!!(*miText)) ) {
        int amp = 0;
        bool nextAmp = FALSE;
        while ( ( amp = miText->find( '&', amp ) ) != -1 ) {
            if ( (uint)amp == miText->length()-1 )
                return;
            miText->remove( amp,1 );
            nextAmp = (*miText)[amp] == '&';    // next time if &&

            if ( !nextAmp ) {     // draw special underlining
                uint ulx = p->fontMetrics().width(*miText, amp);

                uint ulw = p->fontMetrics().width(*miText, amp+1) - ulx;

                p->drawLine( x+ulx, y, x+ulx+ulw, y );
                p->drawLine( x+ulx, y+1, x+ulx+ulw/2, y+1 );
                p->drawLine( x+ulx, y+2, x+ulx+ulw/4, y+2 );
            }
            amp++;
        }
    }
}

/*! \reimp
*
void QSGIStyle::drawPopupMenuItem( QPainter* p, bool checkable, int maxpmw,
                                   int tab, QMenuItem* mi, const QPalette& pal,
                                   bool act, bool enabled,
                                   int x, int y, int w, int h )
{
    const QColorGroup & g = pal.active();
    bool dis = !enabled;
    QColorGroup itemg = dis ? pal.disabled() : pal.active();

    if ( checkable )
        maxpmw = QMAX( maxpmw, sgiCheckMarkSpace );
    int checkcol = maxpmw;

    if (mi && mi->isSeparator() ) {
        p->setPen( g.mid() );
        p->drawLine(x, y, x+w, y );
        return;
    }

    int pw = sgiItemFrame;

    if ( act && !dis ) {
        if ( defaultFrameWidth() > 1 )
            drawPanel( p, x, y, w, h, g, FALSE, pw,
                             &g.brush( QColorGroup::Light ) );
        else
            drawPanel( p, x+1, y+1, w-2, h-2, g, FALSE, 1,
                             &g.brush( QColorGroup::Light ) );
    } else {
        p->fillRect( x, y, w, h, g.brush( QColorGroup::Background ) );
    }

    if ( !mi )
        return;

    if ( mi->isChecked() ) {
        if ( mi->iconSet() ) {
            drawPanel( p, x+sgiItemFrame, y+sgiItemFrame, checkcol, h-2*sgiItemFrame,
                             g, TRUE, 1, &g.brush( QColorGroup::Light ) );
        }
    } else {
        if ( !act )
            p->fillRect( x+sgiItemFrame, y+sgiItemFrame, checkcol, h-2*sgiItemFrame,
                        g.brush( QColorGroup::Background ) );
    }

    if ( mi->iconSet() ) {
        QIconSet::Mode mode = QIconSet::Normal;
        if ( act && !dis )
            mode = QIconSet::Active;
        QPixmap pixmap;
	if ( checkable && mi->isChecked() )
	    pixmap = mi->iconSet()->pixmap( QIconSet::Small, mode, QIconSet::On );
	else
	    pixmap = mi->iconSet()->pixmap( QIconSet::Small, mode );

        int pixw = pixmap.width();
        int pixh = pixmap.height();
        QRect cr( x+sgiItemFrame, y+sgiItemFrame, checkcol, h-2*sgiItemFrame );
        QRect pmr( 0, 0, pixw, pixh );
        pmr.moveCenter( cr.center() );
        p->setPen( itemg.text() );
        p->drawPixmap( pmr.topLeft(), pixmap );
    } else {
        if ( checkable ) {
            int mw = checkcol;
            int mh = h - 2*sgiItemFrame;

            QColorGroup citemg = itemg;
            if ( act && enabled )
                citemg.setColor( QColorGroup::Background, itemg.light() );
            if ( mi->isChecked() )
                drawIndicator( p, x+sgiItemFrame, y+sgiItemFrame, mw, mh, citemg,
                                QButton::On, act, enabled );
        }
    }

    p->setPen( g.buttonText() );

    QColor discol;
    if ( dis ) {
        discol = itemg.text();
        p->setPen( discol );
    }

    int xm = sgiItemFrame + checkcol + sgiItemHMargin;

    if ( mi->custom() ) {
        int m = sgiItemVMargin;
        p->save();
        mi->custom()->paint( p, itemg, act, enabled,
                             x+xm, y+m, w-xm-tab+1, h-2*m );
        p->restore();
    }

    QString s = mi->text();
    if ( !!s ) {
        int t = s.find( '\t' );
        int m = sgiItemVMargin;
        const int text_flags = AlignVCenter | DontClip | SingleLine; //special underline for &x

        QString miText = s;
        if ( t>=0 ) {
            p->drawText(x+w-tab-sgiItemHMargin-sgiItemFrame,
                        y+m, tab, h-2*m, text_flags, miText.mid( t+1 ) );
            miText = s.mid( 0, t );
        }
        QRect br = p->fontMetrics().boundingRect( x+xm, y+m, w-xm-tab+1, h-2*m,
                text_flags, mi->text() );

        drawSGIPrefix( p, br.x()+p->fontMetrics().leftBearing(miText[0]),
                br.y()+br.height()+p->fontMetrics().underlinePos()-2, &miText );
        p->drawText( x+xm, y+m, w-xm-tab+1, h-2*m, text_flags, miText, miText.length() );
    } else {
        if ( mi->pixmap() ) {
            QPixmap *pixmap = mi->pixmap();
            if ( pixmap->depth() == 1 )
                p->setBackgroundMode( OpaqueMode );
            p->drawPixmap( x+xm, y+sgiItemFrame, *pixmap );
            if ( pixmap->depth() == 1 )
                p->setBackgroundMode( TransparentMode );
        }
    }
    if ( mi->popup() ) {
        int dim = (h-2*sgiItemFrame) / 2;
        drawArrow( p, RightArrow, FALSE, x+w-sgiArrowHMargin-sgiItemFrame-dim,
                   y+h/2-dim/2, dim, dim, g, TRUE );
    }
}

/*!
    \reimp
*
void QSGIStyle::drawMenuBarItem( QPainter* p, int x, int y, int w, int h,
                                 QMenuItem* mi, QColorGroup& g,
                                 bool active, bool, bool )
{
    if ( active ) {
        p->setPen( QPen( g.shadow(), 1) );
        p->drawRect( x, y, w, h );
        qDrawShadePanel( p, QRect(x+1,y+1,w-2,h-2), g, FALSE, 2,
                         &g.brush( QColorGroup::Light ));
    } else {
        p->fillRect( x, y, w, h, g.brush( QColorGroup::Button ));
    }

    if ( mi->pixmap() )
        drawItem( p, x, y, w, h, AlignCenter|DontClip|SingleLine,
                g, mi->isEnabled(), mi->pixmap(), "", -1, &g.buttonText() );

    if ( !!mi->text() ) {
        QString* text = new QString(mi->text());
        QRect br = p->fontMetrics().boundingRect( x, y-2, w+1, h,
                AlignCenter|DontClip|SingleLine|ShowPrefix, mi->text() );

        drawSGIPrefix( p, br.x()+p->fontMetrics().leftBearing((*text)[0]),
                br.y()+br.height()+p->fontMetrics().underlinePos()-2, text );
        p->drawText( x, y-2, w+1, h, AlignCenter|DontClip|SingleLine, *text, text->length() );
        delete text;
    }
}
*/

#endif // QT_NO_STYLE_SGI
