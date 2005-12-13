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

#include "qbrush.h"
#include "qpixmap.h"
#include "qbitmap.h"
#include "qpixmapcache.h"
#include "qdatastream.h"
#include "qvariant.h"
#include "qline.h"
#include "qdebug.h"

const uchar *qt_patternForBrush(int brushStyle, bool invert)
{
    Q_ASSERT(brushStyle > Qt::SolidPattern && brushStyle < Qt::LinearGradientPattern);
    if(invert) {
        static const uchar dense1_pat[] = { 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff };
        static const uchar dense2_pat[] = { 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd, 0xff };
        static const uchar dense3_pat[] = { 0x55, 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55, 0xee };
        static const uchar dense4_pat[] = { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 };
        static const uchar dense5_pat[] = { 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11 };
        static const uchar dense6_pat[] = { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 };
        static const uchar dense7_pat[] = { 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00 };
        static const uchar hor_pat[]    = { 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 };
        static const uchar ver_pat[]    = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };
        static const uchar cross_pat[]  = { 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x10, 0x10 };
        static const uchar bdiag_pat[]  = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
        static const uchar fdiag_pat[]  = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
        static const uchar dcross_pat[] = { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 };
        static const uchar *const pat_tbl[] = {
            dense1_pat, dense2_pat, dense3_pat, dense4_pat, dense5_pat,
            dense6_pat, dense7_pat,
            hor_pat, ver_pat, cross_pat, bdiag_pat, fdiag_pat, dcross_pat };
        return pat_tbl[brushStyle - Qt::Dense1Pattern];
    }
    static const uchar dense1_pat[] = { 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00 };
    static const uchar dense2_pat[] = { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 };
    static const uchar dense3_pat[] = { 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11 };
    static const uchar dense4_pat[] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
    static const uchar dense5_pat[] = { 0x55, 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55, 0xee };
    static const uchar dense6_pat[] = { 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd, 0xff };
    static const uchar dense7_pat[] = { 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff };
    static const uchar hor_pat[]    = { 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff };
    static const uchar ver_pat[]    = { 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef };
    static const uchar cross_pat[]  = { 0xef, 0xef, 0xef, 0x00, 0xef, 0xef, 0xef, 0xef };
    static const uchar bdiag_pat[]  = { 0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe };
    static const uchar fdiag_pat[]  = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };
    static const uchar dcross_pat[] = { 0x7e, 0xbd, 0xdb, 0xe7, 0xe7, 0xdb, 0xbd, 0x7e };
    static const uchar *const pat_tbl[] = {
        dense1_pat, dense2_pat, dense3_pat, dense4_pat, dense5_pat,
        dense6_pat, dense7_pat,
        hor_pat, ver_pat, cross_pat, bdiag_pat, fdiag_pat, dcross_pat };
    return pat_tbl[brushStyle - Qt::Dense1Pattern];
}

QPixmap qt_pixmapForBrush(int brushStyle, bool invert)
{
    QPixmap pm;
    QString key = "$qt-brush$" + QString::number(brushStyle) + QString::number((int)invert);
    if (!QPixmapCache::find(key, pm)) {
        pm = QBitmap::fromData(QSize(8, 8), qt_patternForBrush(brushStyle, invert),
                               QImage::Format_MonoLSB);
        QPixmapCache::insert(key, pm);
    }

    return pm;
}


struct QTexturedBrushData : public QBrushData
{
    QPixmap pixmap;
};

struct QGradientBrushData : public QBrushData
{
    QGradient gradient;
};


/*!
    \class QBrush

    \brief The QBrush class defines the fill pattern of shapes drawn
    by QPainter.

    \ingroup multimedia
    \ingroup shared

    A brush has a style, a color, a gradient and a texture.

    The brush style() defines the fill pattern using the
    Qt::BrushStyle enum. The default brush style is Qt::NoBrush
    (depending on how you construct a brush). This style tells the
    painter to not fill shapes. The standard style for filling is
    Qt::SolidPattern. The style can be set when the brush is created
    using the appropiate constructor, and in addition the setStyle()
    function provides means for altering the style once the brush is
    constructed.

    \image brush-styles.png Brush Styles

    The brush color() defines the color of the fill pattern. The color
    can either be one of Qt's predefined colors, Qt::GlobalColor, or
    any other custom QColor. The currently set color can be retrieved
    and altered using the color() and setColor() functions,
    respectively.

    The gradient() defines the gradient fill used when the current
    style is either Qt::LinearGradientPattern,
    Qt::RadialGradientPattern or Qt::ConicalGradient Pattern. The
    gradient can only be set when constructing the brush, while the
    texture() can be set using the appropiate constructor or by using
    the setTexture() function. The texture() defines the pixmap used
    when the current style is Qt::TexturePattern.

    Note that applying setTexture() makes style() ==
    Qt::TexturePattern, independently of previous style
    settings. Also, calling setColor() will not make a difference if
    the style is a gradient. The same is the case if the style is
    Qt::TexturePattern style unless the current texture is a QBitmap.

    The isOpaque() function returns true if the brush is fully opaque
    otherwise false. A brush is considered opaque if:

    \list
    \o The alpha component of the color() is 255.
    \o Its texture() does not an alpha channel and is not a QBitmap.
    \o The colors in the gradient() all have an alpha component that is 255.
    \endlist

    \table 100%
    \row
    \o \inlineimage brush-outline.png Outlines
    \o

    To specify the style and color of lines and outlines, use the
    QPainter's \l {QPen}{pen} combined with Qt::PenStyle and
    Qt::GlobalColor:

    \code
        QPainter painter(this);

        painter.setBrush(Qt::cyan);
        painter.setPen(Qt::darkCyan);
        painter.drawRect(0, 0, 100,100);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::darkGreen);
        painter.drawRect(40, 40, 100, 100);
    \endcode

    Note that, by default, QPainter renders the outline (using the
    currently set pen) when drawing shapes. Use \l {Qt::NoPen}{\c
    painter.setPen(Qt::NoPen)} to disable this behavior.

    \endtable

    \sa Qt::BrushStyle, QPainter, QColor
*/

class QBrushStatic
{
public:
    QBrushData *pointer;
    bool destroyed;

    inline QBrushStatic()
        : pointer(0), destroyed(false)
    { }

    inline ~QBrushStatic()
    {
        if (!pointer->ref.deref())
            delete pointer;
        pointer = 0;
        destroyed = true;
    }
};

static QBrushData *nullBrushInstance()
{
    static QBrushStatic defaultBrush;
    if (!defaultBrush.pointer && !defaultBrush.destroyed) {
        QBrushData *x = new QBrushData;
        x->ref = 1; x->style = Qt::BrushStyle(0); x->color = Qt::black;
        if (!q_atomic_test_and_set_ptr(&defaultBrush.pointer, 0, x))
            delete x;
    }
    return defaultBrush.pointer;
}

/*!
  \internal
  Initializes the brush.
*/

void QBrush::init(const QColor &color, Qt::BrushStyle style)
{
    switch(style) {
    case Qt::NoBrush:
        d = nullBrushInstance();
        d->ref.ref();
        return;
    case Qt::TexturePattern:
        d = new QTexturedBrushData;
        static_cast<QTexturedBrushData *>(d)->pixmap = QPixmap();
        break;
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
        d = new QGradientBrushData;
        break;
    default:
        d = new QBrushData;
        break;
    }
    d->ref = 1;
    d->style = style;
    d->color = color;
}

/*!
    Constructs a default black brush with the style Qt::NoBrush
    (i.e. this brush will not fill shapes).
*/

QBrush::QBrush()
{
    d = nullBrushInstance();
    Q_ASSERT(d);
    d->ref.ref();
}

/*!
    Constructs a brush with a black color and a texture set to the
    given \a pixmap. The style is set to Qt::TexturePattern.

    \sa setTexture()
*/

QBrush::QBrush(const QPixmap &pixmap)
{
// ## if pixmap was image, we could pick a nice color rather than
// assuming black.
    init(Qt::black, Qt::TexturePattern);
    setTexture(pixmap);
}

/*!
    Constructs a black brush with the given \a style.

    \sa setStyle()
*/

QBrush::QBrush(Qt::BrushStyle style)
{
    init(Qt::black, style);
}

/*!
    Constructs a brush with the given \a color and \a style.

    \sa setColor(), setStyle()
*/

QBrush::QBrush(const QColor &color, Qt::BrushStyle style)
{
    init(color, style);
}

/*!
    \fn QBrush::QBrush(Qt::GlobalColor color, Qt::BrushStyle style)

    Constructs a brush with the given \a color and \a style.

    \sa setColor(), setStyle()
*/
QBrush::QBrush(Qt::GlobalColor color, Qt::BrushStyle style)
{
    init(color, style);
}

/*!
    Constructs a brush with the given \a color and the custom pattern
    stored in \a pixmap.

    The style is set to Qt::TexturePattern. The color will only have
    an effect for QBitmaps.

    \sa setColor(), setPixmap()
*/

QBrush::QBrush(const QColor &color, const QPixmap &pixmap)
{
    init(color, Qt::TexturePattern);
    setTexture(pixmap);
}

/*!

    Constructs a brush with the given \a color and the custom pattern
    stored in \a pixmap.

    The style is set to Qt::TexturePattern. The color will only have
    an effect for QBitmaps.

    \sa setColor(), setPixmap()
*/
QBrush::QBrush(Qt::GlobalColor color, const QPixmap &pixmap)
{
    init(color, Qt::TexturePattern);
    setTexture(pixmap);
}

/*!
    Constructs a copy of \a other.
*/

QBrush::QBrush(const QBrush &other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Constructs a brush based on the given \a gradient.

    The brush style is set to the corresponding gradient style (either
    Qt::LinearGradientPattern, Qt::RadialGradientPattern or
    Qt::ConicalGradientPattern).
*/
QBrush::QBrush(const QGradient &gradient)
{
    Q_ASSERT_X(gradient.type() != QGradient::NoGradient, "QBrush::QBrush",
               "QGradient should not be used directly, use the linear, radial\n"
               "or conical gradients instead");

    const Qt::BrushStyle enum_table[] = {
        Qt::LinearGradientPattern,
        Qt::RadialGradientPattern,
        Qt::ConicalGradientPattern
    };

    init(QColor(), enum_table[gradient.type()]);
    QGradientBrushData *grad = static_cast<QGradientBrushData *>(d);
    grad->gradient = gradient;
}

/*!
    Destroys the brush.
*/

QBrush::~QBrush()
{
    if (!d->ref.deref())
        cleanUp(d);
}

void QBrush::cleanUp(QBrushData *x)
{
    switch (x->style) {
    case Qt::TexturePattern:
        delete static_cast<QTexturedBrushData*>(x);
        break;
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
        delete static_cast<QGradientBrushData*>(x);
        break;
    default:
        delete x;
    }
}


void QBrush::detach(Qt::BrushStyle newStyle)
{
    if (newStyle == d->style && d->ref == 1)
        return;

    QBrushData *x;
    switch(newStyle) {
    case Qt::TexturePattern:
        x = new QTexturedBrushData;
        static_cast<QTexturedBrushData*>(x)->pixmap =
            d->style == Qt::TexturePattern ? static_cast<QTexturedBrushData *>(d)->pixmap : QPixmap();
        break;
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
        x = new QGradientBrushData;
        static_cast<QGradientBrushData *>(x)->gradient =
            static_cast<QGradientBrushData *>(d)->gradient;
        break;
    default:
        x = new QBrushData;
        break;
    }
    x->ref = 1;
    x->style = newStyle;
    x->color = d->color;
    x = qAtomicSetPtr(&d, x);
    if (!x->ref.deref())
        cleanUp(x);
}


/*!
    \fn QBrush &QBrush::operator=(const QBrush &brush)

    Assigns the given \a brush to \e this brush and returns a
    reference to \e this brush.
*/

QBrush &QBrush::operator=(const QBrush &b)
{
    QBrushData *x = b.d;
    x->ref.ref();
    x = qAtomicSetPtr(&d, x);
    if (!x->ref.deref())
        cleanUp(x);
    return *this;
}

/*!
   Returns the brush as a QVariant
*/
QBrush::operator QVariant() const
{
    return QVariant(QVariant::Brush, this);
}

/*!
    \fn Qt::BrushStyle QBrush::style() const

    Returns the brush style.

    \sa setStyle()
*/

/*!
    Sets the brush style to \a style.

    \sa style()
*/

void QBrush::setStyle(Qt::BrushStyle style)
{
    if (d->style == style)
        return;
    if (style == Qt::TexturePattern)
        qWarning("QBrush::setStyle: TexturePattern is for internal use");
    detach(style);
    d->style = style;
}


/*!
    \fn const QColor &QBrush::color() const

    Returns the brush color.

    \sa setColor()
*/

/*!
    \fn void QBrush::setColor(const QColor &color)

    Sets the brush color to the given \a color.

    Note that calling setColor() will not make a difference if the
    style is a gradient. The same is the case if the style is
    Qt::TexturePattern style unless the current texture is a QBitmap.

    \sa color()
*/

void QBrush::setColor(const QColor &c)
{
    detach(d->style);
    d->color = c;
}

/*!
    \fn void QBrush::setColor(Qt::GlobalColor color)
    \overload

    Sets the brush color to the given \a color.
*/


#ifdef QT3_SUPPORT

/*!
    \fn void QBrush::setPixmap(const QPixmap &pixmap)

    \compat

    Sets a custom pattern for this brush.

    Use setTexture() instead.
*/

/*!
    \fn QPixmap *QBrush::pixmap() const

    Returns a pointer to the custom brush pattern.

    Use texture() instead.
*/
QPixmap *QBrush::pixmap() const
{
    if (d->style != Qt::TexturePattern)
        return 0;
    QTexturedBrushData *data  = static_cast<QTexturedBrushData*>(d);
    return data->pixmap.isNull() ? 0 : &data->pixmap;
}
#endif

/*!
    \fn QPixmap QBrush::texture() const

    Returns the custom brush pattern, or a null pixmap if no custom brush pattern
    has been set.

    \sa setTexture()
*/
QPixmap QBrush::texture() const
{
    return d->style == Qt::TexturePattern
                     ? static_cast<const QTexturedBrushData*>(d)->pixmap : QPixmap();
}

/*!
    Sets the brush pixmap to \a pixmap. The style is set to
    Qt::TexturePattern.

    The current brush color will only have an effect for monochrome
    pixmaps, i.e. for QPixmap::depth() == 1 (\l {QBitmap}{QBitmaps}).

    \sa texture()
*/

void QBrush::setTexture(const QPixmap &pixmap)
{
    if (!pixmap.isNull()) {
        detach(Qt::TexturePattern);
        QTexturedBrushData *data = static_cast<QTexturedBrushData *>(d);
        data->pixmap = pixmap;
    } else {
        detach(Qt::NoBrush);
    }
}


/*!
    Returns the gradient describing this brush.
*/
const QGradient *QBrush::gradient() const
{
    if (d->style == Qt::LinearGradientPattern
        || d->style == Qt::RadialGradientPattern
        || d->style == Qt::ConicalGradientPattern) {
        return &static_cast<const QGradientBrushData *>(d)->gradient;
    }
    return 0;
}


/*!
    Returns true if the brush is fully opaque otherwise false. A brush
    is considered opaque if:

    \list
    \i The alpha component of the color() is 255.
    \i Its texture() does not an alpha channel and is not a QBitmap.
    \i The colors in the gradient() all have an alpha component that is 255.
    \endlist
*/

bool QBrush::isOpaque() const
{
    bool opaqueColor = d->color.alpha() == 255;

    // Test awfully simple case first
    if (d->style == Qt::SolidPattern)
        return opaqueColor;

    if (d->style == Qt::LinearGradientPattern
        || d->style == Qt::RadialGradientPattern
        || d->style == Qt::ConicalGradientPattern) {
        QGradientStops stops = gradient()->stops();
        for (int i=0; i<stops.size(); ++i)
            if (stops.at(i).second.alpha() != 255)
                return false;
        return true;
    } else if (d->style == Qt::TexturePattern) {
        return !texture().hasAlpha();
    }

    return opaqueColor;
}


/*!
    \fn bool QBrush::operator!=(const QBrush &brush) const

    Returns true if the brush is different from the given \a brush;
    otherwise returns false.

    Two brushes are different if they have different styles, colors or
    pixmaps.

    \sa operator==()
*/

/*!
    \fn bool QBrush::operator==(const QBrush &brush) const

    Returns true if the brush is equal to the given \a brush;
    otherwise returns false.

    Two brushes are equal if they have equal styles, colors and
    pixmaps.

    \sa operator!=()
*/

bool QBrush::operator==(const QBrush &b) const
{
    if (b.d == d)
        return true;
    if (b.d->style == d->style && b.d->color == d->color) {
        switch (d->style) {
        case Qt::TexturePattern: {
            QPixmap us = static_cast<QTexturedBrushData *>(d)->pixmap;
            QPixmap them = static_cast<QTexturedBrushData *>(b.d)->pixmap;
            return ((us.isNull() && them.isNull()) || us.serialNumber() == them.serialNumber());
        }
        case Qt::LinearGradientPattern:
        case Qt::RadialGradientPattern:
        case Qt::ConicalGradientPattern:
            {
                QGradientBrushData *d1 = static_cast<QGradientBrushData *>(d);
                QGradientBrushData *d2 = static_cast<QGradientBrushData *>(b.d);
                return d1->gradient == d2->gradient;
            }
        default:
            return true;
        }
    }
    return false;
}

/*!
    \fn QBrush::operator const QColor&() const

    Returns the brush's color.

    Use color() instead.
*/

#ifndef QT_NO_DEBUG_STREAM
/*!
  \internal
*/
QDebug operator<<(QDebug dbg, const QBrush &b)
{
#ifndef Q_BROKEN_DEBUG_STREAM
    dbg.nospace() << "QBrush(" << b.color() << ',' << b.style() << ')';
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QBrush to QDebug");
    return dbg;
    Q_UNUSED(b);
#endif
}
#endif

/*****************************************************************************
  QBrush stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QBrush &brush)
    \relates QBrush

    Writes the given \a brush to the given \a stream and returns a
    reference to the \a stream.

    \sa {Format of the QDataStream Operators}
*/

QDataStream &operator<<(QDataStream &s, const QBrush &b)
{
    s << (quint8)b.style() << b.color();
    if (b.style() == Qt::TexturePattern) {
        s << b.texture();
    } else if (b.style() == Qt::LinearGradientPattern
               || b.style() == Qt::RadialGradientPattern
               || b.style() == Qt::ConicalGradientPattern) {
        const QGradient *gradient = b.gradient();
        s << gradient->type();
        s << gradient->stops();

        if (gradient->type() == QGradient::LinearGradient) {
            s << static_cast<const QLinearGradient *>(gradient)->start();
            s << static_cast<const QLinearGradient *>(gradient)->finalStop();
        } else if (gradient->type() == QGradient::RadialGradient) {
            s << static_cast<const QRadialGradient *>(gradient)->center();
            s << static_cast<const QRadialGradient *>(gradient)->focalPoint();
            s << static_cast<const QRadialGradient *>(gradient)->radius();
        } else { // type == Conical
            s << static_cast<const QConicalGradient *>(gradient)->center();
            s << static_cast<const QConicalGradient *>(gradient)->angle();
        }
    }
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QBrush &brush)
    \relates QBrush

    Reads the given \a brush from the given \a stream and returns a
    reference to the \a stream.

    \sa {Format of the QDataStream Operators}
*/

QDataStream &operator>>(QDataStream &s, QBrush &b)
{
    quint8 style;
    QColor color;
    s >> style;
    s >> color;
    if (style == Qt::TexturePattern) {
        QPixmap pm;
        s >> pm;
        b = QBrush(color, pm);
    } else if (style == Qt::LinearGradientPattern
               || style == Qt::RadialGradientPattern
               || style == Qt::ConicalGradientPattern) {

        int type_as_int;
        QGradient::Type type;
        QGradientStops stops;

        s >> type_as_int;
        type = QGradient::Type(type_as_int);

        s >> stops;

        if (type == QGradient::LinearGradient) {
            QPointF p1, p2;
            s >> p1;
            s >> p2;
            QLinearGradient lg(p1, p2);
            lg.setStops(stops);
            b = QBrush(lg);
        } else if (type == QGradient::RadialGradient) {
            QPointF center, focal;
            double radius;
            s >> center;
            s >> focal;
            s >> radius;
            QRadialGradient rg(center, radius, focal);
            rg.setStops(stops);
            b = QBrush(rg);
        } else { // type == QGradient::ConicalGradient
            QPointF center;
            double angle;
            s >> center;
            s >> angle;
            QConicalGradient cg(center, angle);
            cg.setStops(stops);
            b = QBrush(cg);
        }

    } else {
        b = QBrush(color, (Qt::BrushStyle)style);
    }
    return s;
}
#endif // QT_NO_DATASTREAM

/*******************************************************************************
 * QGradient implementations
 */


/*!
    \class QGradient

    \brief The QGradient class is used in combination with QBrush to
    specify gradient fills.

    Qt currently supports three types of gradient fills: linear,
    radial and conical. Each of these is represented by a subclass of
    QGradient: QLinearGradient, QRadialGradient and QConicalGradient.

    The colors in a gradient is defined using stop points, which is a
    position and a color. The set of stop points describes how the
    gradient area should be filled. A diagonal linear gradient from
    black at (100, 100) to white at (200, 200) could be specified like
    this:

    \quotefromfile snippets/brush/brush.cpp
    \skipto LINEAR
    \skipto QLinearGradient
    \printuntil Qt::white

    A gradient can have an arbitrary number of stop points. The
    following would create a radial gradient starting with
    red in the center, blue and then green on the edges:

    \quotefromfile snippets/brush/brush.cpp
    \skipto RADIAL
    \skipto QRadialGradient
    \printuntil Qt::green

    It is possible to repeat or reflect the gradient outside the area
    by specifiying spread. The default is to pad the outside area with
    the color at the closest stop point.

    \sa QLinearGradient, QRadialGradient, QConicalGradient
*/

/*!
    \internal
*/
QGradient::QGradient()
{
}


/*!
    \enum QGradient::Type

    Specifies the type of gradient.

    \value LinearGradient The gradient is a linear gradient.
    \value RadialGradient The gradient is a radial gradient.
    \value ConicalGradient The gradient is a conical gradient.
    \value NoGradient No gradient is used.
*/

/*!
    \enum QGradient::Spread

    Specifies how the areas outside the gradient area should be
    filled.

    \value PadSpread The areas are filled with the closes stop
    color. This is the default.

    \value RepeatSpread The gradient repeats outside the gradient
    area.

    \value ReflectSpread The gradient is reflected outside the
    gradient area.
*/

/*!
    \fn void QGradient::setSpread(Spread method)

    Specifies the spread \a method that should be used for this
    gradient. This function only has effect for linear and
    radial gradients.
*/

/*!
    \fn QGradient::Spread QGradient::spread() const

    Returns the spread method use by this gradient. The default is
    PadSpread.
*/

/*!
    \fn QGradient::Type QGradient::type() const

    Returns the type of gradient.
*/

/*!
    Sets another stop point at the relative position \a pos with
    color \a color. The position \a pos must be in the range 0 to 1.
*/

void QGradient::setColorAt(qreal pos, const QColor &color)
{
    if (pos > 1 || pos < 0) {
        qWarning("QGradient::setColorAt(), colors positions must be specified in the range 0 to 1");
        return;
    }

    int index = 0;
    while (index < m_stops.size() && m_stops.at(index).first < pos) ++index;
    m_stops.insert(index, QGradientStop(pos, color));
}

/*!
    Replaces the current set of stop points with \a stops. The
    positions of the stop points must be in the range 0 to 1 and must be
    sorted with the lowest point first.
*/
void QGradient::setStops(const QGradientStops &stops)
{
    m_stops.clear();
    for (int i=0; i<stops.size(); ++i)
        setColorAt(stops.at(i).first, stops.at(i).second);
}


/*!
    Returns the stops for this gradient.

    If no stops have been specified a gradient of black at 0 to white
    at 1 is used.
*/
QGradientStops QGradient::stops() const
{
    if (m_stops.isEmpty()) {
        QGradientStops tmp;
        tmp << QGradientStop(0, Qt::black) << QGradientStop(1, Qt::white);
        return tmp;
    }
    return m_stops;
}


/*!
    \internal
*/
bool QGradient::operator==(const QGradient &gradient) const
{
    if (gradient.m_type != m_type || gradient.m_spread != m_spread) return false;

    if (m_type == LinearGradient) {
        if (m_data.linear.x1 != gradient.m_data.linear.x1
            || m_data.linear.y1 != gradient.m_data.linear.y1
            || m_data.linear.x2 != gradient.m_data.linear.x2
            || m_data.linear.y2 != gradient.m_data.linear.y2)
            return false;
    } else if (m_type == RadialGradient) {
        if (m_data.radial.cx != gradient.m_data.radial.cx
            || m_data.radial.cy != gradient.m_data.radial.cy
            || m_data.radial.fx != gradient.m_data.radial.fx
            || m_data.radial.fy != gradient.m_data.radial.fy
            || m_data.radial.radius != gradient.m_data.radial.radius)
            return false;
    } else { // m_type == ConicalGradient
        if (m_data.conical.cx != gradient.m_data.conical.cx
            || m_data.conical.cy != gradient.m_data.conical.cy
            || m_data.conical.angle != gradient.m_data.conical.angle)
            return false;
    }

    return stops() == gradient.stops();
}

/*!
    Returns true if the gradient is the same as the other \a gradient
    specified; otherwise returns false.
*/
bool QGradient::operator==(const QGradient &gradient)
{
    return const_cast<const QGradient *>(this)->operator==(gradient);
}


/*!
    \class QLinearGradient qbrush.h

    \brief The QLinearGradient class is used in combination with QBrush to
    specify a linear gradient brush.

    \sa QBrush
*/


/*!
    Constructs a linear gradient with interpolation area between \a
    start and \a finalStop. The positions \a start and \a finalStop
    are specified using logical coordinates.
*/
QLinearGradient::QLinearGradient(const QPointF &start, const QPointF &finalStop)
{
    m_type = LinearGradient;
    m_spread = PadSpread;
    m_data.linear.x1 = start.x();
    m_data.linear.y1 = start.y();
    m_data.linear.x2 = finalStop.x();
    m_data.linear.y2 = finalStop.y();
}

/*!
    \overload

    Constructs a linear gradient with interpolation area between \a
    xStart, \a yStart and \a xFinalStop, \a yFinalStop. The positions
    are specified using logical coordinates.
*/
QLinearGradient::QLinearGradient(qreal xStart, qreal yStart, qreal xFinalStop, qreal yFinalStop)
{
    m_type = LinearGradient;
    m_spread = PadSpread;
    m_data.linear.x1 = xStart;
    m_data.linear.y1 = yStart;
    m_data.linear.x2 = xFinalStop;
    m_data.linear.y2 = yFinalStop;
}


/*!
    Returns the start point of this linear gradient in logical
    coordinates.
*/

QPointF QLinearGradient::start() const
{
    Q_ASSERT(m_type == LinearGradient);
    return QPointF(m_data.linear.x1, m_data.linear.y1);
}


/*!
    Returns the final stop point of this linear gradient in logical
    coordinates.
*/

QPointF QLinearGradient::finalStop() const
{
    Q_ASSERT(m_type == LinearGradient);
    return QPointF(m_data.linear.x2, m_data.linear.y2);
}


/*!
    \class QRadialGradient
    \brief The QRadialGradient class is used in combination with QBrush to
    specify a radial gradient brush.

    \sa QBrush
*/

static QPointF qt_radial_gradient_adapt_focal_point(const QPointF &center,
                                                    qreal radius,
                                                    const QPointF &focalPoint)
{
    // We have a one pixel buffer zone to avoid numerical instability on the
    // circle border
    const qreal compensated_radius = radius - 1;
    QLineF line(center, focalPoint);
    if (line.length() >= (compensated_radius))
        line.setLength(compensated_radius);
    return line.p2();
}

/*!
    Constructs a radial gradient centered at \a center with radius \a
    radius.  The \a focalPoint can be used to define the focal point
    of the gradient inside the circle.

    The default focalPoint is the circle center.
*/

QRadialGradient::QRadialGradient(const QPointF &center, qreal radius, const QPointF &focalPoint)
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = center.x();
    m_data.radial.cy = center.y();
    m_data.radial.radius = radius;

    QPointF adapted_focal = qt_radial_gradient_adapt_focal_point(center, radius, focalPoint);
    m_data.radial.fx = adapted_focal.x();
    m_data.radial.fy = adapted_focal.y();
}



/*!
    Constructs a radial gradient centered at \a cx, \a cy with radius
    \a radius.  The focal point \a fx, \a fy can be used to define the
    focal point of the gradient inside the circle.

    The default focalPoint is the circle center.
*/

QRadialGradient::QRadialGradient(qreal cx, qreal cy, qreal radius, qreal fx, qreal fy)
{
    m_type = RadialGradient;
    m_spread = PadSpread;
    m_data.radial.cx = cx;
    m_data.radial.cy = cy;
    m_data.radial.radius = radius;

    QPointF adapted_focal = qt_radial_gradient_adapt_focal_point(QPointF(cx, cy),
                                                                 radius,
                                                                 QPointF(fx, fy));

    m_data.radial.fx = adapted_focal.x();
    m_data.radial.fy = adapted_focal.y();
}


/*!
    Returns the center of this radial gradient in logical coordinates.
*/

QPointF QRadialGradient::center() const
{
    Q_ASSERT(m_type == RadialGradient);
    return QPointF(m_data.radial.cx, m_data.radial.cy);
}


/*!
    Returns the radius of the radial gradient in logical coordinates.
*/

qreal QRadialGradient::radius() const
{
    Q_ASSERT(m_type == RadialGradient);
    return m_data.radial.radius;
}


/*!
    Returns the focal point of this radial gradient in logical
    coordinates.
*/

QPointF QRadialGradient::focalPoint() const
{
    Q_ASSERT(m_type == RadialGradient);
    return QPointF(m_data.radial.fx, m_data.radial.fy);
}


/*!
    \class QConicalGradient qbrush.h

    \brief The QConicalGradient class is used in combination with QBrush to
    specify a conical gradient brush.

    \sa QBrush
*/


/*!
    Constructs a conical centered at \a center and starting at
    \a angle. The angle is specified in degrees between 0 and 360.
*/

QConicalGradient::QConicalGradient(const QPointF &center, qreal angle)
{
    m_type = ConicalGradient;
    m_spread = PadSpread;
    m_data.conical.cx = center.x();
    m_data.conical.cy = center.y();
    m_data.conical.angle = angle;
}


/*!
    Constructs a conical centered at \a cx, \a cy and starting at
    \a angle. The angle is specified in degrees between 0 and 360.
*/

QConicalGradient::QConicalGradient(qreal cx, qreal cy, qreal angle)
{
    m_type = ConicalGradient;
    m_spread = PadSpread;
    m_data.conical.cx = cx;
    m_data.conical.cy = cy;
    m_data.conical.angle = angle;
}


/*!
    Returns the center of the conical gradient in logical coordinates
*/

QPointF QConicalGradient::center() const
{
    Q_ASSERT(m_type == ConicalGradient);
    return QPointF(m_data.conical.cx, m_data.conical.cy);
}


/*!
    Returns the start angle of the conical gradient in logical coordinates
*/

qreal QConicalGradient::angle() const
{
    Q_ASSERT(m_type == ConicalGradient);
    return m_data.conical.angle;
}

/*!
    \typedef QGradientStop
    \relates QGradient

    Typedef for QPair<qreal, QColor>.
*/

/*!
    \typedef QGradientStops
    \relates QGradient

    Typedef for QVector<QGradientStop>.
*/
