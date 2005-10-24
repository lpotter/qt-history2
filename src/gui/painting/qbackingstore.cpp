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
#include <qglobal.h>
#include <qapplication.h>
#ifdef Q_WS_WIN
# include "qt_windows.h"
# include <private/qpaintengine_raster_p.h>
#endif
#include "qbackingstore_p.h"
#include "private/qwidget_p.h"
#include <qdebug.h>
#include <qstack.h>
#include <qevent.h>
#ifdef Q_WS_X11
# include "private/qt_x11_p.h"
#endif

#ifdef Q_WS_QWS
#include <qwsdisplay_qws.h>
#include <qapplication.h>
#include <qwsmanager_qws.h>
#include <unistd.h>
#endif

/*****************************************************************************
  Top Level Window backing store
 *****************************************************************************/

extern bool qt_sendSpontaneousEvent(QObject*, QEvent*); // qapplication_xxx.cpp

bool QWidgetBackingStore::paintOnScreen(QWidget *w)
{
#ifdef Q_WS_QWS
    Q_UNUSED(w);
    return false;
#else
    if (w && (w->testAttribute(Qt::WA_PaintOnScreen)))
        return true;
    static signed char checked_env = -1;
    if(checked_env == -1)
        checked_env = (qgetenv("QT_ONSCREEN_PAINT") == "1") ? 1 : 0;

    return (checked_env == 1);
#endif
}

#ifdef Q_WS_QWS
static void qt_showYellowThing(QWidget *widget, const QRegion &rgn, int msec)
{
    static int yWinId = 0;

    if (yWinId == 0) {
        yWinId = QWidget::qwsDisplay()->takeId();
        QWidget::qwsDisplay()->nameRegion(yWinId, "Debug flush paint", "Silly yellow thing");
        QWidget::qwsDisplay()->setAltitude(yWinId, 1, true);
    }


    QRegion globalRgn = rgn;
    if (widget)
        globalRgn.translate(widget->mapToGlobal(QPoint()));

    QWidget::qwsDisplay()->requestRegion(yWinId, -1, false, globalRgn);
    QWidget::qwsDisplay()->setAltitude(yWinId, 1, true);
    QWidget::qwsDisplay()->repaintRegion(yWinId, false, globalRgn);

    ::usleep(500*msec);
    QWidget::qwsDisplay()->requestRegion(yWinId, -1, false, QRegion());
    ::usleep(500*msec);
}

#else
static void qt_showYellowThing(QWidget *widget, const QRegion &toBePainted, int msec)
{
    //flags to fool painter
    bool paintUnclipped = widget->testAttribute(Qt::WA_PaintUnclipped);
    if (!QWidgetBackingStore::paintOnScreen(widget))
        widget->setAttribute(Qt::WA_PaintUnclipped);

    bool setFlag = widget && !widget->testAttribute(Qt::WA_WState_InPaintEvent);
    if(setFlag)
        widget->setAttribute(Qt::WA_WState_InPaintEvent);


    //setup the engine
    QPaintEngine *pe = widget->paintEngine();
    pe->setSystemClip(toBePainted);
    {
        QPainter p(widget);
        p.setClipRegion(toBePainted);
        p.fillRect(widget->rect(), Qt::yellow);
        p.end();
    }

    if(setFlag)
        widget->setAttribute(Qt::WA_WState_InPaintEvent, false);

    //restore
    widget->setAttribute(Qt::WA_PaintUnclipped, paintUnclipped);
    pe->setSystemClip(QRegion());

    //flush
    if (pe->type() == QPaintEngine::Raster) {
        QRasterPaintEngine *rpe = static_cast<QRasterPaintEngine *>(pe);
        rpe->flush(widget, QPoint());
    }
    QApplication::syncX();

#if defined(Q_OS_UNIX)
    ::usleep(1000*msec);
#elif defined(Q_OS_WIN)
    ::Sleep(msec);
#endif

}
#endif

static bool qt_flushPaint(QWidget *widget, const QRegion &toBePainted)
{
    static int checked_env = -1;
    if(checked_env == -1)
        checked_env = qgetenv("QT_FLUSH_PAINT").toInt();

    if (checked_env == 0)
        return false;

    qt_showYellowThing(widget, toBePainted, checked_env*10);

    return true;
}

static bool qt_flushUpdate(QWidget *widget, const QRegion &rgn)
{
    static int checked_env = -1;
    if(checked_env == -1) {
        checked_env = qgetenv("QT_FLUSH_UPDATE").toInt();
    }

    if (checked_env == 0)
        return false;

    qt_showYellowThing(widget, rgn, checked_env*10);

    return true;
}


void qt_syncBackingStore(QRegion rgn, QWidget *widget, bool recursive)
{
    if (!QWidgetBackingStore::paintOnScreen(widget)) {
        QWidget *tlw = widget->window();
        tlw->d_func()->topData()->backingStore->cleanRegion(rgn, widget, recursive);
    } else {
        widget->repaint(rgn);
    }
}
void qt_syncBackingStore(QRegion rgn, QWidget *widget)
{
    qt_syncBackingStore(rgn, widget, false);
}

#if defined(Q_WS_X11)
void qt_syncBackingStore(QWidget *widget)
{
    // dirtyOnScreen may get out of sync when widget is scrolled or moved
    widget->d_func()->dirtyOnScreen &= widget->d_func()->clipRect();

    const QRegion dirty =  widget->d_func()->dirtyOnScreen;
    QWidget *tlw = widget->window();
    if (!QWidgetBackingStore::paintOnScreen(widget)) {
        QWidgetBackingStore *bs = tlw->d_func()->topData()->backingStore;
        bs->cleanRegion(dirty, widget);
    } else {
        widget->repaint(dirty);
    }
}
#elif defined(Q_WS_QWS)
void qt_syncBackingStore(QWidget *widget)
{
    QWidget *tlw = widget->window();
    QTLWExtra *topData = tlw->d_func()->topData();

    QWidgetBackingStore *wbs = topData->backingStore;
    QRegion toClean = wbs->dirty_on_screen;

#if 0 // debug
    qDebug() << "qt_syncBackingStore" << tlw << tlw->rect();
    qDebug() << "dirty ==" << wbs->dirty;
    qDebug() << "dirty_on_screen ==" << wbs->dirty_on_screen;
#endif
    if (!toClean.isEmpty())
        topData->backingStore->cleanRegion(toClean, tlw);
}
#endif

/*
   A version of QRect::intersects() that does not normalize the rects.
*/
static inline bool qRectIntersects(const QRect &r1, const QRect &r2)
{
    return (qMax(r1.left(), r2.left()) <= qMin(r1.right(), r2.right()) &&
             qMax(r1.top(), r2.top()) <= qMin(r1.bottom(), r2.bottom()));
}

QWidgetBackingStore::QWidgetBackingStore(QWidget *t) : tlw(t)
#ifdef Q_WS_WIN
                                                     , buffer(t)
#endif
                                                     , dirtyBufferSize(false)
{

}

QWidgetBackingStore::~QWidgetBackingStore()
{

}

/*
  Widget's coordinate system
  move whole rect by dx,dy
  rect must be valid
  doesn't generate any updates
*/
void QWidgetBackingStore::bltRect(const QRect &rect, int dx, int dy, QWidget *widget)
{
#if defined(Q_WS_X11) || defined(Q_WS_QWS)
    if (buffer.isNull())
        return;
#endif

    QPoint pos(widget->mapTo(tlw, rect.topLeft()));

#if defined(Q_WS_WIN)
    QRasterPaintEngine *engine = (QRasterPaintEngine*) buffer.paintEngine();
    HDC engine_dc = engine->getDC();
    if (!engine_dc)
        return;

    BitBlt(engine_dc, pos.x()+dx, pos.y()+dy, rect.width(), rect.height(),
           engine_dc, pos.x(), pos.y(), SRCCOPY);
    engine->releaseDC(engine_dc);
#elif defined(Q_WS_X11)
//    qDebug("XCreateGC");
    GC gc = XCreateGC(tlw->d_func()->xinfo.display(), buffer.handle(), 0, 0);
//    qDebug() << "XCopyArea" << pos << rect << "dx" << dy << "dy" << dy;
    XCopyArea(X11->display, buffer.handle(), buffer.handle(), gc,
              pos.x(), pos.y(), rect.width(), rect.height(),
              pos.x()+dx, pos.y()+dy);
//    qDebug("XFreeGC");
    XFreeGC(tlw->d_func()->xinfo.display(), gc);
//    qDebug("done");
#elif defined(Q_WS_QWS)
    pos += topLevelOffset();
    QRect bsrect(pos, rect.size());
    buffer.lock(true);
    buffer.blit(bsrect, pos + QPoint(dx,dy));
    buffer.unlock();
#endif
}


//parent's coordinates; move whole rect; update parent and widget
//assume the screen blt has already been done, so we don't need to refresh that part
void QWidgetPrivate::moveRect(const QRect &rect, int dx, int dy)
{
    Q_Q(QWidget);
    if (!q->isVisible())
        return;

    QWidget *tlw = q->window();
    QTLWExtra* x = tlw->d_func()->topData();


    static int accelEnv = -1;
    if (accelEnv == -1) {
        accelEnv = qgetenv("QT_NO_FAST_MOVE").toInt() == 0;
    }

    QWidget *pw = q->parentWidget();
    QWidgetPrivate *pd = pw->d_func();
    QRect clipR = pd->clipRect();
    QRect newRect = rect.translated(dx,dy);

    bool accelerateMove = accelEnv &&  isOpaque()  && !isOverlapped(rect & clipR);

    if (!accelerateMove) {
        QRegion parentR(rect & clipR);
        if (q->mask().isEmpty()) {
            parentR -= newRect;
        } else {
            // invalidateBuffer() excludes anything outside the mask
            parentR += newRect & clipR;
        }
        pd->invalidateBuffer(parentR);
        invalidateBuffer((newRect & clipR).translated(-data.crect.topLeft()));
    } else {
        QRect destRect = rect.intersect(clipR).translated(dx,dy).intersect(clipR);
        QRect sourceRect = destRect.translated(-dx, -dy);
        QWidgetBackingStore *wbs = x->backingStore;
        if (sourceRect.isValid())
            wbs->bltRect(sourceRect, dx, dy, pw);


        QRegion childExpose = newRect & clipR;
        childExpose -= destRect;

        QPoint toplevelOffset = pw->mapTo(tlw, QPoint());
        QRect newDirty = (wbs->dirty & sourceRect.translated(toplevelOffset)).boundingRect().translated(QPoint(dx,dy) - toplevelOffset);
        childExpose += newDirty;

        childExpose.translate(-data.crect.topLeft());
        invalidateBuffer(childExpose);

        QRegion parentExpose = rect & clipR;
        parentExpose -= newRect;
        if (!q->mask().isEmpty()) {
            parentExpose += QRegion(newRect) - q->mask().translated(data.crect.topLeft());
        }
        pd->invalidateBuffer(parentExpose);
#ifdef Q_WS_QWS
        //QWS does not have native child widgets: copy everything to screen, just like scrollRect()
        pd->dirtyWidget_sys(QRegion(sourceRect)+destRect);
#endif
    }
}

//widget's coordinates; scroll within rect;  only update widget
void QWidgetPrivate::scrollRect(const QRect &rect, int dx, int dy)
{
    Q_Q(QWidget);
    QWidget *tlw = q->window();
    QTLWExtra* x = tlw->d_func()->topData();


    static int accelEnv = -1;
    if (accelEnv == -1) {
        accelEnv = qgetenv("QT_NO_FAST_SCROLL").toInt() == 0;
    }

    bool accelerateScroll = accelEnv &&  isOpaque()  && !isOverlapped(data.crect);

    if (!accelerateScroll) {
        invalidateBuffer(rect);
    } else {
        QRect scrollRect = rect & clipRect();

        QRect destRect = scrollRect.translated(dx,dy).intersect(scrollRect);
        QRect sourceRect = destRect.translated(-dx, -dy);

        QWidgetBackingStore *wbs = x->backingStore;

        QPoint toplevelOffset = q->mapTo(tlw, QPoint());


        if (sourceRect.isValid())
            wbs->bltRect(sourceRect, dx, dy, q);

        QRegion childExpose = scrollRect;
        childExpose -= destRect;
//        childExpose += (wbs->dirty & sourceRect.translated(toplevelOffset)).boundingRect().translated(QPoint(dx,dy) - toplevelOffset);
        QRect newDirty = (wbs->dirty & sourceRect.translated(toplevelOffset)).boundingRect().translated(QPoint(dx,dy) - toplevelOffset);
//         qDebug() << "scrollRect" << q << rect << dx << dy << "dirty" << wbs->dirty << "newDirty" << newDirty;
        childExpose += newDirty;
        invalidateBuffer(childExpose);

        // Instead of using native scroll-on-screen, we copy from
        // backingstore, giving only one screen update for each
        // scroll, and a solid appearance
        dirtyWidget_sys(rect);
    }
}

void QWidgetBackingStore::dirtyRegion(const QRegion &rgn, QWidget *widget)
{
    QRegion wrgn(rgn);
    Q_ASSERT(widget->window() == tlw);
    if(!widget->isVisible() || !widget->updatesEnabled())
        return;
    wrgn &= widget->d_func()->clipRect();
    if (!widget->mask().isEmpty())
        wrgn &= widget->mask();
#ifndef Q_WS_QWS
    widget->d_func()->dirtyWidget_sys(wrgn);
#endif
    if(!QWidgetBackingStore::paintOnScreen(widget)) {
        wrgn.translate(widget->mapTo(tlw, QPoint(0, 0)));
        dirty += wrgn;
#ifdef Q_WS_QWS
        tlw->d_func()->dirtyWidget_sys(wrgn); //optimization: don't translate twice
#endif
    }
}

void QWidgetBackingStore::copyToScreen(const QRegion &rgn, QWidget *widget, const QPoint &offset, bool recursive)
{
    if (rgn.isEmpty())
        return;
#ifdef Q_WS_QWS
    Q_UNUSED(offset);
    Q_UNUSED(recursive);
    QWidget *win = widget->window();
    QBrush bgBrush = win->palette().brush(win->backgroundRole());
    bool opaque = bgBrush.style() == Qt::NoBrush || bgBrush.isOpaque();
    QRegion globalrgn = rgn;
    globalrgn.translate(win->geometry().topLeft());
    win->qwsDisplay()->repaintRegion(win->data->winid, opaque, globalrgn);
    widget->d_func()->cleanWidget_sys(rgn);

    qt_flushUpdate(0, globalrgn);
#else
    if (!QWidgetBackingStore::paintOnScreen(widget)) {
        widget->d_func()->cleanWidget_sys(rgn);

        qt_flushUpdate(widget, rgn);

        QPoint wOffset = widget->data->wrect.topLeft();

#if defined(Q_WS_WIN)
        QRasterPaintEngine *engine = (QRasterPaintEngine*) buffer.paintEngine();
        HDC engine_dc = engine->getDC();
        HDC widget_dc = (HDC) widget->d_func()->hd;
        bool tmp_widget_dc = false;
        if (!widget_dc) {
            widget_dc = GetDC(widget->winId());
            tmp_widget_dc = true;
        }
        QRect br = rgn.boundingRect();
        QRect wbr = rgn.boundingRect().translated(wOffset);
        BitBlt(widget_dc, wbr.x(), wbr.y(), wbr.width(), wbr.height(),
               engine_dc, br.x() + offset.x(), br.y() + offset.y(), SRCCOPY);
        if (tmp_widget_dc)
            ReleaseDC(widget->winId(), widget_dc);
        engine->releaseDC(engine_dc);
#elif defined(Q_WS_X11)
        extern void *qt_getClipRects(const QRegion &r, int &num); // in qpaintengine_x11.cpp
        GC gc = XCreateGC(X11->display, buffer.handle(), 0, 0);
        QRegion wrgn(rgn);
        QRect br = rgn.boundingRect();
        if (!wOffset.isNull())
            wrgn.translate(-wOffset);
        QRect wbr = wrgn.boundingRect();
        int num;
        XRectangle *rects = (XRectangle *)qt_getClipRects(wrgn, num);
//         qDebug() << "XSetClipRectangles";
//         for  (int i = 0; i < num; ++i)
//             qDebug() << " " << i << rects[i].x << rects[i].x << rects[i].y << rects[i].width << rects[i].height;
        XSetClipRectangles(X11->display, gc, 0, 0, rects, num, YXBanded );
        XSetGraphicsExposures(X11->display, gc, False);
//         XFillRectangle(X11->display, widget->handle(), gc, 0, 0, widget->width(), widget->height());
        XCopyArea(X11->display, buffer.handle(), widget->handle(), gc,
                  br.x() + offset.x(), br.y() + offset.y(), br.width(), br.height(), wbr.x(), wbr.y());
        XFreeGC(X11->display, gc);
#endif
    }

    if(recursive) {
        const QObjectList children = widget->children();
        for(int i = 0; i < children.size(); ++i) {
            if(QWidget *child = qobject_cast<QWidget*>(children.at(i))) {
                if(!child->isWindow()) {
                    if (qRectIntersects(rgn.boundingRect().translated(-child->pos()), child->rect())) {
                        QRegion childRegion(rgn);
                        childRegion.translate(-child->pos());
                        childRegion &= child->d_func()->clipRect();
                        if(!childRegion.isEmpty())
                            copyToScreen(childRegion, child, offset+child->pos(), recursive);
                    }
                }
            }
        }
    }
#endif
}

void QWidgetBackingStore::cleanRegion(const QRegion &rgn, QWidget *widget, bool recursiveCopyToScreen)
{
    if (!widget->isVisible() || !widget->updatesEnabled() || !tlw->testAttribute(Qt::WA_Mapped) || rgn.isEmpty())
        return;

    if(!QWidgetBackingStore::paintOnScreen(widget)) {
        QRegion toClean;

#ifdef Q_WS_QWS
        //QWExtra *extra = tlw->d_func()->extra;
        QRect tlwFrame = tlw->frameGeometry();
        QSize tlwSize = tlwFrame.size();
#else
        QSize tlwSize = tlw->size();
#endif
        if (buffer.size() != tlwSize || dirtyBufferSize) {
#if defined(Q_WS_X11)
            extern int qt_x11_preferred_pixmap_depth;
            int old_qt_x11_preferred_pixmap_depth = qt_x11_preferred_pixmap_depth;
            qt_x11_preferred_pixmap_depth = widget->x11Info().depth();
            QPixmap::x11SetDefaultScreen(widget->x11Info().screen());
            buffer = QPixmap(tlwSize);
            qt_x11_preferred_pixmap_depth = old_qt_x11_preferred_pixmap_depth;
#elif defined(Q_WS_WIN)
            if (buffer.paintEngine())
                ((QRasterPaintEngine *)buffer.paintEngine())->releaseBuffer();
#elif defined(Q_WS_QWS)
            QRegion tlwRegion = tlwFrame;
            tlwOffset = tlw->geometry().topLeft() - tlwFrame.topLeft();
            if (!tlw->d_func()->extra->mask.isEmpty()) {
                tlwRegion = tlw->d_func()->extra->mask;
                tlwRegion.translate(tlw->geometry().topLeft());
                tlwRegion &= tlwFrame;
                tlwSize = tlwRegion.boundingRect().size();
                tlwOffset = tlw->geometry().topLeft() - tlwRegion.boundingRect().topLeft();
            }
            buffer.create(tlwSize);
            QBrush bgBrush = tlw->palette().brush(tlw->backgroundRole());
            bool opaque = bgBrush.style() == Qt::NoBrush || bgBrush.isOpaque();
            QWidget::qwsDisplay()->requestRegion(tlw->data->winid, buffer.memoryId(), opaque, tlwRegion);
            QTLWExtra *topextra = tlw->d_func()->extra->topextra;
#ifndef QT_NO_QWS_MANAGER
            if (topextra->qwsManager)
                QApplication::postEvent(topextra->qwsManager, new QPaintEvent(tlwFrame));
#endif
#endif
            toClean = QRegion(0, 0, tlw->width(), tlw->height());
            dirtyBufferSize = false;
        } else {
            toClean = dirty;
        }

        if(!toClean.isEmpty()) {
            dirty -= toClean;
#ifdef Q_WS_QWS
            buffer.lock();
#endif
            //cleanBuffer(toClean, tlw, tlwOffset, Recursive|AsRoot);
            paintToBuffer(toClean, tlw, tlwOffset);
#ifdef Q_WS_QWS
            buffer.unlock();
#endif
        }
        QRegion toFlush = rgn;
        toFlush.translate(widget->mapTo(tlw, QPoint()));
        if (recursiveCopyToScreen)
            copyToScreen(toFlush, tlw, tlwOffset, recursiveCopyToScreen);
        else
            copyToScreen(rgn, widget, widget->mapTo(tlw, QPoint()), false);
    }
}

#ifdef Q_WS_QWS
void QWidgetBackingStore::releaseBuffer()
{
    buffer.detach();
    QWidget::qwsDisplay()->requestRegion(tlw->data->winid, 0, true, QRegion(0));
}
#endif


bool QWidgetBackingStore::isOpaque(const QWidget *widget)
{
    return widget->d_func()->isOpaque();
}

#ifdef Q_WS_QWS
#define PAINTDEVICE buffer.pixmap()
#else
#define PAINTDEVICE (&buffer)
#endif

void QWidgetBackingStore::paintToBuffer(const QRegion &rgn, QWidget *widget, const QPoint &offset, bool asRoot)
{
    if (!widget->isVisible() || !widget->updatesEnabled() || rgn.isEmpty())
        return;

    QRegion toBePainted  = rgn & widget->d_func()->clipRect(); //(rgn & widget->d_func()->visibleRegion());
    widget->d_func()->subtractOpaqueChildren(toBePainted, widget->rect(), QPoint());

    if (!toBePainted.isEmpty()) {
        if (!QWidgetBackingStore::paintOnScreen(widget)) {
            //update the "in paint event" flag
            if (widget->testAttribute(Qt::WA_WState_InPaintEvent))
                qWarning("QWidget::repaint: recursive repaint detected.");
            widget->setAttribute(Qt::WA_WState_InPaintEvent);

            //clip away the new area
            bool flushed = qt_flushPaint(widget, toBePainted);
            QPainter::setRedirected(widget, PAINTDEVICE, -offset);
            QRegion wrgn = toBePainted;
            wrgn.translate(offset);
            PAINTDEVICE->paintEngine()->setSystemClip(wrgn);

            //paint the background
            if((asRoot && !widget->testAttribute(Qt::WA_NoBackground) && !widget->testAttribute(Qt::WA_NoSystemBackground))
               || widget->d_func()->hasBackground()) {

                QPainter p(widget);
                const QBrush bg = widget->palette().brush(widget->backgroundRole());
#ifdef Q_WS_QWS
                if (widget->isWindow())
                    p.setCompositionMode(QPainter::CompositionMode_Source); //copy alpha straight in
#endif
                if (bg.style() == Qt::TexturePattern)
                    p.drawTiledPixmap(toBePainted.boundingRect(), bg.texture(), toBePainted.boundingRect().topLeft());
                else
                    p.fillRect(toBePainted.boundingRect(), bg);
            }

#if 0
            qDebug() << "painting" << widget << "opaque ==" << hasBackground(widget);
            qDebug() << "clipping to" << toBePainted << "location == " << offset
                     << "geometry ==" << QRect(widget->mapTo(tlw, QPoint(0, 0)), widget->size());
            fflush(stderr);
#endif

            //actually send the paint event
            widget->setAttribute(Qt::WA_PendingUpdate, false);
            QPaintEvent e(toBePainted);
            qt_sendSpontaneousEvent(widget, &e);

            //restore
            PAINTDEVICE->paintEngine()->setSystemClip(QRegion());
            QPainter::restoreRedirected(widget);

            widget->setAttribute(Qt::WA_WState_InPaintEvent, false);
            if(!widget->testAttribute(Qt::WA_PaintOutsidePaintEvent) && widget->paintingActive())
                qWarning("It is dangerous to leave painters active on a widget outside of the PaintEvent");

            if (flushed)
                copyToScreen(toBePainted, widget, offset, false);
        } else if(widget == tlw) {
            QPainter p(PAINTDEVICE);
            p.setClipRegion(toBePainted);
            const QBrush bg = widget->palette().brush(widget->backgroundRole());
            if (bg.style() == Qt::TexturePattern)
                p.drawTiledPixmap(widget->rect(), bg.texture());
            else
                p.fillRect(widget->rect(), bg);
        }
    }

    //always be recursive
    {
        const QObjectList children = widget->children();
        for(int i = 0; i < children.size(); ++i) {
            if(QWidget *child = qobject_cast<QWidget*>(children.at(i))) {
                if(!child->isWindow() && child->isVisible() && child->updatesEnabled()) {
                    if (qRectIntersects(rgn.boundingRect().translated(-child->pos()), child->rect())) {
                        QRegion childRegion(rgn);
                        childRegion.translate(-child->pos());
                        childRegion &= child->d_func()->clipRect();
                        if(QWExtra *extra = child->d_func()->extraData()) {
                            if(!extra->mask.isEmpty())
                                childRegion &= extra->mask;
                        }
                        if(!childRegion.isEmpty())
                            paintToBuffer(childRegion, child, offset+child->pos(), false);
                    }
                }
            }
        }
    }
}

/* cross-platform QWidget code */

void QWidgetPrivate::invalidateBuffer(const QRegion &rgn)
{
    if(qApp && qApp->closingDown())
        return;
    Q_Q(QWidget);
    QWidget *tlw = q->window();
    QTLWExtra* x = tlw->d_func()->topData();
    x->backingStore->dirtyRegion(rgn, q);

    q->setAttribute(Qt::WA_PendingUpdate);
}

void QWidget::repaint(const QRegion& rgn)
{
    if (testAttribute(Qt::WA_WState_ConfigPending)) {
        update(rgn);
        return;
    }

    if (!isVisible() || !updatesEnabled() || rgn.isEmpty())
        return;
//    qDebug() << "repaint" << this << rgn;
    if (!QWidgetBackingStore::paintOnScreen(this)) {
        QWidget *tlw = window();
        QTLWExtra* x = tlw->d_func()->topData();
        x->backingStore->dirtyRegion(rgn, this);
        x->backingStore->cleanRegion(rgn, this);
    } else {
        Q_D(QWidget);
        d->cleanWidget_sys(rgn);


        //     qDebug() << "QWidget::repaint paintOnScreen" << this << "region" << rgn;


        //update the "in paint event" flag
        if (testAttribute(Qt::WA_WState_InPaintEvent))
            qWarning("QWidget::repaint: recursive repaint detected.");
        setAttribute(Qt::WA_WState_InPaintEvent);

        qt_flushPaint(this, rgn);

        QPaintEngine *engine = paintEngine();

        QRegion systemClipRgn(rgn);
#ifndef Q_WS_QWS //QWS doesn't need wrect
        if (!data->wrect.topLeft().isNull()) {
            QPainter::setRedirected(this, this, data->wrect.topLeft());
            systemClipRgn.translate(-data->wrect.topLeft());
        }
#endif
        engine->setSystemClip(systemClipRgn);

        //paint the background
        QPaintEvent e(rgn);
        if (!testAttribute(Qt::WA_NoBackground)
            && !testAttribute(Qt::WA_NoSystemBackground)) {
            d->composeBackground(rgn.boundingRect());
#ifdef QT3_SUPPORT
            e.setErased(true);
#endif
        }

        //actually send the paint event
        setAttribute(Qt::WA_PendingUpdate, false);
        QApplication::sendSpontaneousEvent(this, &e);

#ifdef Q_WS_WIN
        if (engine && engine->type() == QPaintEngine::Raster) {
            bool tmp_dc = !d->hd;
            if (tmp_dc)
                d->hd = GetDC(winId());
            static_cast<QRasterPaintEngine *>(engine)->flush(this, QPoint(0, 0));
            if (tmp_dc) {
                ReleaseDC(winId(), (HDC)d->hd);
                d->hd = 0;
            }
        }
#endif
#ifndef Q_WS_QWS
        if (!data->wrect.topLeft().isNull())
            QPainter::restoreRedirected(this);
#endif
        engine->setSystemClip(QRegion());

        setAttribute(Qt::WA_WState_InPaintEvent, false);
        if(!testAttribute(Qt::WA_PaintOutsidePaintEvent) && paintingActive())
            qWarning("It is dangerous to leave painters active on a widget outside of the PaintEvent");
    }
}

void QWidget::update()
{
    update(rect());
}

void QWidget::update(const QRect &r)
{
    update(QRegion(r));
}

void QWidget::update(const QRegion& rgn)
{
    if(!isVisible() || !updatesEnabled() || rgn.isEmpty())
        return;

    if (testAttribute(Qt::WA_WState_InPaintEvent)) {
        QApplication::postEvent(this, new QUpdateLaterEvent(rgn));
    } else {
        QWidget *tlw = window();
        QTLWExtra* x = tlw->d_func()->topData();
        x->backingStore->dirtyRegion(rgn, this);

        setAttribute(Qt::WA_PendingUpdate);
    }
}

