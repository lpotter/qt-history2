/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "hello.h"
#include <qpushbutton.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qevent.h>
#include <qpixmap.h>


/*
  Constructs a Hello widget. Starts a 40 ms animation timer.
*/

Hello::Hello(const char *text, QWidget *parent)
    : QWidget(parent), t(text), b(0)
{
    setAttribute(Qt::WA_PaintOnScreen);
    QTimer *timer = new QTimer(this);
    connect( timer, SIGNAL(timeout()), SLOT(animate()) );
    timer->start( 40 );

    resize( 260, 130 );
}

/*
  Returns the rectangle covered by the text
*/

QRect Hello::textRect()
{
    QFontMetrics fm = fontMetrics();
    int w = fm.width(t) + 20;
    int h = fm.height() * 2;
    int pmx = width()/2 - w/2;
    int pmy = height()/2 - h/2;
    return QRect(pmx, pmy, w, h);
}

/*
  This private slot is called each time the timer fires.
*/

void Hello::animate()
{
    b = (b + 1) & 15;
    update(textRect());
}


/*
  Handles mouse button release events for the Hello widget.

  We emit the clicked() signal when the mouse is released inside
  the widget.
*/

void Hello::mouseReleaseEvent( QMouseEvent *e )
{
    if ( rect().contains( e->pos() ) )
        emit clicked();
}


/*
  Handles paint events for the Hello widget.

  Flicker-free update. The text is first drawn in the pixmap and the
  pixmap is then blt'ed to the screen.
*/

void Hello::paintEvent( QPaintEvent * )
{
    static int sin_tbl[16] = {
        0, 38, 71, 92, 100, 92, 71, 38,	0, -38, -71, -92, -100, -92, -71, -38};

    if ( t.isEmpty() )
        return;

    // 1: Compute some sizes, positions etc.
    QRect r = textRect();
    int w = r.width();
    int h = r.height();
    int pmx = r.x();
    int pmy = r.y();

    // 2: Create the pixmap and fill it with the widget's background
    QPixmap pm( w, h );
    pm.fill( this, pmx, pmy );

    // 3: Paint the pixmap. Cool wave effect
    QPainter p;
    QFontMetrics fm = fontMetrics();
    int x = 10;
    int y = h/2 + fm.descent();
    int i = 0;
    p.begin( &pm );
    p.setFont( font() );
    while ( !t[i].isNull() ) {
        int i16 = (b+i) & 15;
        p.setPen( QColor((15-i16)*16,255,255,QColor::Hsv) );
        p.drawText(x, y-sin_tbl[i16]*h/800, t.mid(i,1));
        x += fm.width( t[i] );
        i++;
    }
    p.end();

    // 4: Copy the pixmap to the Hello widget
//    bitBlt( this, pmx, pmy, &pm );
    p.begin(this);
    p.drawPixmap(pmx,pmy, pm);
}
