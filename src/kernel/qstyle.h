/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qstyle.h#14 $
**
** Definition of QStyle class
**
** Created : 980616
**
** Copyright (C) 1998 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QSTYLE_H
#define QSTYLE_H

#ifndef QT_H
#include "qwindowdefs.h"
#include "qobject.h"
#endif // QT_H

class QPushButton;
class QScrollBar;

class Q_EXPORT QStyle: public QObject
{
    GUIStyle gs;
public:
    QStyle(GUIStyle);
    QStyle();
    virtual ~QStyle();





#ifndef NO_QT1_COMPAT
    operator GUIStyle() const { return gs; }
    int operator==(GUIStyle s) const { return gs==s; }
    int operator!=(GUIStyle s) const { return gs!=s; }
#endif

    GUIStyle guiStyle() const { return gs; }

    virtual void initialize( QApplication*);

    virtual void polish( QWidget* );

    virtual QRect itemRect( QPainter *p, int x, int y, int w, int h,
		    int flags, bool enabled,
		    const QPixmap *pixmap, const QString& text, int len=-1 );

    virtual void drawItem( QPainter *p, int x, int y, int w, int h,
		    int flags, const QColorGroup &g, bool enabled,
		    const QPixmap *pixmap, const QString& text,
			   int len=-1, bool bright_text = FALSE );


    virtual void drawSeparator( QPainter *p, int x1, int y1, int x2, int y2,
		     const QColorGroup &g, bool sunken = TRUE,
		     int lineWidth = 1, int midLineWidth = 0 );

    virtual void drawRect( QPainter *p, int x, int y, int w, int h,
		    const QColor &, int lineWidth = 1,
		    const QBrush *fill = 0 );

    virtual void drawRectStrong( QPainter *p, int x, int y, int w, int h,
		     const QColorGroup &, bool sunken=FALSE,
		     int lineWidth = 1, int midLineWidth = 0,
		     const QBrush *fill = 0 );

    virtual void drawButton( QPainter *p, int x, int y, int w, int h,
		     const QColorGroup &g, bool sunken = FALSE,
		     const QBrush *fill = 0 ) = 0;

    virtual QRect buttonRect( int x, int y, int w, int h);

    virtual void drawButtonMask( QPainter *p, int x, int y, int w, int h);
			
    virtual void drawBevelButton( QPainter *p, int x, int y, int w, int h,
		     const QColorGroup &g, bool sunken = FALSE,
		     const QBrush *fill = 0 ) = 0;

    virtual void drawToolButton( QPainter *p, int x, int y, int w, int h,
		     const QColorGroup &g, bool sunken = FALSE,
		     const QBrush *fill = 0 );

    virtual void drawPanel( QPainter *p, int x, int y, int w, int h,
		    const QColorGroup &, bool sunken=FALSE,
		    int lineWidth = 1, const QBrush *fill = 0 );

    enum ArrowType { UpArrow, DownArrow, LeftArrow, RightArrow };

    virtual void drawArrow( QPainter *p, ArrowType type, bool down,
		     int x, int y, int w, int h,
		     const QColorGroup &g, bool enabled, const QBrush *fill = 0 ) = 0;

    // "radio button"
    virtual QSize exclusiveIndicatorSize() const = 0;
    virtual void drawExclusiveIndicator( QPainter* p, int x, int y, int w, int h,
		    const QColorGroup &g, bool on, bool down = FALSE, bool enabled = TRUE ) = 0;
    virtual void drawExclusiveIndicatorMask( QPainter *p, int x, int y, int w, int h, bool on);

    // "check box"
    virtual QSize indicatorSize() const = 0;
    virtual void drawIndicator( QPainter* p, int x, int y, int w, int h, const QColorGroup &g,
				bool on, bool down = FALSE, bool enabled = TRUE ) = 0;
    virtual void drawIndicatorMask( QPainter *p, int x, int y, int w, int h, bool on);

    // "combo box"
    virtual void drawComboButton( QPainter *p, int x, int y, int w, int h,
				  const QColorGroup &g, bool sunken = FALSE,
				  bool editable = FALSE, 
				  bool enabled = TRUE,
				  const QBrush *fill = 0 );
    virtual QRect comboButtonRect( int x, int y, int w, int h);
    virtual QRect comboButtonFocusRect( int x, int y, int w, int h);

    virtual void drawComboButtonMask( QPainter *p, int x, int y, int w, int h);


    // focus
    virtual void drawFocusRect( QPainter*,
		    const QRect&, const QColorGroup &, const QColor* bg = 0 ) = 0;


    // push buttons
    virtual void drawPushButton( QPushButton* btn, QPainter *p);
    virtual void drawPushButtonLabel( QPushButton* btn, QPainter *p);


    // scrollbars
    enum ScrollControl { ADD_LINE = 0x1 , SUB_LINE = 0x2 , ADD_PAGE = 0x4,
			    SUB_PAGE = 0x8 , FIRST    = 0x10, LAST	= 0x20,
			    SLIDER   = 0x40, NONE     = 0x80 };

    virtual void scrollbarMetrics( const QScrollBar*, int *, int *, int * ) = 0;
    virtual void drawScrollbarControls( QPainter*,  const QScrollBar*, int sliderStart, uint controls, uint activeControl ) = 0;

    // sliders
    enum SliderDirection {SlUp,SlDown,SlLeft,SlRight};
    virtual int sliderLength() const = 0;
    virtual void drawSlider( QPainter *p,
			     int x, int y, int w, int h,
			     const QColorGroup &g,
			     SliderDirection dir) = 0;

    virtual void drawSliderMask( QPainter *p,
				 int x, int y, int w, int h,
				 SliderDirection dir);
    virtual void drawSliderGroove( QPainter *p,
				   int x, int y, int w, int h,
				   const QColorGroup& g, QCOORD c,
				   bool horizontal ) = 0;
    virtual void drawSliderGrooveMask( QPainter *p,
				       int x, int y, int w, int h,
				       QCOORD c,
				       bool horizontal);
    virtual int maximumSliderDragDistance() const;

};






#endif // QSTYLE_H
