#include <qpaintdevice.h>

#include "fontenginexft.h"

#include <qstring.h>
#include <qpaintdevice.h>
#include "qpainter.h"
#include <qt_x11.h>
#include "qtextlayout.h"
#include "opentype.h"

#include <stdlib.h>

// #define FONTENGINE_DEBUG

class HackPaintDevice : public QPaintDevice
{
public:
    HackPaintDevice() : QPaintDevice( 0 ) {}
    XftDraw *xftDrawHandle() const {
	return (XftDraw *)rendhd;
    }
};

// ### all this won't work with Xft2!!!!
// we need to encapsulate these in some methods to get it working!

inline XftFontStruct *
getFontStruct( XftFont *font )
{
    if (font->core)
	return 0;
    return font->u.ft.font;
}

// ditto
static inline void getGlyphInfo(XGlyphInfo *xgi, XftFont *font, int glyph)
{
    XftTextExtents32(QPaintDevice::x11AppDisplay(), font, (XftChar32 *) &glyph, 1, xgi);
}




FontEngineXft::FontEngineXft( XftFont *font, XftPattern *pattern, int cmap )
    : _font( font ), _pattern( pattern ), _openType( 0 ), _cmap( cmap )
{
    XftFontStruct *xftfs = getFontStruct( _font );
    if ( xftfs ) {
	// dirty hack: we set the charmap in the Xftfreetype to -1, so XftFreetype assumes no encoding and
	// really draws glyph indices. The FT_Face still has the Unicode encoding to we can convert from
	// Unicode to glyph index
	xftfs->charmap = -1;
    }
}

FontEngineXft::~FontEngineXft()
{
    XftFontClose( QPaintDevice::x11AppDisplay(),_font );
    XftPatternDestroy( _pattern );
    _font = 0;
    _pattern = 0;
    delete _openType;
}

FontEngineIface::Error FontEngineXft::stringToCMap( const QChar *str,  int len, GlyphIndex *glyphs, int *nglyphs ) const
{
    if ( *nglyphs < len ) {
	*nglyphs = len;
	return OutOfMemory;
    }

    XftFontStruct *fs = getFontStruct( _font );
    if ( !fs ) {
	for ( int i = 0; i < len; i++ )
	    glyphs[i] = str[i].unicode();
    } else {
	for ( int i = 0; i < len; i++ )
	    glyphs[i] = FT_Get_Char_Index (fs->face, str[i].unicode() );
    }
    *nglyphs = len;
    return NoError;
}

void FontEngineXft::draw( QPainter *p, int x, int y, const GlyphIndex *glyphs,
			   const Offset *advances, const Offset *offsets, int numGlyphs, bool reverse )
{
    if ( !numGlyphs )
	return;

    Qt::BGMode bgmode = p->backgroundMode();
    XftDraw *draw = ((HackPaintDevice *)p->device())->xftDrawHandle();
    const QColor &bgcolor = p->backgroundColor();
    const QColor &pen = p->pen().color();

    if (bgmode != Qt::TransparentMode) {
	XftColor col;
	col.color.red = bgcolor.red()     | bgcolor.red() << 8;
	col.color.green = bgcolor.green() | bgcolor.green() << 8;
	col.color.blue = bgcolor.blue()   | bgcolor.blue() << 8;
	col.color.alpha = 0xffff;
	col.pixel = bgcolor.pixel();
	// ### not quite correct, should rather use the bounding box here.
	QGlyphInfo bb = boundingBox( glyphs, advances, offsets, numGlyphs );
	XftDrawRect(draw, &col, x+bb.x,  y + bb.y, bb.width, bb.height );
    }

    XftColor col;
    col.color.red = pen.red () | pen.red() << 8;
    col.color.green = pen.green () | pen.green() << 8;
    col.color.blue = pen.blue () | pen.blue() << 8;
    col.color.alpha = 0xffff;
    col.pixel = pen.pixel();
#ifdef FONTENGINE_DEBUG
    p->save();
    p->setBrush( Qt::white );
    QGlyphInfo ci = boundingBox( glyphs, advances, offsets, numGlyphs );
    p->drawRect( x + ci.x, y + ci.y, ci.width, ci.height );
    p->drawRect( x + ci.x, y + 100 + ci.y, ci.width, ci.height );
//     qDebug("bounding rect=%d %d (%d/%d)", ci.x, ci.y, ci.width, ci.height );
    p->restore();
    int xp = x;
    int yp = y;
#endif
    if ( reverse ) {
	int i = numGlyphs;
	while( i-- ) {
	    Offset adv = advances[i];
	    // 	    qDebug("advance = %d/%d", adv.x, adv.y );
	    x += adv.x;
	    y += adv.y;
	    QGlyphInfo gi = boundingBox( glyphs[i] );
	    XftDrawString16 (draw, &col, _font, x-offsets[i].x-gi.xoff, y+offsets[i].y-gi.yoff,
			     (XftChar16 *) (glyphs+i), 1);
#ifdef FONTENGINE_DEBUG
	    p->drawRect( x - offsets[i].x - gi.xoff + gi.x, y + 100 + offsets[i].y - gi.yoff + gi.y, gi.width, gi.height );
	    p->drawLine( x - offsets[i].x - gi.xoff, y + 150 + 5*i , x - offsets[i].x, y + 150 + 5*i );

#endif
	}
    } else {
	int i = 0;
	while ( i < numGlyphs ) {
	    XftDrawString16 (draw, &col, _font, x+offsets[i].x, y+offsets[i].y,
			     (XftChar16 *) (glyphs+i), 1);
	    Offset adv = advances[i];
	    // 	    qDebug("advance = %d/%d", adv.x, adv.y );
	    x += adv.x;
	    y += adv.y;
	    i++;
	}
    }
#ifdef FONTENGINE_DEBUG
    if ( !reverse ) {
	x = xp;
	y = yp;
	p->save();
	p->setPen( Qt::red );
	for ( int i = 0; i < numGlyphs; i++ ) {
	    QGlyphInfo ci = boundingBox( glyphs[i] );
	    p->drawRect( x + ci.x + offsets[i].x, y + 100 + ci.y + offsets[i].y, ci.width, ci.height );
	    qDebug("bounding ci[%d]=%d %d (%d/%d) / %d %d   offs=(%d/%d) advance=(%d/%d)", i, ci.x, ci.y, ci.width, ci.height,
		   ci.xoff, ci.yoff, offsets[i].x, offsets[i].y,
		   advances[i].x, advances[i].y);
	    x += advances[i].x;
	    y += advances[i].y;
	}
	p->restore();
    }
#endif
}

QGlyphInfo FontEngineXft::boundingBox( const GlyphIndex *glyphs, const Offset *advances, const Offset *offsets, int numGlyphs )
{
    XGlyphInfo xgi;

    QGlyphInfo overall;
    int ymax = 0;
    int xmax = 0;
    for (int i = 0; i < numGlyphs; i++) {
	getGlyphInfo( &xgi, _font, glyphs[i] );
	int x = overall.xoff + offsets[i].x - xgi.x;
	int y = overall.yoff + offsets[i].y - xgi.y;
	overall.x = QMIN( overall.x, x );
	overall.y = QMIN( overall.y, y );
	xmax = QMAX( xmax, x + xgi.width );
	ymax = QMAX( ymax, y + xgi.height );
	overall.xoff += advances[i].x;
	overall.yoff -= advances[i].y;
    }
    overall.height = ymax - overall.y;
    overall.width = xmax - overall.x;

    return overall;
}

QGlyphInfo FontEngineXft::boundingBox( GlyphIndex glyph )
{
    XGlyphInfo xgi;
    getGlyphInfo( &xgi, _font, glyph );
    return QGlyphInfo( -xgi.x, -xgi.y, xgi.width, xgi.height, xgi.xOff, -xgi.yOff );
}



int FontEngineXft::ascent() const
{
    return _font->ascent;
}

int FontEngineXft::descent() const
{
    return _font->descent;
}

int FontEngineXft::leading() const
{
    int l = qRound( (ascent() + descent() ) * 0.15 );
    return (l > 0) ? l : 1;
}

int FontEngineXft::maxCharWidth() const
{
    return _font->max_advance_width;
}

int FontEngineXft::cmap() const
{
    return _cmap;
}

const char *FontEngineXft::name() const
{
    return "xft";
}

bool FontEngineXft::canRender( const QChar *string,  int len )
{
    GlyphIndex glyphs[256];
    int nglyphs = 255;
    GlyphIndex *g = glyphs;
    if ( stringToCMap( string, len, g, &nglyphs ) == OutOfMemory ) {
	g = (GlyphIndex *)malloc( nglyphs*sizeof(GlyphIndex) );
	stringToCMap( string, len, g, &nglyphs );
    }

    bool allExist = TRUE;
    for ( int i = 0; i < nglyphs; i++ ) {
	if ( !XftGlyphExists(QPaintDevice::x11AppDisplay(), _font, g[i]) ) {
	    allExist = FALSE;
	    break;
	}
    }

    if ( nglyphs > 255 )
	free( g );
	return allExist;

}

OpenTypeIface *FontEngineXft::openTypeIface() const
{
//     qDebug("openTypeIface requested!");
    if ( _openType )
	return _openType;
    XftFontStruct *xftfs = getFontStruct( _font );
    if ( !xftfs ) {
	qDebug("font is core font!");
	return 0;
    }

    FontEngineXft *that = (FontEngineXft *)this;

    that->_openType = new OpenTypeIface( xftfs->face );
    return _openType;
}


FontEngineIface::Type FontEngineXft::type() const
{
    return Xft;
}
