/****************************************************************************
**
** Definition of QPaintEngine(for Mac) private data.
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

#ifndef __QPAINTENGINE_MAC_P_H__
#define __QPAINTENGINE_MAC_P_H__

#include "qpaintengine_p.h"

/*****************************************************************************
  QuickDraw Private data
 *****************************************************************************/
static int ropCodes[] = {			// ROP translation table
    patCopy, subPin, patXor, patBic, notPatCopy,
    notPatOr, notPatXor, notPatBic,
    666, 666, 666, 666, 666, 666, 666, 666, 666
};

class paintevent_item;
class QQuickDrawPaintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QQuickDrawPaintEngine);
public:
    QQuickDrawPaintEnginePrivate()
	: QPaintEnginePrivate()
    {
	saved = 0;
	paintevent = 0;
	clip.serial = 0;
	clip.dirty = locked = unclipped = false;
	clip.pdev = clip.paintable = QRegion();
	brush_style_pix = 0;
	offx = offy = 0;
    }

    struct {
	QPen pen;
	QBrush brush;
	Qt::RasterOp rop;
	QRegion clip;
	struct {
	    QPoint origin;
	    Qt::BGMode mode;
	    QBrush brush;
	} bg;
    } current;

    int offx, offy;
    QPixmap *brush_style_pix;
    uint unclipped : 1, locked : 1;
    QMacSavedPortInfo *saved;
    paintevent_item *paintevent;

    struct {
	QRegion pdev, paintable;
	uint dirty : 1, serial : 15;
    } clip;

};

/*****************************************************************************
  Private data
 *****************************************************************************/
class QCoreGraphicsPaintEnginePrivate : public QQuickDrawPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QQuickDrawPaintEngine);
public:
    QCoreGraphicsPaintEnginePrivate()
    {
	hd = 0;
        shading = 0;
    }

    //state info
    struct {
	QPen pen;
	QBrush brush;
	struct {
	    QPoint origin;
	    Qt::BGMode mode;
	    QBrush brush;
	} bg;
    } current;

    //cg strucutres
    CGContextRef hd;
    CGShadingRef shading;

    //flags
    uint antiAliasingEnabled : 1;

    //internal functions
    inline void strokePath() {
        if(current.pen.style() != Qt::NoPen)
            CGContextStrokePath(hd);
    }
    inline void fillPath() { 
        if(current.brush.style() == Qt::LinearGradientPattern) {
            CGContextSaveGState(hd);
            CGContextClip(hd);
            CGContextDrawShading(hd, shading);
            CGContextRestoreGState(hd);
        } else if(current.brush.style() != Qt::NoBrush) {
            CGContextFillPath(hd);
        }
    }
};

#endif /* __QPAINTENGINE_MAC_P_H__ */
