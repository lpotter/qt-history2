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

#include "qimage.h"
#include "qdatastream.h"
#include "qbuffer.h"
#include "qmap.h"
#include "qmatrix.h"
#include "qimagereader.h"
#include "qimagewriter.h"
#include "qstringlist.h"
#include "qvariant.h"
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#if defined(Q_WS_X11)
#include <private/qt_x11_p.h>

QImage::Endian qX11BitmapBitOrder()
{
    return BitmapBitOrder(qt_xdisplay()) == MSBFirst ? QImage::BigEndian : QImage::LittleEndian;
}
#endif

#ifdef QT_RASTER_IMAGEENGINE
#include <private/qpaintengine_raster_p.h>
#else
#include <qpaintengine.h>
#endif

#ifdef Q_WS_QWS
#include <private/qpaintengine_qws_p.h>
#include "qscreen_qws.h"
#endif

#ifndef Q_WS_QWS
static inline QRgb qt_conv16ToRgb(ushort c) {
    const int r = (c & 0xf800) >> (11 - (8-5));
    const int g = (c & 0x07e0) >> (6 - (8-6));
    const int b = (c & 0x001f) << (8-5);

    return qRgb(r, g, b);
}

inline ushort qt_convRgbTo16(QRgb c)
{
    return (qRed(c) << (11 - (8-5))) + (qGreen(c) << (6 - (8-6))) + (qBlue(c) >> (8-5));
}
#endif


#if defined(Q_CC_DEC) && defined(__alpha) && (__DECCXX_VER-0 >= 50190001)
#pragma message disable narrowptr
#endif

struct QImageData {        // internal image data
    QImageData();
    ~QImageData();

    QAtomic ref;

    int w;                    // image width
    int h;                    // image height
    int d;                    // image depth
    int ncols;                // number of colors
    int nbytes;               // number of bytes data
    QRgb *ctbl;               // color table
    uchar **bits;             // image data
    bool alpha;               // alpha buffer
    int bitordr;              // bit order (1 bit depth)
    int ser_no;               // serial number

    int  dpmx;                // dots per meter X (or 0)
    int  dpmy;                // dots per meter Y (or 0)
    QPoint  offset;           // offset in pixels

#ifndef QT_NO_IMAGE_TEXT
    QMap<QImageTextKeyLang, QString> text_lang;

    QStringList languages() const
    {
        QStringList r;
        QMap<QImageTextKeyLang,QString>::const_iterator it = text_lang.begin();
        for (; it != text_lang.end(); ++it) {
            r.removeAll(it.key().lang);
            r.append(it.key().lang);
        }
        return r;
    }
    QStringList keys() const
    {
        QStringList r;
        QMap<QImageTextKeyLang,QString>::const_iterator it = text_lang.begin();
        for (; it != text_lang.end(); ++it) {
            r.removeAll(it.key().key);
            r.append(it.key().key);
        }
        return r;
    }
#endif
#ifndef QT_NO_IMAGEIO
    bool doImageIO(const QImage *image, QImageWriter* io, int quality) const;
#endif

    QPaintEngine *paintEngine;
};

QImageData::QImageData()
{
    static int serial = 0;

    ser_no = ++serial;
    ref = 0;

    w = h = d = ncols = 0;
    nbytes = 0;
    ctbl = 0;
    bits = 0;
    ncols = 0;
    bitordr = QImage::IgnoreEndian;
    alpha = false;

    dpmx = 0;
    dpmy = 0;
    offset = QPoint(0,0);

    paintEngine = 0;
}

QImageData::~QImageData()
{
    if (bits)
        free(bits);
    if (ctbl)
        free(ctbl);
    ctbl = 0;
    bits = 0;
}

/*!
    \class QImage
    \brief The QImage class provides a hardware-independent pixmap
    representation with direct access to the pixel data.

    \ingroup multimedia
    \ingroup shared
    \mainclass

    Qt provides two classes for handling image data: QImage and QPixmap.
    QImage is designed and optimized for I/O, and for direct pixel
    access/manipulation. QPixmap is designed and optimized for drawing.
    There are functions to convert between QImage and QPixmap:
    QPixmap::convertToImage() and QPixmap::convertFromImage().

    An image has the parameters \l{width()}{width}, \l{height()}{height}
    and \l{depth()}{depth} (bits per pixel, bpp), a color table, and the
    actual \l{bits()}{pixels}. QImage supports 1-bpp, 8-bpp, and 32-bpp
    image data. 1-bpp and 8-bpp images use a color lookup table; the pixel
    value is a color table index.

    32-bpp images contain RGB values encoded in 24 bits, and do not use a
    color table. The most significant byte is used for the
    \l{setAlphaBuffer()}{alpha buffer}.

    An entry in the color table is an RGB triplet encoded as an \c qRgb
    value. Use the color() function to obtain an entry from the table,
    and the qRed(), qGreen(), and qBlue() functions (\c qcolor.h) to access
    the components. The qRgb() function is used to make an RGB triplet
    suitable for use with the setColor() function.

    1-bpp (monochrome) images have a color table with a most two
    colors. There are two different formats: big endian (MSB first) or
    little endian (LSB first) bit order. To access a single bit you
    will must do some bit shifts:

    \code
        QImage image;
        // sets bit at (x, y) to 1
        if (image.bitOrder() == QImage::LittleEndian)
            image.scanLine(y)[x >> 3] |= 1 << (x & 7);
        else
            image.scanLine(y)[x >> 3] |= 1 << (7 - (x & 7));
    \endcode

    If this looks complicated, it might be a good idea to convert the
    1-bpp image to an 8-bpp image using convertDepth().

    8-bpp images are much easier to work with than 1-bpp images
    because they have a single byte per pixel:

    \code
        QImage image;

        // set entry 19 in the color table to yellow
        image.setColor(19, qRgb(255, 255, 0));

        // set 8 bit pixel at (x,y) to value yellow (in color table)
        image.scanLine(y)[x] = 19;
    \endcode

    32-bpp images ignore the color table; instead, each pixel contains
    the RGB triplet. 24 bits contain the RGB value; the most
    significant byte is reserved for the alpha buffer.

    \code
        QImage image;

        // sets 32 bit pixel at (x,y) to yellow.
        uint *ptr = reinterpret_cast<uint *>(image.scanLine(y)) + x;
        *ptr = qRgb(255, 255, 0);
    \endcode

    On Qt/Embedded, scanlines are aligned to the pixel depth and may
    be padded to any degree, while on all other platforms, the
    scanlines are 32-bit aligned for all depths. The constructor
    taking a \c{uchar*} argument always expects 32-bit aligned data.
    On Qt/Embedded, an additional constructor allows the number of
    bytes-per-line to be specified.

    Pixel colors are retrieved with pixel() and set with setPixel().

    QImage supports a variety of functions that can be used to obtain
    information about the image. width(), height(), dotsPerMeterX(), and
    dotsPerMeterY() provide information about the image size and resolution.
    The depth(), numColors(), isGrayscale(), and colorTable() functions
    provide information about the color depth and available color components
    used to store the image data. The hasAlphaBuffer() function indicates
    whether the image has an associated alpha buffer.

    It is possible to determine whether a color image can be safely
    converted to a grayscale image by using the allGray() function. The
    bitOrder(), bytesPerLine(), and numBytes() functions provide low-level
    information about the data stored in the image.

    QImage also supports a number of functions for creating a new
    image that is a transformed version of the original. For example,
    copy(), convertBitOrder(), convertDepth(), createAlphaMask(),
    createHeuristicMask(), mirror(), scale(), swapRGB()
    and transform(). There are also functions for changing attributes of
    an image in-place, for example, setAlphaBuffer(), setColor(),
    setDotsPerMeterX() and setDotsPerMeterY() and setNumColors().

    Images can be loaded and saved in the supported formats. Images
    are saved to a file with save(). Images are loaded from a file
    with load() (or in the constructor) or from an array of data with
    loadFromData(). The lists of supported formats are available from
    QImageReader::supportedImageFormats() and
    QImageWriter::supportedImageFormat().

    When loading an image, the file name can be either refer to an
    actual file on disk or to one of the application's embedded
    resources. See the \l{resources.html}{Resource System} overview
    for details on how to embed images and other resource files in
    the application's executable.

    Strings of text may be added to images using setText().

    The QImage class uses \l{shclass.html}{implicit sharing}, so you
    can pass QImage objects around by value.

    New image formats can be added as \link plugins-howto.html
    plugins\endlink.

    \sa QImageReader, QImageWriter, QPixmap QColor {shclass.html}{Shared Classes}
*/

/*!
    \enum QImage::Endian

    This enum type is used to describe the endianness of the CPU and
    graphics hardware.

    \value IgnoreEndian  Endianness does not matter. Useful for some
                         operations that are independent of endianness.
    \value BigEndian     Network byte order, as on SPARC and Motorola CPUs.
    \value LittleEndian  PC/Alpha byte order.
*/

/*!
    \enum QImage::InvertMode

    This enum type is used to describe how pixel values should be
    inverted in the invertPixels() function.

    \value InvertRgb    Invert only the RGB values and leave the alpha
                        channel unchanged.

    \value InvertRgba   Invert all channels, including the alpha channel.
*/


/*****************************************************************************
  QImage member functions
 *****************************************************************************/

// table to flip bits
static const uchar bitflip[256] = {
    /*
        open OUT, "| fmt";
        for $i (0..255) {
            print OUT (($i >> 7) & 0x01) | (($i >> 5) & 0x02) |
                      (($i >> 3) & 0x04) | (($i >> 1) & 0x08) |
                      (($i << 7) & 0x80) | (($i << 5) & 0x40) |
                      (($i << 3) & 0x20) | (($i << 1) & 0x10), ", ";
        }
        close OUT;
    */
    0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
    8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
    4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
    12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
    2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
    10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
    6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
    14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
    1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
    9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
    5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
    13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
    3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
    11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
    7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
    15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255
};

const uchar *qt_get_bitflip_array()                        // called from QPixmap code
{
    return bitflip;
}


/*!
    Constructs a null image.

    \sa isNull()
*/

QImage::QImage()
    : QPaintDevice(QInternal::Image)
{
    d = new QImageData;
    ++d->ref;
}

/*!
    Constructs an image with \a w width, \a h height, \a depth bits
    per pixel, \a numColors colors and bit order \a bitOrder.

    Using this constructor is the same as first constructing a null
    image and then calling the create() function.

    \sa create()
*/

QImage::QImage(int w, int h, int depth, int numColors, Endian bitOrder)
    : QPaintDevice(QInternal::Image)
{
    d = new QImageData;
    ++d->ref;
    create(w, h, depth, numColors, bitOrder);
}

/*!
    Constructs an image with size \a size pixels, depth \a depth bits,
    \a numColors and \a bitOrder endianness.

    Using this constructor is the same as first constructing a null
    image and then calling the create() function.

    \sa create()
*/
QImage::QImage(const QSize& size, int depth, int numColors, Endian bitOrder)
    : QPaintDevice(QInternal::Image)
{
    d = new QImageData;
    ++d->ref;
    create(size, depth, numColors, bitOrder);
}

#ifndef QT_NO_IMAGEIO
/*!
    Constructs an image and tries to load the image from the file \a
    fileName.

    If \a format is specified, the loader attempts to read the image
    using the specified format. If \a format is not specified (which
    is the default), the loader reads a few bytes from the header to
    guess the file format.

    If the loading of the image failed, this object is a \link
    isNull() null\endlink image.

    The QImageReader documentation lists the supported image formats and
    explains how to add extra formats.

    The file name can be either refer to an actual file on disk or to
    one of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how
    to embed images and other resource files in the application's
    executable.

    \sa load(), isNull(), QImageReader
*/

QImage::QImage(const QString &fileName, const char *format)
    : QPaintDevice(QInternal::Image)
{
    d = new QImageData;
    ++d->ref;
    load(fileName, format);
}

/*!
    Constructs an image and tries to load the image from the file \a
    fileName.

    If \a format is specified, the loader attempts to read the image
    using the specified format. If \a format is not specified (which
    is the default), the loader reads a few bytes from the header to
    guess the file format.

    If the loading of the image failed, this object is a \link
    isNull() null\endlink image.

    The QImageReader documentation lists the supported image formats and
    explains how to add extra formats.

    The file name can be either refer to an actual file on disk or to
    one of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how
    to embed images and other resource files in the application's
    executable.

    You can disable this constructor by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications. This
    can be useful if you want to ensure that all user-visible strings
    go through QObject::tr(), for example.

    \sa QString::fromAscii(),     \sa load(), isNull(), QImageReader
*/
QImage::QImage(const char *fileName, const char *format)
    : QPaintDevice(QInternal::Image)
{
    // ### Qt 5: if you remove the QImage(const QByteArray &) QT3_SUPPORT
    // constructor, remove this constructor as well. The constructor here
    // exists so that QImage("foo.png") compiles without ambiguity.
    d = new QImageData;
    ++d->ref;
    load(QString::fromAscii(fileName), format);
}

extern bool qt_read_xpm_image_or_array(QIODevice *device, const char * const *source, QImage &image);

/*!
    Constructs an image from \a xpm, which must be a valid XPM image.

    Errors are silently ignored.

    Note that it's possible to squeeze the XPM variable a little bit
    by using an unusual declaration:

    \code
        static const char * const start_xpm[] = {
            "16 15 8 1",
            "a c #cec6bd",
        ....
    \endcode

    The extra \c const makes the entire definition read-only, which is
    slightly more efficient (e.g. when the code is in a shared
    library) and ROMable when the application is to be stored in ROM.
*/

QImage::QImage(const char * const xpm[])
    : QPaintDevice(QInternal::Image)
{
    d = new QImageData;
    ++d->ref;

    QImage image;
    if (!qt_read_xpm_image_or_array(0, xpm, image)) {
        // We use a qFatal rather than disabling the whole function,
        // as this constructor may be ambiguous.
        qFatal("QImage::QImage(), XPM is not supported");
    }

    operator=(image);
}

/*!
    \fn QImage::QImage(const QByteArray &data)

    Use QImage::fromData(\a data) instead.

    \oldcode
        QByteArray data;
        ...
        QImage image(data);
    \newcode
        QByteArray data;
        ...
        QImage image = QImage::fromData(data);
    \endcode
*/
#endif //QT_NO_IMAGEIO


/*!
    Constructs a \link shclass.html shallow copy\endlink of \a image.
*/

QImage::QImage(const QImage &image)
    : QPaintDevice(QInternal::Image)
{
    d = image.d;
    ++d->ref;
}

/*!
    Constructs an image \a w pixels wide, \a h pixels high with a
    color depth of \a depth, that uses an existing memory buffer, \a
    data. The buffer must remain valid throughout the life of the
    QImage. The image does not delete the buffer at destruction.

    If \a colortable is 0, a color table sufficient for \a numColors
    will be allocated (and destructed later).

    Note that \a data must be 32-bit aligned.

    The endianness is given in \a bitOrder.
*/
QImage::QImage(uchar* data, int w, int h, int depth, const QRgb* colortable, int numColors, Endian bitOrder)
    : QPaintDevice(QInternal::Image)
{
    d = new QImageData;
    ++d->ref;

    if (w <= 0 || h <= 0 || depth <= 0 || numColors < 0)
        return;                                        // invalid parameter(s)
    d->w = w;
    d->h = h;
    d->d = depth;
    d->ncols = depth != 32 ? numColors : 0;
    if (!data)
        return;            // Image header info can be saved without needing to allocate memory.
    int bpl = ((w*depth+31)/32)*4;        // bytes per scanline
    d->nbytes = bpl*h;
    if (colortable) {
        d->ctbl = (QRgb *)realloc(d->ctbl, numColors*sizeof(QRgb));
        memcpy(d->ctbl, colortable, numColors*sizeof(QRgb));
    }
    uchar** jt = (uchar**)malloc(h*sizeof(uchar*));
    for (int j=0; j<h; j++) {
        jt[j] = data + j * bpl;
    }
    d->bits = jt;
    d->bitordr = bitOrder;
}

#ifdef Q_WS_QWS

/*!
    Constructs an image that uses an existing memory buffer. The
    buffer must remain valid for the life of the QImage. The image
    does not delete the buffer at destruction. The buffer is passed as
    \a data. The image's width is \a w and its height is \a h. The
    color depth is \a depth. \a bpl specifies the number of bytes per
    line.

    If \a colortable is 0, a color table sufficient for \a numColors
    will be allocated (and destructed later).

    The endianness is specified by \a bitOrder.

    \warning This constructor is only available on Qt/Embedded.
*/
QImage::QImage(uchar* data, int w, int h, int depth, int bpl, const QRgb* colortable, int numColors, Endian bitOrder)
    : QPaintDevice(QInternal::Image)
{
    d = new QImageData;
    ++d->ref;

    if (!data || w <= 0 || h <= 0 || depth <= 0 || numColors < 0)
        return;                                        // invalid parameter(s)
    d->w = w;
    d->h = h;
    d->d = depth;
    d->ncols = numColors;
    d->nbytes = bpl * h;
    if (colortable) {
        d->ctbl = (QRgb *)realloc(d->ctbl, numColors*sizeof(QRgb));
        memcpy(d->ctbl, colortable, numColors*sizeof(QRgb));
    }
    uchar** jt = (uchar**)malloc(h*sizeof(uchar*));
    for (int j=0; j<h; j++) {
        jt[j] = data + j * bpl;
    }
    d->bits = jt;
    d->bitordr = bitOrder;
}
#endif // Q_WS_QWS

/*!
    Destroys the image and cleans up.
*/

QImage::~QImage()
{
    if (d && !--d->ref)
        delete d;
}

/*!
    Assigns a \link shclass.html shallow copy\endlink of \a image to
    this image and returns a reference to this image.

    \sa copy()
*/

QImage &QImage::operator=(const QImage &image)
{
    QImageData *x = image.d;
    ++x->ref;
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref)
        delete x;
    return *this;
}

/*!
   Returns the image as a QVariant
*/
QImage::operator QVariant() const
{
    extern bool qRegisterGuiVariant();
    static const bool b = qRegisterGuiVariant();
    Q_UNUSED(b)
    return QVariant(QVariant::Image, this);
}

/*!
    Detaches from shared image d and makes sure that this image is
    the only one referring to the data.

    If multiple images share common data, this image makes a copy of
    the data and detaches itself from the sharing mechanism.
    Nothing is done if there is just a single reference.

    \sa copy()
*/

void QImage::detach()
{
    if (d->ref != 1)
        *this = copy();
}

/*!
    Returns a \link shclass.html deep copy\endlink of the image.

    \sa detach()
*/

QImage QImage::copy() const
{
    if (isNull()) {
        // maintain the fields of invalid QImages when copied
        return QImage(0, width(), height(), depth(), colorTable(), numColors(), bitOrder());
    } else {
        QImage image;
        image.create(width(), height(), depth(), numColors(), bitOrder());
#ifdef Q_WS_QWS
        // Qt/Embedded can create images with non-default bpl
        // make sure we don't crash.
        if (image.numBytes() != numBytes())
            for (int i = 0; i < height(); i++)
                memcpy(image.scanLine(i), scanLine(i), image.bytesPerLine());
        else
#endif
            memcpy(image.bits(), bits(), numBytes());
        memcpy(image.colorTable(), colorTable(), numColors() * sizeof(QRgb));
        image.setAlphaBuffer(hasAlphaBuffer());
        image.d->dpmx = dotsPerMeterX();
        image.d->dpmy = dotsPerMeterY();
        image.d->offset = offset();
#ifndef QT_NO_IMAGE_TEXT
        image.d->text_lang = d->text_lang;
#endif
        return image;
    }
}

/*!
    \fn inline QImage QImage::copy(int x, int y, int w, int h, Qt::ImageConversionFlags flags = QFlag()) const
    \overload

    Returns a \link shclass.html deep copy\endlink of a sub-area of
    the image.

    The returned image is always \a w by \a h pixels in size, and is
    copied from position \a x, \a y in this image. In areas beyond
    this image pixels are filled with pixel 0.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a
    flags to specify how you'd prefer this to happen.

    \sa bitBlt() Qt::ImageConversionFlags
*/

/*!
    \overload

    Returns a \link shclass.html deep copy\endlink of a sub-area of
    the image.

    The returned image always has the size of the rectangle \a r. In
    areas beyond this image pixels are filled with pixel 0.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a
    flags to specify how you'd prefer this to happen.

    \sa bitBlt() Qt::ImageConversionFlags
*/
QImage QImage::copy(const QRect& r, Qt::ImageConversionFlags flags) const
{
    int x = r.x();
    int y = r.y();
    int w = r.width();
    int h = r.height();

    int dx = 0;
    int dy = 0;
    if (w <= 0 || h <= 0) return QImage(); // Nothing to copy

    QImage image(w, h, depth(), numColors(), bitOrder());

    if (x < 0 || y < 0 || x + w > width() || y + h > height()) {
        // bitBlt will not cover entire image - clear it.
        image.fill(0);
        if (x < 0) {
            dx = -x;
            x = 0;
        }
        if (y < 0) {
            dy = -y;
            y = 0;
        }
    }

    bool has_alpha = hasAlphaBuffer();
    if (has_alpha) {
        // alpha channel should be only copied, not used by bitBlt(), and
        // this is mutable, we will restore the image state before returning
        QImage *that = (QImage *) this;
        that->setAlphaBuffer(false);
    }
    memcpy(image.colorTable(), colorTable(), numColors()*sizeof(QRgb));
    bitBlt(&image, dx, dy, this, x, y, -1, -1, flags);
    if (has_alpha) {
        // restore image state
        QImage *that = (QImage *) this;
        that->setAlphaBuffer(true);
    }
    image.setAlphaBuffer(hasAlphaBuffer());
    image.d->dpmx = dotsPerMeterX();
    image.d->dpmy = dotsPerMeterY();
    image.d->offset = offset();
#ifndef QT_NO_IMAGE_TEXT
    image.d->text_lang = d->text_lang;
#endif
    return image;
}


/*!
    \fn bool QImage::isNull() const

    Returns true if it is a null image; otherwise returns false.

    A null image has all parameters set to zero and no allocated data.
*/
bool QImage::isNull() const
{
    return d->bits == 0;
}

/*!
    \fn int QImage::width() const

    Returns the width of the image.

    \sa height() size() rect()
*/
int QImage::width() const
{
    return d->w;
}

/*!
    \fn int QImage::height() const

    Returns the height of the image.

    \sa width() size() rect()
*/
int QImage::height() const
{
    return d->h;
}

/*!
    \fn QSize QImage::size() const

    Returns the size of the image, i.e. its width and height.

    \sa width() height() rect()
*/
QSize QImage::size() const
{
    return QSize(d->w, d->h);
}

/*!
    \fn QRect QImage::rect() const

    Returns the enclosing rectangle (0, 0, width(), height()) of the
    image.

    \sa width() height() size()
*/
QRect QImage::rect() const
{
    return QRect(0, 0, d->w, d->h);
}

/*!
    \fn int QImage::depth() const

    Returns the depth of the image.

    The image depth is the number of bits used to encode a single
    pixel, also called bits per pixel (bpp) or bit planes of an image.

    The supported depths are 1, 8, 16 (Qt/Embedded only) and 32.

    \sa convertDepth()
*/
int QImage::depth() const
{
    return d->d;
}

/*!
    \fn int QImage::numColors() const

    Returns the size of the color table for the image.

    Notice that numColors() returns 0 for 16-bpp (Qt/Embedded only)
    and 32-bpp images because these images do not use color tables,
    but instead encode pixel values as RGB triplets.

    \sa setNumColors() colorTable()
*/
int QImage::numColors() const
{
    return d->ncols;
}

/*!
    \fn QImage::Endian QImage::bitOrder() const

    Returns the bit order for the image.

    If it is a 1-bpp image, this function returns either
    QImage::BigEndian or QImage::LittleEndian.

    If it is not a 1-bpp image, this function returns
    QImage::IgnoreEndian.

    \sa depth()
*/
QImage::Endian QImage::bitOrder() const
{
    return static_cast<Endian>(d->bitordr);
}

/*!
    Returns a pointer to the scanline pointer table.

    This is the beginning of the data block for the image.

    \sa bits() scanLine()
*/
uchar **QImage::jumpTable()
{
    detach();
    return d->bits;
}

/*!
    \overload
*/
const uchar * const *QImage::jumpTable() const
{
    return d->bits;
}

/*!
    Returns a pointer to the color table.

    \sa numColors()
*/
QRgb *QImage::colorTable()
{
    detach();
    return d->ctbl;
}

/*!
    \overload
*/
const QRgb *QImage::colorTable() const
{
    return d->ctbl;
}


/*!
    Returns the number of bytes occupied by the image data.

    \sa bytesPerLine() bits()
*/
int QImage::numBytes() const
{
    return d->nbytes;
}


/*!
    Returns the number of bytes per image scanline. This is equivalent
    to numBytes()/height().

    \sa numBytes() scanLine()
*/
int QImage::bytesPerLine() const
{
    return d->h ? d->nbytes / d->h : 0;
}


/*!
    Returns the color in the color table at index \a i. The first
    color is at index 0.

    A color value is an RGB triplet. Use the \link ::qRed()
    qRed()\endlink, \link ::qGreen() qGreen()\endlink and \link
    ::qBlue() qBlue()\endlink functions (defined in \c qcolor.h) to
    get the color value components.

    \sa setColor() numColors() QColor
*/
QRgb QImage::color(int i) const
{
    Q_ASSERT(i < numColors());
    return d->ctbl ? d->ctbl[i] : QRgb(uint(-1));
}

/*!
    Sets a color in the color table at index \a i to \a c.

    A color value is an RGB triplet. Use the \link ::qRgb()
    qRgb()\endlink function (defined in \c qcolor.h) to make RGB
    triplets.

    \sa color() setNumColors() numColors()
*/
void QImage::setColor(int i, QRgb c)
{
    detach();
    Q_ASSERT(i < numColors());
    if (d->ctbl)
        d->ctbl[i] = c;
}

/*!
    Returns a pointer to the pixel data at the scanline with index \a
    i. The first scanline is at index 0.

    The scanline data is aligned on a 32-bit boundary.

    \warning If you are accessing 32-bpp image data, cast the returned
    pointer to \c{QRgb*} (QRgb has a 32-bit size) and use it to
    read/write the pixel value. You cannot use the \c{uchar*} pointer
    directly, because the pixel format depends on the byte order on
    the underlying platform. Hint: use \link ::qRed() qRed()\endlink,
    \link ::qGreen() qGreen()\endlink and \link ::qBlue()
    qBlue()\endlink, etc. (qcolor.h) to access the pixels.

    \warning If you are accessing 16-bpp image data, you must handle
    endianness yourself. (Qt/Embedded only)

    \sa bytesPerLine() bits() jumpTable()
*/
uchar *QImage::scanLine(int i)
{
    detach();
    Q_ASSERT(i < height());
    return d->bits ? d->bits[i] : 0;
}

/*!
    \overload
*/
const uchar *QImage::scanLine(int i) const
{
    Q_ASSERT(i < height());
    return d->bits ? d->bits[i] : 0;
}


/*!
    Returns a pointer to the first pixel data. This is equivalent to
    scanLine(0).

    \sa numBytes() scanLine() jumpTable()
*/
uchar *QImage::bits()
{
    detach();
    return d->bits ? d->bits[0] : 0;
}

/*!
    \overload
*/
const uchar *QImage::bits() const
{
    return d->bits ? d->bits[0] : 0;
}



/*!
    Resets all image parameters and deallocates the image data.
*/

void QImage::reset()
{
    *this = QImage();
}


/*!
    Fills the entire image with the pixel value \a pixel.

    If the \link depth() depth\endlink of this image is 1, only the
    lowest bit is used. If you say fill(0), fill(2), etc., the image
    is filled with 0s. If you say fill(1), fill(3), etc., the image is
    filled with 1s. If the depth is 8, the lowest 8 bits are used.

    If the depth is 32 and the image has no alpha buffer, the \a pixel
    value is written to each pixel in the image. If the image has an
    alpha buffer, only the 24 RGB bits are set and the upper 8 bits
    (alpha value) are left unchanged.

    Note: QImage::pixel() returns the color of the pixel at the given
    coordinates; QColor::pixel() returns the pixel value of the
    underlying window system (essentially an index value), so normally
    you will want to use QImage::pixel() to use a color from an
    existing image or QColor::rgb() to use a specific color.

    \sa invertPixels() depth() hasAlphaBuffer() create()
*/

void QImage::fill(uint pixel)
{
    detach();
    if (depth() == 1 || depth() == 8) {
        if (depth() == 1) {
            if (pixel & 1)
                pixel = 0xffffffff;
            else
                pixel = 0;
        } else {
            uint c = pixel & 0xff;
            pixel = c | ((c << 8) & 0xff00) | ((c << 16) & 0xff0000) |
                    ((c << 24) & 0xff000000);
        }
        int bpl = bytesPerLine();
        for (int i=0; i<height(); i++)
            memset(scanLine(i), pixel, bpl);
#ifndef QT_NO_IMAGE_16_BIT
    } else if (depth() == 16) {
        for (int i=0; i<height(); i++) {
            //optimize with 32-bit writes, since image is always aligned
            uint *p = (uint *)scanLine(i);
            uint *end = (uint*)(((ushort*)p) + width());
            pixel &= 0xffff;
            uint fill = (pixel<<16) + pixel;
            while (p < end)
                *p++ = fill;
        }
#endif        // QT_NO_IMAGE_16_BIT
#ifndef QT_NO_IMAGE_TRUECOLOR
    } else if (depth() == 32) {
        if (hasAlphaBuffer()) {
            pixel &= 0x00ffffff;
            for (int i=0; i<height(); i++) {
                uint *p = (uint *)scanLine(i);
                uint *end = p + width();
                while (p < end) {
                    *p = (*p & 0xff000000) | pixel;
                    p++;
                }
            }
        } else {
            for (int i=0; i<height(); i++) {
                uint *p = (uint *)scanLine(i);
                uint *end = p + width();
                while (p < end)
                    *p++ = pixel;
            }
        }
#endif // QT_NO_IMAGE_TRUECOLOR
    }
}

/*!
    Inverts all pixel values in the image.

    If the depth is 32: if \a mode is InvertRgba, the alpha bits are
    also inverted, otherwise they are left unchanged.

    If the depth is not 32, the argument \a mode has no meaning. The
    default mode is InvertRgb, which leaves the alpha channel
    unchanged.

    Note that inverting an 8-bit image means to replace all pixels
    using color index \e i with a pixel using color index 255 minus \e
    i. Similarly for a 1-bit image. The color table is not changed.

    \sa fill() depth() hasAlphaBuffer()
*/

void QImage::invertPixels(InvertMode mode)
{
    detach();
    int n = numBytes();
    if (depth() != 32) {
        quint8 *p = (quint8*)bits();
        quint8 *end = p + n;
        while (p < end)
            *p++ ^= 0xff;
    } else {
        quint32 *p = (quint32*)bits();
        quint32 *end = p + n/4;
        uint xorbits = 0x00ffffff;
        if (mode == InvertRgba && depth() == 32)
            xorbits = 0xffffffff;
        while (p < end)
            *p++ ^= xorbits;
    }
}

/*!
    \fn void QImage::invertPixels(bool invertAlpha)

    Use the invertPixels() function that takes a QImage::InvertMode
    parameter instead.
*/

/*! \fn QImage::Endian QImage::systemByteOrder()

    Determines the host computer byte order. Returns
    QImage::LittleEndian (LSB first) or QImage::BigEndian (MSB first).

    \sa systemBitOrder()
*/

// Windows defines these
#if defined(write)
# undef write
#endif
#if defined(close)
# undef close
#endif
#if defined(read)
# undef read
#endif

/*!
    Resizes the color table to \a numColors colors.

    If the color table is expanded all the extra colors will be set to
    black (RGB 0,0,0).

    \sa numColors() color() setColor() colorTable()
*/

void QImage::setNumColors(int numColors)
{
    detach();
    if (numColors == d->ncols)
        return;
    if (numColors == 0) {                        // use no color table
        if (d->ctbl)
            free(d->ctbl);
        d->ctbl = 0;
        d->ncols = 0;
        return;
    }
    d->ctbl = (QRgb*)realloc(d->ctbl, numColors*sizeof(QRgb));
    if (numColors > d->ncols)
        memset(d->ctbl + d->ncols, 0, (numColors-d->ncols)*sizeof(QRgb));
    d->ncols = numColors;
}


/*!
    Returns true if alpha buffer mode is enabled; otherwise returns
    false.

    \sa setAlphaBuffer()
*/
bool QImage::hasAlphaBuffer() const
{
    return d->alpha;
}


/*!
    Enables alpha buffer mode if \a enable is true, otherwise disables
    it. The default setting is disabled.

    An 8-bpp image has 8-bit pixels. A pixel is an index into the
    \link color() color table\endlink, which contains 32-bit color
    values. In a 32-bpp image, the 32-bit pixels are the color values.

    This 32-bit value is encoded as follows: The lower 24 bits are
    used for the red, green, and blue components. The upper 8 bits
    contain the alpha component.

    The alpha component specifies the transparency of a pixel. 0 means
    completely transparent and 255 means opaque. The alpha component
    is ignored if you do not enable alpha buffer mode.

    The alpha buffer is used to set a mask when a QImage is translated
    to a QPixmap.

    \sa hasAlphaBuffer() createAlphaMask()
*/

void QImage::setAlphaBuffer(bool enable)
{
    detach();
    d->alpha = enable;
}


/*!
    Sets the image \a width, \a height, \a depth, its number of colors
    (in \a numColors), and bit order. Returns true if successful, or
    false if the parameters are incorrect or if memory cannot be
    allocated.

    The \a width and \a height is limited to 32767. \a depth must be
    1, 8, or 32. If \a depth is 1, \a bitOrder must be set to
    either QImage::LittleEndian or QImage::BigEndian. For other depths
    \a bitOrder must be QImage::IgnoreEndian.

    This function allocates a color table and a buffer for the image
    data. The image data is not initialized.

    The image buffer is allocated as a single block that consists of a
    table of \link scanLine() scanline\endlink pointers (jumpTable())
    and the image data (bits()).

    \sa fill() width() height() depth() numColors() bitOrder()
    jumpTable() scanLine() bits() bytesPerLine() numBytes()
*/

bool QImage::create(int width, int height, int depth, int numColors,
                    Endian bitOrder)
{
    reset();                                        // reset old data
    if (width <= 0 || height <= 0 || depth <= 0 || numColors < 0)
        return false;                                // invalid parameter(s)
    if (depth == 1 && bitOrder == IgnoreEndian) {
        qWarning("QImage::create: Bit order is required for 1 bpp images");
        return false;
    }
    if (depth != 1)
        bitOrder = IgnoreEndian;

    if (depth == 24)
        qWarning("QImage::create: 24-bpp images no longer supported, "
                  "use 32-bpp instead");
    switch (depth) {
    case 1:
    case 8:
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
#endif
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
#endif
        break;
    default:                                // invalid depth
        return false;
    }

    if (depth == 32)
        numColors = 0;
    setNumColors(numColors);
    if (d->ncols != numColors)                // could not alloc color table
        return false;

    if (INT_MAX / depth < width) { // sanity check for potential overflow
        setNumColors(0);
        return false;
    }
// Qt/Embedded doesn't waste memory on unnecessary padding.
#ifdef Q_WS_QWS
    const int bpl = (width*depth+7)/8;                // bytes per scanline
    const int pad = 0;
#else
    const int bpl = ((width*depth+31) >> 5) << 2; // bytes per scanline (must be multiple of 8)
    const int pad = bpl - (width*depth)/8;        // pad with zeros
#endif
    if (INT_MAX / bpl < height) { // sanity check for potential overflow
        setNumColors(0);
        return false;
    }
    int nbytes = bpl*height;                        // image size
    int ptbl   = height*sizeof(uchar*);                // pointer table size
    int size   = nbytes + ptbl;                        // total size of data block
    uchar **p  = (uchar **)malloc(size);        // alloc image bits
    Q_CHECK_PTR(p);
    if (!p) {                                        // no memory
        setNumColors(0);
        return false;
    }
    d->w = width;
    d->h = height;
    d->d = depth;
    d->nbytes  = nbytes;
    d->bitordr = bitOrder;
    d->bits = p;                                // set image pointer
    //uchar *d = (uchar*)p + ptbl;                // setup scanline pointers
    uchar *d = (uchar*)(p + height);                // setup scanline pointers
    while (height--) {
        *p++ = d;
        if (pad)
            memset(d+bpl-pad, 0, pad);
        d += bpl;
    }
    return true;
}

/*!
    \fn bool QImage::create(const QSize& size, int depth, int numColors, Endian bitOrder)
    \overload

    The width and height are specified in the \a size argument.
*/
bool QImage::create(const QSize& size, int depth, int numColors,
                     QImage::Endian bitOrder)
{
    return create(size.width(), size.height(), depth, numColors, bitOrder);
}


/*****************************************************************************
  Internal routines for converting image depth.
 *****************************************************************************/

//
// convert_32_to_8:  Converts a 32 bits depth (true color) to an 8 bit
// image with a colormap. If the 32 bit image has more than 256 colors,
// we convert the red,green and blue bytes into a single byte encoded
// as 6 shades of each of red, green and blue.
//
// if dithering is needed, only 1 color at most is available for alpha.
//
#ifndef QT_NO_IMAGE_TRUECOLOR
struct QRgbMap {
    QRgbMap() : rgb(0xffffffff) { }
    bool used() const { return rgb!=0xffffffff; }
    uchar  pix;
    QRgb  rgb;
};

static bool convert_32_to_8(const QImage *src, QImage *dst, Qt::ImageConversionFlags flags, QRgb* palette=0, int palette_count=0)
{
    register QRgb *p;
    uchar  *b;
    bool    do_quant = false;
    int            y, x;

    if (!dst->create(src->width(), src->height(), 8, 256))
        return false;

    const int tablesize = 997; // prime
    QRgbMap table[tablesize];
    int   pix=0;
    QRgb amask = src->hasAlphaBuffer() ? 0xffffffff : 0x00ffffff;
    if (src->hasAlphaBuffer())
        dst->setAlphaBuffer(true);

    if (palette) {
        // Preload palette into table.

        p = palette;
        // Almost same code as pixel insertion below
        while (palette_count-- > 0) {
            // Find in table...
            int hash = (*p & amask) % tablesize;
            for (;;) {
                if (table[hash].used()) {
                    if (table[hash].rgb == (*p & amask)) {
                        // Found previous insertion - use it
                        break;
                    } else {
                        // Keep searching...
                        if (++hash == tablesize) hash = 0;
                    }
                } else {
                    // Cannot be in table
                    Q_ASSERT (pix != 256);        // too many colors
                    // Insert into table at this unused position
                    dst->setColor(pix, (*p & amask));
                    table[hash].pix = pix++;
                    table[hash].rgb = *p & amask;
                    break;
                }
            }
            p++;
        }
    }

    if ((flags & Qt::DitherMode_Mask) == Qt::PreferDither) {
        do_quant = true;
    } else {
        for (y=0; y<src->height(); y++) {        // check if <= 256 colors
            p = (QRgb *)src->scanLine(y);
            b = dst->scanLine(y);
            x = src->width();
            while (x--) {
                // Find in table...
                int hash = (*p & amask) % tablesize;
                for (;;) {
                    if (table[hash].used()) {
                        if (table[hash].rgb == (*p & amask)) {
                            // Found previous insertion - use it
                            break;
                        } else {
                            // Keep searching...
                            if (++hash == tablesize) hash = 0;
                        }
                    } else {
                        // Cannot be in table
                        if (pix == 256) {        // too many colors
                            do_quant = true;
                            // Break right out
                            x = 0;
                            y = src->height();
                        } else {
                            // Insert into table at this unused position
                            dst->setColor(pix, (*p & amask));
                            table[hash].pix = pix++;
                            table[hash].rgb = (*p & amask);
                        }
                        break;
                    }
                }
                *b++ = table[hash].pix;                // May occur once incorrectly
                p++;
            }
        }
    }
    int ncols = do_quant ? 256 : pix;

    static uint bm[16][16];
    static int init=0;
    if (!init) {
        // Build a Bayer Matrix for dithering

        init = 1;
        int n, i, j;

        bm[0][0]=0;

        for (n=1; n<16; n*=2) {
            for (i=0; i<n; i++) {
                for (j=0; j<n; j++) {
                    bm[i][j]*=4;
                    bm[i+n][j]=bm[i][j]+2;
                    bm[i][j+n]=bm[i][j]+3;
                    bm[i+n][j+n]=bm[i][j]+1;
                }
            }
        }

        for (i=0; i<16; i++)
            for (j=0; j<16; j++)
                bm[i][j]<<=8;
    }

    dst->setNumColors(ncols);

    if (do_quant) {                                // quantization needed

#define MAX_R 5
#define MAX_G 5
#define MAX_B 5
#define INDEXOF(r,g,b) (((r)*(MAX_G+1)+(g))*(MAX_B+1)+(b))

        int rc, gc, bc;

        for (rc=0; rc<=MAX_R; rc++)                // build 6x6x6 color cube
            for (gc=0; gc<=MAX_G; gc++)
                for (bc=0; bc<=MAX_B; bc++) {
                    dst->setColor(INDEXOF(rc,gc,bc),
                        (amask&0xff000000)
                        | qRgb(rc*255/MAX_R, gc*255/MAX_G, bc*255/MAX_B));
                }

        int sw = src->width();

        int* line1[3];
        int* line2[3];
        int* pv[3];
        if ((flags & Qt::Dither_Mask) == Qt::DiffuseDither) {
            line1[0] = new int[src->width()];
            line2[0] = new int[src->width()];
            line1[1] = new int[src->width()];
            line2[1] = new int[src->width()];
            line1[2] = new int[src->width()];
            line2[2] = new int[src->width()];
            pv[0] = new int[sw];
            pv[1] = new int[sw];
            pv[2] = new int[sw];
        }

        for (y=0; y < src->height(); y++) {
            p = (QRgb *)src->scanLine(y);
            b = dst->scanLine(y);
            QRgb *end = p + sw;

            // perform quantization
            if ((flags & Qt::Dither_Mask) == Qt::ThresholdDither) {
#define DITHER(p,m) ((uchar) ((p * (m) + 127) / 255))
                while (p < end) {
                    rc = qRed(*p);
                    gc = qGreen(*p);
                    bc = qBlue(*p);

                    *b++ =
                        INDEXOF(
                            DITHER(rc, MAX_R),
                            DITHER(gc, MAX_G),
                            DITHER(bc, MAX_B)
                       );

                    p++;
                }
#undef DITHER
            } else if ((flags & Qt::Dither_Mask) == Qt::OrderedDither) {
#define DITHER(p,d,m) ((uchar) ((((256 * (m) + (m) + 1)) * (p) + (d)) / 65536))

                int x = 0;
                while (p < end) {
                    uint d = bm[y&15][x&15];

                    rc = qRed(*p);
                    gc = qGreen(*p);
                    bc = qBlue(*p);

                    *b++ =
                        INDEXOF(
                            DITHER(rc, d, MAX_R),
                            DITHER(gc, d, MAX_G),
                            DITHER(bc, d, MAX_B)
                       );

                    p++;
                    x++;
                }
#undef DITHER
            } else { // Diffuse
                int endian = (QImage::systemByteOrder() == QImage::BigEndian);
                int x;
                const uchar* q = src->scanLine(y);
                const uchar* q2 = src->scanLine(y+1 < src->height() ? y + 1 : 0);
                for (int chan = 0; chan < 3; chan++) {
                    b = dst->scanLine(y);
                    int *l1 = (y&1) ? line2[chan] : line1[chan];
                    int *l2 = (y&1) ? line1[chan] : line2[chan];
                    if (y == 0) {
                        for (int i=0; i<sw; i++)
                            l1[i] = q[i*4+chan+endian];
                    }
                    if (y+1 < src->height()) {
                        for (int i=0; i<sw; i++)
                            l2[i] = q2[i*4+chan+endian];
                    }
                    // Bi-directional error diffusion
                    if (y&1) {
                        for (x=0; x<sw; x++) {
                            int pix = qMax(qMin(5, (l1[x] * 5 + 128)/ 255), 0);
                            int err = l1[x] - pix * 255 / 5;
                            pv[chan][x] = pix;

                            // Spread the error around...
                            if (x+1<sw) {
                                l1[x+1] += (err*7)>>4;
                                l2[x+1] += err>>4;
                            }
                            l2[x]+=(err*5)>>4;
                            if (x>1)
                                l2[x-1]+=(err*3)>>4;
                        }
                    } else {
                        for (x=sw; x-->0;) {
                            int pix = qMax(qMin(5, (l1[x] * 5 + 128)/ 255), 0);
                            int err = l1[x] - pix * 255 / 5;
                            pv[chan][x] = pix;

                            // Spread the error around...
                            if (x > 0) {
                                l1[x-1] += (err*7)>>4;
                                l2[x-1] += err>>4;
                            }
                            l2[x]+=(err*5)>>4;
                            if (x+1 < sw)
                                l2[x+1]+=(err*3)>>4;
                        }
                    }
                }
                if (endian) {
                    for (x=0; x<sw; x++) {
                        *b++ = INDEXOF(pv[0][x],pv[1][x],pv[2][x]);
                    }
                } else {
                    for (x=0; x<sw; x++) {
                        *b++ = INDEXOF(pv[2][x],pv[1][x],pv[0][x]);
                    }
                }
            }
        }

#ifndef QT_NO_IMAGE_DITHER_TO_1
        if (src->hasAlphaBuffer()) {
            const int trans = 216;
            dst->setColor(trans, 0x00000000);        // transparent
            QImage mask = src->createAlphaMask(flags);
            uchar* m;
            for (y=0; y < src->height(); y++) {
                uchar bit = 0x80;
                m = mask.scanLine(y);
                b = dst->scanLine(y);
                int w = src->width();
                for (x = 0; x<w; x++) {
                    if (!(*m&bit))
                        b[x] = trans;
                    if (!(bit >>= 1)) {
                        bit = 0x80;
                        while (x<w-1 && *++m == 0xff) // skip chunks
                            x+=8;
                    }
                }
            }
        }
#endif

        if ((flags & Qt::Dither_Mask) == Qt::DiffuseDither) {
            delete [] line1[0];
            delete [] line2[0];
            delete [] line1[1];
            delete [] line2[1];
            delete [] line1[2];
            delete [] line2[2];
            delete [] pv[0];
            delete [] pv[1];
            delete [] pv[2];
        }

#undef MAX_R
#undef MAX_G
#undef MAX_B
#undef INDEXOF

    }

    return true;
}


static bool convert_8_to_32(const QImage *src, QImage *dst)
{
    if (!dst->create(src->width(), src->height(), 32))
        return false;                                // create failed
    dst->setAlphaBuffer(src->hasAlphaBuffer());
    for (int y=0; y<dst->height(); y++) {        // for each scan line...
        register uint *p = (uint *)dst->scanLine(y);
        const uchar  *b = src->scanLine(y);
        uint *end = p + dst->width();
        while (p < end)
            *p++ = src->color(*b++);
    }
    return true;
}


static bool convert_1_to_32(const QImage *src, QImage *dst)
{
    if (!dst->create(src->width(), src->height(), 32))
        return false;                                // could not create
    dst->setAlphaBuffer(src->hasAlphaBuffer());
    for (int y=0; y<dst->height(); y++) {        // for each scan line...
        register uint *p = (uint *)dst->scanLine(y);
        const uchar *b = src->scanLine(y);
        int x;
        if (src->bitOrder() == QImage::BigEndian) {
            for (x=0; x<dst->width(); x++) {
                *p++ = src->color((*b >> (7 - (x & 7))) & 1);
                if ((x & 7) == 7)
                    b++;
            }
        } else {
            for (x=0; x<dst->width(); x++) {
                *p++ = src->color((*b >> (x & 7)) & 1);
                if ((x & 7) == 7)
                    b++;
            }
        }
    }
    return true;
}
#endif // QT_NO_IMAGE_TRUECOLOR

static bool convert_1_to_8(const QImage *src, QImage *dst)
{
    if (!dst->create(src->width(), src->height(), 8, 2))
        return false;                                // something failed
    dst->setAlphaBuffer(src->hasAlphaBuffer());
    if (src->numColors() >= 2) {
        dst->setColor(0, src->color(0));        // copy color table
        dst->setColor(1, src->color(1));
    } else {
        // Unlikely, but they do exist
        if (src->numColors() >= 1)
            dst->setColor(0, src->color(0));
        else
            dst->setColor(0, 0xffffffff);
        dst->setColor(1, 0xff000000);
    }
    for (int y=0; y<dst->height(); y++) {        // for each scan line...
        register uchar *p = dst->scanLine(y);
        const uchar *b = src->scanLine(y);
        int x;
        if (src->bitOrder() == QImage::BigEndian) {
            for (x=0; x<dst->width(); x++) {
                *p++ = (*b >> (7 - (x & 7))) & 1;
                if ((x & 7) == 7)
                    b++;
            }
        } else {
            for (x=0; x<dst->width(); x++) {
                *p++ = (*b >> (x & 7)) & 1;
                if ((x & 7) == 7)
                    b++;
            }
        }
    }
    return true;
}

#ifndef QT_NO_IMAGE_DITHER_TO_1
//
// dither_to_1:  Uses selected dithering algorithm.
//

static bool dither_to_1(const QImage *src, QImage *dst,
                        Qt::ImageConversionFlags flags, bool fromalpha)
{
    if (!dst->create(src->width(), src->height(), 1, 2, QImage::BigEndian))
        return false;                                // something failed

    enum { Threshold, Ordered, Diffuse } dithermode;

    if (fromalpha) {
        if ((flags & Qt::AlphaDither_Mask) == Qt::DiffuseAlphaDither)
            dithermode = Diffuse;
        else if ((flags & Qt::AlphaDither_Mask) == Qt::OrderedAlphaDither)
            dithermode = Ordered;
        else
            dithermode = Threshold;
    } else {
        if ((flags & Qt::Dither_Mask) == Qt::ThresholdDither)
            dithermode = Threshold;
        else if ((flags & Qt::Dither_Mask) == Qt::OrderedDither)
            dithermode = Ordered;
        else
            dithermode = Diffuse;
    }

    dst->setColor(0, qRgb(255, 255, 255));
    dst->setColor(1, qRgb( 0,          0,   0));
    int          w = src->width();
    int          h = src->height();
    int          d = src->depth();
    uchar gray[256];                                // gray map for 8 bit images
    bool  use_gray = d == 8;
    if (use_gray) {                                // make gray map
        if (fromalpha) {
            // Alpha 0x00 -> 0 pixels (white)
            // Alpha 0xFF -> 1 pixels (black)
            for (int i=0; i<src->numColors(); i++)
                gray[i] = (255 - (src->color(i) >> 24));
        } else {
            // Pixel 0x00 -> 1 pixels (black)
            // Pixel 0xFF -> 0 pixels (white)
            for (int i=0; i<src->numColors(); i++)
                gray[i] = qGray(src->color(i) & 0x00ffffff);
        }
    }

    switch (dithermode) {
      case Diffuse: {
        int *line1 = new int[w];
        int *line2 = new int[w];
        int bmwidth = (w+7)/8;
        if (!(line1 && line2))
            return false;
        int *b1, *b2;
        int wbytes = w * (d/8);
        register const uchar *p = src->bits();
        const uchar *end = p + wbytes;
        b2 = line2;
        if (use_gray) {                        // 8 bit image
            while (p < end)
                *b2++ = gray[*p++];
#ifndef QT_NO_IMAGE_TRUECOLOR
        } else {                                // 32 bit image
            if (fromalpha) {
                while (p < end) {
                    *b2++ = 255 - (*(uint*)p >> 24);
                    p += 4;
                }
            } else {
                while (p < end) {
                    *b2++ = qGray(*(uint*)p);
                    p += 4;
                }
            }
#endif
        }
        int x, y;
        for (y=0; y<h; y++) {                        // for each scan line...
            int *tmp = line1; line1 = line2; line2 = tmp;
            bool not_last_line = y < h - 1;
            if (not_last_line) {                // calc. grayvals for next line
                p = src->scanLine(y+1);
                end = p + wbytes;
                b2 = line2;
                if (use_gray) {                // 8 bit image
                    while (p < end)
                        *b2++ = gray[*p++];
#ifndef QT_NO_IMAGE_TRUECOLOR
                } else {                        // 24 bit image
                    if (fromalpha) {
                        while (p < end) {
                            *b2++ = 255 - (*(uint*)p >> 24);
                            p += 4;
                        }
                    } else {
                        while (p < end) {
                            *b2++ = qGray(*(uint*)p);
                            p += 4;
                        }
                    }
#endif
                }
            }

            int err;
            uchar *p = dst->scanLine(y);
            memset(p, 0, bmwidth);
            b1 = line1;
            b2 = line2;
            int bit = 7;
            for (x=1; x<=w; x++) {
                if (*b1 < 128) {                // black pixel
                    err = *b1++;
                    *p |= 1 << bit;
                } else {                        // white pixel
                    err = *b1++ - 255;
                }
                if (bit == 0) {
                    p++;
                    bit = 7;
                } else {
                    bit--;
                }
                if (x < w)
                    *b1 += (err*7)>>4;                // spread error to right pixel
                if (not_last_line) {
                    b2[0] += (err*5)>>4;        // pixel below
                    if (x > 1)
                        b2[-1] += (err*3)>>4;        // pixel below left
                    if (x < w)
                        b2[1] += err>>4;        // pixel below right
                }
                b2++;
            }
        }
        delete [] line1;
        delete [] line2;
      } break;
      case Ordered: {
        static uint bm[16][16];
        static int init=0;
        if (!init) {
            // Build a Bayer Matrix for dithering

            init = 1;
            int n, i, j;

            bm[0][0]=0;

            for (n=1; n<16; n*=2) {
                for (i=0; i<n; i++) {
                    for (j=0; j<n; j++) {
                        bm[i][j]*=4;
                        bm[i+n][j]=bm[i][j]+2;
                        bm[i][j+n]=bm[i][j]+3;
                        bm[i+n][j+n]=bm[i][j]+1;
                    }
                }
            }

            // Force black to black
            bm[0][0]=1;
        }

        dst->fill(0);
        uchar** mline = dst->jumpTable();
#ifndef QT_NO_IMAGE_TRUECOLOR
        if (d == 32) {
            uint** line = (uint**)src->jumpTable();
            for (int i=0; i<h; i++) {
                uint  *p = line[i];
                uint  *end = p + w;
                uchar *m = mline[i];
                int bit = 7;
                int j = 0;
                if (fromalpha) {
                    while (p < end) {
                        if ((*p++ >> 24) >= bm[j++&15][i&15])
                            *m |= 1 << bit;
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                } else {
                    while (p < end) {
                        if ((uint)qGray(*p++) < bm[j++&15][i&15])
                            *m |= 1 << bit;
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                }
            }
        } else
#endif // QT_NO_IMAGE_TRUECOLOR
            /* (d == 8) */ {
            const uchar* const* line = src->jumpTable();
            for (int i=0; i<h; i++) {
                const uchar *p = line[i];
                const uchar *end = p + w;
                uchar *m = mline[i];
                int bit = 7;
                int j = 0;
                while (p < end) {
                    if ((uint)gray[*p++] < bm[j++&15][i&15])
                        *m |= 1 << bit;
                    if (bit == 0) {
                        m++;
                        bit = 7;
                    } else {
                        bit--;
                    }
                }
            }
        }
      } break;
      default: { // Threshold:
        dst->fill(0);
        uchar** mline = dst->jumpTable();
#ifndef QT_NO_IMAGE_TRUECOLOR
        if (d == 32) {
            uint** line = (uint**)src->jumpTable();
            for (int i=0; i<h; i++) {
                uint  *p = line[i];
                uint  *end = p + w;
                uchar *m = mline[i];
                int bit = 7;
                if (fromalpha) {
                    while (p < end) {
                        if ((*p++ >> 24) >= 128)
                            *m |= 1 << bit;        // Set mask "on"
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                } else {
                    while (p < end) {
                        if (qGray(*p++) < 128)
                            *m |= 1 << bit;        // Set pixel "black"
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                }
            }
        } else
#endif //QT_NO_IMAGE_TRUECOLOR
            if (d == 8) {
            const uchar* const * line = src->jumpTable();
            for (int i=0; i<h; i++) {
                const uchar *p = line[i];
                const uchar *end = p + w;
                uchar *m = mline[i];
                int bit = 7;
                while (p < end) {
                    if (gray[*p++] < 128)
                        *m |= 1 << bit;                // Set mask "on"/ pixel "black"
                    if (bit == 0) {
                        m++;
                        bit = 7;
                    } else {
                        bit--;
                    }
                }
            }
        }
      }
    }
    return true;
}
#endif

#ifndef QT_NO_IMAGE_16_BIT
static inline bool is16BitGray(ushort c)
{
    int r=(c & 0xf800) >> 11;
    int g=(c & 0x07e0) >> 6; //green/2
    int b=(c & 0x001f);
    return r == g && g == b;
}


static bool convert_16_to_32(const QImage *src, QImage *dst)
{
    if (!dst->create(src->width(), src->height(), 32))
        return false;                                // create failed
    dst->setAlphaBuffer(src->hasAlphaBuffer());
    for (int y=0; y<dst->height(); y++) {        // for each scan line...
        register uint *p = (uint *)dst->scanLine(y);
        ushort  *s = (ushort*)src->scanLine(y);
        uint *end = p + dst->width();
        while (p < end)
            *p++ = qt_conv16ToRgb(*s++);
    }
    return true;
}


static bool convert_32_to_16(const QImage *src, QImage *dst)
{
    if (!dst->create(src->width(), src->height(), 16))
        return false;                                // create failed
    dst->setAlphaBuffer(src->hasAlphaBuffer());
    for (int y=0; y<dst->height(); y++) {        // for each scan line...
        register ushort *p = (ushort *)dst->scanLine(y);
        uint  *s = (uint*)src->scanLine(y);
        ushort *end = p + dst->width();
        while (p < end)
            *p++ = qt_convRgbTo16(*s++);
    }
    return true;
}


#endif

/*!
    Converts the depth (bpp) of the image to \a depth and returns the
    converted image. The original image is not changed.

    The \a depth argument must be 1, 8, 16 (Qt/Embedded only) or 32.

    Returns \c *this if \a depth is equal to the image depth, or a
    \link isNull() null\endlink image if this image cannot be
    converted.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a
    flags to specify how you'd prefer this to happen.

    \sa Qt::ImageConversionFlags depth() isNull()
*/

QImage QImage::convertDepth(int depth, Qt::ImageConversionFlags flags) const
{
    QImage image;
    if (d->d == depth)
        image = *this;                                // no conversion
#ifndef QT_NO_IMAGE_DITHER_TO_1
    else if ((d->d == 8 || d->d == 32) && depth == 1) // dither
        dither_to_1(this, &image, flags, false);
#endif
#ifndef QT_NO_IMAGE_TRUECOLOR
    else if (d->d == 32 && depth == 8)        // 32 -> 8
        convert_32_to_8(this, &image, flags);
    else if (d->d == 8 && depth == 32)        // 8 -> 32
        convert_8_to_32(this, &image);
#endif
    else if (d->d == 1 && depth == 8)        // 1 -> 8
        convert_1_to_8(this, &image);
#ifndef QT_NO_IMAGE_TRUECOLOR
    else if (d->d == 1 && depth == 32)        // 1 -> 32
        convert_1_to_32(this, &image);
#endif
#ifndef QT_NO_IMAGE_16_BIT
    else if (d->d == 16 && depth != 16) {
        QImage tmp;
        convert_16_to_32(this, &tmp);
        image = tmp.convertDepth(depth, flags);
    } else if (d->d != 16 && depth == 16) {
        QImage tmp = convertDepth(32, flags);
        convert_32_to_16(&tmp, &image);
    }
#endif
    else {
        if (isNull())
            qWarning("QImage::convertDepth: Image is a null image");
        else
            qWarning("QImage::convertDepth: Depth %d not supported", depth);
    }
    return image;
}

/*!
    \overload
*/

QImage QImage::convertDepth(int depth) const
{
    return convertDepth(depth, 0);
}

/*!
    Returns true if (\a x, \a y) is a valid coordinate in the image;
    otherwise returns false.

    \sa width() height() pixelIndex()
*/

bool QImage::valid(int x, int y) const
{
    return x >= 0 && x < width()
        && y >= 0 && y < height();
}

/*!
    Returns the pixel index at the given coordinates.

    If (\a x, \a y) is not \link valid() valid\endlink, or if the
    image is not a paletted image (depth() \> 8), the results are
    undefined.

    \sa valid() depth()
*/

int QImage::pixelIndex(int x, int y) const
{
    if (x < 0 || x >= width()) {
        qWarning("QImage::pixel: x=%d out of range", x);
        return -12345;
    }
    const uchar * s = scanLine(y);
    switch(depth()) {
    case 1:
        if (bitOrder() == QImage::LittleEndian)
            return (*(s + (x >> 3)) >> (x & 7)) & 1;
        else
            return (*(s + (x >> 3)) >> (7- (x & 7))) & 1;
    case 8:
        return (int)s[x];
#ifndef QT_NO_IMAGE_TRUECOLOR
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
#endif
    case 32:
        qWarning("QImage::pixelIndex: Not applicable for %d-bpp images (no palette)", depth());
        return 0;
#endif //QT_NO_IMAGE_TRUECOLOR
    }
    return 0;
}


/*!
    Returns the color of the pixel at the coordinates (\a x, \a y).

    If (\a x, \a y) is not \link valid() on the image\endlink, the
    results are undefined.

    \sa setPixel() qRed() qGreen() qBlue() valid()
*/

QRgb QImage::pixel(int x, int y) const
{
    if (x < 0 || x >= width()) {
        qWarning("QImage::pixel: x=%d out of range", x);
        return 12345;
    }
    const uchar * s = scanLine(y);
    switch(depth()) {
    case 1:
        if (bitOrder() == QImage::LittleEndian)
            return color((*(s + (x >> 3)) >> (x & 7)) & 1);
        else
            return color((*(s + (x >> 3)) >> (7- (x & 7))) & 1);
    case 8:
        return color((int)s[x]);
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
        return qt_conv16ToRgb(((ushort*)s)[x]);
#endif
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
        return ((QRgb*)s)[x];
#endif
    default:
        return 100367;
    }
}


/*!
    Sets the pixel index or color at the coordinates (\a x, \a y) to
    \a index_or_rgb.

    If (\a x, \a y) is not \link valid() valid\endlink, the result is
    undefined.

    If the image is a paletted image (depth() \<= 8) and \a
    index_or_rgb \>= numColors(), the result is undefined.

    \sa pixelIndex() pixel() qRgb() qRgba() valid()
*/

void QImage::setPixel(int x, int y, uint index_or_rgb)
{
    detach();
    if (x < 0 || x >= width()) {
        qWarning("QImage::setPixel: x=%d out of range", x);
        return;
    }
    if (depth() == 1) {
        uchar * s = scanLine(y);
        if (index_or_rgb > 1) {
            qWarning("QImage::setPixel: index=%d out of range",
                     index_or_rgb);
        } else if (bitOrder() == QImage::LittleEndian) {
            if (index_or_rgb==0)
                *(s + (x >> 3)) &= ~(1 << (x & 7));
            else
                *(s + (x >> 3)) |= (1 << (x & 7));
        } else {
            if (index_or_rgb==0)
                *(s + (x >> 3)) &= ~(1 << (7-(x & 7)));
            else
                *(s + (x >> 3)) |= (1 << (7-(x & 7)));
        }
    } else if (depth() == 8) {
        if (index_or_rgb > (uint)numColors()) {
            qWarning("QImage::setPixel: index=%d out of range",
                     index_or_rgb);
            return;
        }
        uchar * s = scanLine(y);
        s[x] = index_or_rgb;
#ifndef QT_NO_IMAGE_16_BIT
    } else if (depth() == 16) {
        ushort * s = (ushort*)scanLine(y);
        s[x] = qt_convRgbTo16(index_or_rgb);
#endif
#ifndef QT_NO_IMAGE_TRUECOLOR
    } else if (depth() == 32) {
        QRgb * s = (QRgb*)scanLine(y);
        s[x] = index_or_rgb;
#endif
    }
}


/*!
    Converts the bit order of the image to \a bitOrder and returns the
    converted image. The original image is not changed.

    Returns \c *this if the \a bitOrder is equal to the image bit
    order, or a \link isNull() null\endlink image if this image cannot
    be converted.

    \sa bitOrder() systemBitOrder() isNull()
*/

QImage QImage::convertBitOrder(Endian bitOrder) const
{
    if (isNull() || d->d != 1 ||                // invalid argument(s)
         !(bitOrder == BigEndian || bitOrder == LittleEndian)) {
        QImage nullImage;
        return nullImage;
    }
    if (d->bitordr == bitOrder)                // nothing to do
        return copy();

    QImage image(d->w, d->h, 1, d->ncols, bitOrder);

    int bpl = (width() + 7) / 8;
    for (int y = 0; y < d->h; y++) {
        register const uchar *p = jumpTable()[y];
        const uchar *end = p + bpl;
        uchar *b = image.jumpTable()[y];
        while (p < end)
            *b++ = bitflip[*p++];
    }

    image.setDotsPerMeterX(dotsPerMeterX());
    image.setDotsPerMeterY(dotsPerMeterY());

    memcpy(image.colorTable(), colorTable(), numColors()*sizeof(QRgb));
    return image;
}

/*!
    Returns true if all the colors in the image are shades of gray
    (i.e. their red, green and blue components are equal); otherwise
    returns false.

    This function is slow for large 16-bit (Qt/Embedded only) and 32-bit images.

    \sa isGrayscale()
*/
bool QImage::allGray() const
{
#ifndef QT_NO_IMAGE_TRUECOLOR
    if (depth()==32) {
        int p = width()*height();
        QRgb* b = (QRgb*)bits();
        while (p--)
            if (!qIsGray(*b++))
                return false;
#ifndef QT_NO_IMAGE_16_BIT
    } else if (depth()==16) {
        int p = width()*height();
        ushort* b = (ushort*)bits();
        while (p--)
            if (!is16BitGray(*b++))
                return false;
#endif
    } else
#endif //QT_NO_IMAGE_TRUECOLOR
    {
        if (!d->ctbl) return true;
        for (int i=0; i<numColors(); i++)
            if (!qIsGray(d->ctbl[i]))
                return false;
    }
    return true;
}

/*!
    For 16-bit (Qt/Embedded only) and 32-bit images, this function is
    equivalent to allGray().

    For 8-bpp images, this function returns true if color(i) is
    QRgb(i,i,i) for all indexes of the color table; otherwise returns
    false.

    \sa allGray() depth()
*/
bool QImage::isGrayscale() const
{
    switch (depth()) {
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
#ifndef QT_NO_IMAGE_16_BIT
    case 16:
#endif
        return allGray();
#endif //QT_NO_IMAGE_TRUECOLOR
    case 8: {
        for (int i=0; i<numColors(); i++)
            if (d->ctbl[i] != qRgb(i,i,i))
                return false;
        return true;
        }
    }
    return false;
}


/*!
    \fn QImage QImage::smoothScale(int width, int height, Qt::AspectRatioMode mode) const

    Use scale(\a width, \a height, \a mode, Qt::SmoothTransformation) instead.
*/

/*!
    \fn QImage QImage::smoothScale(const QSize &size, Qt::AspectRatioMode mode) const
    \overload

    Use scale(\a size, \a mode, Qt::SmoothTransformation) instead.
*/

/*!
    \fn QImage QImage::scale(int w, int h, Qt::AspectRatioMode aspectRatioMode,
                             Qt::TransformationMode transformMode) const

    Returns a copy of the image scaled to a rectangle of width \a w
    and height \a h according to \a aspectRatioMode and \a transformMode.

    \list
    \i If \a aspectRatioMode is \c Qt::IgnoreAspectRatio, the image
       is scaled to (\a w, \a h).
    \i If \a aspectRatioMode is \c Qt::KeepAspectRatio, the image is
       scaled to a rectangle as large as possible inside (\a w, \a
       h), preserving the aspect ratio.
    \i If \a aspectRatioMode is \c Qt::KeepAspectRatioByExpanding,
       the image is scaled to a rectangle as small as possible
       outside (\a w, \a h), preserving the aspect ratio.
    \endlist

    If either the width \a w or the height \a h is zero or negative,
    this function returns a \l{isNull()}{null} image.

    \sa scaleWidth() scaleHeight() transform()
*/

/*!
    \fn QImage QImage::scale(const QSize &size, Qt::AspectRatioMode aspectMode, Qt::TransformationMode transformMode) const

    \overload

    Scales the image to the given \a size, using the aspect ratio and
    transformation modes specified by \a aspectMode and \a transformMode.
*/
#ifndef QT_NO_IMAGE_TRANSFORMATION
QImage QImage::scale(const QSize& s, Qt::AspectRatioMode aspectMode, Qt::TransformationMode mode) const
{
    if (isNull()) {
        qWarning("QImage::scale: Image is a null image");
        return copy();
    }
    if (s.isEmpty())
        return QImage();

    QSize newSize = size();
    newSize.scale(s, aspectMode);
    if (newSize == size())
        return copy();

    QImage img;
    QMatrix wm;
    wm.scale((double)newSize.width() / width(), (double)newSize.height() / height());
    img = transform(wm, mode);
    return img;
}
#endif

/*!
    Returns a scaled copy of the image. The returned image has a width
    of \a w pixels. This function automatically calculates the height
    of the image so that the ratio of the image is preserved.

    If \a w is 0 or negative a \link isNull() null\endlink image is
    returned.

    \sa scale() scaleHeight() transform()
*/
#ifndef QT_NO_IMAGE_TRANSFORMATION
QImage QImage::scaleWidth(int w) const
{
    if (isNull()) {
        qWarning("QImage::scaleWidth: Image is a null image");
        return copy();
    }
    if (w <= 0)
        return QImage();

    QMatrix wm;
    double factor = (double) w / width();
    wm.scale(factor, factor);
    return transform(wm);
}
#endif

/*!
    Returns a scaled copy of the image. The returned image has a
    height of \a h pixels. This function automatically calculates the
    width of the image so that the ratio of the image is preserved.

    If \a h is 0 or negative a \link isNull() null\endlink image is
    returned.

    \sa scale() scaleWidth() transform()
*/
#ifndef QT_NO_IMAGE_TRANSFORMATION
QImage QImage::scaleHeight(int h) const
{
    if (isNull()) {
        qWarning("QImage::scaleHeight: Image is a null image");
        return copy();
    }
    if (h <= 0)
        return QImage();

    QMatrix wm;
    double factor = (double) h / height();
    wm.scale(factor, factor);
    return transform(wm);
}
#endif


/*!
    Returns the actual matrix used for transforming a image with \a w
    width and \a h height and matrix \a matrix.

    When transforming a image with transform(), the transformation matrix
    is internally adjusted to compensate for unwanted translation,
    i.e. transform() returns the smallest image containing all
    transformed points of the original image.

    This function returns the modified matrix, which maps points
    correctly from the original image into the new image.

    \sa transform(), QMatrix
*/
#ifndef QT_NO_PIXMAP_TRANSFORMATION
QMatrix QImage::trueMatrix(const QMatrix &matrix, int w, int h)
{
    const qreal dt = qreal(0.);
    qreal x1,y1, x2,y2, x3,y3, x4,y4;                // get corners
    qreal xx = qreal(w);
    qreal yy = qreal(h);

    QMatrix mat(matrix.m11(), matrix.m12(), matrix.m21(), matrix.m22(), 0., 0.);

    mat.map(dt, dt, &x1, &y1);
    mat.map(xx, dt, &x2, &y2);
    mat.map(xx, yy, &x3, &y3);
    mat.map(dt, yy, &x4, &y4);

    qreal ymin = y1;                                // lowest y value
    if (y2 < ymin) ymin = y2;
    if (y3 < ymin) ymin = y3;
    if (y4 < ymin) ymin = y4;
    qreal xmin = x1;                                // lowest x value
    if (x2 < xmin) xmin = x2;
    if (x3 < xmin) xmin = x3;
    if (x4 < xmin) xmin = x4;

    qreal ymax = y1;                                // lowest y value
    if (y2 > ymax) ymax = y2;
    if (y3 > ymax) ymax = y3;
    if (y4 > ymax) ymax = y4;
    qreal xmax = x1;                                // lowest x value
    if (x2 > xmax) xmax = x2;
    if (x3 > xmax) xmax = x3;
    if (x4 > xmax) xmax = x4;

    if (xmax-xmin > 1.0)
        xmin -= xmin/(xmax-xmin);
    if (ymax-ymin > 1.0)
        ymin -= ymin/(ymax-ymin);

    mat.setMatrix(matrix.m11(), matrix.m12(), matrix.m21(), matrix.m22(), -xmin, -ymin);
    return mat;
}
#endif // QT_NO_WMATRIX

static QImage smoothXForm(const QImageData *src, const QMatrix &matrix);

/*!
    Returns a copy of the image that is transformed with the transformation
    matrix specified by \a matrix and using the transformation mode specified
    by \a mode.

    The transformation \a matrix is internally adjusted to compensate
    for unwanted translation; i.e. the image produced is the smallest image
    that contains all the transformed points of the original image.

    \sa scale() QPixmap::transform() QPixmap::trueMatrix() QMatrix
*/
#ifndef QT_NO_IMAGE_TRANSFORMATION
QImage QImage::transform(const QMatrix &matrix, Qt::TransformationMode mode) const
{
    // This function uses the same algorithm as (and steals quite some
    // code from) QPixmap::transform().

    if (isNull())
        return *this;

    if (depth() == 16) {
        // inefficient
        return convertDepth(32).transform(matrix, mode);
    }

    int bpp = depth();

    if (bpp == 32 && mode == Qt::SmoothTransformation)
        return smoothXForm(this->d, matrix);

    // source image data
    int ws = width();
    int hs = height();
    int sbpl = bytesPerLine();
    const uchar *sptr = bits();

    // target image data
    int wd;
    int hd;


    // compute size of target image
    QMatrix mat = trueMatrix(matrix, ws, hs);
    if (mat.m12() == 0.0F && mat.m21() == 0.0F) {
        if (mat.m11() == 1.0F && mat.m22() == 1.0F) // identity matrix
            return copy();
        hd = qRound(mat.m22() * hs);
        wd = qRound(mat.m11() * ws);
        hd = qAbs(hd);
        wd = qAbs(wd);
    } else {                                        // rotation or shearing
        QPolygon a(QRect(0, 0, ws, hs));
        a = mat.map(a);
        QRect r = a.boundingRect().normalize();
        wd = r.width();
        hd = r.height();
    }

    bool invertible;
    mat = mat.inverted(&invertible);                // invert matrix
    if (hd == 0 || wd == 0 || !invertible)        // error, return null image
        return QImage();

    // create target image (some of the code is from QImage::copy())
    QImage dImage(wd, hd, depth(), numColors(), bitOrder());
    memcpy(dImage.colorTable(), colorTable(), numColors()*sizeof(QRgb));
    dImage.setAlphaBuffer(hasAlphaBuffer());
    dImage.d->dpmx = dotsPerMeterX();
    dImage.d->dpmy = dotsPerMeterY();

    switch (bpp) {
        // initizialize the data
        case 1:
            memset(dImage.bits(), 0, dImage.numBytes());
            break;
        case 8:
            if (dImage.d->ncols < 256) {
                // colors are left in the color table, so pick that one as transparent
                dImage.setNumColors(dImage.d->ncols+1);
                dImage.setColor(dImage.d->ncols-1, 0x00);
                memset(dImage.bits(), dImage.d->ncols-1, dImage.numBytes());
            } else {
                memset(dImage.bits(), 0, dImage.numBytes());
            }
            break;
        case 16:
            memset(dImage.bits(), 0xff, dImage.numBytes());
            break;
        case 32:
            memset(dImage.bits(), 0x00, dImage.numBytes());
            break;
    }

    int type;
    if (bitOrder() == BigEndian)
        type = QT_XFORM_TYPE_MSBFIRST;
    else
        type = QT_XFORM_TYPE_LSBFIRST;
    int dbpl = dImage.bytesPerLine();
    qt_xForm_helper(mat, 0, type, bpp, dImage.bits(), dbpl, 0, hd, sptr, sbpl, ws, hs);
    return dImage;
}
#endif

/*!
    Builds and returns a 1-bpp mask from the alpha buffer in this
    image. Returns a \link isNull() null\endlink image if \link
    setAlphaBuffer() alpha buffer mode\endlink is disabled.

    See QPixmap::convertFromImage() for a description of the \a
    flags argument.

    The returned image has little-endian bit order, which you can
    convert to big-endianness using convertBitOrder().

    \sa createHeuristicMask() hasAlphaBuffer() setAlphaBuffer()
*/
#ifndef QT_NO_IMAGE_DITHER_TO_1
QImage QImage::createAlphaMask(Qt::ImageConversionFlags flags) const
{
    if (isNull() || !hasAlphaBuffer())
        return QImage();

    if (depth() == 1) {
        // A monochrome pixmap, with alpha channels on those two colors.
        // Pretty unlikely, so use less efficient solution.
        return convertDepth(8, flags)
                .createAlphaMask(flags);
    }

    QImage mask1;
    dither_to_1(this, &mask1, flags, true);
    return mask1;
}
#endif

#ifndef QT_NO_IMAGE_HEURISTIC_MASK
/*!
    Creates and returns a 1-bpp heuristic mask for this image. It
    works by selecting a color from one of the corners, then chipping
    away pixels of that color starting at all the edges.

    The four corners vote for which color is to be masked away. In
    case of a draw (this generally means that this function is not
    applicable to the image), the result is arbitrary.

    The returned image has little-endian bit order, which you can
    convert to big-endianness using convertBitOrder().

    If \a clipTight is true the mask is just large enough to cover the
    pixels; otherwise, the mask is larger than the data pixels.

    This function disregards the \link hasAlphaBuffer() alpha buffer
    \endlink.

    \sa createAlphaMask()
*/

QImage QImage::createHeuristicMask(bool clipTight) const
{
    if (isNull()) {
        QImage nullImage;
        return nullImage;
    }
    if (depth() != 32) {
        QImage img32 = convertDepth(32);
        return img32.createHeuristicMask(clipTight);
    }

#define PIX(x,y)  (*((QRgb*)scanLine(y)+x) & 0x00ffffff)

    int w = width();
    int h = height();
    QImage m(w, h, 1, 2, QImage::LittleEndian);
    m.setColor(0, 0xffffff);
    m.setColor(1, 0);
    m.fill(0xff);

    QRgb background = PIX(0,0);
    if (background != PIX(w-1,0) &&
         background != PIX(0,h-1) &&
         background != PIX(w-1,h-1)) {
        background = PIX(w-1,0);
        if (background != PIX(w-1,h-1) &&
             background != PIX(0,h-1) &&
             PIX(0,h-1) == PIX(w-1,h-1)) {
            background = PIX(w-1,h-1);
        }
    }

    int x,y;
    bool done = false;
    uchar *ypp, *ypc, *ypn;
    while(!done) {
        done = true;
        ypn = m.scanLine(0);
        ypc = 0;
        for (y = 0; y < h; y++) {
            ypp = ypc;
            ypc = ypn;
            ypn = (y == h-1) ? 0 : m.scanLine(y+1);
            QRgb *p = (QRgb *)scanLine(y);
            for (x = 0; x < w; x++) {
                // slowness here - it's possible to do six of these tests
                // together in one go. oh well.
                if ((x == 0 || y == 0 || x == w-1 || y == h-1 ||
                       !(*(ypc + ((x-1) >> 3)) & (1 << ((x-1) & 7))) ||
                       !(*(ypc + ((x+1) >> 3)) & (1 << ((x+1) & 7))) ||
                       !(*(ypp + (x     >> 3)) & (1 << (x     & 7))) ||
                       !(*(ypn + (x     >> 3)) & (1 << (x     & 7)))) &&
                     (       (*(ypc + (x     >> 3)) & (1 << (x     & 7)))) &&
                     ((*p & 0x00ffffff) == background)) {
                    done = false;
                    *(ypc + (x >> 3)) &= ~(1 << (x & 7));
                }
                p++;
            }
        }
    }

    if (!clipTight) {
        ypn = m.scanLine(0);
        ypc = 0;
        for (y = 0; y < h; y++) {
            ypp = ypc;
            ypc = ypn;
            ypn = (y == h-1) ? 0 : m.scanLine(y+1);
            QRgb *p = (QRgb *)scanLine(y);
            for (x = 0; x < w; x++) {
                if ((*p & 0x00ffffff) != background) {
                    if (x > 0)
                        *(ypc + ((x-1) >> 3)) |= (1 << ((x-1) & 7));
                    if (x < w-1)
                        *(ypc + ((x+1) >> 3)) |= (1 << ((x+1) & 7));
                    if (y > 0)
                        *(ypp + (x >> 3)) |= (1 << (x & 7));
                    if (y < h-1)
                        *(ypn + (x >> 3)) |= (1 << (x & 7));
                }
                p++;
            }
        }
    }

#undef PIX

    return m;
}
#endif //QT_NO_IMAGE_HEURISTIC_MASK

#ifndef QT_NO_IMAGE_MIRROR
/*
  This code is contributed by Philipp Lang,
  GeneriCom Software Germany (www.generi.com)
  under the terms of the QPL, Version 1.0
*/

/*!
    \overload

    Returns a mirror of the image, mirrored in the horizontal and/or
    the vertical direction depending on whether \a horizontal and \a
    vertical are set to true or false. The original image is not
    changed.

    \sa scale()
*/
QImage QImage::mirror(bool horizontal, bool vertical) const
{
    int w = width();
    int h = height();
    if ((w <= 1 && h <= 1) || (!horizontal && !vertical))
        return copy();

    // Create result image, copy colormap
    QImage result(w, h, depth(), numColors(), bitOrder());
    memcpy(result.colorTable(), colorTable(), numColors()*sizeof(QRgb));
    result.setAlphaBuffer(hasAlphaBuffer());

    if (depth() == 1)
        w = (w+7)/8;
    int dxi = horizontal ? -1 : 1;
    int dxs = horizontal ? w-1 : 0;
    int dyi = vertical ? -1 : 1;
    int dy = vertical ? h-1: 0;

    // 1 bit, 8 bit
    if (depth() == 1 || depth() == 8) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            quint8* ssl = (quint8*)(d->bits[sy]);
            quint8* dsl = (quint8*)(result.d->bits[dy]);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
#ifndef QT_NO_IMAGE_TRUECOLOR
#ifndef QT_NO_IMAGE_16_BIT
    // 16 bit
    else if (depth() == 16) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            quint16* ssl = (quint16*)(d->bits[sy]);
            quint16* dsl = (quint16*)(result.d->bits[dy]);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
#endif
    // 32 bit
    else if (depth() == 32) {
        for (int sy = 0; sy < h; sy++, dy += dyi) {
            quint32* ssl = (quint32*)(d->bits[sy]);
            quint32* dsl = (quint32*)(result.d->bits[dy]);
            int dx = dxs;
            for (int sx = 0; sx < w; sx++, dx += dxi)
                dsl[dx] = ssl[sx];
        }
    }
#endif

    // special handling of 1 bit images for horizontal mirroring
    if (horizontal && depth() == 1) {
        int shift = width() % 8;
        for (int y = h-1; y >= 0; y--) {
            quint8* a0 = (quint8*)(result.d->bits[y]);
            // Swap bytes
            quint8* a = a0+dxs;
            while (a >= a0) {
                *a = bitflip[*a];
                a--;
            }
            // Shift bits if unaligned
            if (shift != 0) {
                a = a0+dxs;
                quint8 c = 0;
                if (bitOrder() == QImage::LittleEndian) {
                    while (a >= a0) {
                        quint8 nc = *a << shift;
                        *a = (*a >> (8-shift)) | c;
                        --a;
                        c = nc;
                    }
                } else {
                    while (a >= a0) {
                        quint8 nc = *a >> shift;
                        *a = (*a << (8-shift)) | c;
                        --a;
                        c = nc;
                    }
                }
            }
        }
    }

    return result;
}

/*!
    Returns a QImage which is a vertically mirrored copy of this
    image. The original QImage is not changed.
*/

QImage QImage::mirror() const
{
    return mirror(false,true);
}
#endif //QT_NO_IMAGE_MIRROR

/*!
    Returns a QImage in which the values of the red and blue
    components of all pixels have been swapped, effectively converting
    an RGB image to a BGR image. The original QImage is not changed.
*/

QImage QImage::swapRGB() const
{
    QImage res = copy();
    if (!isNull()) {
#ifndef QT_NO_IMAGE_TRUECOLOR
        if (depth() == 32) {
            for (int i=0; i < height(); i++) {
                uint *p = (uint*)scanLine(i);
                uint *q = (uint*)res.scanLine(i);
                uint *end = p + width();
                while (p < end) {
                    *q = ((*p << 16) & 0xff0000) | ((*p >> 16) & 0xff) |
                         (*p & 0xff00ff00);
                    p++;
                    q++;
                }
            }
#ifndef QT_NO_IMAGE_16_BIT
        } else if (depth() == 16) {
            qWarning("QImage::swapRGB not implemented for 16bpp");
#endif
        } else
#endif //QT_NO_IMAGE_TRUECOLOR
            {
            uint* p = (uint*)colorTable();
            uint* q = (uint*)res.colorTable();
            if (p && q) {
                for (int i=0; i < numColors(); i++) {
                    *q = ((*p << 16) & 0xff0000) | ((*p >> 16) & 0xff) |
                         (*p & 0xff00ff00);
                    p++;
                    q++;
                }
            }
        }
    }
    return res;
}

#ifndef QT_NO_IMAGEIO
/*!
    Loads an image from the file \a fileName. Returns true if the
    image was successfully loaded; otherwise returns false.

    If \a format is specified, the loader attempts to read the image
    using the specified format. If \a format is not specified (which
    is the default), the loader reads a few bytes from the header to
    guess the file format.

    The QImageReader documentation lists the supported image formats and
    explains how to add extra formats.

    The file name can be either refer to an actual file on disk or to
    one of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how
    to embed images and other resource files in the application's
    executable.

    \sa loadFromData() save() imageFormat() QPixmap::load() QImageReader
*/

bool QImage::load(const QString &fileName, const char* format)
{
    QImage image = QImageReader(fileName, format).read();
    if (!image.isNull()) {
        operator=(image);
        return true;
    }
    return false;
}

/*!
    Loads an image from the first \a len bytes of binary data in \a
    data. Returns true if the image was successfully loaded; otherwise
    returns false.

    If \a format is specified, the loader attempts to read the image
    using the specified format. If \a format is not specified (which
    is the default), the loader reads a few bytes from the header to
    guess the file format.

    The QImageReader documentation lists the supported image formats and
    explains how to add extra formats.

    \sa load() save() imageFormat() QPixmap::loadFromData() QImageReader
*/

bool QImage::loadFromData(const uchar *data, int len, const char *format)
{
    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(data), len);
    return loadFromData(a, format);
}

/*!
    \overload

    Loads an image from the QByteArray \a data.
*/
bool QImage::loadFromData(const QByteArray &data, const char *format)
{
    QBuffer b;
    b.setData(data);
    b.open(QIODevice::ReadOnly);
    QImage image = QImageReader(&b, format).read();
    if (!image.isNull()) {
        operator=(image);
        return true;
    }
    return false;
}

/*!
    \fn QImage QImage::fromData(const uchar *data, int size, const char *format)

    Constructs a QImage from the first \a size bytes of binary data
    in \a data. If the loading of the image failed, this object is a \link
    isNull() null\endlink image.

    If \a format is specified, the loader attempts to read the image
    using the specified format. If \a format is not specified (which
    is the default), the loader reads a few bytes from the header to
    guess the file format.

    The QImageReader documentation lists the supported image formats and
    explains how to add extra formats.

    \sa load() save() imageFormat() QPixmap::loadFromData() QImageReader
*/

/*!
    \fn QImage QImage::fromData(const QByteArray &data, const char *format)

    \overload

    Loads an image from the QByteArray \a data.
*/

/*!
    Saves the image to the file \a fileName, using the image file
    format \a format and a quality factor of \a quality. \a quality
    must be in the range 0 to 100 or -1. Specify 0 to obtain small
    compressed files, 100 for large uncompressed files, and -1 (the
    default) to use the default settings.

    Returns true if the image was successfully saved; otherwise
    returns false.

    \sa load() loadFromData() imageFormat() QPixmap::save() QImageReader
*/
bool QImage::save(const QString &fileName, const char *format, int quality) const
{
    if (isNull())
        return false;
    QImageWriter writer(fileName, format);
    return d->doImageIO(this, &writer, quality);
}

/*!
    \overload

    This function writes a QImage to the QIODevice, \a device. This
    can be used, for example, to save an image directly into a
    QByteArray:

    \code
        QImage image;
        QByteArray ba;
        QBuffer buffer(ba);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG"); // writes image into ba in PNG format
    \endcode
*/

bool QImage::save(QIODevice* device, const char* format, int quality) const
{
    if (isNull())
        return false;                                // nothing to save
    QImageWriter writer(device, format);
    return d->doImageIO(this, &writer, quality);
}

/* \internal
*/

bool QImageData::doImageIO(const QImage *image, QImageWriter *writer, int quality) const
{
    if (quality > 100  || quality < -1)
        qWarning("QPixmap::save: quality out of range [-1,100]");
    if (quality >= 0)
        writer->setQuality(qMin(quality,100));
    return writer->write(*image);
}
#endif //QT_NO_IMAGEIO

/*****************************************************************************
  QImage stream functions
 *****************************************************************************/
#if !defined(QT_NO_DATASTREAM) && !defined(QT_NO_IMAGEIO)
/*!
    \relates QImage

    Writes the image \a image to the stream \a s as a PNG image, or as a
    BMP image if the stream's version is 1.

    Note that writing the stream to a file will not produce a valid image file.

    \sa QImage::save()
    \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<(QDataStream &s, const QImage &image)
{
    if (s.version() >= 5) {
        if (image.isNull()) {
            s << (qint32) 0; // null image marker
            return s;
        } else {
            s << (qint32) 1;
            // continue ...
        }
    }
    QImageWriter writer(s.device(), s.version() == 1 ? "bmp" : "png");
    writer.write(image);
    return s;
}

/*!
    \relates QImage

    Reads an image from the stream \a s and stores it in \a image.

    \sa QImage::load()
    \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>(QDataStream &s, QImage &image)
{
    if (s.version() >= 5) {
        qint32 nullMarker;
        s >> nullMarker;
        if (!nullMarker) {
            image = QImage(); // null image
            return s;
        }
    }
    image = QImageReader(s.device(), 0).read();
    return s;
}
#endif


/*!
    Returns an image with depth \a d, using the \a palette_count
    colors pointed to by \a palette. If \a d is 1 or 8, the returned
    image will have its color table ordered the same as \a palette.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a
    flags to specify how you'd prefer this to happen.

    Note: currently no closest-color search is made. If colors are
    found that are not in the palette, the palette may not be used at
    all. This result should not be considered valid because it may
    change in future implementations.

    Currently inefficient for non-32-bit images.

    \sa Qt::ImageConversionFlags
*/
#ifndef QT_NO_IMAGE_TRUECOLOR
QImage QImage::convertDepthWithPalette(int d, QRgb* palette, int palette_count, Qt::ImageConversionFlags flags) const
{
    if (depth() == 1) {
        return convertDepth(8, flags)
               .convertDepthWithPalette(d, palette, palette_count, flags);
    } else if (depth() == 8) {
        return convertDepth(32, flags)
               .convertDepthWithPalette(d, palette, palette_count, flags);
    } else {
        QImage result;
        convert_32_to_8(this, &result,
            (flags&~Qt::DitherMode_Mask) | Qt::AvoidDither,
            palette, palette_count);
        return result.convertDepth(d);
    }
}
#endif

static bool haveSamePalette(const QImage& a, const QImage& b)
{
    if (a.depth() != b.depth()) return false;
    if (a.numColors() != b.numColors()) return false;
    const QRgb* ca = a.colorTable();
    const QRgb* cb = b.colorTable();
    for (int i=a.numColors(); i--;)
        if (*ca++ != *cb++)
            return false;
    return true;
}

/*!
    \relates QImage

    Copies a block of pixels from \a src to \a dst. The pixels
    copied from source (src) are converted according to
    \a flags if it is incompatible with the destination
    (\a dst).

    \a sx, \a sy is the top-left pixel in \a src, \a dx, \a dy
    is the top-left position in \a dst and \a sw, \a sh is the
    size of the copied block.

    The copying is clipped if areas outside \a src or \a dst are
    specified.

    If \a sw is -1, it is adjusted to src->width(). Similarly, if \a
    sh is -1, it is adjusted to src->height().

    Currently inefficient for non 32-bit images.
*/
void bitBlt(QImage *dst, int dx, int dy, const QImage *src, int sx, int sy, int sw, int sh,
            Qt::ImageConversionFlags flags)
{
    dst->detach();

    // Parameter correction
    if (sw < 0) sw = src->width();
    if (sh < 0) sh = src->height();
    if (sx < 0) { dx -= sx; sw += sx; sx = 0; }
    if (sy < 0) { dy -= sy; sh += sy; sy = 0; }
    if (dx < 0) { sx -= dx; sw += dx; dx = 0; }
    if (dy < 0) { sy -= dy; sh += dy; dy = 0; }
    if (sx + sw > src->width()) sw = src->width() - sx;
    if (sy + sh > src->height()) sh = src->height() - sy;
    if (dx + sw > dst->width()) sw = dst->width() - dx;
    if (dy + sh > dst->height()) sh = dst->height() - dy;
    if (sw <= 0 || sh <= 0) return; // Nothing left to copy
    if ((dst->d == src->d) && dx==sx && dy==sy) return; // Same pixels

    // "Easy" to copy if both same depth and one of:
    //   - 32 bit
    //   - 8 bit, identical palette
    //   - 1 bit, identical palette and byte-aligned area
    //
    bool byteAligned = true;
    if (dst->depth() == 1)
        byteAligned = !(dx & 7) && !(sx & 7) && !((sw & 7) && (sx+sw < src->width()) || (dx+sw < dst->width()));

    if (!byteAligned && !haveSamePalette(*dst,*src) && dst->depth() != 32) {
        QImage dstconv = dst->convertDepth(32);
        bitBlt(&dstconv, dx, dy, src, sx, sy, sw, sh, (flags&~Qt::DitherMode_Mask) | Qt::AvoidDither);
        *dst = dstconv.convertDepthWithPalette(dst->depth(), dst->colorTable(), dst->numColors());
        return;
    }

    // Now assume palette can be ignored

    if (dst->depth() != src->depth()) {
        if (sw == src->width() && sh == src->height() || dst->depth()==32) {
            QImage srcconv = src->convertDepth(dst->depth(), flags);
            bitBlt(dst, dx, dy, &srcconv, sx, sy, sw, sh, flags);
        } else {
            QImage srcconv = src->copy(sx, sy, sw, sh); // ie. bitBlt
            bitBlt(dst, dx, dy, &srcconv, 0, 0, sw, sh, flags);
        }
        return;
    }

    // Now assume both are the same depth and are 32-bit or 8-bit with compatible palettes.

    switch (dst->depth()) {
    case 1: {
        uchar* d = dst->scanLine(dy) + dx/8;
        const uchar* s = src->scanLine(sy) + sx/8;
        const int bw = (sw+7)/8;
        const int dd = dst->bytesPerLine();
        const int ds = src->bytesPerLine();
        while (sh--) {
            memcpy(d, s, bw);
            d += dd;
            s += ds;
        }
    }
        break;
    case 8: {
        uchar* d = dst->scanLine(dy) + dx;
        const uchar* s = src->scanLine(sy) + sx;
        const int dd = dst->bytesPerLine();
        const int ds = src->bytesPerLine();
        while (sh--) {
            memcpy(d, s, sw);
            d += dd;
            s += ds;
        }
    }
        break;
#ifndef QT_NO_IMAGE_TRUECOLOR
    case 32:
        if (src->hasAlphaBuffer()) {
            QRgb* d = (QRgb*)dst->scanLine(dy) + dx;
            QRgb* s = (QRgb*)src->scanLine(sy) + sx;
            const int dd = dst->width() - sw;
            const int ds = src->width() - sw;
            while (sh--) {
                for (int t=sw; t--;) {
                    unsigned char a = qAlpha(*s);
                    if (a == 255)
                        *d++ = *s++;
                    else if (a == 0)
                        ++d,++s; // nothing
                    else {
                        unsigned char r = ((qRed(*s)-qRed(*d)) * a) / 256 + qRed(*d);
                        unsigned char g = ((qGreen(*s)-qGreen(*d)) * a) / 256 + qGreen(*d);
                        unsigned char b = ((qBlue(*s)-qBlue(*d)) * a) / 256 + qBlue(*d);
                        a = qMax(qAlpha(*d),int(a)); // alternatives...
                        *d++ = qRgba(r,g,b,a);
                        ++s;
                    }
                }
                d += dd;
                s += ds;
            }
        } else {
            QRgb* d = (QRgb*)dst->scanLine(dy) + dx;
            QRgb* s = (QRgb*)src->scanLine(sy) + sx;
            const int dd = dst->width();
            const int ds = src->width();
            const int b = sw*sizeof(QRgb);
            while (sh--) {
                memcpy(d, s, b);
                d += dd;
                s += ds;
            }
        }
        break;
#endif // QT_NO_IMAGE_TRUECOLOR
    }
}

/*!
    Returns true if this image and image \a i have the same contents;
    otherwise returns false. The comparison can be slow, unless there
    is some obvious difference, such as different widths, in which
    case the function will return quickly.

    \sa operator=()
*/

bool QImage::operator==(const QImage & i) const
{
    // same object, or shared?
    if (i.d == d)
        return true;
    // obviously different stuff?
    if (i.d->h != d->h || i.d->w != d->w || i.d->alpha != d->alpha)
        return false;
    // that was the fast bit...
    QImage i1 = convertDepth(32);
    QImage i2 = i.convertDepth(32);
    int l;
    if (d->alpha) {
        for(l=0; l < d->h; l++)
            if (memcmp(i1.scanLine(l), i2.scanLine(l), 4*d->w))
                return false;
    } else {
        //alpha channel undefined, so we must mask it out
        for(l=0; l < d->h; l++) {
            int w = d->w;
            const uint *p1 = reinterpret_cast<const uint*>(i1.scanLine(l));
            const uint *p2 = reinterpret_cast<uint*>(i2.scanLine(l));
            while (w--) {
                if ((*p1++ & 0x00ffffff) != (*p2++ & 0x00ffffff))
                    return false;
            }
        }
    }
    return true;
}


/*!
    Returns true if this image and image \a i have different contents;
    otherwise returns false. The comparison can be slow, unless there
    is some obvious difference, such as different widths, in which
    case the function will return quickly.

    \sa operator=()
*/

bool QImage::operator!=(const QImage & i) const
{
    return !(*this == i);
}




/*!
    Returns the number of pixels that fit horizontally in a physical
    meter. This and dotsPerMeterY() define the intended scale and
    aspect ratio of the image.

    \sa setDotsPerMeterX()
*/
int QImage::dotsPerMeterX() const
{
    return d->dpmx;
}

/*!
    Returns the number of pixels that fit vertically in a physical
    meter. This and dotsPerMeterX() define the intended scale and
    aspect ratio of the image.

    \sa setDotsPerMeterY()
*/
int QImage::dotsPerMeterY() const
{
    return d->dpmy;
}

/*!
    Sets the value returned by dotsPerMeterX() to \a x.
*/
void QImage::setDotsPerMeterX(int x)
{
    detach();
    d->dpmx = x;
}

/*!
    Sets the value returned by dotsPerMeterY() to \a y.
*/
void QImage::setDotsPerMeterY(int y)
{
    detach();
    d->dpmy = y;
}

/*!
    \fn QPoint QImage::offset() const

    Returns the number of pixels by which the image is intended to be
    offset by when positioning relative to other images.
*/
QPoint QImage::offset() const
{
    return d->offset;
}


/*!
    Sets the value returned by offset() to \a p.
*/
void QImage::setOffset(const QPoint& p)
{
    detach();
    d->offset = p;
}
#ifndef QT_NO_IMAGE_TEXT

/*!
    Returns the string recorded for the keyword \a key in language \a
    lang, or in a default language if \a lang is 0.
*/
QString QImage::text(const char* key, const char* lang) const
{
    QImageTextKeyLang x(key,lang);
    return d->text_lang.value(x);
}

/*!
    \overload

    Returns the string recorded for the keyword and language \a kl.
*/
QString QImage::text(const QImageTextKeyLang& kl) const
{
    return d->text_lang.value(kl);
}

/*!
    Returns the language identifiers for which some texts are
    recorded.

    Note that if you want to iterate over the list, you should iterate
    over a copy, e.g.

    \code
        QStringList list = myImage.textLanguages();
        QStringList::Iterator it = list.begin();
        while(it != list.end()) {
            myProcessing(*it);
            ++it;
        }
    \endcode

    \sa textList() text() setText() textKeys()
*/
QStringList QImage::textLanguages() const
{
    return d->languages();
}

/*!
    Returns the keywords for which some texts are recorded.

    Note that if you want to iterate over the list, you should iterate
    over a copy, e.g.

    \code
        QStringList list = myImage.textKeys();
        QStringList::Iterator it = list.begin();
        while(it != list.end()) {
            myProcessing(*it);
            ++it;
        }
    \endcode

    \sa textList() text() setText() textLanguages()
*/
QStringList QImage::textKeys() const
{
    return d->keys();
}

/*!
    Returns a list of QImageTextKeyLang objects that enumerate all the
    texts key/language pairs set by setText() for this image.
*/
QList<QImageTextKeyLang> QImage::textList() const
{
    return d->text_lang.keys();
}

/*!
    Records string \a s for the keyword \a key. The \a key should be
    a portable keyword recognizable by other software - some suggested
    values can be found in
    \link http://www.libpng.org/pub/png/spec/PNG-Chunks.html#C.Anc-text
    the PNG specification\endlink. \a s can be any text. \a lang should
    specify the language code (see
    \link ftp://ftp.isi.edu/in-notes/1766 RFC 1766\endlink) or 0.
*/
void QImage::setText(const char* key, const char* lang, const QString& s)
{
    detach();
    QImageTextKeyLang x(key, lang);
    d->text_lang.insert(x, s);
}

#endif // QT_NO_IMAGE_TEXT

/*
    Sets the image bits to the \a pixmap contents and returns a
    reference to the image.

    If the image shares data with other images, it will first
    dereference the shared data.

    Makes a call to QPixmap::convertToImage().
*/

/*! \fn QImage::Endian QImage::systemBitOrder()

    Determines the bit order of the display hardware. Returns
    QImage::LittleEndian (LSB first) or QImage::BigEndian (MSB first).

    \sa systemByteOrder()
*/


/*!
    \internal

    Used by QPainter to retreive a paint engine for the image.
*/

QPaintEngine *QImage::paintEngine() const
{
#ifdef QT_RASTER_IMAGEENGINE
    if (!d->paintEngine) {
        d->paintEngine = new QRasterPaintEngine();
    }
#endif
    return d->paintEngine;
}


int QImage::metric(PaintDeviceMetric metric) const
{
    switch (metric) {
    case PdmWidth:
        return d->w;
        break;

    case PdmHeight:
        return d->h;
        break;

    case PdmWidthMM:
        return d->w / d->dpmx;
        break;

    case PdmHeightMM:
        return d->h / d->dpmy;
        break;

    case PdmNumColors:
        return d->ncols;
        break;

    case PdmDepth:
        return d->d;
        break;

    case PdmDpiX:
        return (int)(d->dpmx / 0.0254);
        break;

    case PdmDpiY:
        return (int)(d->dpmy / 0.0254);
        break;

    case PdmPhysicalDpiX:
        return (int)(d->dpmx / 0.0254);
        break;

    case PdmPhysicalDpiY:
        return (int)(d->dpmy / 0.0254);
        break;

    default:
        qWarning("QImage::metric(), Unhandled metric type: %d\n", metric);
        break;
    }
    return 0;
}



/*****************************************************************************
  QPixmap (and QImage) helper functions
 *****************************************************************************/
/*
  This internal function contains the common (i.e. platform independent) code
  to do a transformation of pixel data. It is used by QPixmap::transform() and by
  QImage::transform().

  \a trueMat is the true transformation matrix (see QPixmap::trueMatrix()) and
  \a xoffset is an offset to the matrix.

  \a msbfirst specifies for 1bpp images, if the MSB or LSB comes first and \a
  depth specifies the colordepth of the data.

  \a dptr is a pointer to the destination data, \a dbpl specifies the bits per
  line for the destination data, \a p_inc is the offset that we advance for
  every scanline and \a dHeight is the height of the destination image.

  \a sprt is the pointer to the source data, \a sbpl specifies the bits per
  line of the source data, \a sWidth and \a sHeight are the width and height of
  the source data.
*/

#ifndef QT_NO_PIXMAP_TRANSFORMATION
#undef IWX_MSB
#define IWX_MSB(b)        if (trigx < maxws && trigy < maxhs) {                              \
                            if (*(sptr+sbpl*(trigy>>12)+(trigx>>15)) &                      \
                                 (1 << (7-((trigx>>12)&7))))                              \
                                *dptr |= b;                                              \
                        }                                                              \
                        trigx += m11;                                                      \
                        trigy += m12;
        // END OF MACRO
#undef IWX_LSB
#define IWX_LSB(b)        if (trigx < maxws && trigy < maxhs) {                              \
                            if (*(sptr+sbpl*(trigy>>12)+(trigx>>15)) &                      \
                                 (1 << ((trigx>>12)&7)))                              \
                                *dptr |= b;                                              \
                        }                                                              \
                        trigx += m11;                                                      \
                        trigy += m12;
        // END OF MACRO
#undef IWX_PIX
#define IWX_PIX(b)        if (trigx < maxws && trigy < maxhs) {                              \
                            if ((*(sptr+sbpl*(trigy>>12)+(trigx>>15)) &              \
                                 (1 << (7-((trigx>>12)&7)))) == 0)                      \
                                *dptr &= ~b;                                              \
                        }                                                              \
                        trigx += m11;                                                      \
                        trigy += m12;
        // END OF MACRO
bool qt_xForm_helper(const QMatrix &trueMat, int xoffset, int type, int depth,
                     uchar *dptr, int dbpl, int p_inc, int dHeight,
                     const uchar *sptr, int sbpl, int sWidth, int sHeight)
{
    int m11 = qRound(trueMat.m11()*4096.0);
    int m12 = qRound(trueMat.m12()*4096.0);
    int m21 = qRound(trueMat.m21()*4096.0);
    int m22 = qRound(trueMat.m22()*4096.0);
    int dx  = qRound(trueMat.dx()*4096.0);
    int dy  = qRound(trueMat.dy()*4096.0);

    int m21ydx = dx + (xoffset<<16);
    int m22ydy = dy;
    uint trigx;
    uint trigy;
    uint maxws = sWidth<<12;
    uint maxhs = sHeight<<12;

    for (int y=0; y<dHeight; y++) {                // for each target scanline
        trigx = m21ydx;
        trigy = m22ydy;
        uchar *maxp = dptr + dbpl;
        if (depth != 1) {
            switch (depth) {
                case 8:                                // 8 bpp transform
                while (dptr < maxp) {
                    if (trigx < maxws && trigy < maxhs)
                        *dptr = *(sptr+sbpl*(trigy>>12)+(trigx>>12));
                    trigx += m11;
                    trigy += m12;
                    dptr++;
                }
                break;

                case 16:                        // 16 bpp transform
                while (dptr < maxp) {
                    if (trigx < maxws && trigy < maxhs)
                        *((ushort*)dptr) = *((ushort *)(sptr+sbpl*(trigy>>12) +
                                                     ((trigx>>12)<<1)));
                    trigx += m11;
                    trigy += m12;
                    dptr++;
                    dptr++;
                }
                break;

                case 24:                        // 24 bpp transform
                while (dptr < maxp) {
                    if (trigx < maxws && trigy < maxhs) {
                        const uchar *p2 = sptr+sbpl*(trigy>>12) + ((trigx>>12)*3);
                        dptr[0] = p2[0];
                        dptr[1] = p2[1];
                        dptr[2] = p2[2];
                    }
                    trigx += m11;
                    trigy += m12;
                    dptr += 3;
                }
                break;

                case 32:                        // 32 bpp transform
                while (dptr < maxp) {
                    if (trigx < maxws && trigy < maxhs)
                        *((uint*)dptr) = *((uint *)(sptr+sbpl*(trigy>>12) +
                                                   ((trigx>>12)<<2)));
                    trigx += m11;
                    trigy += m12;
                    dptr += 4;
                }
                break;

                default: {
                return false;
                }
            }
        } else  {
            switch (type) {
                case QT_XFORM_TYPE_MSBFIRST:
                    while (dptr < maxp) {
                        IWX_MSB(128);
                        IWX_MSB(64);
                        IWX_MSB(32);
                        IWX_MSB(16);
                        IWX_MSB(8);
                        IWX_MSB(4);
                        IWX_MSB(2);
                        IWX_MSB(1);
                        dptr++;
                    }
                    break;
                case QT_XFORM_TYPE_LSBFIRST:
                    while (dptr < maxp) {
                        IWX_LSB(1);
                        IWX_LSB(2);
                        IWX_LSB(4);
                        IWX_LSB(8);
                        IWX_LSB(16);
                        IWX_LSB(32);
                        IWX_LSB(64);
                        IWX_LSB(128);
                        dptr++;
                    }
                    break;
#  if defined(Q_WS_WIN)
                case QT_XFORM_TYPE_WINDOWSPIXMAP:
                    while (dptr < maxp) {
                        IWX_PIX(128);
                        IWX_PIX(64);
                        IWX_PIX(32);
                        IWX_PIX(16);
                        IWX_PIX(8);
                        IWX_PIX(4);
                        IWX_PIX(2);
                        IWX_PIX(1);
                        dptr++;
                    }
                    break;
#  endif
            }
        }
        m21ydx += m21;
        m22ydy += m22;
        dptr += p_inc;
    }
    return true;
}
#undef IWX_MSB
#undef IWX_LSB
#undef IWX_PIX
#endif // QT_NO_PIXMAP_TRANSFORMATION

struct Qargb {
    Qargb() : a(0), r(0), g(0), b(0) {}
    Qargb(uint pixel, uint scale) {
        a = qAlpha(pixel) * scale;
        r =   qRed(pixel) * scale;
        g = qGreen(pixel) * scale;
        b =  qBlue(pixel) * scale;
    }
    inline Qargb &operator += (const Qargb &o) {
        a += o.a;
        r += o.r;
        g += o.g;
        b += o.b;
        return *this;
    }
    inline Qargb &operator *(uint scale) {
        a *= scale;
        r *= scale;
        g *= scale;
        b *= scale;
        return *this;
    }
    inline QRgb toPixel(uint scale) const {
        return qRgba(r/scale, g/scale, b/scale, a/scale);
    }
    uint a;
    uint r;
    uint g;
    uint b;
};

static void scaleX(QImage *image, int height, int iwidth, int owidth)
{
//     qDebug("scaleX: height=%d, iwidth=%d, owidth=%d", height, iwidth, owidth);
    if (iwidth == owidth)
        return;

    Q_ASSERT(image->width() >= qMax(iwidth, owidth));
    Q_ASSERT(height <= image->height());

    uchar **bits = image->jumpTable();
    if (owidth > iwidth) {
        for (int y = 0; y < height; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(bits[y]);

            int p = owidth;
            int q = iwidth;
            Qargb argb;
            int ix = iwidth - 1;
            int ox = owidth - 1;
            while (ox >= 0) {
                if (p > q) {
                    if (q)
                        argb += Qargb(line[ix], q);
                    line[ox] = argb.toPixel(iwidth);
                    argb = Qargb();
                    --ox;
                    p -= q;
                    q = iwidth;
                } else {
                    argb += Qargb(line[ix], p);
                    q -= p;
                    p = owidth;
                    --ix;
                }
            }
        }
    } else {
        const uint bytes = (iwidth - owidth)*sizeof(QRgb);
        for (int y = 0; y < height; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(bits[y]);

            int p = owidth;
            int q = iwidth;
            Qargb argb;
            int ix = 0;
            int ox = 0;
            while (ox < owidth) {
                if (p > q) {
                    if (q)
                        argb += Qargb(line[ix], q);
                    line[ox] = argb.toPixel(iwidth);
                    argb = Qargb();
                    ++ox;
                    p -= q;
                    q = iwidth;
                } else {
                    argb += Qargb(line[ix], p);
                    q -= p;
                    p = owidth;
                    ++ix;
                }
            }
            memset(line + owidth, 0, bytes);
        }
    }
}

static void scaleY(QImage *image, int width, int iheight, int oheight)
{
//     qDebug("scaleY: width=%d, iheight=%d, oheight=%d", width, iheight, oheight);
    if (iheight == oheight)
        return;

    Q_ASSERT(image->height() >= qMax(iheight, oheight));
    Q_ASSERT(width <= image->width());

    QRgb **bits = reinterpret_cast<QRgb **>(image->jumpTable());
    Qargb *argb = (Qargb *) malloc(width * sizeof(Qargb));
    memset(argb, 0, width*sizeof(Qargb));
    int p = oheight;
    int q = iheight;
    if (oheight > iheight) {
        int iy = iheight - 1;
        int oy = oheight - 1;
        while (oy >= 0) {
            if (p > q) {
                if (q)
                    for (int x = 0; x < width; ++x)
                        argb[x] += Qargb(bits[iy][x], q);
                for (int x = 0; x < width; ++x)
                    bits[oy][x] = argb[x].toPixel(iheight);
                memset(argb, 0, width*sizeof(Qargb));
                --oy;
                p -= q;
                q = iheight;
            } else {
                for (int x = 0; x < width; ++x)
                    argb[x] += Qargb(bits[iy][x], p);
                q -= p;
                p = oheight;
                --iy;
            }
        }
    } else {
        int iy = 0;
        int oy = 0;
        while (oy < oheight) {
            if (p > q) {
                if (q)
                    for (int x = 0; x < width; ++x)
                        argb[x] += Qargb(bits[iy][x], q);
                for (int x = 0; x < width; ++x)
                    bits[oy][x] = argb[x].toPixel(iheight);
                memset(argb, 0, width*sizeof(Qargb));
                ++oy;
                p -= q;
                q = iheight;
            } else {
                for (int x = 0; x < width; ++x)
                    argb[x] += Qargb(bits[iy][x], p);
                q -= p;
                p = oheight;
                ++iy;
            }
        }
        for (; oy < oheight; ++oy)
            memset(bits[oy], 0, width*sizeof(QRgb));
    }
    free(argb);
}


static void shearX(QImage *image, int height, int iwidth, qreal shear)
{
//     qDebug("shearX: height=%d, iwidth=%d, shear=%f", height, iwidth, shear);
    if (qAbs(shear*height) < 0.3)
        return;

    Q_ASSERT(image->width() >= iwidth + qRound(qAbs(shear*(height-1))));
    Q_ASSERT(height <= image->height());

    const qreal skewoffset = shear < 0 ? -shear*(height-1) : 0;

    uchar **bits = image->jumpTable();
    for (int y = 0; y < height; ++y) {
        const qreal skew = shear*y + skewoffset;
        int skewi = qIntCast(skew*256);
        int fraction = 0xff - skewi & 0xff;
        skewi >>= 8;
        QRgb *line = reinterpret_cast<QRgb *>(bits[y]);

        uint lastPixel = 0;
        for (int x = iwidth - 1; x >= 0; --x) {
            QRgb pixel = line[x];
            QRgb next = pixel;
            next = //next & 0x00ffffff + ((fraction*qAlpha(next))>>8 <<24);
                qRgba(
                    (fraction * qRed(pixel)) >> 8, (fraction * qGreen(pixel)) >> 8,
                    (fraction * qBlue(pixel)) >> 8, fraction * qAlpha(pixel) >> 8);

            line[x+skewi] = pixel - next + lastPixel;
            lastPixel = next;
        }
        if (skewi > 0) {
            line[skewi - 1] = lastPixel;
            if (skewi > 1)
                memset(line, 0, (skewi-1)*sizeof(QRgb));
        }
    }
}


static void shearY(QImage *image, int width, int iheight, qreal shear, qreal offset)
{
//     qDebug("shearY: width=%d, iheight=%d, shear=%f", width, iheight, shear);
    if (qAbs(shear*width) < 0.3)
        return;

    Q_ASSERT(image->height() >= iheight + qRound(qAbs(shear*(width-1))));
    Q_ASSERT(width <= image->width());

    QRgb **bits = reinterpret_cast<QRgb **>(image->jumpTable());
    for (int x = 0; x < width; ++x) {
        const qreal skew = shear*(x - offset);
        int skewi = qIntCast(skew*256);
        int fraction;
        if (skew >= 0) {
            fraction = 0xff - skewi & 0xff;
            skewi >>= 8;
            uint lastPixel = 0;
            for (int y = iheight - 1; y >= 0; --y) {
                QRgb pixel = bits[y][x];
                QRgb next = pixel;
                next = //next & 0x00ffffff + ((fraction*qAlpha(next))>>8 <<24);
                    qRgba(
                        (fraction * qRed(pixel)) >> 8, (fraction * qGreen(pixel)) >> 8,
                        (fraction * qBlue(pixel)) >> 8, fraction * qAlpha(pixel) >> 8);

                bits[y+skewi][x] = pixel - next + lastPixel;
                lastPixel = next;
            }
            if (skewi > 0) {
                bits[--skewi][x] = lastPixel;
                while (skewi > 0)
                    bits[--skewi][x] = 0;
            }
        } else {
            fraction = skewi & 0xff;
            skewi >>= 8;
            skewi -= 1;
            uint lastPixel = 0;
            for (int y = -skewi; y < iheight - 1; ++y) {
                QRgb pixel = bits[y][x];
                QRgb next = pixel;
                next = //next & 0x00ffffff + ((fraction*qAlpha(next))>>8 <<24);
                    qRgba(
                        (fraction * qRed(pixel)) >> 8, (fraction * qGreen(pixel)) >> 8,
                        (fraction * qBlue(pixel)) >> 8, fraction * qAlpha(pixel) >> 8);

                bits[y+skewi][x] = pixel - next + lastPixel;
                lastPixel = next;
            }
            bits[iheight + skewi][x] = lastPixel;
            ++skewi;
            while (skewi < 0) {
                bits[iheight + skewi][x] = 0;
                skewi++;
            }
        }
    }
}

QImage smoothXForm(const QImageData *d, const QMatrix &matrix)
{
    Q_ASSERT(d->d == 32);
    Q_ASSERT(d->w > 0 && d->h > 0);

    // avoid degenerate transformations
    if (qAbs(matrix.det()) < 0.001)
        return QImage();

    /* we decompose the full transformation into a possible mirroring operation,
       a n*90 degree rotation (n = 0...3), a scale along x, a scale along y,
       a shear along x and a shear along y.

       To avoid artifacts, the shearing transformations have to be minimized.

       To do so we calculate the projection of the unit vectors:

       / x1 \ = mat * / 1 \
       \ y1 /         \ 0 /

       / x2 \ = mat * / 0 \
       \ y2 /         \ 1 /

       To minimize the shearing error we need to find two orthogonal
       axes of the coordinate system that are closest to the projected
       unit vectors.
    */

    qreal v1x = matrix.m11(),
           v1y = matrix.m12(),
           v2x = matrix.m21(),
           v2y = matrix.m22();

//     qDebug("\nv1=(%f/%f), v2=(%f/%f)", v1x, v1y, v2x, v2y);

    bool v1Horizontal = qAbs(v1x) > qAbs(v1y);
    bool v2Horizontal = qAbs(v2x) > qAbs(v2y);

    if (v1Horizontal == true && v2Horizontal == true) {
        // both want horizontal, move the one to vertical where the error would be smaller.
        if (qAbs(v1y/v1x) <= qAbs(v2y/v2x))
            v2Horizontal = false;
        else
            v1Horizontal = false;
    } else if (v1Horizontal == false && v2Horizontal == false) {
        // both want vertical, move the one to horizontal where the error would be smaller.
        if (qAbs(v1x/v1y) < qAbs(v2x/v2y))
            v2Horizontal = true;
        else
            v1Horizontal = true;
    }
//     qDebug("v1Horizontal=%d, v2Horizontal=%d", v1Horizontal, v2Horizontal);

    /* Now that we know this, we can turn/mirror the image to the corresponding axes */

    uint v1Axis;
    if (v1Horizontal)
        v1Axis = v1x > 0 ? 0 : 2;
    else
        v1Axis = v1y > 0 ? 1 : 3;
    uint v2Axis;
    if (v2Horizontal)
        v2Axis = v2x > 0 ? 0 : 2;
    else
        v2Axis = v2y > 0 ? 1 : 3;

    bool mirror = false; // mirror vertically
    int rotate = 0;
    if ((v1Axis < v2Axis && !(v1Axis == 0 && v2Axis==3)) || (v1Axis == 3 && v2Axis == 0)) {
        rotate = v1Axis;
    } else {
        mirror = true;
        rotate = (4 - v1Axis) % 4;
    }

//     qDebug("v1Axis=%d, v2Axis=%d, mirror=%d, rotate=%d", v1Axis, v2Axis, mirror, rotate);

    // now mirror and rotate the image.
    // #### optimise and don't do a deep copy if !rotate && !mirror.

    bool swapAxes = rotate & 0x1;
    QImage result(swapAxes ? d->h : d->w, swapAxes ? d->w : d->h, d->d);
    result.setAlphaBuffer(d->alpha);

    QMatrix mat;
    switch(rotate) {
    case 0:
        if (!mirror) {
            const uchar * const *slines = d->bits;
            uchar **dlines = result.jumpTable();
            const int bpl = d->nbytes / d->h;
            for (int i = 0; i < d->h; ++i)
                memcpy(dlines[i], slines[i], bpl);
        } else {
            const uchar * const *slines = d->bits + d->h - 1;
            uchar **dlines = result.jumpTable();
            const int bpl = d->nbytes / d->h;
            for (int i = d->h; i > 0; --i) {
                memcpy(*dlines, *slines, bpl);
                ++dlines;
                --slines;
            }
        }
        mat = QMatrix(v1x, v1y, v2x, v2y, 0, 0);
        break;
    case 1: {
        const uchar * const * slines = d->bits;
        uchar **dlines = result.jumpTable();
        int dlinesi = 1;
        if (mirror) {
            dlines += d->w - 1;
            dlinesi = -1;
        }
        for (int i = 0; i < d->w; ++i, dlines += dlinesi) {
            quint32 *dl = (quint32 *)(*dlines) + d->h - 1;
            for (int j = 0; j < d->h; ++j) {
                *dl = ((quint32 *)*(slines+j))[i];
                --dl;
            }
        }
        mat = QMatrix(-v2x, -v2y, v1x, v1y, 0, 0);
        break;
    }
    case 2: {
        int dy;
        int dyi;
        if (mirror) {
            dy = 0;
            dyi = 1;
        } else {
            dy = d->h - 1;
            dyi = -1;
        }
        quint32 **rbits = (quint32 **)result.jumpTable();
        quint32 **sbits = (quint32 **)d->bits;
        for (int sy = 0; sy < d->h; sy++, dy += dyi) {
            const quint32* sl = sbits[sy];
            quint32* dl = rbits[dy] + d->w - 1;
            for (int sx = 0; sx < d->w; sx++) {
                *dl = *sl;
                ++sl;
                --dl;
            }
        }
        mat = QMatrix(-v1x, -v1y, -v2x, -v2y, 0, 0);
        break;
    }
    case 3: {
        const uchar * const * slines = d->bits;
        uchar **dlines = result.jumpTable();
        int dlinesi = 1;
        if (!mirror) {
            dlines += d->w - 1;
            dlinesi = -1;
        }
        for (int i = 0; i < d->w; ++i, dlines += dlinesi) {
            quint32 *dl = (quint32 *)(*dlines);
            for (int j = 0; j < d->h; ++j) {
                *dl = ((quint32 *)*(slines+j))[i];
                ++dl;
            }
        }
        mat = QMatrix(v2x, v2y, -v1x, -v1y, 0, 0);
        break;
    }
    default:
        Q_ASSERT(false);
    }

    if (mirror)
        mat = QMatrix(1, 0, 0, -1, 0, 0) * mat;
    /* Now we've reduced the rotational part to the area -45 -- 45 degrees.

      after these preparations all that's left to do to get to our transformed image is to scale
      it to the projection of v1 and v2 onto the two chosen axes, and then shear.
    */
    Q_ASSERT(mat.det() > 0);


    qreal scale_x = mat.m11();
    qreal scale_y = mat.m22() - mat.m12()*mat.m21()/mat.m11();
    qreal shear_y = mat.m12()/scale_x;
    qreal shear_x = mat.m21()/scale_y;

//     qDebug("scale_x=%f,scale_y=%f, shear_x=%f, shear_y=%f", scale_x,scale_y,shear_x,shear_y);

    int iwidth = result.width();
    int owidth = qRound(iwidth*scale_x);

    int iheight = result.height();
    int oheight = qRound(iheight*scale_y);

    QSize s = result.size();
    s = s.expandedTo(QSize(owidth, oheight));
    int sheared_width = owidth + qAbs(qRound(shear_x*oheight));
    int sheared_height = oheight + qAbs(qRound(shear_y*sheared_width));
    s = s.expandedTo(QSize(sheared_width, sheared_height));
    QImage result2(s, 32);
    result2.fill(0);
//     qDebug("result2: width %d height %d", result2.width(), result2.height());
    for (int y = 0; y < result.height(); ++y)
        memcpy(result2.jumpTable()[y], result.jumpTable()[y], result.bytesPerLine());

    scaleX(&result2, result.height(), iwidth, owidth);
    scaleY(&result2, owidth, iheight, oheight);
    shearX(&result2, oheight, owidth, shear_x);

    qreal offset = shear_y < 0
                    ? (shear_x < 0 ? sheared_width : owidth)
                    : (shear_x < 0 ? -shear_x*oheight : 0);
//     qDebug("shear_x=%f, oheight=%d, offset=%f", shear_x, oheight, offset);
    shearY(&result2, sheared_width, oheight, shear_y, offset);

    QImage final(qMin(result2.width(), qRound(qAbs(v1x*d->w) + qAbs(v2x*d->h))),
                 qMin(result2.height(), qRound(qAbs(v1y*d->w) + qAbs(v2y*d->h))), 32);
    final.setAlphaBuffer(true);
//     qDebug("result2.height() = %d, final.height() = %d", result2.height(), final.height());
    int yoff = 0;//(result2.height() - final.height())/2;
    for (int y = 0; y < final.height(); ++y)
        memcpy(final.jumpTable()[y], result2.jumpTable()[y+yoff], final.bytesPerLine());

    return final;
}

/*!
    \fn QImage QImage::xForm(const QMatrix &matrix) const

    Use transform(\a matrix) instead.
*/

/*!
    Returns a number that uniquely identifies the contents of this
    QImage object. This means that multiple QImage objects can have
    the same serial number as long as they refer to the same contents.

    A null image always have a serial number of 0.

    An example of where this is useful is for caching QImages.
*/

int QImage::serialNumber() const
{
    if (isNull())
        return 0;
    else
        return d->ser_no;
}

/*!
    Returns true if the image is detached; otherwise returns false.

    \sa detach()
*/

bool QImage::isDetached() const
{
    return d->ref == 1;
}
