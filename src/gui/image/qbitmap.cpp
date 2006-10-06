/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qbitmap.h"
#include "qpixmap_p.h"
#include "qimage.h"
#include "qvariant.h"
#include <qpainter.h>
#if defined(Q_WS_X11)
#include <private/qt_x11_p.h>
#endif

/*!
    \class QBitmap
    \brief The QBitmap class provides monochrome (1-bit depth) pixmaps.

    \ingroup multimedia
    \ingroup shared

    The QBitmap class is a monochrome off-screen paint device used
    mainly for creating custom QCursor and QBrush objects,
    constructing QRegion objects, and for setting masks for pixmaps
    and widgets.

    QBitmap is a QPixmap subclass ensuring a depth of 1, except for
    null objects which have a depth of 0. If a pixmap with a depth
    greater than 1 is assigned to a bitmap, the bitmap will be
    dithered automatically.

    Use the QColor objects Qt::color0 and Qt::color1 when drawing on a
    QBitmap object (or a QPixmap object with depth 1).

    Painting with Qt::color0 sets the bitmap bits to 0, and painting
    with Qt::color1 sets the bits to 1. For a bitmap, 0-bits indicate
    background (or transparent pixels) and 1-bits indicate foreground
    (or opaque pixels). Use the clear() function to set all the bits
    to Qt::color0. Note that using the Qt::black and Qt::white colors
    make no sense because the QColor::pixel() value is not necessarily
    0 for black and 1 for white.

    The QBitmap class provides the transformed() function returning a
    transformed copy of the bitmap; use the QMatrix argument to
    translate, scale, shear, and rotate the bitmap. In addition,
    QBitmap provides the static fromData() function which returns a
    bitmap constructed from the given \c uchar data, and the static
    fromImage() function returning a converted copy of a QImage
    object.

    Just like the QPixmap class, QBitmap is optimized by the use of
    implicit data sharing. For more information, see the {Implicit
    Data Sharing} documentation.

    \sa  QPixmap, QImage, QImageReader, QImageWriter
*/


/*!
    Constructs a null bitmap.

    \sa QPixmap::isNull()
*/

QBitmap::QBitmap()
    : QPixmap(QSize(0, 0), BitmapType)
{
}

/*!
    \fn QBitmap::QBitmap(int width, int height)

    Constructs a bitmap with the given \a width and \a height. The pixels
    inside are uninitialized.

    \sa clear()
*/

QBitmap::QBitmap(int w, int h)
    : QPixmap(QSize(w, h), BitmapType)
{
}

/*!
    Constructs a bitmap with the given \a size.  The pixels in the
    bitmap are uninitialized.

    \sa clear()
*/

QBitmap::QBitmap(const QSize &size)
    : QPixmap(size, BitmapType)
{
}

/*!
    \fn QBitmap::clear()

    Clears the bitmap, setting all its bits to Qt::color0.
*/

/*!
    Constructs a bitmap that is a copy of the given \a pixmap.

    If the pixmap has a depth greater than 1, the resulting bitmap
    will be dithered automatically.

    \sa QPixmap::depth(), fromImage(), fromData()
*/

QBitmap::QBitmap(const QPixmap &pixmap)
{
    QBitmap::operator=(pixmap);
}

/*!
    \fn QBitmap::QBitmap(const QImage &image)

    Constructs a bitmap that is a copy of the given \a image.

    Use the static fromImage() function instead.
*/

/*!
    Constructs a bitmap from the file specified by the given \a
    fileName. If the file does not exist, or has an unknown format,
    the bitmap becomes a null bitmap.

    The \a fileName and \a format parameters are passed on to the
    QPixmap::load() function. If the file format uses more than 1 bit
    per pixel, the resulting bitmap will be dithered automatically.

    \sa QPixmap::isNull(), QImageReader::imageFormat()
*/

QBitmap::QBitmap(const QString& fileName, const char *format)
    : QPixmap(QSize(0, 0), BitmapType)
{
    load(fileName, format, Qt::MonoOnly);
}

/*!
    \overload

    Assigns the given \a pixmap to this bitmap and returns a reference
    to this bitmap.

    If the pixmap has a depth greater than 1, the resulting bitmap
    will be dithered automatically.

    \sa QPixmap::depth()
 */

QBitmap &QBitmap::operator=(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {                        // a null pixmap
        QBitmap bm(0, 0);
        QBitmap::operator=(bm);
    } else if (pixmap.depth() == 1) {                // 1-bit pixmap
        QPixmap::operator=(pixmap);                // shallow assignment
    } else {                                        // n-bit depth pixmap
        QImage image;
        image = pixmap.toImage();                                // convert pixmap to image
        *this = fromImage(image);                                // will dither image
    }
    return *this;
}


#ifdef QT3_SUPPORT
QBitmap::QBitmap(int w, int h, const uchar *bits, bool isXbitmap)
{
    *this = fromData(QSize(w, h), bits, isXbitmap ? QImage::Format_MonoLSB : QImage::Format_Mono);
}


QBitmap::QBitmap(const QSize &size, const uchar *bits, bool isXbitmap)
{
    *this = fromData(size, bits, isXbitmap ? QImage::Format_MonoLSB : QImage::Format_Mono);
}
#endif

/*!
  Destroys the bitmap.
*/
QBitmap::~QBitmap()
{
}

/*!
   Returns the bitmap as a QVariant.
*/
QBitmap::operator QVariant() const
{
    return QVariant(QVariant::Bitmap, this);
}

/*!
    \fn QBitmap &QBitmap::operator=(const QImage &image)
    \overload

    Converts the given \a image to a bitmap, and assigns the result to
    this bitmap. Returns a reference to the bitmap.

    Use the static fromImage() function instead.
*/

/*!
    Returns a copy of the given \a image converted to a bitmap using
    the specified image conversion \a flags.

    \sa fromData()
*/
QBitmap QBitmap::fromImage(const QImage &image, Qt::ImageConversionFlags flags)
{
    if (image.isNull())
        return QBitmap();
    QImage img = image.convertToFormat(QImage::Format_MonoLSB, flags);
#if defined (Q_WS_WIN) || defined (Q_WS_QWS)
    QBitmap bm;
    bm.data->image = img;

    // Swap colors to match so that default config draws more correctly.
    // black bits -> black pen in QPainter
    if (image.numColors() == 2 && qGray(image.color(0)) < qGray(image.color(1))) {
        QRgb color0 = image.color(0);
        QRgb color1 = image.color(1);
        bm.data->image.setColor(0, color1);
        bm.data->image.setColor(1, color0);
        bm.data->image.invertPixels();
    }
    return bm;
#elif defined(Q_WS_X11)
    QBitmap bm;
    // make sure image.color(0) == Qt::color0 (white) and image.color(1) == Qt::color1 (black)
    const QRgb c0 = QColor(Qt::black).rgb();
    const QRgb c1 = QColor(Qt::white).rgb();
    if (img.color(0) == c0 && img.color(1) == c1) {
        img.invertPixels();
        img.setColor(0, c1);
        img.setColor(1, c0);
    }

    char  *bits;
    uchar *tmp_bits;
    int w = img.width();
    int h = img.height();
    int bpl = (w+7)/8;
    int ibpl = img.bytesPerLine();
    if (bpl != ibpl) {
        tmp_bits = new uchar[bpl*h];
        bits = (char *)tmp_bits;
        uchar *p, *b;
        int y;
        b = tmp_bits;
        p = img.scanLine(0);
        for (y = 0; y < h; y++) {
            memcpy(b, p, bpl);
            b += bpl;
            p += ibpl;
        }
    } else {
        bits = (char *)img.bits();
        tmp_bits = 0;
    }
    bm.data->hd = (Qt::HANDLE)XCreateBitmapFromData(bm.data->xinfo.display(),
                                                    RootWindow(bm.data->xinfo.display(), bm.data->xinfo.screen()),
                                                    bits, w, h);

#ifndef QT_NO_XRENDER
    if (X11->use_xrender)
        bm.data->picture = XRenderCreatePicture(X11->display, bm.data->hd,
                                                XRenderFindStandardFormat(X11->display, PictStandardA1), 0, 0);
#endif // QT_NO_XRENDER

    if (tmp_bits)                                // Avoid purify complaint
        delete [] tmp_bits;
    bm.data->w = w;  bm.data->h = h;  bm.data->d = 1;

    return bm;
#else
    const QRgb c0 = QColor(Qt::black).rgb();
    const QRgb c1 = QColor(Qt::white).rgb();
    if (img.color(0) == c0 && img.color(1) == c1) {
        img.invertPixels();
        img.setColor(0, c1);
        img.setColor(1, c0);
    }
    return QBitmap(QPixmap::fromImage(img, flags|Qt::MonoOnly));
#endif
}

/*!
    Constructs a bitmap with the given \a size, and sets the contents to
    the \a bits supplied.

    The bitmap data has to be byte aligned and provided in in the bit
    order specified by \a monoFormat. The mono format must be either
    QImage::Format_Mono or QImage::Format_MonoLSB. Use
    QImage::Format_Mono to specify data on the XBM format.

    \sa fromImage()

*/
QBitmap QBitmap::fromData(const QSize &size, const uchar *bits, QImage::Format monoFormat)
{
    Q_ASSERT(monoFormat == QImage::Format_Mono || monoFormat == QImage::Format_MonoLSB);

    QImage image(size, monoFormat);
    image.setColor(0, Qt::color0);
    image.setColor(1, Qt::color1);

    // Need to memcpy each line separatly since QImage is 32bit aligned and
    // this data is only byte aligned...
    int bytesPerLine = (size.width() + 7) / 8;
    for (int y = 0; y < size.height(); ++y)
        memcpy(image.scanLine(y), bits + bytesPerLine * y, bytesPerLine);
    return QBitmap::fromImage(image);
}


/*!
    Returns a copy of this bitmap, transformed according to the given
    \a matrix.

    \sa QPixmap::transformed()
*/

QBitmap QBitmap::transformed(const QMatrix &matrix) const
{
    return transformed(QTransform(matrix));
}

QBitmap QBitmap::transformed(const QTransform &matrix) const
{
    QBitmap bm = QPixmap::transformed(matrix);
    return bm;
}

#ifdef QT3_SUPPORT
/*!
    \fn QBitmap QBitmap::xForm(const QMatrix &matrix) const

    Returns a copy of this bitmap, transformed according to the given
    \a matrix.

    Use transformed() instead.
*/

/*!
    \fn QBitmap::QBitmap(const QSize &size, bool clear)

    Constructs a bitmap with the given \a size. If \a clear is true,
    the bits are initialized to Qt::color0.

    Use the corresponding QBitmap() constructor instead, and then call
    the clear() function if the \a clear parameter is true.
*/

/*!
    \fn QBitmap::QBitmap(int width, int height, bool clear)

    Constructs a bitmap with the given \a width and \a height.  If \a
    clear is true, the bits are initialized to Qt::color0.

    Use the corresponding QBitmap() constructor instead, and then call
    the clear() function if the \a clear parameter is true.
*/

/*!
    \fn QBitmap::QBitmap(int width, int height, const uchar *bits, bool isXbitmap)

    Constructs a bitmap with the given \a width and \a height, and
    sets the contents to the \a bits supplied. The \a isXbitmap flag
    should be true if \a bits was generated by the X11 bitmap
    program.

    Use the static fromData() function instead. If \a isXbitmap is
    true, use the default bit order(QImage_FormatMonoLSB) otherwise
    use QImage::Format_Mono.

    \omit
    The X bitmap bit order is little endian.  The QImage
    documentation discusses bit order of monochrome images. Opposed to
    QImage, the data has to be byte aligned.

    Example (creates an arrow bitmap):
    \code
        uchar arrow_bits[] = { 0x3f, 0x1f, 0x0f, 0x1f, 0x3b, 0x71, 0xe0, 0xc0 };
        QBitmap bm(8, 8, arrow_bits, true);
    \endcode
    \endomit
*/


/*!
  \fn QBitmap::QBitmap(const QSize &size, const uchar *bits, bool isXbitmap)

    \overload

    Constructs a bitmap with the given \a size, and sets the contents
    to the \a bits supplied. The \a isXbitmap flag should be true if
    \a bits was generated by the X11 bitmap program.

    \omit
    The X bitmap bit order is little endian.  The QImage documentation
    discusses bit order of monochrome images.
    \endomit

    Use the static fromData() function instead. If \a isXbitmap is
    true, use the default bit order(QImage_FormatMonoLSB) otherwise
    use QImage::Format_Mono.
*/
#endif
