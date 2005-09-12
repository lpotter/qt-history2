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
**/

#ifndef QPAINTENGINE_RASTER_P_H
#define QPAINTENGINE_RASTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qpaintengine.h>
#include <QtGui/qpainterpath.h>

#include <private/qdatabuffer_p.h>
#include <private/qdrawhelper_p.h>
#include <private/qpaintengine_p.h>
#include <private/qstroker_p.h>

#include <stdlib.h>

class QFTOutlineMapper;
class QRasterPaintEnginePrivate;
class QRasterBuffer;

/*******************************************************************************
 * QRasterPaintEngine
 */
class QRasterPaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QRasterPaintEngine)
public:
    QRasterPaintEngine();
    ~QRasterPaintEngine();
    bool begin(QPaintDevice *device);
    bool end();

    void updateState(const QPaintEngineState &state);
    void updateMatrix(const QMatrix &matrix);

    void updateClipRegion(const QRegion &region, Qt::ClipOperation op);
    void updateClipPath(const QPainterPath &path, Qt::ClipOperation op);

    void drawPath(const QPainterPath &path);
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    void fillPath(const QPainterPath &path, QSpanFillData *fillData);

    void drawEllipse(const QRectF &rect);

    void drawRects(const QRect  *rects, int rectCount);
    void drawRects(const QRectF *rects, int rectCount);

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                   Qt::ImageConversionFlags falgs = Qt::AutoColor);
    void drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &sr);
    void drawTextItem(const QPointF &p, const QTextItem &textItem);

    void drawLines(const QLine *line, int lineCount);
    void drawLines(const QLineF *line, int lineCount);

    void drawPoints(const QPointF *points, int pointCount);

#ifdef Q_NO_USING_KEYWORD
    inline void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode)
        { QPaintEngine::drawPolygon(points, pointCount, mode); }
    inline void drawPoints(const QPoint *points, int pointCount) { QPaintEngine::drawPoints(points, pointCount); }
    inline void drawEllipse(const QRect &rect) { QPaintEngine::drawEllipse(rect); }
#else
    using QPaintEngine::drawPolygon;
    using QPaintEngine::drawPoints;
    using QPaintEngine::drawEllipse;
#endif

    void setFlushOnEnd(bool flush);
    void flush(QPaintDevice *device, const QPoint &offset);

    void releaseBuffer();

    QSize size() const;

#ifndef QT_NO_DEBUG
    void saveBuffer(const QString &s) const;
#endif

#ifdef Q_WS_WIN
    HDC getDC() const;
    void releaseDC(HDC hdc) const;
#endif
#ifdef Q_WS_QWS
    //QWS hack
    void alphaPenBlt(const void* src, int bpl, bool mono, int rx,int ry,int w,int h);
    void qwsFillRect(int x, int y, int w, int h, const QBrush &brush);
#endif

    Type type() const { return Raster; }

    QPoint coordinateOffset() const;

protected:
    QRasterPaintEngine(QRasterPaintEnginePrivate &d);
private:
    void init();
};


/*******************************************************************************
 * QRasterPaintEnginePrivate
 */
class QRasterPaintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QRasterPaintEngine)
public:

    void fillForBrush(const QBrush &brush, QSpanFillData *data);
    void addClip(QSpanFillData *data);
    void clippedFillForBrush(const QBrush &brush, QSpanFillData *data);
    void updateClip_helper(const QPainterPath &path, Qt::ClipOperation);

    void drawBitmap(const QPointF &pos, const QPixmap &image, QSpanFillData *fill);
    QImage colorizeBitmap(const QImage &image, const QColor &color);

    QMatrix brushMatrix() const {
        QMatrix m(matrix);
        m.translate(brushOffset.x(), brushOffset.y());
        return m;
    }
#ifdef Q_WS_X11
    void drawMulti(const QPointF &p, const QTextItem &textItem);
    void drawBox(const QPointF &p, const QTextItem &textItem);
    void drawXLFD(const QPointF &p, const QTextItem &textItem);
#endif

    QPointF brushOffset;
    QBrush brush;
    QBrush bgBrush;
    QPen pen;
    QMatrix matrix;
    QPainter::CompositionMode compositionMode;

    QPaintDevice *device;
    QFTOutlineMapper *outlineMapper;
    QRasterBuffer *rasterBuffer;
#ifdef Q_WS_WIN
    QRasterBuffer *fontRasterBuffer;
#endif

    QPainterPath baseClip;
    QRect deviceRect;

    QSpanFillData spanFillData;

    DrawHelper *drawHelper;

    QStroker basicStroker;
    QDashStroker *dashStroker;
    QStrokerOps *stroker;

    QImage tempImage;

    int deviceDepth;

    uint txop;

    uint has_pen : 1;
    uint has_brush : 1;
    uint fast_pen : 1;
    uint opaqueBackground : 1;
    uint clipEnabled : 1;
    uint antialiased : 1;
    uint bilinear : 1;
    uint flushOnEnd : 1;
    uint mono_surface : 1;
    uint int_xform : 1;
};

class QClipData {
public:
    QClipData(int height);
    ~QClipData();
    int clipSpanHeight;
    struct ClipLine {
        int count;
        QSpan *spans;
    } *clipLines;

    int allocated;
    int count;
    QSpan *spans;

    void appendSpan(int x, int length, int y, int coverage);
    void appendSpans(const QSpan *s, int num);
    void fixup();
};

inline void QClipData::appendSpan(int x, int length, int y, int coverage)
{
    if (count == allocated) {
        allocated *= 2;
        spans = (QSpan *)realloc(spans, allocated*sizeof(QSpan));
    }
    spans[count].x = x;
    spans[count].len = length;
    spans[count].y = y;
    spans[count].coverage = coverage;
    ++count;
}

inline void QClipData::appendSpans(const QSpan *s, int num)
{
    if (count + num >= allocated) {
        while (allocated < count + num)
            allocated *= 2;
        spans = (QSpan *)realloc(spans, allocated*sizeof(QSpan));
    }
    memcpy(spans+count, s, num*sizeof(QSpan));
    count += num;
}

/*******************************************************************************
 * QRasterBuffer
 */
class QRasterBuffer
{
public:
#if defined(Q_WS_WIN)
    QRasterBuffer() : m_hdc(0), m_bitmap(0), m_width(0), m_height(0), m_buffer(0) { init(); }

    HDC hdc() const { return m_hdc; }
#elif defined(Q_WS_X11)
    QRasterBuffer() : m_width(0), m_height(0), m_buffer(0) { init(); }
#elif defined(Q_WS_MAC)
    QRasterBuffer() : m_data(0), m_width(0), m_height(0), m_buffer(0) { init(); }
# if defined(QMAC_NO_COREGRAPHICS)
    GWorldPtr m_data;
# else
    CGImageRef m_data;
#endif
#endif
    ~QRasterBuffer();

    void init();

    void prepare(QImage *image);
#ifdef Q_WS_QWS
    void prepare(QPixmap *pix);
#endif
    void prepare(int w, int h);
    void prepareBuffer(int w, int h);
    
    void resetBuffer(int val=0);

    uchar *scanLine(int y) { Q_ASSERT(y>=0); Q_ASSERT(y<m_height); return m_buffer + y * bytes_per_line; }

#ifndef QT_NO_DEBUG
    QImage clipImage() const;
    QImage bufferImage() const;
#endif

    void flushToARGBImage(QImage *image) const;
    
    int width() const { return m_width; }
    int height() const { return m_height; }
    int bytesPerLine() const { return bytes_per_line; }

    uchar *buffer() const { return m_buffer; }

    QClipData *clip;

    void resetClip() { delete clip; clip = 0; }
private:
#if defined(Q_WS_WIN)
    HDC m_hdc;
    HBITMAP m_bitmap;
#endif

    int m_width;
    int m_height;
    int bytes_per_line;
    uchar *m_buffer;

};

#endif // QPAINTENGINE_RASTER_P_H
