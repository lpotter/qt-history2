/****************************************************************************
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

#include "qfontengine_p.h"
#include "qmemorymanager_qws.h"
#include <private/qunicodetables_p.h>
#include <qpainter.h>
#include <qgfxraster_qws.h>
#include <private/qunicodetables_p.h>
#include <qbitmap.h>
#if Q_Q3PAINTER
#include "qpainter_p.h"
#include "qgc_qws.h"
#define GFX(p) static_cast<QWSGC *>(p->device()->gc())->gfx()
#endif

#include "qgfx_qws.h"



/*QMemoryManager::FontID*/ void *QFontEngine::handle() const
{
    return id;
}

QFontEngine::QFontEngine( const QFontDef& d )
{
    QFontDef fontDef = d;
    id = memorymanager->refFont(fontDef);
}

QFontEngine::~QFontEngine()
{
    memorymanager->derefFont(id);
}


/* returns 0 as glyph index for non existant glyphs */
QFontEngine::Error QFontEngine::stringToCMap( const QChar *str, int len, glyph_t *glyphs, advance_t *advances, int *nglyphs, bool mirrored ) const
{
    if(*nglyphs < len) {
	*nglyphs = len;
	return OutOfMemory;
    }
    if ( mirrored ) {
	for(int i = 0; i < len; i++ )
	    glyphs[i] = ::mirroredChar(str[i]).unicode();
    } else {
	for(int i = 0; i < len; i++ )
	    glyphs[i] = str[i].unicode();
    }
    *nglyphs = len;
    if(advances) {
	for(int i = 0; i < len; i++)
	    if ( ::category(str[i]) == QChar::Mark_NonSpacing )
		advances[i] = 0;
	    else
		advances[i] = memorymanager->lockGlyphMetrics(handle(), str[i].unicode() )->advance;
    }
    return NoError;
}

void QFontEngine::draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags )
{
#ifndef QT_NO_TRANSFORMATIONS
#ifndef Q_Q3PAINTER
    if ( p->d->txop > QPainter::TxScale ) {
#else
    if ( p->txop >= QPainter::TxScale ) {
#endif
	int aw = si->width;
	int ah = si->ascent + si->descent + 1;
	int tx = 0;
	int ty = 0;
	if ( aw == 0 || ah == 0 )
	    return;

#ifndef Q_Q3PAINTER
	QWMatrix mat1 = p->d->matrix;
#else
	QWMatrix mat1 = p->xmat;
#endif
#ifndef QT_NO_PIXMAP_TRANSFORMATION
	QWMatrix mat2 = QPixmap::trueMatrix( mat1, aw, ah );
#endif
	QPixmap *tpm = 0;
	QBitmap *wx_bm = 0;
	if ( memorymanager->fontSmooth(handle()) &&
	     QPaintDevice::qwsDisplay()->supportsDepth(32) )
	{
	    QPixmap pm(aw, ah, 32);
	    QPainter paint(&pm);
	    paint.fillRect(pm.rect(),Qt::black);
	    paint.setPen(QPen(Qt::white));
	    draw( &paint, 0, si->ascent, engine, si, textFlags );
	    paint.end();
	    // Now we have an image with r,g,b gray scale set.
	    // Put this in alpha channel and set pixmap to pen color.
#ifndef Q_Q3PAINTER
	    QRgb bg = p->pen().color().rgb() & 0x00FFFFFF;
#else
	    QRgb bg = p->cpen.color().rgb() & 0x00FFFFFF;
#endif
	    for ( int y = 0; y < ah; y++ ) {
		uint *p = (uint *)pm.scanLine(y);
		for ( int x = 0; x < aw; x++ ) {
		    int a = *p & 0xFF;
		    *p = bg | (a << 24);
		    p++;
		}
	    }
#ifndef QT_NO_PIXMAP_TRANSFORMATION
	    tpm = new QPixmap( pm.xForm( mat2 ) );
#else
	    tpm = new QPixmap( pm );
#endif
	    if ( tpm->isNull() ) {
		delete tpm;
		return;
	    }
	} else {
#if 0
	    bm_key = gen_text_bitmap_key( mat2, dfont, string, len );
	    wx_bm = get_text_bitmap( bm_key );
	    create_new_bm = wx_bm == 0;
	    if ( create_new_bm && !empty )
#endif
	    { // no such cached bitmap
		QBitmap bm( aw, ah, TRUE );	// create bitmap
		QPainter paint;
		paint.begin( &bm );		// draw text in bitmap
		paint.setPen( p->color1 );
		draw( &paint, 0, si->ascent, engine, si, textFlags );
		paint.end();
#ifndef QT_NO_PIXMAP_TRANSFORMATION
		wx_bm = new QBitmap( bm.xForm(mat2) ); // transform bitmap
#else
		wx_bm = new QBitmap( bm );
#endif
		if ( wx_bm->isNull() ) {
		    delete wx_bm;		// nothing to draw
		    return;
		}
	    }
	}
	double fx=x, fy=y - si->ascent, nfx, nfy;
	mat1.map( fx,fy, &nfx,&nfy );
	double tfx=tx, tfy=ty, dx, dy;
#ifndef QT_NO_PIXMAP_TRANSFORMATION
	mat2.map( tfx, tfy, &dx, &dy );     // compute position of bitmap
#endif
	x = qRound(nfx-dx);
	y = qRound(nfy-dy);

	if ( memorymanager->fontSmooth(handle()) &&
	     QPaintDevice::qwsDisplay()->supportsDepth(32) ) {
#if !defined(Q_Q3PAINTER)
	    GFX(p)->setSource( tpm );
	    GFX(p)->setAlphaType(QGfx::InlineAlpha);
	    GFX(p)->blt(x, y, tpm->width(),tpm->height(), 0, 0);
#else
	    p->gfx->setSource( tpm );
	    p->gfx->setAlphaType(QGfx::InlineAlpha);
	    p->gfx->blt(x, y, tpm->width(),tpm->height(), 0, 0);
#endif
	    delete tpm;
	    return;
	} else {
#if !defined(Q_Q3PAINTER)
	    GFX(p)->setSource(wx_bm);
	    GFX(p)->setAlphaType(QGfx::LittleEndianMask);
	    GFX(p)->setAlphaSource(wx_bm->scanLine(0), wx_bm->bytesPerLine());
	    GFX(p)->blt(x, y, wx_bm->width(),wx_bm->height(), 0, 0);
#else
	    p->gfx->setSource(wx_bm);
	    p->gfx->setAlphaType(QGfx::LittleEndianMask);
	    p->gfx->setAlphaSource(wx_bm->scanLine(0), wx_bm->bytesPerLine());
	    p->gfx->blt(x, y, wx_bm->width(),wx_bm->height(), 0, 0);
#endif

#if 0
	    if ( create_new_bm )
		ins_text_bitmap( bm_key, wx_bm );
#else
	    delete wx_bm;
#endif
	}
	return;
    }
#endif

#ifndef QT_NO_TRANSFORMATIONS
# ifndef Q_Q3PAINTER
    if (p->d->txop == QPainter::TxTranslate)
# else
    if ( p->txop == QPainter::TxTranslate )
# endif
#endif
	p->map( x, y, &x, &y );

    if ( textFlags ) {
	int lw = lineThickness();
#ifndef Q_Q3PAINTER
	GFX(p)->setBrush( p->pen().color() );
#else
	p->gfx->setBrush( p->cpen.color() );
#endif
	if ( textFlags & Qt::Underline )
#ifndef Q_Q3PAINTER
	    GFX(p)->fillRect( x, y+underlinePosition(), si->width, lw );
#else
	    p->gfx->fillRect( x, y+underlinePosition(), si->width, lw );
#endif
	if ( textFlags & Qt::StrikeOut )
#ifndef Q_Q3PAINTER
	    GFX(p)->fillRect( x, y-ascent()/3, si->width, lw );
#else
	    p->gfx->fillRect( x, y-ascent()/3, si->width, lw );
#endif
	if ( textFlags & Qt::Overline )
#ifndef Q_Q3PAINTER
	    GFX(p)->fillRect( x, y-ascent()-1, si->width, lw );
	GFX(p)->setBrush( p->brush() );
#else
	    p->gfx->fillRect( x, y-ascent()-1, si->width, lw );
	p->gfx->setBrush( p->cbrush );
#endif
    }

    if ( si->isSpace )
	return;

    glyph_t *glyphs = engine->glyphs( si );
    advance_t *advances = engine->advances( si );
    qoffset_t *offsets = engine->offsets( si );

    struct Pos {
	int x;
	int y;
    };
    Pos _positions[64];
    Pos *positions = _positions;
    if ( si->num_glyphs > 64 )
	positions = new Pos[si->num_glyphs];

    if ( si->analysis.bidiLevel % 2 ) {
	int i = si->num_glyphs;
	while( i-- ) {
	    x += advances[i];
	    glyph_metrics_t gi = boundingBox( glyphs[i] );
	    positions[i].x = x-offsets[i].x-gi.xoff;
	    positions[i].y = y+offsets[i].y-gi.yoff;
	}
    } else {
	int i = 0;
	while( i < si->num_glyphs ) {
	    positions[i].x = x+offsets[i].x;
	    positions[i].y = y+offsets[i].y;
	    x += advances[i];
	    i++;
	}
    }
    QConstString cstr( (QChar *)glyphs, si->num_glyphs );
#if !defined(Q_Q3PAINTER)
    GFX(p)->drawGlyphs(handle(), glyphs, (QPoint *)positions, si->num_glyphs);
#else
    p->internalGfx()->drawGlyphs(handle(), glyphs, (QPoint *)positions, si->num_glyphs);
#endif
    if ( positions != _positions )
	delete [] positions;
}

glyph_metrics_t QFontEngine::boundingBox( const glyph_t *glyphs,
					  const advance_t *advances, const qoffset_t *offsets, int numGlyphs )
{
    Q_UNUSED( glyphs );
    Q_UNUSED( offsets );
    Q_UNUSED( advances );

    if ( numGlyphs == 0 )
	return glyph_metrics_t();

    int w = 0;
    const advance_t *end = advances + numGlyphs;
    while( end > advances )
	w += *(--end);

    return glyph_metrics_t(0, -ascent(), w, ascent()+descent()+1, w, 0 );
}

glyph_metrics_t QFontEngine::boundingBox( glyph_t glyph )
{
    QGlyphMetrics *metrics = memorymanager->lockGlyphMetrics( handle(), glyph);
    return glyph_metrics_t( metrics->bearingx, metrics->bearingy, metrics->width, metrics->height, metrics->advance, 0 );
}

bool QFontEngine::canRender( const QChar *string,  int len )
{
    bool allExist = TRUE;
    while ( len-- )
	if ( !memorymanager->inFont( handle(), string[len].unicode() ) ) {
	    allExist = FALSE;
	    break;
	}

    return allExist;
}


int QFontEngine::ascent() const
{
    return memorymanager->fontAscent(handle());
}

int QFontEngine::descent() const
{
    return memorymanager->fontDescent(handle());
}

int QFontEngine::leading() const
{
    return memorymanager->fontLeading(handle());
}

int QFontEngine::maxCharWidth() const
{
    return memorymanager->fontMaxWidth(handle());
}

int QFontEngine::minLeftBearing() const
{
    return memorymanager->fontMinLeftBearing(handle());
}

int QFontEngine::minRightBearing() const
{
    return memorymanager->fontMinRightBearing(handle());
}

int QFontEngine::underlinePosition() const
{
    return memorymanager->fontUnderlinePos(handle());
}

int QFontEngine::lineThickness() const
{
    return memorymanager->fontLineWidth(handle());
}

