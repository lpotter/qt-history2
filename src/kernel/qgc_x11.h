/****************************************************************************
**
** Definition of QX11GC class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef QX11GC_H
#define QX11GC_H

#include "qabstractgc.h"

class QX11GCPrivate;
class QPainterState;

class QX11GC : public QAbstractGC
{
public:
    QX11GC(const QPaintDevice *);
    ~QX11GC();

    bool begin(const QPaintDevice *pdev, QPainterState *state, bool begin = FALSE);
    bool end();

    void updatePen(QPainterState *ps);
    void updateBrush(QPainterState *ps);
    void updateFont(QPainterState *ps);
    void updateRasterOp(QPainterState *ps);
    void updateBackground(QPainterState *ps);
    void updateXForm(QPainterState *ps);
    void updateClipRegion(QPainterState *ps);

    void setRasterOp(RasterOp r);

    virtual void drawLine(const QPoint &p1, const QPoint &ps);
    virtual void drawRect(const QRect &r);
    virtual void drawPoint(const QPoint &p);
    virtual void drawPoints(const QPointArray &pa, int index = 0, int npoints = -1);
    virtual void drawWinFocusRect(const QRect &r, bool xorPaint, const QColor &bgColor);
    virtual void drawRoundRect(const QRect &r, int xRnd, int yRnd);
    virtual void drawEllipse(const QRect &r);
    virtual void drawArc(const QRect &r, int a, int alen);
    virtual void drawPie(const QRect &r, int a, int alen);
    virtual void drawChord(const QRect &r, int a, int alen);
    virtual void drawLineSegments(const QPointArray &, int index = 0, int nlines = -1);
    virtual void drawPolyline(const QPointArray &pa, int index = 0, int npoints = -1);
    virtual void drawPolygon(const QPointArray &pa, bool winding = false, int index = 0, int npoints = -1);
    virtual void drawConvexPolygon(const QPointArray &, int index = 0, int npoints = -1);
#ifndef QT_NO_BEZIER
    virtual void drawCubicBezier(const QPointArray &, int index = 0);
#endif

    virtual void drawPixmap(const QRect &r, const QPixmap &pm, const QRect &sr);
    virtual void drawTextItem(const QPoint &p, const QTextItem &ti, int textflags);
    virtual void drawTiledPixmap(const QRect &r, const QPixmap &pixmap, const QPoint &s, bool optim);

    virtual Qt::HANDLE handle() const;
    inline Type type() const { return QAbstractGC::X11; }

    static void initialize();
    static void cleanup();

protected:
    friend void qt_cleanup();
    friend void qt_draw_transformed_rect( QPainter *pp,  int x, int y, int w,  int h, bool fill );
    friend void qt_draw_background( QPainter *pp, int x, int y, int w,  int h );
    friend class QPixmap;
    friend class QFontEngineBox;
    friend class QFontEngineXft;
    friend class QFontEngineXLFD;

private:
    QX11GCPrivate *d;

private:
#if defined(Q_DISABLE_COPY)
    QX11GC(const QX11GC &);
    QX11GC &operator=(const QX11GC &);
#endif
};
#endif // QX11GC_H
