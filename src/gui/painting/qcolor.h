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

#ifndef QCOLOR_H
#define QCOLOR_H

#ifndef QT_H
#  include "qwindowdefs.h"
#  include "qstringlist.h"
#  include "qnamespace.h"
#  include "qrgb.h"
#endif // QT_H

class Q_GUI_EXPORT QColor
{
public:
    enum Spec { Rgb, Hsv };

    QColor();
    QColor(Qt::GlobalColor color);
    QColor(int r, int g, int b, int a = 255);
    QColor(int x, int y, int z, Spec);
    QColor(QRgb rgb, uint pixel=0xffffffff);
    QColor(const QString& name);
    QColor(const char *name);
    QColor(const QColor &);
    QColor &operator=(const QColor &);
    QColor &operator=(Qt::GlobalColor color);

    bool   isValid() const;
    bool   isDirty() const;
    QString name() const;
    void   setNamedColor(const QString& name);

    inline QRgb rgb()    const;
    void   setRgb(int r, int g, int b);
    void   setRgb(QRgb rgb);
    inline void getRgb(int *r, int *g, int *b) const;

    void setRgba(int r, int g, int b, int a);
    inline void getRgba(int *r, int *g, int *b, int *a) const;

    int           red()    const;
    int           green()  const;
    int           blue()   const;
    int    alpha()  const;

    void   setHsv(int h, int s, int v);
    void   getHsv(int *h, int *s, int *v) const;

#ifdef QT_COMPAT
    inline QT_COMPAT void rgb(int *r, int *g, int *b) const;
    inline QT_COMPAT void hsv(int *h, int *s, int *v) const { getHsv(h, s, v); }
    inline QT_COMPAT void getHsv(int &h, int &s, int &v) const { getHsv(&h, &s, &v); }
#endif

    QColor light(int f = 150) const;
    QColor dark(int f = 200)        const;

    bool   operator==(const QColor &c) const;
    bool   operator!=(const QColor &c) const;

    uint   alloc(int screen = -1);
    uint   pixel(int screen = -1)  const;

    static int  maxColors();
    static int  numBitPlanes();

    static int  enterAllocContext();
    static void leaveAllocContext();
    static int  currentAllocContext();
    static void destroyAllocContext(int);

#if defined(Q_WS_WIN)
    static const QRgb* palette(int* numEntries = 0);
    static int setPaletteEntries(const QRgb* entries, int numEntries,
                                  int base = -1);
    static HPALETTE hPal()  { return hpal; }
    static uint        realizePal(QWidget *);
#endif

    static void initialize();
    static void cleanup();
    static QStringList colorNames();
    enum { Dirt = 0x44495254, Invalid = 0x49000000 };

private:
    void setSystemNamedColor(const QString& name);
    void setPixel(uint pixel);
    void invalidate();
    static uint argbToPix32(QRgb);
    static bool color_init;
#if defined(Q_WS_WIN)
    static HPALETTE hpal;
#endif
    static enum ColorModel { d8, d32 } colormodel;
    union {
        QRgb argb;
        struct D8 {
            QRgb argb;
            uchar pix;
            uchar invalid;
            uchar dirty;
            uchar direct;
        } d8;
        struct D32 {
            QRgb argb;
            uint pix;
            bool invalid() const { return argb == QColor::Invalid && pix == QColor::Dirt; }
            bool probablyDirty() const { return pix == QColor::Dirt; }
        } d32;
    } d;
};


inline QColor::QColor()
{ d.d32.argb = Invalid; d.d32.pix = Dirt; }

inline QColor::QColor(int r, int g, int b, int a)
{
    d.d32.argb = Invalid;
    d.d32.pix = Dirt;
    if (a!=255)
        setRgba(r, g, b, a);
    else
        setRgb(r, g, b);
}

inline QColor::QColor(const char *name)
{ setNamedColor(QLatin1String(name)); }

inline QColor::QColor(const QString& name)
{ setNamedColor(name); }

inline QColor::QColor(const QColor &c)
{ d.argb = c.d.argb; d.d32.pix = c.d.d32.pix; }

inline QColor &QColor::operator=(const QColor &c)
{ d.argb = c.d.argb; d.d32.pix = c.d.d32.pix; return *this; }

inline QRgb QColor::rgb() const
{ return d.argb; }

inline void QColor::getRgb(int *r, int *g, int *b) const
{ *r = qRed(d.argb); *g = qGreen(d.argb); *b = qBlue(d.argb); }

inline void QColor::getRgba(int *r, int *g, int *b, int *a) const
{ getRgb(r, g, b); *a = qAlpha(d.argb); }

#ifdef QT_COMPAT
inline void QColor::rgb(int *r, int *g, int *b) const
{ *r = qRed(d.argb); *g = qGreen(d.argb); *b = qBlue(d.argb); }
#endif

inline int QColor::red() const
{ return qRed(d.argb); }

inline int QColor::green() const
{ return qGreen(d.argb); }

inline int QColor::blue() const
{ return qBlue(d.argb); }

inline int QColor::alpha() const
{ return qAlpha(d.argb); }

inline bool QColor::isValid() const
{
    if (colormodel == d8)
        return !d.d8.invalid;
    else
        return !d.d32.invalid();
}

inline void QColor::invalidate()
{
    if (colormodel == d8) {
        d.d8.invalid = true;
    } else {
        d.d32.argb = Invalid;
        d.d32.pix = Dirt;
    }
}

inline bool QColor::operator==(const QColor &c) const
{
    return d.argb == c.d.argb && isValid() == c.isValid();
}

inline bool QColor::operator!=(const QColor &c) const
{
    return !operator==(c);
}

#ifndef QT_NO_DEBUG
Q_GUI_EXPORT QDebug operator<<(QDebug, const QColor &);
#endif

/*****************************************************************************
  QColor stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QColor &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QColor &);
#endif

#endif // QCOLOR_H
