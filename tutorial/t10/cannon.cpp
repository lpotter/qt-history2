/****************************************************************
**
** Implementation Cannonfield class, Qt tutorial 10
**
****************************************************************/

#include "cannon.h"
#include <qpainter.h>
#include <qpixmap.h>

CannonField::CannonField( QWidget *parent, const char *name )
        : QWidget( parent, name )
{
    ang = 45;
    f   = 0;
}

void CannonField::setAngle( int degrees )
{
    if ( degrees < 5 )
	degrees = 5;
    if ( degrees > 70 )
	degrees = 70;
    if ( ang == degrees )
	return;
    ang = degrees;
    repaint( cannonRect(), FALSE );
    emit angleChanged( ang );
}

void CannonField::setForce( int newton )
{
    if ( newton < 0 )
	newton = 0;
    if ( newton > 50 )
	newton = 50;
    if ( f == newton )
	return;
    f = newton;
    emit forceChanged( f );
}

void CannonField::paintEvent( QPaintEvent *e )
{
    QRect updateR = e->rect();
    QPainter p;

    p.begin( this );
    if ( updateR.intersects( cannonRect() ) )
	paintCannon( &p );
    p.end();
}

const QRect barrel_rect(33, -4, 15, 8);

void CannonField::paintCannon( QPainter *p )
{
    QPixmap  pix( cannonRect().size() );
    QPainter tmp;

    pix.fill( backgroundColor() );

    tmp.begin( &pix );
    tmp.setBrush( blue );
    tmp.setPen( NoPen );

    tmp.translate( 0, pix.height() - 1 );
    tmp.drawPie( QRect( -35,-35, 70, 70 ), 0, 90*16 );
    tmp.rotate( -ang );
    tmp.drawRect( QRect( barrel_rect ) );
    tmp.end();

    p->drawPixmap( 0, rect().bottom() - (pix.height() - 1), pix );
}

QRect CannonField::cannonRect() const
{
    QRect r( 0, 0, 50, 50 );
    r.setBottomLeft( rect().bottomLeft() );
    return r;
}
