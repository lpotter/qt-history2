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

#ifndef QFONTENGINE_P_H
#define QFONTENGINE_P_H

#ifndef QT_H
#include "qglobal.h"
#endif // QT_H

#ifdef Q_WS_WIN
#include "qt_windows.h"
#endif

#include "qtextengine_p.h"

class QPaintDevice;

struct glyph_metrics_t;
class QChar;
typedef unsigned short glyph_t;
struct qoffset_t;
typedef int advance_t;
class QOpenType;
struct TransformedFont;

#if defined( Q_WS_X11 ) || defined( Q_WS_WIN) || defined( Q_WS_MAC )
class QFontEngine : public QShared
{
public:
    enum Error {
	NoError,
	OutOfMemory
    };

    enum Type {
	// X11 types
	Box,
	XLFD,
	LatinXLFD,
	Xft,

	// MS Windows types
	Win,
	Uniscribe,

	// Apple MacOS types
	Mac,

	// Trolltech QWS types
	QWS
    };

    QFontEngine() {
	count = 0; cache_count = 0;
#ifdef Q_WS_X11
	transformed_fonts = 0;
#endif
    }
    virtual ~QFontEngine();

    /* returns 0 as glyph index for non existant glyphs */
    virtual Error stringToCMap( const QChar *str, int len, glyph_t *glyphs,
				advance_t *advances, int *nglyphs, bool mirrored ) const = 0;

#ifdef Q_WS_X11
    virtual int cmap() const { return -1; }
    virtual QOpenType *openType() const { return 0; }
#endif

    virtual void draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags ) = 0;

    virtual glyph_metrics_t boundingBox( const glyph_t *glyphs,
					 const advance_t *advances,
					 const qoffset_t *offsets, int numGlyphs ) = 0;
    virtual glyph_metrics_t boundingBox( glyph_t glyph ) = 0;

    virtual int ascent() const = 0;
    virtual int descent() const = 0;
    virtual int leading() const = 0;

    virtual int lineThickness() const;
    virtual int underlinePosition() const;

    virtual int maxCharWidth() const = 0;
    virtual int minLeftBearing() const { return 0; }
    virtual int minRightBearing() const { return 0; }

    virtual const char *name() const = 0;

    virtual bool canRender( const QChar *string,  int len ) = 0;

    virtual void setScale( double ) {}
    virtual double scale() const { return 1.; }

    virtual Type type() const = 0;

    QFontDef fontDef;
    uint cache_cost; // amount of mem used in kb by the font
    int cache_count;

#ifdef Q_WS_WIN
    HDC dc() const;
    void getGlyphIndexes( const QChar *ch, int numChars, glyph_t *glyphs, bool mirrored ) const;
    void getCMap();

    QByteArray _name;
    HDC		hdc;
    HFONT	hfont;
    LOGFONT     logfont;
    uint	stockFont   : 1;
    uint	paintDevice : 1;
    uint        useTextOutA : 1;
    uint        ttf         : 1;
    union {
	TEXTMETRICW	w;
	TEXTMETRICA	a;
    } tm;
    int		lw;
    unsigned char *cmap;
    void *script_cache;
    short lbearing;
    short rbearing;
#endif // Q_WS_WIN
#ifdef Q_WS_X11
    TransformedFont *transformed_fonts;
#endif
};
#elif defined( Q_WS_QWS )
class QGfx;

class QFontEngine : public QShared
{
public:
    QFontEngine( const QFontDef& );
   ~QFontEngine();
    /*QMemoryManager::FontID*/ void *handle() const;

    enum Type {
	// X11 types
	Box,
	XLFD,
	Xft,

	// MS Windows types
	Win,
	Uniscribe,

	// Apple MacOS types
	Mac,

	// Trolltech QWS types
	Qws
    };

    enum TextFlags {
	Underline = 0x01,
	Overline  = 0x02,
	StrikeOut = 0x04
    };

    enum Error {
	NoError,
	OutOfMemory
    };
    /* returns 0 as glyph index for non existant glyphs */
    Error stringToCMap( const QChar *str, int len, glyph_t *glyphs, advance_t *advances, int *nglyphs, bool mirrored ) const;

    void draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags );

    glyph_metrics_t boundingBox( const glyph_t *glyphs,
			       const advance_t *advances, const qoffset_t *offsets, int numGlyphs );
    glyph_metrics_t boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;
    int minLeftBearing() const;
    int minRightBearing() const;
    int underlinePosition() const;
    int lineThickness() const;

    Type type() { return Qws; }

    bool canRender( const QChar *string,  int len );

    QFontDef fontDef;
    /*QMemoryManager::FontID*/ void *id;
    int cache_cost;
    int cache_count;
};
#endif // WIN || X11 || MAC



enum IndicFeatures {
    CcmpFeature,
    InitFeature,
    NuktaFeature,
    AkhantFeature,
    RephFeature,
    BelowFormFeature,
    HalfFormFeature,
    PostFormFeature,
    VattuFeature,
    PreSubstFeature,
    AboveSubstFeature,
    BelowSubstFeature,
    PostSubstFeature,
    HalantFeature,
};

#if defined(Q_WS_X11) || defined(Q_WS_WIN)
class QFontEngineBox : public QFontEngine
{
public:
    QFontEngineBox( int size );
    ~QFontEngineBox();

    Error stringToCMap( const QChar *str,  int len, glyph_t *glyphs, advance_t *advances, int *nglyphs, bool mirrored ) const;

    void draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags );

    virtual glyph_metrics_t boundingBox( const glyph_t *glyphs,
				    const advance_t *advances, const qoffset_t *offsets, int numGlyphs );
    glyph_metrics_t boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;
    int minLeftBearing() const { return 0; }
    int minRightBearing() const { return 0; }

#ifdef Q_WS_X11
    int cmap() const;
#endif
    const char *name() const;

    bool canRender( const QChar *string,  int len );

    Type type() const;
    inline int size() const { return _size; }

private:
    friend class QFontPrivate;
    int _size;
};
#endif

#ifdef Q_WS_X11
#include "qt_x11_p.h"


struct TransformedFont
{
    float xx;
    float xy;
    float yx;
    float yy;
    union {
	Font xlfd_font;
#ifndef QT_NO_XFTFREETYPE
	XftFont *xft_font;
#endif
    };
    TransformedFont *next;
};

class QTextCodec;

#ifndef QT_NO_XFTFREETYPE
#include <freetype/freetype.h>
#include "ftxopen.h"

class QFontEngineXft : public QFontEngine
{
public:
    QFontEngineXft( XftFont *font, XftPattern *pattern, int cmap );
    ~QFontEngineXft();

    QOpenType *openType() const;

    Error stringToCMap( const QChar *str,  int len, glyph_t *glyphs, advance_t *advances, int *nglyphs, bool mirrored ) const;

    void draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags );

    virtual glyph_metrics_t boundingBox( const glyph_t *glyphs,
				    const advance_t *advances, const qoffset_t *offsets, int numGlyphs );
    glyph_metrics_t boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int lineThickness() const;
    int underlinePosition() const;
    int maxCharWidth() const;
    int minLeftBearing() const;
    int minRightBearing() const;

    int cmap() const;
    const char *name() const;

    void setScale( double scale );
    double scale() const { return _scale; }

    bool canRender( const QChar *string,  int len );

    Type type() const;
    XftPattern *pattern() const { return _pattern; }

    void recalcAdvances( int len, glyph_t *glyphs, advance_t *advances );

private:
    friend class QFontPrivate;
    XftFont *_font;
    XftPattern *_pattern;
    FT_Face _face;
    QOpenType *_openType;
    int _cmap;
    short lbearing;
    short rbearing;
    float _scale;
    enum { widthCacheSize = 0x800, cmapCacheSize = 0x500 };
    unsigned char widthCache[widthCacheSize];
    glyph_t cmapCache[cmapCacheSize];
};
#endif

class QFontEngineLatinXLFD;

class QFontEngineXLFD : public QFontEngine
{
public:
    QFontEngineXLFD( XFontStruct *fs, const char *name, int cmap );
    ~QFontEngineXLFD();

    Error stringToCMap( const QChar *str,  int len, glyph_t *glyphs, advance_t *advances, int *nglyphs, bool mirrored ) const;

    void draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags );

    virtual glyph_metrics_t boundingBox( const glyph_t *glyphs,
				    const advance_t *advances, const qoffset_t *offsets, int numGlyphs );
    glyph_metrics_t boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;
    int minLeftBearing() const;
    int minRightBearing() const;

    int cmap() const;
    const char *name() const;

    bool canRender( const QChar *string,  int len );

    void setScale( double scale );
    double scale() const { return _scale; }
    Type type() const;

    Qt::HANDLE handle() const { return (Qt::HANDLE) _fs->fid; }

private:
    friend class QFontPrivate;
    XFontStruct *_fs;
    QByteArray _name;
    QTextCodec *_codec;
    float _scale; // needed for printing, to correctly scale font metrics for bitmap fonts
    int _cmap;
    short lbearing;
    short rbearing;
    enum XlfdTransformations {
	XlfdTrUnknown,
	XlfdTrSupported,
	XlfdTrUnsupported
    };
    XlfdTransformations xlfd_transformations;

    friend class QFontEngineLatinXLFD;
};

class QFontEngineLatinXLFD : public QFontEngine
{
public:
    QFontEngineLatinXLFD( XFontStruct *xfs, const char *name, int cmap );
    ~QFontEngineLatinXLFD();

    Error stringToCMap( const QChar *str,  int len, glyph_t *glyphs,
			advance_t *advances, int *nglyphs, bool mirrored ) const;

    void draw( QPainter *p, int x, int y, const QTextEngine *engine,
	       const QScriptItem *si, int textFlags );

    virtual glyph_metrics_t boundingBox( const glyph_t *glyphs,
					 const advance_t *advances,
					 const qoffset_t *offsets, int numGlyphs );
    glyph_metrics_t boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;
    int minLeftBearing() const;
    int minRightBearing() const;

    int cmap() const { return -1; } // ###
    const char *name() const;

    bool canRender( const QChar *string,  int len );

    void setScale( double scale );
    double scale() const { return _engines[0]->scale(); }
    Type type() const { return LatinXLFD; }

    Qt::HANDLE handle() const { return ((QFontEngineXLFD *) _engines[0])->handle(); }

private:
    void findEngine( const QChar &ch );

    QFontEngine **_engines;
    int _count;

    glyph_t   glyphIndices [0x200];
    advance_t glyphAdvances[0x200];
    glyph_t euroIndex;
    advance_t euroAdvance;
};

class QScriptItem;
class QTextEngine;

#ifndef QT_NO_XFTFREETYPE
class QOpenType
{
public:
    QOpenType( FT_Face face );
    ~QOpenType();

    bool supportsScript( unsigned int script );

    void applyGSUBFeature(unsigned int feature, bool *where = 0);
    void applyGPOSFeatures();


    void init(glyph_t *glyphs, GlyphAttributes *glyphAttributes, int num_glyphs,
	      unsigned short *logClusters, int len, int char_offset = 0);
    void appendTo(QTextEngine *engine, QScriptItem *si, bool doLogClusters = TRUE);

    const int *mapping(int &len);
    inline void setLength(int len) { str->length = len; }
    unsigned short *glyphs() { return str->string; }
private:
    bool loadTables( FT_ULong script);

    FT_Face face;
    TTO_GDEF gdef;
    TTO_GSUB gsub;
    TTO_GPOS gpos;
    FT_UShort script_index;
    FT_ULong current_script;
    bool hasGDef : 1;
    bool hasGSub : 1;
    bool hasGPos : 1;
    TTO_GSUB_String *str;
    TTO_GSUB_String *tmp;
    TTO_GPOS_Data *positions;
    GlyphAttributes *tmpAttributes;
    unsigned short *tmpLogClusters;
    int length;
    int orig_nglyphs;
};
#endif // QT_NO_XFTFREETYPE

#elif defined( Q_WS_MAC )

#include "qt_mac.h"
class QFontEngineMac : public QFontEngine
{
#if 0
    ATSFontMetrics *info;
#else
    FontInfo *info;
#endif
    short fnum;
    int psize;
    QMacFontInfo *internal_fi;
    friend class QGLContext;
    friend class QFontPrivate;
    friend class QMacSetFontInfo;

public:
    QFontEngineMac() : QFontEngine(), info(NULL), fnum(-1), internal_fi(NULL) { }

    Error stringToCMap( const QChar *str, int len, glyph_t *glyphs, advance_t *advances, int *nglyphs, bool mirrored ) const;

    void draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags );

    glyph_metrics_t boundingBox( const glyph_t *glyphs,
			       const advance_t *advances, const qoffset_t *offsets, int numGlyphs );
    glyph_metrics_t boundingBox( glyph_t glyph );

    int ascent() const { return (int)info->ascent; }
    int descent() const { return (int)info->descent; }
    int leading() const { return (int)info->leading; }
#if 0
    int maxCharWidth() const { return (int)info->maxAdvanceWidth; }
#else
    int maxCharWidth() const { return info->widMax; }
#endif

    const char *name() const { return "ATSUI"; }

    bool canRender( const QChar *string,  int len );

    Type type() const { return QFontEngine::Mac; }

    void calculateCost();

    enum { WIDTH=0x01, DRAW=0x02, EXISTS=0x04 };
    int doTextTask(const QChar *s, int pos, int use_len, int len, uchar task, int =-1, int y=-1,
		   QPaintDevice *dev=NULL, const QRegion *rgn=NULL) const;
};

#elif defined( Q_WS_WIN )

class QFontEngineWin : public QFontEngine
{
public:
    QFontEngineWin( const char *name, HDC, HFONT, bool, LOGFONT );

    Error stringToCMap( const QChar *str, int len, glyph_t *glyphs, advance_t *advances, int *nglyphs, bool mirrored ) const;

    void draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags );

    glyph_metrics_t boundingBox( const glyph_t *glyphs,
			       const advance_t *advances, const qoffset_t *offsets, int numGlyphs );
    glyph_metrics_t boundingBox( glyph_t glyph );

    int ascent() const;
    int descent() const;
    int leading() const;
    int maxCharWidth() const;
    int minLeftBearing() const;
    int minRightBearing() const;

    const char *name() const;

    bool canRender( const QChar *string,  int len );

    Type type() const;
};

#if 0
class QFontEngineUniscribe : public QFontEngineWin
{
public:
    void draw( QPainter *p, int x, int y, const QTextEngine *engine, const QScriptItem *si, int textFlags );
    bool canRender( const QChar *string,  int len );

    Type type() const;
};
#endif

#endif // Q_WS_WIN

#endif
