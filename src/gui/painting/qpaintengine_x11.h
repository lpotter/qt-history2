/****************************************************************************
**
** Definition of QX11PaintEngine class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef QX11PAINTENGINE_H
#define QX11PAINTENGINE_H

#include "qpaintengine.h"

class QX11PaintEnginePrivate;
class QPainterState;

class QX11PaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QX11PaintEngine)

public:
    QX11PaintEngine(QPaintDevice *);
    ~QX11PaintEngine();

    bool begin(QPaintDevice *pdev, bool begin = false);
    bool end();

    void updatePen(const QPen &pen);
    void updateBrush(const QBrush &brush, const QPoint &pt);
    void updateFont(const QFont &font);
    void updateRasterOp(Qt::RasterOp rop);
    void updateBackground(Qt::BGMode bgmode, const QBrush &bgBrush);
    void updateXForm(const QWMatrix &matrix);
    void updateClipRegion(const QRegion &region, bool clipEnabled);

    void setRasterOp(RasterOp r);

    virtual void drawLine(const QPoint &p1, const QPoint &ps);
    virtual void drawRect(const QRect &r);
    virtual void drawRects(const QList<QRect> &rects);
    virtual void drawPoint(const QPoint &p);
    virtual void drawPoints(const QPointArray &pa, int index = 0, int npoints = -1);
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

    virtual void drawPixmap(const QRect &r, const QPixmap &pm, const QRect &sr, bool imask);
    virtual void drawTiledPixmap(const QRect &r, const QPixmap &pixmap, const QPoint &s, bool optim);

    virtual Qt::HANDLE handle() const;
    inline Type type() const { return QPaintEngine::X11; }

    static void initialize();
    static void cleanup();

protected:
    QX11PaintEngine(QX11PaintEnginePrivate &dptr, QPaintDevice *);

    friend void qt_cleanup();
    friend void qt_draw_transformed_rect(QPaintEngine *pp,  int x, int y, int w,  int h, bool fill);
    friend void qt_draw_background(QPaintEngine *pp, int x, int y, int w,  int h);
    friend class QPixmap;
    friend class QFontEngineBox;
    friend class QFontEngineXft;
    friend class QFontEngineXLFD;

private:
#if defined(Q_DISABLE_COPY)
    QX11PaintEngine(const QX11PaintEngine &);
    QX11PaintEngine &operator=(const QX11PaintEngine &);
#endif
};
#endif // QX11PAINTENGINE_H
