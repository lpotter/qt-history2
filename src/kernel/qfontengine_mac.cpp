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

#include <private/qapplication_p.h>
#include <private/qfontengine_p.h>
#include <private/qpainter_p.h>
#include <private/qpainter_p.h>
#include <private/qtextengine_p.h>
#include <qbitmap.h>
#include <qpaintengine_mac.h>
#include <qglobal.h>
#include <qpaintdevicemetrics.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qstackarray.h>

#include <ApplicationServices/ApplicationServices.h>

//Externals
void qstring2pstring(QString s, Str255 str, TextEncoding encoding=0, int len=-1); //qglobal.cpp
QByteArray pstring2qstring(const unsigned char *c); //qglobal.cpp
unsigned char * p_str(const QString &); //qglobal.cpp
void unclippedBitBlt(QPaintDevice *dst, int dx, int dy,
		     const QPaintDevice *src, int sx, int sy, int sw, int sh,
		     Qt::RasterOp rop, bool imask, bool set_fore_colour); //qpaintdevice_mac.cpp

//Generic engine
QFontEngine::~QFontEngine()
{
}

int QFontEngine::lineThickness() const
{
  // ad hoc algorithm
  int score = fontDef.pixelSize * fontDef.weight;
  int lth = score / 700;

  // looks better with thicker line for small pointsizes
  if(lth < 2 && score >= 1050)
      lth = 2;
  else if(lth == 0)
      lth = 1;
  return lth;
}

int QFontEngine::underlinePosition() const
{
  int pos = ((lineThickness() * 2) + 3) / 6;
  return pos ? pos : 1;
}

struct QATSUStyle {
    ATSUStyle style;
    QColor rgb;
    int ascent, descent, leading, maxWidth; //slight cache
};

//Mac (ATSUI) engine
QFontEngineMac::QFontEngineMac() : QFontEngine(), fontref(0), internal_fi(0)
{
    memset(widthCache, '\0', widthCacheSize);
}

QFontEngineMac::~QFontEngineMac()
{
    if(internal_fi)
	delete internal_fi;
}

QFontEngine::Error
QFontEngineMac::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs,
			     bool /*mirrored*/) const
{
    if(*nglyphs < len) {
	*nglyphs = len;
	return OutOfMemory;
    }
    *nglyphs = len;
    for(int i = 0; i < len; i++) {
	glyphs[i].glyph = str[i].unicode();
	if(str[i].unicode() < widthCacheSize && widthCache[str[i].unicode()])
	    glyphs[i].advance = widthCache[str[i].unicode()];
	else
	    glyphs[i].advance = doTextTask(str+i, 0, 1, 1, WIDTH);
    }
    return NoError;
}

void
QFontEngineMac::draw(QPainter *p, int x, int y, const QGlyphFragment &si, int textFlags)
{
    int txop = p->d->txop;
    QWMatrix xmat = p->d->matrix;

    if(txop >= QPainter::TxScale) {
	int aw = si.width, ah = si.ascent + si.descent + 1;
	if(aw == 0 || ah == 0)
	    return;
	QWMatrix mat1 = xmat, mat2 = QPixmap::trueMatrix(mat1, aw, ah);
	QBitmap *wx_bm = 0;
	{
	    QBitmap bm(aw, ah, TRUE);	// create bitmap
	    QPainter paint;
	    paint.begin(&bm);		// draw text in bitmap
	    paint.setPen(Qt::color1);
	    draw(&paint, 0, si.ascent, si, textFlags);
	    paint.end();
	    wx_bm = new QBitmap(bm.xForm(mat2)); // transform bitmap
	    if(wx_bm->isNull()) {
		delete wx_bm;		// nothing to draw
		return;
	    }
	}

	QPixmap pm(wx_bm->width(), wx_bm->height());
	if(p->backgroundMode() != QPainter::OpaqueMode) {
	    QPainter paint(&pm);
	    paint.fillRect(0, 0, pm.width(), pm.height(), p->pen().color());
	    pm.setMask(*wx_bm);
	} else { //This is untested code, I need to find a test case, FIXME --Sam
	    pm = *wx_bm;
	    QBitmap bm(pm.width(), pm.height(), TRUE);
	    bm.fill(Qt::color1);
	    bm = bm.xForm(mat2);
	    pm.setMask(bm);
	}
	double nfx, nfy;
	mat1.map(x, y - si.ascent, &nfx, &nfy);
	double dx, dy;
	mat2.map(0, 0, &dx, &dy);     // compute position of bitmap
	unclippedBitBlt(p->device(), qRound(nfx-dx), qRound(nfy-dy), &pm, 0, 0, -1, -1, Qt::CopyROP, FALSE, FALSE );
	delete wx_bm;
	return;
    } else if(txop == QPainter::TxTranslate) {
	p->map(x, y, &x, &y);
    }

    if(p->d->engine && p->d->engine->type() == QPaintEngine::QuickDraw) {
	QQuickDrawPaintEngine *mgc = (QQuickDrawPaintEngine*)p->d->engine;
	mgc->updateState(mgc->state);
	mgc->setupQDPort(false, 0, 0);
	mgc->setupQDFont();
    }

    QGlyphLayout *glyphs = si.glyphs;
    if(p->backgroundMode() == Qt::OpaqueMode) {
	glyph_metrics_t br = boundingBox(glyphs, si.num_glyphs);
	p->fillRect(x+br.x, y+br.y, br.width, br.height, p->backgroundColor());
    }


    int w = 0;
    uchar task = DRAW;
    if(textFlags != 0)
	task |= WIDTH; //I need the width for these..
    if(si.analysis.bidiLevel % 2 ) {
	glyphs += si.num_glyphs;
	for(int i = 0; i < si.num_glyphs; i++) {
	    glyphs--;
	    w += doTextTask((QChar*)&glyphs->glyph, 0, 1, 1, task, x, y, p);
	    x += glyphs->advance;
	}
    } else {
	QStackArray<unsigned short> g(si.num_glyphs);
	for (int i = 0; i < si.num_glyphs; ++i)
	    g[i] = glyphs[i].glyph;
	w = doTextTask((QChar*)g.data(), 0, si.num_glyphs, si.num_glyphs, task, x, y, p);
    }
    if(w && textFlags != 0) {
	int lw = lineThickness();
	if(textFlags & Qt::Underline)
	    p->drawRect(x, y+underlinePosition(), (si.analysis.bidiLevel % 2) ? -w : w, lw);
	if(textFlags & Qt::Overline)
	    p->drawRect(x, y + (ascent() + 1), (si.analysis.bidiLevel % 2) ? -w : w, lw);
	if(textFlags & Qt::StrikeOut)
	    p->drawRect(x, y + (ascent() / 3), (si.analysis.bidiLevel % 2) ? -w : w, lw);
    }
}

glyph_metrics_t
QFontEngineMac::boundingBox(const QGlyphLayout *glyphs, int numGlyphs)
{
    int w = 0;
    const QGlyphLayout *end = glyphs + numGlyphs;
    while(end > glyphs)
	w += (--end)->advance;
    return glyph_metrics_t(0, -(ascent()), w, ascent()+descent()+1, w, 0);
}

glyph_metrics_t
QFontEngineMac::boundingBox(glyph_t glyph)
{
    int w = doTextTask((QChar*)&glyph, 0, 1, 1, WIDTH);
    return glyph_metrics_t(0, -(ascent()), w, ascent()+descent()+1, w, 0 );
}

bool
QFontEngineMac::canRender(const QChar *string,  int len)
{
    return doTextTask(string, 0, len, len, EXISTS);
}

void
QFontEngineMac::calculateCost()
{
    // ### don't know how to get the number of glyphs from the font,
    // ### so default to 1024
    cache_cost = (ascent() + descent() + 1) * maxCharWidth() * 1024;
}

//Create a cacheable ATSUStyle
QATSUStyle *QFontEngineMac::getFontStyle() const
{
    if(internal_fi)
	return internal_fi;

    const int arr_guess = 7;
    int arr = 0;
    ATSUAttributeTag tags[arr_guess];
    ByteCount valueSizes[arr_guess];
    ATSUAttributeValuePtr values[arr_guess];
    tags[arr] = kATSUSizeTag; //font size
    Fixed fsize = FixRatio(fontDef.pixelSize, 1);
    valueSizes[arr] = sizeof(fsize);
    values[arr] = &fsize;
    arr++;
    tags[arr] = kATSUFontTag;  //font
    ATSUFontID fond = fontref;
    valueSizes[arr] = sizeof(fond);
    values[arr] = &fond;
    arr++;
    tags[arr] = kATSUQDItalicTag;
    valueSizes[arr] = sizeof(Boolean);
    Boolean italicBool = fontDef.italic ? true : false;
    values[arr] = &italicBool;
    arr++;
    tags[arr] = kATSUQDBoldfaceTag;
    valueSizes[arr] = sizeof(Boolean);
    Boolean boldBool = ((fontDef.weight == QFont::Bold) ? true : false);
    values[arr] = &boldBool;
    arr++;
    if(arr > arr_guess) //this won't really happen, just so I will not miss the case
	qWarning("Qt: internal: %d, WH0A %d: arr_guess overflow", __LINE__, arr);

    //create style
    QATSUStyle *ret = new QATSUStyle;
    ret->ascent = ret->descent = ret->leading = ret->maxWidth = -1;
    ATSUCreateStyle(&ret->style);
    if(OSStatus e = ATSUSetAttributes(ret->style, arr, tags, valueSizes, values)) {
	qWarning("Qt: internal: %ld: unexpected condition reached %s:%d", e, __FILE__, __LINE__);
	delete ret;
	ret = NULL;
    } else {
	const int feat_guess=5;
	int feats=0;
	ATSUFontFeatureType feat_types[feat_guess];
	ATSUFontFeatureSelector feat_values[feat_guess];
	feat_types[feats] = kLigaturesType;
	feat_values[feats] = kRareLigaturesOffSelector;
	feats++;
	feat_types[feats] = kLigaturesType;
	feat_values[feats] = kCommonLigaturesOffSelector;
	feats++;
	if(feats > feat_guess) //this won't really happen, just so I will not miss the case
	    qWarning("Qt: internal: %d: WH0A feat_guess underflow %d", __LINE__, feats);
	if(OSStatus e = ATSUSetFontFeatures(ret->style, feats, feat_types, feat_values))
	    qWarning("Qt: internal: %ld: unexpected condition reached %s:%d", e, __FILE__, __LINE__);
    }
    internal_fi = ret; //cache it
    const_cast<QFontEngineMac*>(this)->calculateCost();
    return ret;
}

static inline int qt_mac_get_measurement(ATSUStyle style, ATSUAttributeTag tag)
{
    ATSUTextMeasurement ret=0;
    OSStatus result = ATSUGetAttribute(style, tag, sizeof(ret), &ret, 0);
    if(result != noErr && result != kATSUNotSetErr)
	qWarning("%s:%d -- This can't really happen!! %ld", __FILE__, __LINE__, result);
    return FixRound(ret);
}

int
QFontEngineMac::ascent() const
{
    QATSUStyle *st = getFontStyle();
    if(st->ascent != -1)
	return st->ascent;
    return st->ascent = qt_mac_get_measurement(st->style, kATSUAscentTag);
}
int
QFontEngineMac::descent() const
{
    QATSUStyle *st = getFontStyle();
    if(st->descent != -1)
	return st->descent;
    return st->descent = qt_mac_get_measurement(st->style, kATSUDescentTag);
}

int
QFontEngineMac::leading() const
{
    QATSUStyle *st = getFontStyle();
    if(st->leading != -1)
	return st->leading;
    return st->leading = qt_mac_get_measurement(st->style, kATSULeadingTag);
}

int
QFontEngineMac::maxCharWidth() const
{
    QATSUStyle *st = getFontStyle();
    if(st->maxWidth != -1)
	return st->maxWidth;
    {     // I hate doing this, but I don't see a better way just yet - so I'll just take the width of the captial m 'M'
	QChar ch = 'M';
	st->maxWidth = doTextTask(&ch, 0, 1, 1, WIDTH);
    }
    return st->maxWidth;
}


int
QFontEngineMac::doTextTask(const QChar *s, int pos, int use_len, int len, uchar task, int x, int y, QPainter *p) const
{
    QATSUStyle *st = getFontStyle();
    if(!st) //can't really happen, but just to be sure..
	return 0;

    int ret = 0;
    if(task & DRAW) {
	Q_ASSERT(p); //really need a p to do any drawing!!!

	if(p->device()->devType() == QInternal::Widget) { //offset correctly..
	    QPoint pos = posInWindow((QWidget*)p->device());
	    x += pos.x();
	    y += pos.y();
	}

	QColor rgb = p->pen().color();
	if(rgb != st->rgb) {
	    st->rgb = rgb;
	    const ATSUAttributeTag tag = kATSUColorTag;
	    ::RGBColor fcolor;
	    fcolor.red = rgb.red()*256;
	    fcolor.green = rgb.green()*256;
	    fcolor.blue = rgb.blue()*256;
	    ByteCount size = sizeof(fcolor);
	    ATSUAttributeValuePtr value = &fcolor;
	    if(OSStatus e = ATSUSetAttributes(st->style, 1, &tag, &size, &value)) {
		qWarning("Qt: internal: %ld: This shouldn't happen %s:%d", e, __FILE__, __LINE__);
		return 0;
	    }
	}
    }

    {
	CGAffineTransform tf = CGAffineTransformIdentity;
#if QT_MACOSX_VERSION >= 0x1020
	if(fontDef.stretch != 100)
	    tf = CGAffineTransformScale(tf, fontDef.stretch/100, 1);
#endif
#ifdef USE_CORE_GRAPHICS
	if(task & DRAW) { //we need to flip the translation here because we flip it internally
	    int height = 0;
	    if(p->device()->devType() == QInternal::Widget)
		height = ((QWidget*)p->device())->topLevelWidget()->height();
	    else
		height = p->device()->metric(QPaintDeviceMetrics::PdmHeight);
	    tf = CGAffineTransformTranslate(CGAffineTransformIdentity, 0, height);
	    tf = CGAffineTransformScale(tf, 1, -1);
	}
#endif
	const ATSUAttributeTag tag = kATSUFontMatrixTag;
	ByteCount size = sizeof(tf);
	ATSUAttributeValuePtr value = &tf;
	if(OSStatus e = ATSUSetAttributes(st->style, 1, &tag, &size, &value)) {
	    qWarning("Qt: internal: %ld: This shouldn't happen %s:%d", e, __FILE__, __LINE__);
	    return 0;
	}
    }

    if((task & WIDTH)) {
 	bool use_cached_width = TRUE;
 	for(int i = 0; i < use_len; i++) {
 	    if(s[i].unicode() >= widthCacheSize || !widthCache[s[i].unicode()]) {
 		use_cached_width = FALSE;
 		break;
 	    }
 	    ret += widthCache[s[i].unicode()];
 	}
 	if(use_cached_width && task == WIDTH)
	    return ret;
    }

    //create layout
    ATSUTextLayout alayout;
    const UniCharCount count = use_len;
    Q_UNUSED(len);
    if(OSStatus e = ATSUCreateTextLayoutWithTextPtr((UniChar *)(s)+pos, 0,
						    count, use_len, 1, &count,
						    &st->style, &alayout)) {
	qWarning("Qt: internal: %ld: Unexpected condition reached %s:%d", e, __FILE__, __LINE__);
	return 0;
    }
    const int arr_guess = 5;
    int arr = 0;
    ATSUAttributeTag tags[arr_guess];
    ByteCount valueSizes[arr_guess];
    ATSUAttributeValuePtr values[arr_guess];
    tags[arr] = kATSULineLayoutOptionsTag;
    ATSLineLayoutOptions layopts = kATSLineHasNoOpticalAlignment | kATSLineIgnoreFontLeading | kATSLineFractDisable;
    if(fontDef.styleStrategy & QFont::NoAntialias)
	layopts |= kATSLineNoAntiAliasing;

#if QT_MACOSX_VERSION >= 0x1020
    if(qMacVersion() == Qt::MV_10_DOT_1)
	layopts |= kATSLineIsDisplayOnly;
    else
	layopts |= kATSLineDisableAutoAdjustDisplayPos | kATSLineDisableAllLayoutOperations |
		   kATSLineUseDeviceMetrics;
#else
    layopts |= kATSLineIsDisplayOnly;
#endif
    valueSizes[arr] = sizeof(layopts);
    values[arr] = &layopts;
    arr++;

    tags[arr] = kATSUCGContextTag;
    CGContextRef ctx = NULL;
#ifdef USE_CORE_GRAPHICS
    if(p && p->device()) {
	ctx = (CGContextRef)p->handle();
    } else {
	static QPixmap *pixmap = NULL;
	if(!pixmap)
	    pixmap = new QPixmap(1, 1, 32);
	ctx = (CGContextRef)pixmap->macCGHandle();
    }
#else
    CGrafPtr port = NULL;
    if(p && p->device()) {
	if(p->device()->devType() == QInternal::Widget)
	    port = GetWindowPort((WindowPtr)p->device()->handle());
	else
	    port = (CGrafPtr)p->device()->handle();
    } else {
	static QPixmap *pixmap = NULL;
	if(!pixmap)
	    pixmap = new QPixmap(1, 1, 32);
	port = (CGrafPtr)pixmap->handle();
    }
    if(OSStatus err = QDBeginCGContext(port, &ctx)) {
	qWarning("Qt: internal: WH0A, QDBeginCGContext(%ld) failed. %s:%d", err, __FILE__, __LINE__);
	ATSUDisposeTextLayout(alayout);
	return 0;
    }

    RgnHandle rgnh = NULL;
    if(p && p->d->engine && p->d->engine->type() == QPaintEngine::QuickDraw) {
	QRegion rgn;
	QQuickDrawPaintEngine *mgc = (QQuickDrawPaintEngine*)p->d->engine;
	mgc->setupQDPort(false, 0, &rgn);
	if(!rgn.isEmpty())
	    rgnh = rgn.handle(TRUE);
    }
    if(rgnh) {
	Rect clipr;
	GetPortBounds(port, &clipr);
	ClipCGContextToRegion(ctx, &clipr, rgnh);
    }
#endif
    valueSizes[arr] = sizeof(ctx);
    values[arr] = &ctx;
    arr++;

    if(arr > arr_guess) //this won't really happen, just so I will not miss the case
	qWarning("Qt: internal: %d: WH0A, arr_guess underflow %d", __LINE__, arr);
    if(OSStatus e = ATSUSetLayoutControls(alayout, arr, tags, valueSizes, values)) {
	qWarning("Qt: internal: %ld: Unexpected condition reached %s:%d", e, __FILE__, __LINE__);
	ATSUDisposeTextLayout(alayout);
#ifndef USE_CORE_GRAPHICS
	QDEndCGContext(port, &ctx);
#endif
	return 0;
    }

    ATSUSetTransientFontMatching(alayout, true);
    //do required task now
    if(task & EXISTS) {
	if(task != EXISTS)
	    qWarning("Qt: EXISTS must appear by itself!");
	ATSUFontID fid;
	UniCharArrayOffset off;
	UniCharCount off_len;
	if(ATSUMatchFontsToText(alayout, kATSUFromTextBeginning, kATSUToTextEnd, &fid, &off, &off_len) != kATSUFontsNotMatched)
	    ret = 1;
    } else if((task & WIDTH) && !ret) {
	ATSUTextMeasurement left, right, bottom, top;
#if QT_MACOSX_VERSION >= 0x1020
	if(qMacVersion() >= Qt::MV_10_DOT_2)
	    ATSUGetUnjustifiedBounds(alayout, kATSUFromTextBeginning, kATSUToTextEnd,
				     &left, &right, &bottom, &top);
	else
#endif
	    ATSUMeasureText(alayout, kATSUFromTextBeginning, kATSUToTextEnd,
			    &left, &right, &bottom, &top);
	ret = FixRound(right-left);
 	if(use_len == 1 && s->unicode() < widthCacheSize)
 	    widthCache[s->unicode()] = ret;
    }
    if(task & DRAW) {
	int drawy = y;
#ifndef USE_CORE_GRAPHICS
	int height = 0;
	if(p->device()->devType() == QInternal::Widget)
	    height = ((QWidget*)p->device())->topLevelWidget()->height();
	else
	    height = p->device()->metric(QPaintDeviceMetrics::PdmHeight);
	drawy = height-drawy;
#endif
	ATSUDrawText(alayout, kATSUFromTextBeginning, kATSUToTextEnd, FixRatio(x, 1), FixRatio(drawy, 1));
    }
    //cleanup
    ATSUDisposeTextLayout(alayout);
#ifndef USE_CORE_GRAPHICS
    QDEndCGContext(port, &ctx);
#endif
    return ret;
}



