#ifndef QPAINTER_P_H
#define QPAINTER_P_H

#include "qbrush.h"
#include "qfont.h"
#include "qpen.h"
#include "qregion.h"
#include "qvector.h"
#include "qwmatrix.h"

#include "qpainter.h"

class QAbstractGC;

class QPainterState
{
public:
    QPainterState( const QPainterState *s = 0 )
    {
	if (s) {
	    font = QFont(s->font);
	    pfont = s->pfont;
	    pen = QPen(s->pen);
	    brush = QBrush(s->brush);
	    bgOrigin = s->bgOrigin;
	    bgBrush = QBrush(s->bgBrush);
 	    clipRegion = QRegion(s->clipRegion);
	    clipEnabled = s->clipEnabled;
	    rasterOp = s->rasterOp;
	    bgMode = s->bgMode;
	    VxF = s->VxF;
	    WxF = s->WxF;
#ifndef QT_NO_TRANSFORMATIONS
	    worldMatrix = s->worldMatrix;
	    matrix = s->matrix;
#else
	    xlatex = s->xlatex;
	    xlatey = s->xlatey;
#endif
	    wx = s->wx;
	    wy = s->wy;
	    ww = s->ww;
	    wh = s->wh;
	    vx = s->vx;
	    vy = s->vy;
	    vw = s->vw;
	    vh = s->vh;
	    painter = s->painter;
	} else {
	    bgBrush = Qt::white;
	    bgMode = QPainter::TransparentMode;
	    rasterOp = Qt::CopyROP;
	    clipEnabled = false;
	    WxF = false;
	    VxF = false;
	    wx = wy = ww = wh = 0;
	    vx = vy = vw = vh = 0;
	    pfont = 0;
	    painter = 0;
	}
    }

    QPoint 	bgOrigin;
    QFont 	font;
    QFont       *pfont;
    QPen 	pen;
    QBrush 	brush;
    QBrush 	bgBrush;		// background brush
    QRegion	clipRegion;
    QColor      bgColor;
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix    worldMatrix; 	    	// World transformation matrix, not window and viewport
    QWMatrix    matrix;			// Complete transformation matrix, including win and view.
#else
    int         xlatex;
    int         xlatey;
#endif
    int	        wx, wy, ww, wh;		// window rectangle
    int 	vx, vy, vw, vh;		// viewport rectangle

    uint	clipEnabled:1;
    uint 	WxF:1;			// World transformation
    uint    	VxF:1;			// View transformation

    Qt::RasterOp rasterOp;
    Qt::BGMode bgMode;
    QPainter *painter;
};


class QPainterPrivate
{
public:
    QPainterPrivate()
	: txop(0), txinv(0), device(0), gc(0)
    {
	states.push_back(new QPainterState());
	state = states.back();
    }

    ~QPainterPrivate()
    {
	for (int i=0; i<states.size(); ++i)
	    delete states.at(i);
    }

    QPoint redirection_offset;

    QPainterState *state;
    QVector<QPainterState*> states;

#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix matrix; // reloaded stinks...
    QWMatrix invMatrix;

    int txop;
    uint txinv:1;
#endif

    QPaintDevice *device;
    QAbstractGC *gc;
};

#endif // QPAINTER_P_H
