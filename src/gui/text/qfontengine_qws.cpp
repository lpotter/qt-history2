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

#include "qfontengine_p.h"
#include <private/qunicodetables_p.h>
#include <qwsdisplay_qws.h>
#include <qvarlengtharray.h>
#include <private/qpainter_p.h>
#include "qpaintengine_qws.h"
#include "qtextengine_p.h"
#include "qopentype_p.h"

FT_Library QFontEngineFT::ft_library = 0;

class QGlyph {
public:
    QGlyph() :
        advance(0),
        data(0) {}
    ~QGlyph() {}

    Q_UINT8 pitch;
    Q_UINT8 width;
    Q_UINT8 height;

    Q_INT8 bearingx;      // Difference from pen position to glyph's left bbox
    Q_UINT8 advance;       // Difference between pen positions
    Q_INT8 bearingy;      // Used for putting characters on baseline

    bool mono :1;
    uint reserved:15;
    uchar* data;
};


static void render(FT_Face face, glyph_t index, QGlyph *result, bool smooth)
{
    FT_Error err;

    err=FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
    if (err) {
        qDebug("failed loading glyph %d from font", index);
        Q_ASSERT(!err);
    }

    if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL;
        if (!smooth)
            render_mode = FT_RENDER_MODE_MONO;
        err = FT_Render_Glyph(face->glyph, render_mode);
        if (err) {
            qDebug("failed rendering glyph %d from font", index);
            Q_ASSERT(!err);
        }
    }

    FT_Bitmap bm = face->glyph->bitmap;
    result->pitch = bm.pitch;
    result->width = bm.width;
    result->height = bm.rows;

    int size = bm.pitch*bm.rows;
    result->data = new uchar[size];
    result->mono = bm.pixel_mode == ft_pixel_mode_mono;
    if (size) {
        memcpy(result->data, bm.buffer, size);
    } else {
        result->data = 0;
    }
    result->bearingx = face->glyph->metrics.horiBearingX/64;
    result->advance = face->glyph->metrics.horiAdvance/64;
    result->bearingy = face->glyph->metrics.horiBearingY/64;
}


FT_Face QFontEngineFT::handle() const
{
    return face;
}

QFontEngineFT::QFontEngineFT(const QFontDef& d, const QPaintDevice *pd, FT_Face ft_face)
{
    _openType = 0;
    fontDef = d;
    face = ft_face;
////    _scale = pd ? (pd->resolution()<<8)/72 : 1<<8;
#warning "QPaintDevice::resolution() -- must find scale somehow"
    _scale =  1<<8; //###################

    smooth = FT_IS_SCALABLE(face);
    if (fontDef.styleStrategy & QFont::NoAntialias)
        smooth = false;
    rendered_glyphs = new QGlyph *[face->num_glyphs];
    memset(rendered_glyphs, 0, face->num_glyphs*sizeof(QGlyph *));
    cache_cost = face->num_glyphs*6*8; // ##########
}

QFontEngineFT::~QFontEngineFT()
{
    for (int i = 0; i < face->num_glyphs; ++i)
        delete rendered_glyphs[i];
    delete [] rendered_glyphs;
    FT_Done_Face(face);
}


QFontEngine::FECaps QFontEngineFT::capabilites() const
{
    return NoTransformations;
}


/* returns 0 as glyph index for non existant glyphs */
bool QFontEngineFT::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags flags) const
{
    if(*nglyphs < len) {
        *nglyphs = len;
        return false;
    }
    if (flags & QTextEngine::RightToLeft) {
        for(int i = 0; i < len; i++) {
            unsigned short ch = ::mirroredChar(str[i]).unicode();
            if (ch == 0xa0) ch = 0x20;
            glyphs[i].glyph = FT_Get_Char_Index(face, ch);
        }
    } else {
        for(int i = 0; i < len; i++) {
            unsigned short ch = str[i].unicode();
            if (ch == 0xa0) ch = 0x20;
            glyphs[i].glyph = FT_Get_Char_Index(face, ch);
        }
    }
    *nglyphs = len;
    for(int i = 0; i < len; i++) {
        int g = glyphs[i].glyph;
        if (!rendered_glyphs[g]) {
            Q_ASSERT(g < face->num_glyphs);
            rendered_glyphs[g] = new QGlyph;
            render(face, g, rendered_glyphs[g], smooth);
            if (::category(str[i]) == QChar::Mark_NonSpacing)
                rendered_glyphs[g]->advance = 0;
        }
        glyphs[i].advance.rx() = rendered_glyphs[g]->advance;
        glyphs[i].advance.ry() = 0;
    }
    return true;
}


void QFontEngineFT::draw(QPaintEngine *p, int x, int y, const QTextItem &si, int textFlags)
{
    Q_ASSERT(p->painterState()->txop < QPainterPrivate::TxScale);

    if (p->painterState()->txop == QPainterPrivate::TxTranslate)
        p->painterState()->painter->map(x, y, &x, &y);


    QWSPaintEngine *qpe = static_cast<QWSPaintEngine*>(p);

    if (textFlags) {
        int lw = qRound(lineThickness());
        lw = qMax(1, lw);

        p->updateBrush(p->painterState()->pen.color(), QPoint(0,0));

        if (textFlags & Qt::TextUnderline)
            qpe->fillRect(x, y+qRound(underlinePosition()), si.width, lw);
        if (textFlags & Qt::TextStrikeOut)
            qpe->fillRect(x, y-qRound(ascent())/3, si.width, lw);
        if (textFlags & Qt::TextOverline)
            qpe->fillRect(x, y-qRound(ascent())-1, si.width, lw);

        p->updateBrush(p->painterState()->brush, p->painterState()->bgOrigin);
    }

    QGlyphLayout *glyphs = si.glyphs;

#ifdef DEBUG_LOCKS
    qDebug("unaccelerated drawText grab");
#endif

#if !defined(QT_NO_QWS_MULTIPROCESS) && !defined(QT_PAINTER_LOCKING)
//######## verify that we really need this!!!
    QWSDisplay::grab(); // we need it later, and grab-must-precede-lock
#endif

#ifdef DEBUG_LOCKS
    qDebug("unaccelerated drawText lock");
#endif


    if (si.right_to_left)
        glyphs += si.num_glyphs - 1;
    for(int i = 0; i < si.num_glyphs; i++) {
        const QGlyphLayout *g = glyphs + (si.right_to_left ? -i : i);
        const QGlyph *glyph = rendered_glyphs[g->glyph];
        Q_ASSERT(glyph);
        int myw = glyph->width;
        int myx = x + qRound(g->offset.x() + glyph->bearingx);
        int myy = y + qRound(g->offset.y() - glyph->bearingy);

        if(glyph->width != 0 && glyph->height != 0 && glyph->pitch != 0)
            qpe->alphaPenBlt(glyph->data, glyph->pitch, glyph->mono, myx,myy,myw,glyph->height,0,0);

        x += qRound(g->advance.x());
    }
#ifdef DEBUG_LOCKS
    qDebug("unaccelerated drawText unlock");
#endif
#ifdef DEBUG_LOCKS
    qDebug("unaccelerated drawText ungrab");
#endif
#if !defined(QT_NO_QWS_MULTIPROCESS) && !defined(QT_PAINTER_LOCKING)
    QWSDisplay::ungrab();
#endif
}

glyph_metrics_t QFontEngineFT::boundingBox(const QGlyphLayout *glyphs, int numGlyphs)
{
    if (numGlyphs == 0)
        return glyph_metrics_t();

    float w = 0;
    const QGlyphLayout *end = glyphs + numGlyphs;
    while(end > glyphs)
        w += (--end)->advance.x();
    w *= _scale;
    return glyph_metrics_t(0, -ascent(), w, ascent()+descent()+1, w, 0);
}

glyph_metrics_t QFontEngineFT::boundingBox(glyph_t glyph)
{
    const QGlyph *g = rendered_glyphs[glyph];
    Q_ASSERT(g);
    return glyph_metrics_t(g->bearingx*_scale, g->bearingy*_scale,
                            g->width*_scale, g->height*_scale,
                            g->advance*_scale, 0);
}

static void addCurve(QPainterPath *path, const QPointF &cp, const QPointF &endPoint,
                     int startOff, int nOff, FT_GlyphSlot g)
{
    int j;
    QPointF c0 = QPointF(g->outline.points[startOff-1].x/64., -g->outline.points[startOff-1].y/64.);
    QPointF current = QPointF(g->outline.points[startOff].x/64., -g->outline.points[startOff].y/64.);
    for(j = 1; j <= nOff; j++) {
        QPointF next = (j == nOff)
                       ? endPoint
                       : QPointF(g->outline.points[startOff + j].x/64., -g->outline.points[startOff + j].y/64.);
        QPointF c3 = (j == nOff) ? next : (next + current)/2;
        QPointF c1 = (2*current + c0)/3;
        QPointF c2 = (2*current + c3)/3;
//         qDebug("curveTo %f/%f %f/%f %f/%f", (cp + c1).x(),  (cp + c1).y(),
//                (cp + c2).x(),  (cp + c2).y(), (cp + c3).x(),  (cp + c3).y());
        path->curveTo(cp + c1, cp + c2, cp + c3);
        c0 = c3;
        current = next;
    }
}

void QFontEngineFT::addOutlineToPath(float x, float y, const QGlyphLayout *glyphs, int numGlyphs, QPainterPath *path)
{
    if (FT_IS_SCALABLE(face)) {
        QPointF point = QPointF(x, y);
        for (int i = 0; i < numGlyphs; i++) {
            FT_UInt glyph = glyphs[i].glyph;
            QPointF cp = point + glyphs[i].offset;
            point += glyphs[i].advance;

            FT_Load_Glyph(face, glyph, FT_LOAD_NO_HINTING|FT_LOAD_NO_BITMAP);

            FT_GlyphSlot g = face->glyph;
            if (g->format != FT_GLYPH_FORMAT_OUTLINE)
                continue;

            // convert the outline to a painter path
            int i = 0;
            for (int c = 0; c < g->outline.n_contours; ++c) {
                QPointF p = cp + QPointF(g->outline.points[i].x/64., -g->outline.points[i].y/64.);
                 //qDebug("contour: %d -- %d", i, g->outline.contours[c]);
                 //qDebug("first point at %f %f", p.x(), p.y());
                path->moveTo(p);

                int first = i;
                int startOff = 0;
                int nOff = 0;
                ++i;
                while (i <= g->outline.contours[c]) {
                    QPointF p = cp + QPointF(g->outline.points[i].x/64., -g->outline.points[i].y/64.);
//                     qDebug("     point at %f %f, on curve=%d", p.x(), p.y(), g->outline.tags[i] & 1);
                    if (!(g->outline.tags[i] & 1)) {
                        /* Off curve */
                        if (!startOff) {
                            startOff = i;
                            nOff = 1;
                        } else {
                            ++nOff;
                        }
                    } else {
                        /* On Curve */
                        if (startOff) {
                            // ###### fix 3rd order beziers
                            addCurve(path, cp, QPointF(g->outline.points[i].x/64., -g->outline.points[i].y/64.),
                                     startOff, nOff, g);
                            startOff = 0;
                        } else {
                            p = cp + QPointF(g->outline.points[i].x/64., -g->outline.points[i].y/64.);
                            path->lineTo(p);
                        }
                    }
                    ++i;
                }
                QPointF end(g->outline.points[first].x/64., -g->outline.points[first].y/64.);
                if (startOff)
                    addCurve(path, cp, end, startOff, nOff, g);
                else
                    path->lineTo(end + cp);
            }
        }
    }
}

bool QFontEngineFT::canRender(const QChar *string,  int len)
{
    while (len--)
        for(int i = 0; i < len; i++)
            if (!FT_Get_Char_Index(face, string[i].unicode()))
                return false;

    return true;
}

#define FLOOR(x)  ((x) & -64)
#define CEIL(x)   (((x)+63) & -64)
#define TRUNC(x)  ((x) >> 6)

float QFontEngineFT::ascent() const
{
    return face->size->metrics.ascender/64.;
}

float QFontEngineFT::descent() const
{
    return -face->size->metrics.descender/64.;
}

float QFontEngineFT::leading() const
{
    return (face->size->metrics.height
            - face->size->metrics.ascender /*ascent*/
            + face->size->metrics.descender)/64.;
}

float QFontEngineFT::maxCharWidth() const
{
    return face->size->metrics.max_advance/64.;
}

float QFontEngineFT::minLeftBearing() const
{
    return 0;
//     return (memorymanager->fontMinLeftBearing(handle())*_scale)>>8;
}

float QFontEngineFT::minRightBearing() const
{
    return 0;
//     return (memorymanager->fontMinRightBearing(handle())*_scale)>>8;
}

float QFontEngineFT::underlinePosition() const
{
    return FT_MulFix(face->underline_position, face->size->metrics.y_scale)/64.;
}

float QFontEngineFT::lineThickness() const
{
    return FT_MulFix(face->underline_thickness, face->size->metrics.y_scale)/64.;
}

QFontEngine::Type QFontEngineFT::type() const
{
    return Freetype;
}


QOpenType *QFontEngineFT::openType() const
{
//     qDebug("openTypeIface requested!");
    if (_openType)
        return _openType;

    if (!FT_IS_SFNT(face))
        return 0;

    QFontEngineFT *that = const_cast<QFontEngineFT *>(this);
    that->_openType = new QOpenType(that, that->face);
    return _openType;
}

void QFontEngineFT::recalcAdvances(int len, QGlyphLayout *glyphs, QTextEngine::ShaperFlags) const
{
    for (int i = 0; i < len; i++) {
        FT_UInt g = glyphs[i].glyph;
        if (!rendered_glyphs[g]) {
            rendered_glyphs[g] = new QGlyph;
            render(face, g, rendered_glyphs[g], smooth);
        }
        glyphs[i].advance.rx() = (rendered_glyphs[g]->advance);//*_scale)>>8;
        glyphs[i].advance.ry() = 0;
    }
}



// box font engine
QFontEngineBox::QFontEngineBox(int size) : _size(size)
{
    //qDebug("box font engine created!");
    cache_cost = 1;
}

QFontEngineBox::~QFontEngineBox()
{
}

QFontEngine::FECaps QFontEngineBox::capabilites() const
{
    return FullTransformations;
}

bool QFontEngineBox::stringToCMap(const QChar *, int len, QGlyphLayout *glyphs, int *nglyphs, QTextEngine::ShaperFlags) const
{
    if(*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    for(int i = 0; i < len; i++)
        glyphs[i].glyph = 0;
    *nglyphs = len;

    for(int i = 0; i < len; i++) {
        (glyphs++)->advance.rx() = _size;
        (glyphs++)->advance.ry() = 0.;
    }

    return true;
}

void QFontEngineBox::draw(QPaintEngine *p, int x, int y, const QTextItem &si, int textFlags)
{
    Q_UNUSED(p);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(si);
    Q_UNUSED(textFlags);
    //qDebug("QFontEngineBox::draw(%d, %d, numglyphs=%d", x, y, numGlyphs);
}

glyph_metrics_t QFontEngineBox::boundingBox(const QGlyphLayout *, int numGlyphs)
{
    glyph_metrics_t overall;
    overall.x = overall.y = 0;
    overall.width = _size*numGlyphs;
    overall.height = _size;
    overall.xoff = overall.width;
    overall.yoff = 0;
    return overall;
}

glyph_metrics_t QFontEngineBox::boundingBox(glyph_t)
{
    return glyph_metrics_t(0, _size, _size, _size, _size, 0);
}

float QFontEngineBox::ascent() const
{
    return _size;
}

float QFontEngineBox::descent() const
{
    return 0;
}

float QFontEngineBox::leading() const
{
    int l = qRound(_size * 0.15);
    return (l > 0) ? l : 1;
}

float QFontEngineBox::maxCharWidth() const
{
    return _size;
}

const char *QFontEngineBox::name() const
{
    return "null";
}

bool QFontEngineBox::canRender(const QChar *, int)
{
    return true;
}

QFontEngine::Type QFontEngineBox::type() const
{
    return Box;
}


float QFontEngine::lineThickness() const
{
    // ad hoc algorithm
    int score = fontDef.weight * fontDef.pixelSize;
    int lw = score / 700;

    // looks better with thicker line for small pointsizes
    if (lw < 2 && score >= 1050) lw = 2;
    if (lw == 0) lw = 1;

    return lw;
}

float QFontEngine::underlinePosition() const
{
    return ((lineThickness() * 2) + 3) / 6;
}


QFontEngine::~QFontEngine()
{
}
