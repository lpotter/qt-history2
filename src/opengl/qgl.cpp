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

#include "qapplication.h"
#include "qplatformdefs.h"
#include "qgl.h"
#include <qdebug.h>
#include "qpixmap.h"
#include "qimage.h"
#include "qgl_p.h"
#include <private/qpaintengine_opengl_p.h>
#include "qcolormap.h"
#include "qcache.h"
#include "qfile.h"

extern Q_GUI_EXPORT qint64 qt_image_id(const QImage &image);
extern Q_GUI_EXPORT qint64 qt_pixmap_id(const QPixmap &pixmap);

QThreadStorage<QGLThreadContext *> qgl_context_storage;

Q_GLOBAL_STATIC(QGLFormat, qgl_default_format)

class QGLDefaultOverlayFormat: public QGLFormat
{
public:
    inline QGLDefaultOverlayFormat()
    {
        setOption(QGL::FormatOption(0xffff << 16)); // turn off all options
        setOption(QGL::DirectRendering);
        setPlane(1);
    }
    Q_GLOBAL_STATIC(QGLDefaultOverlayFormat, instance)
};

QGLExtensions::Extensions QGLExtensions::glExtensions = 0;
#ifndef APIENTRY
# define APIENTRY
#endif
typedef void (APIENTRY *pfn_glCompressedTexImage2DARB) (GLenum, GLint, GLenum, GLsizei,
                                                        GLsizei, GLint, GLsizei, const GLvoid *);
static pfn_glCompressedTexImage2DARB qt_glCompressedTexImage2DARB = 0;

#if defined(Q_WS_X11)
#include "private/qt_x11_p.h"
#define INT32 dummy_INT32
#define INT8 dummy_INT8
#include <GL/glx.h>
#undef INT32
#undef INT8
#include "qx11info_x11.h"
#elif defined(Q_WS_MAC)
# include <private/qt_mac_p.h>
#endif

#include <stdlib.h> // malloc

#ifndef APIENTRY
#define APIENTRY
#endif

/*!
    \namespace QGL

    \brief The QGL namespace specifies miscellaneous identifiers used
    in the Qt OpenGL module.

    \ingroup multimedia
*/

/*!
    \enum QGL::FormatOption

    This enum specifies the format options.

    \value DoubleBuffer
    \value DepthBuffer
    \value Rgba
    \value AlphaChannel
    \value AccumBuffer
    \value StencilBuffer
    \value StereoBuffers
    \value DirectRendering
    \value HasOverlay
    \value SampleBuffers
    \value SingleBuffer
    \value NoDepthBuffer
    \value ColorIndex
    \value NoAlphaChannel
    \value NoAccumBuffer
    \value NoStencilBuffer
    \value NoStereoBuffers
    \value IndirectRendering
    \value NoOverlay
    \value NoSampleBuffers

    \sa {Sample Buffers Example}
*/

/*****************************************************************************
  QGLFormat implementation
 *****************************************************************************/


/*!
    \class QGLFormat
    \brief The QGLFormat class specifies the display format of an OpenGL
    rendering context.

    \ingroup multimedia

    A display format has several characteristics:
    \list
    \i \link setDoubleBuffer() Double or single buffering.\endlink
    \i \link setDepth() Depth buffer.\endlink
    \i \link setRgba() RGBA or color index mode.\endlink
    \i \link setAlpha() Alpha channel.\endlink
    \i \link setAccum() Accumulation buffer.\endlink
    \i \link setStencil() Stencil buffer.\endlink
    \i \link setStereo() Stereo buffers.\endlink
    \i \link setDirectRendering() Direct rendering.\endlink
    \i \link setOverlay() Presence of an overlay.\endlink
    \i \link setPlane() The plane of an overlay format.\endlink
    \i \link setSampleBuffers() Multisample buffers.\endlink
    \endlist

    You can also specify preferred bit depths for the depth buffer,
    alpha buffer, accumulation buffer and the stencil buffer with the
    functions: setDepthBufferSize(), setAlphaBufferSize(),
    setAccumBufferSize() and setStencilBufferSize().

    Note that even if you specify that you prefer a 32 bit depth
    buffer (e.g. with setDepthBufferSize(32)), the format that is
    chosen may not have a 32 bit depth buffer, even if there is a
    format available with a 32 bit depth buffer. The main reason for
    this is how the system dependant picking algorithms work on the
    different platforms, and some format options may have higher
    precedence than others.

    You create and tell a QGLFormat object what rendering options you
    want from an OpenGL
    \footnote
        OpenGL is a trademark of Silicon Graphics, Inc. in the
        United States and other countries.
    \endfootnote
    rendering context.

    OpenGL drivers or accelerated hardware may or may not support
    advanced features such as alpha channel or stereographic viewing.
    If you request some features that the driver/hardware does not
    provide when you create a QGLWidget, you will get a rendering
    context with the nearest subset of features.

    There are different ways to define the display characteristics of
    a rendering context. One is to create a QGLFormat and make it the
    default for the entire application:
    \code
    QGLFormat fmt;
    fmt.setAlpha(true);
    fmt.setStereo(true);
    QGLFormat::setDefaultFormat(fmt);
    \endcode

    Or you can specify the desired format when creating an object of
    your QGLWidget subclass:
    \code
    QGLFormat fmt;
    fmt.setDoubleBuffer(false);                 // single buffer
    fmt.setDirectRendering(false);              // software rendering
    MyGLWidget* myWidget = new MyGLWidget(fmt, ...);
    \endcode

    After the widget has been created, you can find out which of the
    requested features the system was able to provide:
    \code
    QGLFormat fmt;
    fmt.setOverlay(true);
    fmt.setStereo(true);
    MyGLWidget* myWidget = new MyGLWidget(fmt, ...);
    if (!myWidget->format().stereo()) {
        // ok, goggles off
        if (!myWidget->format().hasOverlay()) {
            qFatal("Cool hardware required");
        }
    }
    \endcode

    \sa QGLContext, QGLWidget
*/

static inline void transform_point(GLdouble out[4], const GLdouble m[16], const GLdouble in[4])
{
#define M(row,col)  m[col*4+row]
    out[0] =
        M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
    out[1] =
        M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
    out[2] =
        M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
    out[3] =
        M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

static inline GLint qgluProject(GLdouble objx, GLdouble objy, GLdouble objz,
	   const GLdouble model[16], const GLdouble proj[16],
	   const GLint viewport[4],
	   GLdouble * winx, GLdouble * winy, GLdouble * winz)
{
   GLdouble in[4], out[4];

   in[0] = objx;
   in[1] = objy;
   in[2] = objz;
   in[3] = 1.0;
   transform_point(out, model, in);
   transform_point(in, proj, out);

   if (in[3] == 0.0)
      return GL_FALSE;

   in[0] /= in[3];
   in[1] /= in[3];
   in[2] /= in[3];

   *winx = viewport[0] + (1 + in[0]) * viewport[2] / 2;
   *winy = viewport[1] + (1 + in[1]) * viewport[3] / 2;

   *winz = (1 + in[2]) / 2;
   return GL_TRUE;
}

/*!
    Constructs a QGLFormat object with the factory default settings:
    \list
    \i \link setDoubleBuffer() Double buffer:\endlink Enabled.
    \i \link setDepth() Depth buffer:\endlink Enabled.
    \i \link setRgba() RGBA:\endlink Enabled (i.e., color index disabled).
    \i \link setAlpha() Alpha channel:\endlink Disabled.
    \i \link setAccum() Accumulator buffer:\endlink Disabled.
    \i \link setStencil() Stencil buffer:\endlink Disabled.
    \i \link setStereo() Stereo:\endlink Disabled.
    \i \link setDirectRendering() Direct rendering:\endlink Enabled.
    \i \link setOverlay() Overlay:\endlink Disabled.
    \i \link setPlane() Plane:\endlink 0 (i.e., normal plane).
    \i \link setSampleBuffers() Multisample buffers:\endlink Disabled.
    \endlist
*/

QGLFormat::QGLFormat()
{
    d = new QGLFormatPrivate;
}


/*!
    Creates a QGLFormat object that is a copy of the current \link
    defaultFormat() application default format\endlink.

    If \a options is not 0, this copy is modified by these format
    options. The \a options parameter should be \c FormatOption values
    OR'ed together.

    This constructor makes it easy to specify a certain desired format
    in classes derived from QGLWidget, for example:
    \code
    // The rendering in MyGLWidget depends on using
    // stencil buffer and alpha channel
    MyGLWidget::MyGLWidget(QWidget* parent)
        : QGLWidget(QGLFormat(QGL::StencilBuffer | QGL::AlphaChannel), parent)
    {
        if (!format().stencil())
            qWarning("Could not get stencil buffer; results will be suboptimal");
        if (!format().alphaChannel())
            qWarning("Could not get alpha channel; results will be suboptimal");
        ...
    }
    \endcode

    Note that there are \c FormatOption values to turn format settings
    both on and off, e.g. \c DepthBuffer and \c NoDepthBuffer,
    \c DirectRendering and \c IndirectRendering, etc.

    The \a plane parameter defaults to 0 and is the plane which this
    format should be associated with. Not all OpenGL implementations
    supports overlay/underlay rendering planes.

    \sa defaultFormat(), setOption()
*/

QGLFormat::QGLFormat(QGL::FormatOptions options, int plane)
{
    d = new QGLFormatPrivate;
    QGL::FormatOptions newOpts = options;
    d->opts = defaultFormat().d->opts;
    d->opts |= (newOpts & 0xffff);
    d->opts &= ~(newOpts >> 16);
    d->pln = plane;
}

/*!
    Constructs a copy of \a other.
*/

QGLFormat::QGLFormat(const QGLFormat &other)
{
    d = new QGLFormatPrivate;
    *d = *other.d;
}

/*!
    Assigns \a other to this object.
*/

QGLFormat &QGLFormat::operator=(const QGLFormat &other)
{
    *d = *other.d;
    return *this;
}

/*!
    Destroys the QGLFormat.
*/
QGLFormat::~QGLFormat()
{
    delete d;
}

/*!
    \fn bool QGLFormat::doubleBuffer() const

    Returns true if double buffering is enabled; otherwise returns
    false. Double buffering is enabled by default.

    \sa setDoubleBuffer()
*/

/*!
    If \a enable is true sets double buffering; otherwise sets single
    buffering.

    Double buffering is enabled by default.

    Double buffering is a technique where graphics are rendered on an
    off-screen buffer and not directly to the screen. When the drawing
    has been completed, the program calls a swapBuffers() function to
    exchange the screen contents with the buffer. The result is
    flicker-free drawing and often better performance.

    \sa doubleBuffer(), QGLContext::swapBuffers(),
    QGLWidget::swapBuffers()
*/

void QGLFormat::setDoubleBuffer(bool enable)
{
    setOption(enable ? QGL::DoubleBuffer : QGL::SingleBuffer);
}


/*!
    \fn bool QGLFormat::depth() const

    Returns true if the depth buffer is enabled; otherwise returns
    false. The depth buffer is enabled by default.

    \sa setDepth(), setDepthBufferSize()
*/

/*!
    If \a enable is true enables the depth buffer; otherwise disables
    the depth buffer.

    The depth buffer is enabled by default.

    The purpose of a depth buffer (or Z-buffering) is to remove hidden
    surfaces. Pixels are assigned Z values based on the distance to
    the viewer. A pixel with a high Z value is closer to the viewer
    than a pixel with a low Z value. This information is used to
    decide whether to draw a pixel or not.

    \sa depth(), setDepthBufferSize()
*/

void QGLFormat::setDepth(bool enable)
{
    setOption(enable ? QGL::DepthBuffer : QGL::NoDepthBuffer);
}


/*!
    \fn bool QGLFormat::rgba() const

    Returns true if RGBA color mode is set. Returns false if color
    index mode is set. The default color mode is RGBA.

    \sa setRgba()
*/

/*!
    If \a enable is true sets RGBA mode. If \a enable is false sets
    color index mode.

    The default color mode is RGBA.

    RGBA is the preferred mode for most OpenGL applications. In RGBA
    color mode you specify colors as red + green + blue + alpha
    quadruplets.

    In color index mode you specify an index into a color lookup
    table.

    \sa rgba()
*/

void QGLFormat::setRgba(bool enable)
{
    setOption(enable ? QGL::Rgba : QGL::ColorIndex);
}


/*!
    \fn bool QGLFormat::alpha() const

    Returns true if the alpha buffer in the framebuffer is enabled;
    otherwise returns false. The alpha buffer is disabled by default.

    \sa setAlpha(), setAlphaBufferSize()
*/

/*!
    If \a enable is true enables the alpha buffer; otherwise disables
    the alpha buffer.

    The alpha buffer is disabled by default.

    The alpha buffer is typically used for implementing transparency
    or translucency. The A in RGBA specifies the transparency of a
    pixel.

    \sa alpha(), setAlphaBufferSize()
*/

void QGLFormat::setAlpha(bool enable)
{
    setOption(enable ? QGL::AlphaChannel : QGL::NoAlphaChannel);
}


/*!
    \fn bool QGLFormat::accum() const

    Returns true if the accumulation buffer is enabled; otherwise
    returns false. The accumulation buffer is disabled by default.

    \sa setAccum(), setAccumBufferSize()
*/

/*!
    If \a enable is true enables the accumulation buffer; otherwise
    disables the accumulation buffer.

    The accumulation buffer is disabled by default.

    The accumulation buffer is used to create blur effects and
    multiple exposures.

    \sa accum(), setAccumBufferSize()
*/

void QGLFormat::setAccum(bool enable)
{
    setOption(enable ? QGL::AccumBuffer : QGL::NoAccumBuffer);
}


/*!
    \fn bool QGLFormat::stencil() const

    Returns true if the stencil buffer is enabled; otherwise returns
    false. The stencil buffer is disabled by default.

    \sa setStencil(), setStencilBufferSize()
*/

/*!
    If \a enable is true enables the stencil buffer; otherwise
    disables the stencil buffer.

    The stencil buffer is disabled by default.

    The stencil buffer masks certain parts of the drawing area so that
    masked parts are not drawn on.

    \sa stencil(), setStencilBufferSize()
*/

void QGLFormat::setStencil(bool enable)
{
    setOption(enable ? QGL::StencilBuffer: QGL::NoStencilBuffer);
}


/*!
    \fn bool QGLFormat::stereo() const

    Returns true if stereo buffering is enabled; otherwise returns
    false. Stereo buffering is disabled by default.

    \sa setStereo()
*/

/*!
    If \a enable is true enables stereo buffering; otherwise disables
    stereo buffering.

    Stereo buffering is disabled by default.

    Stereo buffering provides extra color buffers to generate left-eye
    and right-eye images.

    \sa stereo()
*/

void QGLFormat::setStereo(bool enable)
{
    setOption(enable ? QGL::StereoBuffers : QGL::NoStereoBuffers);
}


/*!
    \fn bool QGLFormat::directRendering() const

    Returns true if direct rendering is enabled; otherwise returns
    false.

    Direct rendering is enabled by default.

    \sa setDirectRendering()
*/

/*!
    If \a enable is true enables direct rendering; otherwise disables
    direct rendering.

    Direct rendering is enabled by default.

    Enabling this option will make OpenGL bypass the underlying window
    system and render directly from hardware to the screen, if this is
    supported by the system.

    \sa directRendering()
*/

void QGLFormat::setDirectRendering(bool enable)
{
    setOption(enable ? QGL::DirectRendering : QGL::IndirectRendering);
}

/*!
    \fn bool QGLFormat::sampleBuffers() const

    Returns true if multisample buffer support is enabled; otherwise
    returns false.

    The multisample buffer is disabled by default.

    \sa setSampleBuffers()
*/

/*!
    If \a enable is true, a GL context with multisample buffer support
    is picked; otherwise ignored.

    \sa sampleBuffers(), setSamples(), samples()
*/
void QGLFormat::setSampleBuffers(bool enable)
{
    setOption(enable ? QGL::SampleBuffers : QGL::NoSampleBuffers);
}

/*!
    Returns the number of samples per pixel when multisampling is
    enabled. By default, the highest number of samples that is
    available is used.

    \sa setSampleBuffers(), sampleBuffers(), setSamples()
*/
int QGLFormat::samples() const
{
   return d->numSamples;
}

/*!
    Set the preferred number of samples per pixel when multisampling
    is enabled to \a numSamples. By default, the highest number of
    samples available is used.

    \sa setSampleBuffers(), sampleBuffers(), samples()
*/
void QGLFormat::setSamples(int numSamples)
{
    if (numSamples < 0) {
        qWarning("QGLFormat::setSamples: Cannot have negative number of samples per pixel %d", numSamples);
        return;
    }
    d->numSamples = numSamples;
}

/*!
    \since 4.2

    Set the preferred swap interval. This can be used to sync the GL
    drawing into a system window to the vertical refresh of the screen.
    Setting an \a interval value of 0 will turn the vertical refresh syncing
    off, any value higher than 0 will turn the vertical syncing on.

    Under Windows, where the \c{WGL_EXT_swap_control} extension is
    used, the \a interval parameter can be used to set the minimum
    number of video frames that are displayed before a buffer swap
    will occur. In effect, setting the \a interval to 10, means there
    will be 10 vertical retraces between every buffer swap.

    Note that setting the swap interval is only supported under
    Windows and on Mac OS X. Under Windows the
    \c{WGL_EXT_swap_control} extension has to be present.
*/
void QGLFormat::setSwapInterval(int interval)
{
    d->swapInterval = interval;
}

/*!
    \since 4.2

    Returns the currently set swap interval. -1 is returned if setting
    the swap interval isn't supported in the system GL implementation
    (e.g. under X11).
*/
int QGLFormat::swapInterval() const
{
    return d->swapInterval;
}

/*!
    \fn bool QGLFormat::hasOverlay() const

    Returns true if overlay plane is enabled; otherwise returns false.

    Overlay is disabled by default.

    \sa setOverlay()
*/

/*!
    If \a enable is true enables an overlay plane; otherwise disables
    the overlay plane.

    Enabling the overlay plane will cause QGLWidget to create an
    additional context in an overlay plane. See the QGLWidget
    documentation for further information.

    \sa hasOverlay()
*/

void QGLFormat::setOverlay(bool enable)
{
    setOption(enable ? QGL::HasOverlay : QGL::NoOverlay);
}

/*!
    Returns the plane of this format. The default for normal formats
    is 0, which means the normal plane. The default for overlay
    formats is 1, which is the first overlay plane.

    \sa setPlane()
*/
int QGLFormat::plane() const
{
    return d->pln;
}

/*!
    Sets the requested plane to \a plane. 0 is the normal plane, 1 is
    the first overlay plane, 2 is the second overlay plane, etc.; -1,
    -2, etc. are underlay planes.

    Note that in contrast to other format specifications, the plane
    specifications will be matched exactly. This means that if you
    specify a plane that the underlying OpenGL system cannot provide,
    an \link QGLWidget::isValid() invalid\endlink QGLWidget will be
    created.

    \sa plane()
*/
void QGLFormat::setPlane(int plane)
{
    d->pln = plane;
}

/*!
    Sets the format option to \a opt.

    \sa testOption()
*/

void QGLFormat::setOption(QGL::FormatOptions opt)
{
    if (opt & 0xffff)
        d->opts |= opt;
    else
       d->opts &= ~(opt >> 16);
}



/*!
    Returns true if format option \a opt is set; otherwise returns false.

    \sa setOption()
*/

bool QGLFormat::testOption(QGL::FormatOptions opt) const
{
    if (opt & 0xffff)
       return (d->opts & opt) != 0;
    else
       return (d->opts & (opt >> 16)) == 0;
}

/*!
    Set the preferred depth buffer size to \a size.

    \sa depthBufferSize(), setDepth(), depth()
*/
void QGLFormat::setDepthBufferSize(int size)
{
    if (size < 0) {
        qWarning("QGLFormat::setDepthBufferSize: Cannot set negative depth buffer size %d", size);
        return;
    }
    d->depthSize = size;
}

/*!
    Returns the depth buffer size.

    \sa depth(), setDepth(), setDepthBufferSize()
*/
int QGLFormat::depthBufferSize() const
{
   return d->depthSize;
}

/*!
    \since 4.2

    Set the preferred red buffer size to \a size.

    \sa setGreenBufferSize(), setBlueBufferSize(), setAlphaBufferSize()
*/
void QGLFormat::setRedBufferSize(int size)
{
    if (size < 0) {
        qWarning("QGLFormat::setRedBufferSize: Cannot set negative red buffer size %d", size);
        return;
    }
    d->redSize = size;
}

/*!
    \since 4.2

    Returns the red buffer size.

    \sa setRedBufferSize()
*/
int QGLFormat::redBufferSize() const
{
   return d->redSize;
}

/*!
    \since 4.2

    Set the preferred green buffer size to \a size.

    \sa setRedBufferSize(), setBlueBufferSize(), setAlphaBufferSize()
*/
void QGLFormat::setGreenBufferSize(int size)
{
    if (size < 0) {
        qWarning("QGLFormat::setGreenBufferSize: Cannot set negative green buffer size %d", size);
        return;
    }
    d->greenSize = size;
}

/*!
    \since 4.2

    Returns the green buffer size.

    \sa setGreenBufferSize()
*/
int QGLFormat::greenBufferSize() const
{
   return d->greenSize;
}

/*!
    \since 4.2

    Set the preferred blue buffer size to \a size.

    \sa setRedBufferSize(), setGreenBufferSize(), setAlphaBufferSize()
*/
void QGLFormat::setBlueBufferSize(int size)
{
    if (size < 0) {
        qWarning("QGLFormat::setBlueBufferSize: Cannot set negative blue buffer size %d", size);
        return;
    }
    d->blueSize = size;
}

/*!
    \since 4.2

    Returns the blue buffer size.

    \sa setBlueBufferSize()
*/
int QGLFormat::blueBufferSize() const
{
   return d->blueSize;
}

/*!
    Set the preferred alpha buffer size to \a size.
    This function implicitly enables the alpha channel.

    \sa setRedBufferSize(), setGreenBufferSize(), alphaBufferSize()
*/
void QGLFormat::setAlphaBufferSize(int size)
{
    if (size < 0) {
        qWarning("QGLFormat::setAlphaBufferSize: Cannot set negative alpha buffer size %d", size);
        return;
    }
    d->alphaSize = size;
    setOption(QGL::AlphaChannel);
}

/*!
    Returns the alpha buffer size.

    \sa alpha(), setAlpha(), setAlphaBufferSize()
*/
int QGLFormat::alphaBufferSize() const
{
   return d->alphaSize;
}

/*!
    Set the preferred accumulation buffer size, where \a size is the
    bit depth for each RGBA component.

    \sa accum(), setAccum(), accumBufferSize()
*/
void QGLFormat::setAccumBufferSize(int size)
{
    if (size < 0) {
        qWarning("QGLFormat::setAccumBufferSize: Cannot set negative accumulate buffer size %d", size);
        return;
    }
    d->accumSize = size;
}

/*!
    Returns the accumulation buffer size.

    \sa setAccumBufferSize(), accum(), setAccum()
*/
int QGLFormat::accumBufferSize() const
{
   return d->accumSize;
}

/*!
    Set the preferred stencil buffer size to \a size.

    \sa stencilBufferSize(), setStencil(), stencil()
*/
void QGLFormat::setStencilBufferSize(int size)
{
    if (size < 0) {
        qWarning("QGLFormat::setStencilBufferSize: Cannot set negative stencil buffer size %d", size);
        return;
    }
    d->stencilSize = size;
}

/*!
    Returns the stencil buffer size.

    \sa stencil(), setStencil(), setStencilBufferSize()
*/
int QGLFormat::stencilBufferSize() const
{
   return d->stencilSize;
}

/*!
    \fn bool QGLFormat::hasOpenGL()

    Returns true if the window system has any OpenGL support;
    otherwise returns false.

    \warning This function must not be called until the QApplication
    object has been created.
*/



/*!
    \fn bool QGLFormat::hasOpenGLOverlays()

    Returns true if the window system supports OpenGL overlays;
    otherwise returns false.

    \warning This function must not be called until the QApplication
    object has been created.
*/

QGLFormat::OpenGLVersionFlags Q_AUTOTEST_EXPORT qOpenGLVersionFlagsFromString(const QString &versionString)
{
    QGLFormat::OpenGLVersionFlags versionFlags = QGLFormat::OpenGL_Version_None;

    if (versionString.startsWith("OpenGL ES")) {
        QStringList parts = versionString.split(' ');
        if (parts.size() >= 3) {
            if (parts[2].startsWith("1.")) {
                if (parts[1].endsWith("-CM")) {
                    versionFlags |= QGLFormat::OpenGL_ES_Common_Version_1_0 |
                                    QGLFormat::OpenGL_ES_CommonLite_Version_1_0;
                    if (parts[2].startsWith("1.1"))
                        versionFlags |= QGLFormat::OpenGL_ES_Common_Version_1_1 |
                                        QGLFormat::OpenGL_ES_CommonLite_Version_1_1;
                }
                else {
                    // Not -CM, must be CL, CommonLite
                    versionFlags |= QGLFormat::OpenGL_ES_CommonLite_Version_1_0;
                    if (parts[2].startsWith("1.1"))
                        versionFlags |= QGLFormat::OpenGL_ES_CommonLite_Version_1_1;
                }
            }
            else {
                // OpenGL ES version 2.0 or higher
                versionFlags |= QGLFormat::OpenGL_ES_Version_2_0;
            }
        }
        else {
            // if < 3 parts to the name, it is an unrecognised OpenGL ES
            qWarning("Unrecognised OpenGL ES version");
        }
    }
    else {
        // not ES, regular OpenGL, the version numbers are first in the string
        if (versionString.startsWith("1.")) {
            switch (versionString[2].toAscii()) {
            case '5':
                versionFlags |= QGLFormat::OpenGL_Version_1_5;
            case '4':
                versionFlags |= QGLFormat::OpenGL_Version_1_4;
            case '3':
                versionFlags |= QGLFormat::OpenGL_Version_1_3;
            case '2':
                versionFlags |= QGLFormat::OpenGL_Version_1_2;
            case '1':
                versionFlags |= QGLFormat::OpenGL_Version_1_1;
            default:
                break;
            }
        }
        else if (versionString.startsWith("2.")) {
            versionFlags |= QGLFormat::OpenGL_Version_1_1 |
                            QGLFormat::OpenGL_Version_1_2 |
                            QGLFormat::OpenGL_Version_1_3 |
                            QGLFormat::OpenGL_Version_1_4 |
                            QGLFormat::OpenGL_Version_1_5 |
                            QGLFormat::OpenGL_Version_2_0;
            QString minorVersion = versionString.section(' ', 0, 0).section('.', 1, 1);
            if (minorVersion == "1")
                versionFlags |= QGLFormat::OpenGL_Version_2_1;
        }
        else
            qWarning("Unrecognised OpenGL version");
    }
    return versionFlags;
}

/*!
    \enum QGLFormat::OpenGLVersionFlag
    \since 4.2

    This enum describes the various OpenGL versions that are
    recognized by Qt. Use the QGLFormat::openGLVersionFlags() function
    to identify which versions that are supported at runtime.

    \value OpenGL_Version_None  If no OpenGL is present or if no OpenGL context is current.

    \value OpenGL_Version_1_1  OpenGL version 1.1 or higher is present.

    \value OpenGL_Version_1_2  OpenGL version 1.2 or higher is present.

    \value OpenGL_Version_1_3  OpenGL version 1.3 or higher is present.

    \value OpenGL_Version_1_4  OpenGL version 1.4 or higher is present.

    \value OpenGL_Version_1_5  OpenGL version 1.5 or higher is present.

    \value OpenGL_Version_2_0  OpenGL version 2.0 or higher is present.
    Note that version 2.0 supports all the functionality of version 1.5.

    \value OpenGL_Version_2_1  OpenGL version 2.1 or higher is present.

    \value OpenGL_ES_CommonLite_Version_1_0  OpenGL ES version 1.0 Common Lite or higher is present.

    \value OpenGL_ES_Common_Version_1_0  OpenGL ES version 1.0 Common or higher is present.
    The Common profile supports all the features of Common Lite.

    \value OpenGL_ES_CommonLite_Version_1_1  OpenGL ES version 1.1 Common Lite or higher is present.

    \value OpenGL_ES_Common_Version_1_1  OpenGL ES version 1.1 Common or higher is present.
    The Common profile supports all the features of Common Lite.

    \value OpenGL_ES_Version_2_0  OpenGL ES version 2.0 or higher is present.
    Note that OpenGL ES version 2.0 does not support all the features of OpenGL ES 1.x.
    So if OpenGL_ES_Version_2_0 is returned, none of the ES 1.x flags are returned.

    See also \l{http://www.opengl.org} for more information about the different
    revisions of OpenGL.

    \sa openGLVersionFlags()
*/

/*!
    \since 4.2

    Identifies, at runtime, which OpenGL versions that are supported
    by the current platform.

    Note that if OpenGL version 1.5 is supported, its predecessors
    (i.e., version 1.4 and lower) are also supported. To identify the
    support of a particular feature, like multi texturing, test for
    the version in which the feature was first introduced (i.e.,
    version 1.3 in the case of multi texting) to adapt to the largest
    possible group of runtime platforms.

    This function needs a valid current OpenGL context to work;
    otherwise it will return OpenGL_Version_None.

    \sa hasOpenGL(), hasOpenGLOverlays()
*/
QGLFormat::OpenGLVersionFlags QGLFormat::openGLVersionFlags()
{
    static bool firstTime=true;
    static OpenGLVersionFlags versionFlags = OpenGL_Version_None;

    if (!firstTime)
        return versionFlags;

    if (!hasOpenGL())
        return OpenGL_Version_None;

    if (firstTime) {
        firstTime = false;

        QString versionString(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        versionFlags = qOpenGLVersionFlagsFromString(versionString);
    }

    return versionFlags;
}


/*!
    Returns the default QGLFormat for the application. All QGLWidgets
    that are created use this format unless another format is
    specified, e.g. when they are constructed.

    If no special default format has been set using
    setDefaultFormat(), the default format is the same as that created
    with QGLFormat().

    \sa setDefaultFormat()
*/

QGLFormat QGLFormat::defaultFormat()
{
    return *qgl_default_format();
}

/*!
    Sets a new default QGLFormat for the application to \a f. For
    example, to set single buffering as the default instead of double
    buffering, your main() might contain code like this:
    \code
    QApplication a(argc, argv);
    QGLFormat f;
    f.setDoubleBuffer(false);
    QGLFormat::setDefaultFormat(f);
    \endcode

    \sa defaultFormat()
*/

void QGLFormat::setDefaultFormat(const QGLFormat &f)
{
    *qgl_default_format() = f;
}


/*!
    Returns the default QGLFormat for overlay contexts.

    The factory default overlay format is:
    \list
    \i \link setDoubleBuffer() Double buffer:\endlink Disabled.
    \i \link setDepth() Depth buffer:\endlink Disabled.
    \i \link setRgba() RGBA:\endlink Disabled (i.e., color index enabled).
    \i \link setAlpha() Alpha channel:\endlink Disabled.
    \i \link setAccum() Accumulator buffer:\endlink Disabled.
    \i \link setStencil() Stencil buffer:\endlink Disabled.
    \i \link setStereo() Stereo:\endlink Disabled.
    \i \link setDirectRendering() Direct rendering:\endlink Enabled.
    \i \link setOverlay() Overlay:\endlink Disabled.
    \i \link setPlane() Plane:\endlink 1 (i.e., first overlay plane).
    \endlist

    \sa setDefaultFormat()
*/

QGLFormat QGLFormat::defaultOverlayFormat()
{
    return *QGLDefaultOverlayFormat::instance();
}

/*!
    Sets a new default QGLFormat for overlay contexts to \a f. This
    format is used whenever a QGLWidget is created with a format that
    hasOverlay() enabled.

    For example, to get a double buffered overlay context (if
    available), use code like this:

    \code
    QGLFormat f = QGLFormat::defaultOverlayFormat();
    f.setDoubleBuffer(true);
    QGLFormat::setDefaultOverlayFormat(f);
    \endcode

    As usual, you can find out after widget creation whether the
    underlying OpenGL system was able to provide the requested
    specification:

    \code
    // ...continued from above
    MyGLWidget* myWidget = new MyGLWidget(QGLFormat(QGL::HasOverlay), ...);
    if (myWidget->format().hasOverlay()) {
        // Yes, we got an overlay, let's check _its_ format:
        QGLContext* olContext = myWidget->overlayContext();
        if (olContext->format().doubleBuffer())
            ; // yes, we got a double buffered overlay
        else
            ; // no, only single buffered overlays are available
    }
    \endcode

    \sa defaultOverlayFormat()
*/

void QGLFormat::setDefaultOverlayFormat(const QGLFormat &f)
{
    QGLFormat *defaultFormat = QGLDefaultOverlayFormat::instance();
    *defaultFormat = f;
    // Make sure the user doesn't request that the overlays themselves
    // have overlays, since it is unlikely that the system supports
    // infinitely many planes...
    defaultFormat->setOverlay(false);
}


/*!
    Returns true if all the options of the two QGLFormats are equal;
    otherwise returns false.
*/

bool operator==(const QGLFormat& a, const QGLFormat& b)
{
    return (int) a.d->opts == (int) b.d->opts && a.d->pln == b.d->pln && a.d->alphaSize == b.d->alphaSize
        && a.d->accumSize == b.d->accumSize && a.d->stencilSize == b.d->stencilSize
        && a.d->depthSize == b.d->depthSize;
}


/*!
    Returns false if all the options of the two QGLFormats are equal;
    otherwise returns true.
*/

bool operator!=(const QGLFormat& a, const QGLFormat& b)
{
    return !(a == b);
}

/*****************************************************************************
  QGLContext implementation
 *****************************************************************************/
void QGLContextPrivate::init(QPaintDevice *dev, const QGLFormat &format)
{
    Q_Q(QGLContext);
    glFormat = reqFormat = format;
    valid = false;
    q->setDevice(dev);
#if defined(Q_WS_X11)
    gpm = 0;
#endif
#if defined(Q_WS_WIN)
    dc = 0;
    win = 0;
    pixelFormatId = 0;
    cmap = 0;
    hbitmap = 0;
    hbitmap_hdc = 0;
#endif
#if defined(Q_WS_MAC)
    update = false;
#endif
#if defined(Q_WS_QWS)
    dpy = 0;
    cx = 0;
    config = 0;
    surface = 0;
#endif
    crWin = false;
    initDone = false;
    sharing = false;
}

#ifdef Q_WS_WIN

QGLContextPrivate *qt_glctx_get_dptr(QGLContext *ctx) { return ctx->d_ptr; }

#endif

QGLContext* QGLContext::currentCtx = 0;

// returns the highest number closest to v, which is a power of 2
// NB! assumes 32 bit ints
int nearest_gl_texture_size(int v)
{
    int n = 0, last = 0;
    for (int s = 0; s < 32; ++s) {
        if (((v>>s) & 1) == 1) {
            ++n;
            last = s;
        }
    }
    if (n > 1)
        return 1 << (last+1);
    return 1 << last;
}

class QGLTexture {
public:
    QGLTexture(const QGLContext *ctx, GLuint tx_id, qint64 _qt_id, bool _clean = false)
        : context(ctx), id(tx_id), qt_id(_qt_id), clean(_clean) {}
    ~QGLTexture() {
        if (!context->isSharing())
            glDeleteTextures(1, &id);
     }

    const QGLContext *context;
    GLuint id;
    qint64 qt_id;
    bool clean;
};

typedef QCache<QString, QGLTexture> QGLTextureCache;
static int qt_tex_cache_limit = 64*1024; // cache ~64 MB worth of textures - this is not accurate though
static QGLTextureCache *qt_tex_cache = 0;

typedef void (*_qt_pixmap_cleanup_hook)(int);
typedef void (*_qt_image_cleanup_hook)(int);

extern Q_GUI_EXPORT _qt_pixmap_cleanup_hook qt_pixmap_cleanup_hook;
extern Q_GUI_EXPORT _qt_image_cleanup_hook qt_image_cleanup_hook;

// DDS format structure
struct DDSFormat {
    quint32 dwSize;
    quint32 dwFlags;
    quint32 dwHeight;
    quint32 dwWidth;
    quint32 dwLinearSize;
    quint32 dummy1;
    quint32 dwMipMapCount;
    quint32 dummy2[11];
    struct {
        quint32 dummy3[2];
        quint32 dwFourCC;
        quint32 dummy4[5];
    } ddsPixelFormat;
};

// compressed texture pixel formats
#define FOURCC_DXT1  0x31545844
#define FOURCC_DXT2  0x32545844
#define FOURCC_DXT3  0x33545844
#define FOURCC_DXT4  0x34545844
#define FOURCC_DXT5  0x35545844

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

#ifndef GL_GENERATE_MIPMAP_SGIS
#define GL_GENERATE_MIPMAP_SGIS       0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS  0x8192
#endif

Q_GLOBAL_STATIC(QGLShareRegister, _qgl_share_reg);
QGLShareRegister* qgl_share_reg()
{
    return _qgl_share_reg();
}

/*!
    \class QGLContext
    \brief The QGLContext class encapsulates an OpenGL rendering context.

    \ingroup multimedia

    An OpenGL
    \footnote
        OpenGL is a trademark of Silicon Graphics, Inc. in the
        United States and other countries.
    \endfootnote
    rendering context is a complete set of OpenGL state variables.

    The context's \link QGL::FormatOption format\endlink is set in the
    constructor or later with setFormat(). The format options that are
    actually set are returned by format(); the options you asked for
    are returned by requestedFormat(). Note that after a QGLContext
    object has been constructed, the actual OpenGL context must be
    created by explicitly calling the \link create() create()\endlink
    function. The makeCurrent() function makes this context the
    current rendering context. You can make \e no context current
    using doneCurrent(). The reset() function will reset the context
    and make it invalid.

    You can examine properties of the context with, e.g. isValid(),
    isSharing(), initialized(), windowCreated() and
    overlayTransparentColor().

    If you're using double buffering you can swap the screen contents
    with the off-screen buffer using swapBuffers().

    Please note that QGLContext is not thread safe.
*/


/*!
    Constructs an OpenGL context for the paint device \a device, which
    can be a widget or a pixmap. The \a format specifies several
    display options for the context.

    If the underlying OpenGL/Window system cannot satisfy all the
    features requested in \a format, the nearest subset of features
    will be used. After creation, the format() method will return the
    actual format obtained.

    Note that after a QGLContext object has been constructed, \link
    create() create()\endlink must be called explicitly to create
    the actual OpenGL context. The context will be \link isValid()
    invalid\endlink if it was not possible to obtain a GL context at
    all.

    \sa format(), isValid()
*/

QGLContext::QGLContext(const QGLFormat &format, QPaintDevice *device)
{
    d_ptr = new QGLContextPrivate(this);
    Q_D(QGLContext);
    d->init(device, format);
}

/*!
    \overload
    \internal
*/
QGLContext::QGLContext(const QGLFormat &format)
{
    d_ptr = new QGLContextPrivate(this);
    Q_D(QGLContext);
    d->init(0, format);
}

/*!
    Destroys the OpenGL context and frees its resources.
*/

QGLContext::~QGLContext()
{
    Q_D(QGLContext);
    // remove any textures cached in this context
    if (qt_tex_cache) {
        QList<QString> keys = qt_tex_cache->keys();
        for (int i = 0; i < keys.size(); ++i) {
            const QString &key = keys.at(i);
            if (qt_tex_cache->object(key)->context == this)
                qt_tex_cache->remove(key);
        }
        // ### thread safety
        if (qt_tex_cache->size() == 0) {
            qt_pixmap_cleanup_hook = 0;
            qt_image_cleanup_hook = 0;
            delete qt_tex_cache;
            qt_tex_cache = 0;
        }
    }

    reset();
    delete d;
}

/*! \overload

    Reads the DirectDrawSurface (DDS) compressed file \a fileName and
    generates a 2D GL texture from it.

    Only the DXT1, DXT3 and DXT5 DDS formats are supported.

    Note that this will only work if the implementation supports the
    \c GL_ARB_texture_compression and \c GL_EXT_texture_compression_s3tc
    extensions.

    \sa deleteTexture()
*/
GLuint QGLContext::bindTexture(const QString &fileName)
{
    if (!qt_glCompressedTexImage2DARB) {
        qWarning("QGLContext::bindTexture(): The GL implementation does not support texture"
                 "compression extensions.");
        return 0;
    }

    if (!qt_tex_cache)
        qt_tex_cache = new QGLTextureCache(qt_tex_cache_limit);

    QString key(fileName);
    QGLTexture *texture = qt_tex_cache->object(key);

    if (texture && texture->context == this) {
        glBindTexture(GL_TEXTURE_2D, texture->id);
        return texture->id;
    }

    QFile f(fileName);
    f.open(QIODevice::ReadOnly);

    char tag[4];
    f.read(&tag[0], 4);
    if (strncmp(tag,"DDS ", 4) != 0) {
        qWarning("QGLContext::bindTexture(): not a DDS image file.");
        return 0;
    }

    DDSFormat ddsHeader;
    f.read((char *) &ddsHeader, sizeof(DDSFormat));

    if (!ddsHeader.dwLinearSize) {
        qWarning("QGLContext::bindTexture() DDS image size is not valid.");
        return 0;
    }

    int factor = 4;
    int bufferSize = 0;
    int blockSize = 16;
    GLenum format;

    switch(ddsHeader.ddsPixelFormat.dwFourCC) {
    case FOURCC_DXT1:
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        factor = 2;
        blockSize = 8;
        break;
    case FOURCC_DXT3:
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;
    case FOURCC_DXT5:
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
    default:
        qWarning("QGLContext::bindTexture() DDS image format not supported.");
        return 0;
    }

    if (ddsHeader.dwMipMapCount > 1)
        bufferSize = ddsHeader.dwLinearSize * factor;
    else
        bufferSize = ddsHeader.dwLinearSize;

    GLubyte *pixels = (GLubyte *) malloc(bufferSize*sizeof(GLubyte));
    f.seek(ddsHeader.dwSize + 4);
    f.read((char *) pixels, bufferSize);
    f.close();

    GLuint tx_id;
    glGenTextures(1, &tx_id);
    glBindTexture(GL_TEXTURE_2D, tx_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int size;
    int offset = 0;
    int w = ddsHeader.dwWidth;
    int h = ddsHeader.dwHeight;

    // load mip-maps
    for(int i = 0; i < (int) ddsHeader.dwMipMapCount; ++i) {
        if (w == 0) w = 1;
        if (h == 0) h = 1;

        size = ((w+3)/4) * ((h+3)/4) * blockSize;
        qt_glCompressedTexImage2DARB(GL_TEXTURE_2D, i, format, w, h, 0,
                                     size, pixels + offset);
        offset += size;

        // half size for each mip-map level
        w = w/2;
        h = h/2;
    }

    free(pixels);

    int cost = bufferSize/1024;
    qt_tex_cache->insert(key, new QGLTexture(this, tx_id, 0), cost);
    return tx_id;
}

/*
  a hook that removes textures from the cache when a pixmap/image
  is deref'ed
*/
static void qt_gl_clean_cache(const QString &serial)
{
    const QList<QString> keys = qt_tex_cache->keys();
    for (int i = 0; i < keys.count(); ++i) {
        const QString &key = keys.at(i);
        if (key.startsWith(serial)) {
            if (qt_tex_cache->object(key)->clean)
                qt_tex_cache->remove(key);
            break;
        }
    }
}

static void qt_gl_pixmap_cleanup(int serial)
{
    if (qt_tex_cache)
        qt_gl_clean_cache(QString().sprintf("p%08x", serial));
}

static void qt_gl_image_cleanup(int serial)
{
    if (qt_tex_cache)
        qt_gl_clean_cache(QString().sprintf("i%08x", serial));
}


QImage QGLContextPrivate::convertToBGRA(const QImage &image, bool force_premul)
{
    QImage img = image;

    if ((force_premul && image.format() != QImage::Format_ARGB32_Premultiplied)
        || (!force_premul && image.format() != QImage::Format_ARGB32_Premultiplied
            && image.format() != QImage::Format_ARGB32)) {
        img = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        // mirror + swizzle
        QImage res = img.copy();
        for (int i=0; i < img.height(); i++) {
            uint *p = (uint*) img.scanLine(i);
            uint *q = (uint*) res.scanLine(img.height() - i - 1);
            uint *end = p + img.width();
            while (p < end) {
                *q = ((*p << 24) & 0xff000000)
                     | ((*p >> 24) & 0x000000ff)
                     | ((*p << 8) & 0x00ff0000)
                     | ((*p >> 8) & 0x0000ff00);
                p++;
                q++;
            }
        }
        return res;
    } else {
        return img.mirrored();
    }
}

GLuint QGLContextPrivate::bindTexture(const QImage &image, GLenum target, GLint format,
                                      const QString &key, qint64 qt_id, bool clean)
{
    Q_Q(QGLContext);

    if (!qt_tex_cache) {
        qt_tex_cache = new QGLTextureCache(qt_tex_cache_limit);
        qt_pixmap_cleanup_hook = qt_gl_pixmap_cleanup;
        qt_image_cleanup_hook = qt_gl_image_cleanup;
    }

    // Scale the pixmap if needed. GL textures needs to have the
    // dimensions 2^n+2(border) x 2^m+2(border).
    QImage tx;
    int tx_w = nearest_gl_texture_size(image.width());
    int tx_h = nearest_gl_texture_size(image.height());

    // Note: the clean param is only true when a texture is bound
    // from the QOpenGLPaintEngine - in that case we have to force
    // a premultiplied texture format
    if (target == GL_TEXTURE_2D && (tx_w != image.width() || tx_h != image.height()))
        tx = convertToBGRA(image.scaled(tx_w, tx_h), clean);
    else
        tx = convertToBGRA(image, clean);

    GLuint tx_id;
    glGenTextures(1, &tx_id);
    glBindTexture(target, tx_id);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (QGLExtensions::glExtensions & QGLExtensions::GenerateMipmap
        && target == GL_TEXTURE_2D)
    {
        glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
        glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexImage2D(target, 0, format, tx.width(), tx.height(), 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, tx.bits());

    // this assumes the size of a texture is always smaller than the max cache size
    int cost = tx.width()*tx.height()*4/1024;
    if (qt_tex_cache->totalCost() + cost > qt_tex_cache->maxCost()) {
        // the cache is full - make an attempt to remove something
        const QList<QString> keys = qt_tex_cache->keys();
        int i = 0;
        while (i < qt_tex_cache->count()
               && (qt_tex_cache->totalCost() + cost > qt_tex_cache->maxCost())) {
            QGLTexture *tex = qt_tex_cache->object(keys.at(i));
            if (tex->context == q)
                qt_tex_cache->remove(keys.at(i));
            ++i;
        }
    }
    qt_tex_cache->insert(key, new QGLTexture(q, tx_id, qt_id, clean), cost);
    return tx_id;
}

bool QGLContextPrivate::textureCacheLookup(const QString &key, GLuint *id, qint64 *qt_id)
{
    Q_Q(QGLContext);
    if (qt_tex_cache) {
        QGLTexture *texture = qt_tex_cache->object(key);
        if (texture && texture->context == q) {
            *id = texture->id;
            *qt_id = texture->qt_id;
            return true;
        }
    }
    return false;
}

/*! \internal */
GLuint QGLContextPrivate::bindTexture(const QImage &image, GLenum target, GLint format, bool clean)
{
    Q_Q(QGLContext);
    const QString key = QString(QLatin1String("%1_%2_%3")).arg(QString().sprintf("i%08x",image.serialNumber())).arg(target).arg(format);
    GLuint id;
    qint64 qt_id;
    if (textureCacheLookup(key, &id, &qt_id)) {
        if (qt_image_id(image) == qt_id) {
            glBindTexture(target, id);
            return id;
        } else {
            q->deleteTexture(id);
        }
    }
    return bindTexture(image, target, format, key, qt_image_id(image), clean);
}

/*! \internal */
GLuint QGLContextPrivate::bindTexture(const QPixmap &pixmap, GLenum target, GLint format, bool clean)
{
    Q_Q(QGLContext);
    const QString key = QString(QLatin1String("%1_%2_%3")).arg(QString().sprintf("p%08x",pixmap.serialNumber())).arg(target).arg(format);
    GLuint id;
    qint64 qt_id;
    if (textureCacheLookup(key, &id, &qt_id)) {
        if (qt_pixmap_id(pixmap) == qt_id) {
            glBindTexture(target, id);
            return id;
        } else {
            q->deleteTexture(id);
        }
    }
    return bindTexture(pixmap.toImage(), target, format, key, qt_pixmap_id(pixmap), clean);
}

/*!
    Generates and binds a 2D GL texture to the current context, based
    on \a image. The generated texture id is returned and can be used
    in later \c glBindTexture() calls.

    The \a target parameter specifies the texture target. The default
    target is \c GL_TEXTURE_2D.

    The \a format parameter sets the internal format for the
    texture. The default format is \c GL_RGBA8.

    If the GL implementation supports the \c GL_SGIS_generate_mipmap
    extension, mipmaps will be automatically generated for the
    texture. Mipmap generation is only supported for the \c
    GL_TEXTURE_2D target.

    The texture that is generated is cached, so multiple calls to
    bindTexture() with the same QImage will return the same texture
    id.

    \sa deleteTexture()
*/
GLuint QGLContext::bindTexture(const QImage &image, GLenum target, GLint format)
{
    Q_D(QGLContext);
    return d->bindTexture(image, target, format, false);
}

/*! \overload

    Generates and binds a 2D GL texture based on \a pixmap.
*/
GLuint QGLContext::bindTexture(const QPixmap &pixmap, GLenum target, GLint format)
{
    Q_D(QGLContext);
    return d->bindTexture(pixmap, target, format, false);
}

/*!
    Removes the texture identified by \a id from the texture cache. If
    the context is not shared by any other QGLContext,
    glDeleteTextures() will be called to delete the texture from the
    context.

    \sa bindTexture()
*/
void QGLContext::deleteTexture(GLuint id)
{
    if (!qt_tex_cache)
        return;

    QList<QString> keys = qt_tex_cache->keys();
    for (int i = 0; i < keys.size(); ++i) {
        QGLTexture *tex = qt_tex_cache->object(keys.at(i));
        if (tex->id == id && tex->context == this) {
            qt_tex_cache->remove(keys.at(i));
            break;
        }
    }
}

/*!
    This function sets the limit for the texture cache to \a size,
    expressed in kilobytes.

    By default, the cache limit is approximately 64 MB.

    \sa textureCacheLimit()
*/
void QGLContext::setTextureCacheLimit(int size)
{
    qt_tex_cache_limit = size;
    if (qt_tex_cache)
        qt_tex_cache->setMaxCost(qt_tex_cache_limit);
}

/*!
    Returns the current texture cache limit in kilobytes.

    \sa setTextureCacheLimit()
*/
int QGLContext::textureCacheLimit()
{
    return qt_tex_cache_limit;
}

/*!
    \fn QGLFormat QGLContext::format() const

    Returns the frame buffer format that was obtained (this may be a
    subset of what was requested).

    \sa requestedFormat()
*/

/*!
    \fn QGLFormat QGLContext::requestedFormat() const

    Returns the frame buffer format that was originally requested in
    the constructor or setFormat().

    \sa format()
*/

/*!
    Sets a \a format for this context. The context is \link reset()
    reset\endlink.

    Call create() to create a new GL context that tries to match the
    new format.

    \code
    QGLContext *cx;
    //  ...
    QGLFormat f;
    f.setStereo(true);
    cx->setFormat(f);
    if (!cx->create())
        exit(); // no OpenGL support, or cannot render on the specified paintdevice
    if (!cx->format().stereo())
        exit(); // could not create stereo context
    \endcode

    \sa format(), reset(), create()
*/

void QGLContext::setFormat(const QGLFormat &format)
{
    Q_D(QGLContext);
    reset();
    d->glFormat = d->reqFormat = format;
}

/*!
    \internal
*/
void QGLContext::setDevice(QPaintDevice *pDev)
{
    Q_D(QGLContext);
    if (isValid())
        reset();
    d->paintDevice = pDev;
    if (d->paintDevice && (d->paintDevice->devType() != QInternal::Widget
                           && d->paintDevice->devType() != QInternal::Pixmap
                           && d->paintDevice->devType() != QInternal::Pbuffer)) {
        qWarning("QGLContext: Unsupported paint device type");
    }
}

/*!
    \fn bool QGLContext::isValid() const

    Returns true if a GL rendering context has been successfully
    created; otherwise returns false.
*/

/*!
    \fn void QGLContext::setValid(bool valid)
    \internal

    Forces the GL rendering context to be valid.
*/

/*!
    \fn bool QGLContext::isSharing() const

    Returns true if this context is sharing its GL context with
    another QGLContext, otherwise false is returned. Note that context
    sharing might not be supported between contexts with different
    formats.
*/

/*!
    \fn bool QGLContext::deviceIsPixmap() const

    Returns true if the paint device of this context is a pixmap;
    otherwise returns false.
*/

/*!
    \fn bool QGLContext::windowCreated() const

    Returns true if a window has been created for this context;
    otherwise returns false.

    \sa setWindowCreated()
*/

/*!
    \fn void QGLContext::setWindowCreated(bool on)

    If \a on is true the context has had a window created for it. If
    \a on is false no window has been created for the context.

    \sa windowCreated()
*/

/*!
    \fn uint QGLContext::colorIndex(const QColor& c) const

    \internal

    Returns a colormap index for the color c, in ColorIndex mode. Used
    by qglColor() and qglClearColor().
*/


/*!
    \fn bool QGLContext::initialized() const

    Returns true if this context has been initialized, i.e. if
    QGLWidget::initializeGL() has been performed on it; otherwise
    returns false.

    \sa setInitialized()
*/

/*!
    \fn void QGLContext::setInitialized(bool on)

    If \a on is true the context has been initialized, i.e.
    QGLContext::setInitialized() has been called on it. If \a on is
    false the context has not been initialized.

    \sa initialized()
*/

/*!
    \fn const QGLContext* QGLContext::currentContext()

    Returns the current context, i.e. the context to which any OpenGL
    commands will currently be directed. Returns 0 if no context is
    current.

    \sa makeCurrent()
*/

/*!
    \fn QColor QGLContext::overlayTransparentColor() const

    If this context is a valid context in an overlay plane, returns
    the plane's transparent color. Otherwise returns an \link
    QColor::isValid() invalid \endlink color.

    The returned color's \link QColor::pixel() pixel \endlink value is
    the index of the transparent color in the colormap of the overlay
    plane. (Naturally, the color's RGB values are meaningless.)

    The returned QColor object will generally work as expected only
    when passed as the argument to QGLWidget::qglColor() or
    QGLWidget::qglClearColor(). Under certain circumstances it can
    also be used to draw transparent graphics with a QPainter. See the
    examples/opengl/overlay_x11 example for details.
*/


/*!
    Creates the GL context. Returns true if it was successful in
    creating a valid GL rendering context on the paint device
    specified in the constructor; otherwise returns false (i.e. the
    context is invalid).

    After successful creation, format() returns the set of features of
    the created GL rendering context.

    If \a shareContext points to a valid QGLContext, this method will
    try to establish OpenGL display list sharing between this context
    and the \a shareContext. Note that this may fail if the two
    contexts have different formats. Use isSharing() to see if sharing
    succeeded.

    \warning Implementation note: initialization of C++ class
    members usually takes place in the class constructor. QGLContext
    is an exception because it must be simple to customize. The
    virtual functions chooseContext() (and chooseVisual() for X11) can
    be reimplemented in a subclass to select a particular context. The
    problem is that virtual functions are not properly called during
    construction (even though this is correct C++) because C++
    constructs class hierarchies from the bottom up. For this reason
    we need a create() function.

    \sa chooseContext(), format(), isValid()
*/

bool QGLContext::create(const QGLContext* shareContext)
{
    Q_D(QGLContext);
    reset();
    d->valid = chooseContext(shareContext);
    if (d->sharing)  // ok, we managed to share
        qgl_share_reg()->addShare(this, shareContext);
    return d->valid;
}

bool QGLContext::isValid() const
{
    Q_D(const QGLContext);
    return d->valid;
}

void QGLContext::setValid(bool valid)
{
    Q_D(QGLContext);
    d->valid = valid;
}

bool QGLContext::isSharing() const
{
    Q_D(const QGLContext);
    return d->sharing;
}

QGLFormat QGLContext::format() const
{
    Q_D(const QGLContext);
    return d->glFormat;
}

QGLFormat QGLContext::requestedFormat() const
{
    Q_D(const QGLContext);
    return d->reqFormat;
}

 QPaintDevice* QGLContext::device() const
{
    Q_D(const QGLContext);
    return d->paintDevice;
}

bool QGLContext::deviceIsPixmap() const
{
    Q_D(const QGLContext);
    return d->paintDevice->devType() == QInternal::Pixmap;
}


bool QGLContext::windowCreated() const
{
    Q_D(const QGLContext);
    return d->crWin;
}


void QGLContext::setWindowCreated(bool on)
{
    Q_D(QGLContext);
    d->crWin = on;
}

bool QGLContext::initialized() const
{
    Q_D(const QGLContext);
    return d->initDone;
}

void QGLContext::setInitialized(bool on)
{
    Q_D(QGLContext);
    d->initDone = on;
}

const QGLContext* QGLContext::currentContext()
{
    if (qgl_context_storage.hasLocalData())
        return qgl_context_storage.localData()->context;
    return 0;
}

/*!
    \fn bool QGLContext::chooseContext(const QGLContext* shareContext = 0)

    This semi-internal function is called by create(). It creates a
    system-dependent OpenGL handle that matches the format() of \a
    shareContext as closely as possible.

    On Windows, it calls the virtual function choosePixelFormat(),
    which finds a matching pixel format identifier. On X11, it calls
    the virtual function chooseVisual() which finds an appropriate X
    visual. On other platforms it may work differently.
*/


/*!
    \fn void QGLContext::reset()

    Resets the context and makes it invalid.

    \sa create(), isValid()
*/


/*!
    \fn void QGLContext::makeCurrent()

    Makes this context the current OpenGL rendering context. All GL
    functions you call operate on this context until another context
    is made current.

    In some very rare cases the underlying call may fail. If this
    occurs an error message is output to stderr.
*/


/*!
    \fn void QGLContext::swapBuffers() const

    Swaps the screen contents with an off-screen buffer. Only works if
    the context is in double buffer mode.

    \sa QGLFormat::setDoubleBuffer()
*/


/*!
    \fn void QGLContext::doneCurrent()

    Makes no GL context the current context. Normally, you do not need
    to call this function; QGLContext calls it as necessary.
*/


/*!
    \fn QPaintDevice* QGLContext::device() const

    Returns the paint device set for this context.

    \sa QGLContext::QGLContext()
*/

/*!
    \fn void QGLContext::generateFontDisplayLists(const QFont& font, int listBase)

    Generates a set of 256 display lists for the 256 first characters
    in the font \a font. The first list will start at index \a listBase.

    \sa QGLWidget::renderText()
*/


/*****************************************************************************
  QGLWidget implementation
 *****************************************************************************/


/*!
    \class QGLWidget
    \brief The QGLWidget class is a widget for rendering OpenGL graphics.

    \ingroup multimedia
    \mainclass

    QGLWidget provides functionality for displaying OpenGL graphics integrated into
    a Qt application. It is very simple to use. You inherit from it and use the
    subclass like any other QWidget, except that you have the choice between
    using QPainter and standard OpenGL rendering commands.

    QGLWidget provides three convenient virtual functions that you can
    reimplement in your subclass to perform the typical OpenGL tasks:

    \list
    \i paintGL() - Renders the OpenGL scene. Gets called whenever the widget
    needs to be updated.
    \i resizeGL() - Sets up the OpenGL viewport, projection, etc. Gets
    called whenever the the widget has been resized (and also when it
    is shown for the first time because all newly created widgets get a
    resize event automatically).
    \i initializeGL() - Sets up the OpenGL rendering context, defines display
    lists, etc. Gets called once before the first time resizeGL() or
    paintGL() is called.
    \endlist

    Here is a rough outline of how a QGLWidget subclass might look:

    \code
    class MyGLDrawer : public QGLWidget
    {
        Q_OBJECT        // must include this if you use Qt signals/slots

    public:
        MyGLDrawer(QWidget *parent)
            : QGLWidget(parent) {}

    protected:

        void initializeGL()
        {
            // Set up the rendering context, define display lists etc.:
            ...
            glClearColor(0.0, 0.0, 0.0, 0.0);
            glEnable(GL_DEPTH_TEST);
            ...
        }

        void resizeGL(int w, int h)
        {
            // setup viewport, projection etc.:
            glViewport(0, 0, (GLint)w, (GLint)h);
            ...
            glFrustum(...);
            ...
        }

        void paintGL()
        {
            // draw the scene:
            ...
            glRotatef(...);
            glMaterialfv(...);
            glBegin(GL_QUADS);
            glVertex3f(...);
            glVertex3f(...);
            ...
            glEnd();
            ...
        }

    };
    \endcode

    If you need to trigger a repaint from places other than paintGL()
    (a typical example is when using \link QTimer timers\endlink to
    animate scenes), you should call the widget's updateGL() function.

    Your widget's OpenGL rendering context is made current when
    paintGL(), resizeGL(), or initializeGL() is called. If you need to
    call the standard OpenGL API functions from other places (e.g. in
    your widget's constructor or in your own paint functions), you
    must call makeCurrent() first.

    QGLWidget provides functions for requesting a new display \link
    QGLFormat format\endlink and you can also create widgets with
    customized rendering \link QGLContext contexts\endlink.

    You can also share OpenGL display lists between QGLWidgets (see
    the documentation of the QGLWidget constructors for details).

    \section1 Overlays

    The QGLWidget creates a GL overlay context in addition to the
    normal context if overlays are supported by the underlying system.

    If you want to use overlays, you specify it in the \link QGLFormat
    format\endlink. (Note: Overlay must be requested in the format
    passed to the QGLWidget constructor.) Your GL widget should also
    implement some or all of these virtual methods:

    \list
    \i paintOverlayGL()
    \i resizeOverlayGL()
    \i initializeOverlayGL()
    \endlist

    These methods work in the same way as the normal paintGL() etc.
    functions, except that they will be called when the overlay
    context is made current. You can explicitly make the overlay
    context current by using makeOverlayCurrent(), and you can access
    the overlay context directly (e.g. to ask for its transparent
    color) by calling overlayContext().

    On X servers in which the default visual is in an overlay plane,
    non-GL Qt windows can also be used for overlays.

    \section1 Painting Techniques

    As described above, subclass QGLWidget to render pure 3D content in the
    following way:

    \list
    \o Reimplement the QGLWidget::initializeGL() and QGLWidget::resizeGL() to
       set up the OpenGL state and provide a perspective transformation.
    \o Reimplement QGLWidget::paintGL() to paint the 3D scene, calling only
       OpenGL functions to draw on the widget.
    \endlist

    It is also possible to draw 2D graphics onto a QGLWidget subclass, it is necessary
    to reimplement QGLWidget::paintEvent() and do the following:

    \list
    \o Construct a QPainter object.
    \o Initialize it for use on the widget with the QPainter::begin() function.
    \o Draw primitives using QPainter's member functions.
    \o Call QPainter::end() to finish painting.
    \endlist

    Overpainting 2D content on top of 3D content takes a little more effort.
    One approach to doing this is shown in the
    \l{Overpainting Example}{Overpainting} example.

    \e{OpenGL is a trademark of Silicon Graphics, Inc. in the United States and other
    countries.}

    \sa QGLPixelBuffer, {Hello GL Example}, {2D Painting Example}, {Overpainting Example},
        {Grabber Example}
*/

/*!
    Constructs an OpenGL widget with a \a parent widget.

    The \link QGLFormat::defaultFormat() default format\endlink is
    used. The widget will be \link isValid() invalid\endlink if the
    system has no \link QGLFormat::hasOpenGL() OpenGL support\endlink.

    The \a parent and widget flag, \a f, arguments are passed
    to the QWidget constructor.

    If the \a shareWidget parameter points to a valid QGLWidget, this
    widget will share OpenGL display lists with \a shareWidget. If
    this widget and \a shareWidget have different \link format()
    formats\endlink, display list sharing may fail. You can check
    whether display list sharing succeeded by calling isSharing().

    The initialization of OpenGL rendering state, etc. should be done
    by overriding the initializeGL() function, rather than in the
    constructor of your QGLWidget subclass.

    \sa QGLFormat::defaultFormat(), {Textures Example}
*/

QGLWidget::QGLWidget(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
    : QWidget(*(new QGLWidgetPrivate), parent, f | Qt::MSWindowsOwnDC)
{
    Q_D(QGLWidget);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(true); // for compatibility
    d->init(new QGLContext(QGLFormat::defaultFormat(), this), shareWidget);
}


/*!
    Constructs an OpenGL widget with parent \a parent.

    The \a format argument specifies the desired \link QGLFormat
    rendering options \endlink. If the underlying OpenGL/Window system
    cannot satisfy all the features requested in \a format, the
    nearest subset of features will be used. After creation, the
    format() method will return the actual format obtained.

    The widget will be \link isValid() invalid\endlink if the system
    has no \link QGLFormat::hasOpenGL() OpenGL support\endlink.

    The \a parent and widget flag, \a f, arguments are passed
    to the QWidget constructor.

    If the \a shareWidget parameter points to a valid QGLWidget, this
    widget will share OpenGL display lists with \a shareWidget. If
    this widget and \a shareWidget have different \link format()
    formats\endlink, display list sharing may fail. You can check
    whether display list sharing succeeded by calling isSharing().

    The initialization of OpenGL rendering state, etc. should be done
    by overriding the initializeGL() function, rather than in the
    constructor of your QGLWidget subclass.

    \sa QGLFormat::defaultFormat(), isValid()
*/

QGLWidget::QGLWidget(const QGLFormat &format, QWidget *parent, const QGLWidget* shareWidget,
                     Qt::WindowFlags f)
    : QWidget(*(new QGLWidgetPrivate), parent, f | Qt::MSWindowsOwnDC)
{
    Q_D(QGLWidget);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(true); // for compatibility
    d->init(new QGLContext(format, this), shareWidget);
}

/*!
    Constructs an OpenGL widget with parent \a parent.

    The \a context argument is a pointer to the QGLContext that
    you wish to be bound to this widget. This allows you to pass in
    your own QGLContext sub-classes.

    The widget will be \link isValid() invalid\endlink if the system
    has no \link QGLFormat::hasOpenGL() OpenGL support\endlink.

    The \a parent and widget flag, \a f, arguments are passed
    to the QWidget constructor.

    If the \a shareWidget parameter points to a valid QGLWidget, this
    widget will share OpenGL display lists with \a shareWidget. If
    this widget and \a shareWidget have different \link format()
    formats\endlink, display list sharing may fail. You can check
    whether display list sharing succeeded by calling isSharing().

    The initialization of OpenGL rendering state, etc. should be done
    by overriding the initializeGL() function, rather than in the
    constructor of your QGLWidget subclass.

    \sa QGLFormat::defaultFormat(), isValid()
*/
QGLWidget::QGLWidget(QGLContext *context, QWidget *parent, const QGLWidget *shareWidget,
                     Qt::WindowFlags f)
    : QWidget(*(new QGLWidgetPrivate), parent, f | Qt::MSWindowsOwnDC)
{
    Q_D(QGLWidget);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(true); // for compatibility
    d->init(context, shareWidget);
}

/*!
    Destroys the widget.
*/

QGLWidget::~QGLWidget()
{
    Q_D(QGLWidget);
#if defined(GLX_MESA_release_buffers) && defined(QGL_USE_MESA_EXT)
    bool doRelease = (glcx && glcx->windowCreated());
#endif
    delete d->glcx;
#if defined(Q_WGL)
    delete d->olcx;
#endif
#ifdef Q_WS_MAC
    delete d->watcher;
    d->watcher = 0;
#endif
#if defined(GLX_MESA_release_buffers) && defined(QGL_USE_MESA_EXT)
    if (doRelease)
        glXReleaseBuffersMESA(x11Display(), winId());
#endif
    d->cleanupColormaps();
}

/*!
    \fn QGLFormat QGLWidget::format() const

    Returns the format of the contained GL rendering context.
*/

/*!
    \fn bool QGLWidget::doubleBuffer() const

    Returns true if the contained GL rendering context has double
    buffering; otherwise returns false.

    \sa QGLFormat::doubleBuffer()
*/

/*!
    \fn void QGLWidget::setAutoBufferSwap(bool on)

    If \a on is true automatic GL buffer swapping is switched on;
    otherwise it is switched off.

    If \a on is true and the widget is using a double-buffered format,
    the background and foreground GL buffers will automatically be
    swapped after each paintGL() call.

    The buffer auto-swapping is on by default.

    \sa autoBufferSwap(), doubleBuffer(), swapBuffers()
*/

/*!
    \fn bool QGLWidget::autoBufferSwap() const

    Returns true if the widget is doing automatic GL buffer swapping;
    otherwise returns false.

    \sa setAutoBufferSwap()
*/

/*!
    \fn bool QGLWidget::isValid() const

    Returns true if the widget has a valid GL rendering context;
    otherwise returns false. A widget will be invalid if the system
    has no \link QGLFormat::hasOpenGL() OpenGL support\endlink.
*/

bool QGLWidget::isValid() const
{
    Q_D(const QGLWidget);
    return d->glcx->isValid();
}

/*!
    \fn bool QGLWidget::isSharing() const

    Returns true if this widget's GL context is shared with another GL
    context, otherwise false is returned. The GL system may fail to
    provide context sharing if the two QGLWidgets use different
    formats.

    \sa format()
*/

bool QGLWidget::isSharing() const
{
    Q_D(const QGLWidget);
    return d->glcx->isSharing();
}

/*!
    \fn void QGLWidget::makeCurrent()

    Makes this widget the current widget for OpenGL operations, i.e.
    makes the widget's rendering context the current OpenGL rendering
    context.
*/

void QGLWidget::makeCurrent()
{
    Q_D(QGLWidget);
    d->glcx->makeCurrent();
}

/*!
    \fn void QGLWidget::doneCurrent()

    Makes no GL context the current context. Normally, you do not need
    to call this function; QGLContext calls it as necessary. However,
    it may be useful in multithreaded environments.
*/

void QGLWidget::doneCurrent()
{
    Q_D(QGLWidget);
    d->glcx->doneCurrent();
}

/*!
    \fn void QGLWidget::swapBuffers()

    Swaps the screen contents with an off-screen buffer. This only
    works if the widget's format specifies double buffer mode.

    Normally, there is no need to explicitly call this function
    because it is done automatically after each widget repaint, i.e.
    each time after paintGL() has been executed.

    \sa doubleBuffer(), setAutoBufferSwap(), QGLFormat::setDoubleBuffer()
*/

void QGLWidget::swapBuffers()
{
    Q_D(QGLWidget);
    d->glcx->swapBuffers();
}


/*!
    \fn const QGLContext* QGLWidget::overlayContext() const

    Returns the overlay context of this widget, or 0 if this widget
    has no overlay.

    \sa context()
*/



/*!
    \fn void QGLWidget::makeOverlayCurrent()

    Makes the overlay context of this widget current. Use this if you
    need to issue OpenGL commands to the overlay context outside of
    initializeOverlayGL(), resizeOverlayGL(), and paintOverlayGL().

    Does nothing if this widget has no overlay.

    \sa makeCurrent()
*/


/*!
  \obsolete

  Sets a new format for this widget.

  If the underlying OpenGL/Window system cannot satisfy all the
  features requested in \a format, the nearest subset of features will
  be used. After creation, the format() method will return the actual
  rendering context format obtained.

  The widget will be assigned a new QGLContext, and the initializeGL()
  function will be executed for this new context before the first
  resizeGL() or paintGL().

  This method will try to keep any existing display list sharing with
  other QGLWidgets, but it may fail. Use isSharing() to test.

  \sa format(), isSharing(), isValid()
*/

void QGLWidget::setFormat(const QGLFormat &format)
{
    setContext(new QGLContext(format,this));
}




/*!
    \fn const QGLContext *QGLWidget::context() const

    Returns the context of this widget.

    It is possible that the context is not valid (see isValid()), for
    example, if the underlying hardware does not support the format
    attributes that were requested.
*/

/*
  \obsolete

  \fn void QGLWidget::setContext(QGLContext *context,
                                  const QGLContext* shareContext,
                                  bool deleteOldContext)

  Sets a new context for this widget. The QGLContext \a context must
  be created using \e new. QGLWidget will delete \a context when
  another context is set or when the widget is destroyed.

  If \a context is invalid, QGLContext::create() is performed on
  it. The initializeGL() function will then be executed for the new
  context before the first resizeGL() or paintGL().

  If \a context is invalid, this method will try to keep any existing
  display list sharing with other QGLWidgets this widget currently
  has, or (if \a shareContext points to a valid context) start display
  list sharing with that context, but it may fail. Use isSharing() to
  test.

  If \a deleteOldContext is true (the default), the existing context
  will be deleted. You may use false here if you have kept a pointer
  to the old context (as returned by context()), and want to restore
  that context later.

  \sa context(), isSharing()
*/



/*!
    \fn void QGLWidget::updateGL()

    Updates the widget by calling glDraw().
*/

void QGLWidget::updateGL()
{
    if (updatesEnabled())
        glDraw();
}


/*!
    \fn void QGLWidget::updateOverlayGL()

    Updates the widget's overlay (if any). Will cause the virtual
    function paintOverlayGL() to be executed.

    The widget's rendering context will become the current context and
    initializeGL() will be called if it hasn't already been called.
*/


/*!
    This virtual function is called once before the first call to
    paintGL() or resizeGL(), and then once whenever the widget has
    been assigned a new QGLContext. Reimplement it in a subclass.

    This function should set up any required OpenGL context rendering
    flags, defining display lists, etc.

    There is no need to call makeCurrent() because this has already
    been done when this function is called.
*/

void QGLWidget::initializeGL()
{
}


/*!
    This virtual function is called whenever the widget needs to be
    painted. Reimplement it in a subclass.

    There is no need to call makeCurrent() because this has already
    been done when this function is called.
*/

void QGLWidget::paintGL()
{
}


/*!
    \fn void QGLWidget::resizeGL(int width , int height)

    This virtual function is called whenever the widget has been
    resized. The new size is passed in \a width and \a height.
    Reimplement it in a subclass.

    There is no need to call makeCurrent() because this has already
    been done when this function is called.
*/

void QGLWidget::resizeGL(int, int)
{
}



/*!
    This virtual function is used in the same manner as initializeGL()
    except that it operates on the widget's overlay context instead of
    the widget's main context. This means that initializeOverlayGL()
    is called once before the first call to paintOverlayGL() or
    resizeOverlayGL(). Reimplement it in a subclass.

    This function should set up any required OpenGL context rendering
    flags, defining display lists, etc. for the overlay context.

    There is no need to call makeOverlayCurrent() because this has
    already been done when this function is called.
*/

void QGLWidget::initializeOverlayGL()
{
}


/*!
    This virtual function is used in the same manner as paintGL()
    except that it operates on the widget's overlay context instead of
    the widget's main context. This means that paintOverlayGL() is
    called whenever the widget's overlay needs to be painted.
    Reimplement it in a subclass.

    There is no need to call makeOverlayCurrent() because this has
    already been done when this function is called.
*/

void QGLWidget::paintOverlayGL()
{
}


/*!
    \fn void QGLWidget::resizeOverlayGL(int width , int height)

    This virtual function is used in the same manner as paintGL()
    except that it operates on the widget's overlay context instead of
    the widget's main context. This means that resizeOverlayGL() is
    called whenever the widget has been resized. The new size is
    passed in \a width and \a height. Reimplement it in a subclass.

    There is no need to call makeOverlayCurrent() because this has
    already been done when this function is called.
*/

void QGLWidget::resizeOverlayGL(int, int)
{
}




/*!
    \fn void QGLWidget::paintEvent(QPaintEvent *event)

    Handles paint events passed in the \a event parameter. Will cause
    the virtual paintGL() function to be called.

    The widget's rendering context will become the current context and
    initializeGL() will be called if it hasn't already been called.
*/

void QGLWidget::paintEvent(QPaintEvent *)
{
    if (updatesEnabled()) {
        glDraw();
        updateOverlayGL();
    }
}


/*!
    \fn void QGLWidget::resizeEvent(QResizeEvent *event)

    Handles resize events that are passed in the \a event parameter.
    Calls the virtual function resizeGL().
*/


/*!
    \fn void QGLWidget::setMouseTracking(bool enable)

    If \a enable is true then mouse tracking is enabled; otherwise it
    is disabled.
*/


/*!
    Renders the current scene on a pixmap and returns the pixmap.

    You can use this method on both visible and invisible QGLWidgets.

    This method will create a pixmap and a temporary QGLContext to
    render on the pixmap. It will then call initializeGL(),
    resizeGL(), and paintGL() on this context. Finally, the widget's
    original GL context is restored.

    The size of the pixmap will be \a w pixels wide and \a h pixels
    high unless one of these parameters is 0 (the default), in which
    case the pixmap will have the same size as the widget.

    If \a useContext is true, this method will try to be more
    efficient by using the existing GL context to render the pixmap.
    The default is false. Only use true if you understand the risks.
    Note that under Windows a temporary context has to be created
    and usage of the \e useContext parameter is not supported.

    Overlays are not rendered onto the pixmap.

    If the GL rendering context and the desktop have different bit
    depths, the result will most likely look surprising.

    Note that the creation of display lists, modifications of the view
    frustum etc. should be done from within initializeGL(). If this is
    not done, the temporary QGLContext will not be initialized
    properly, and the rendered pixmap may be incomplete/corrupted.
*/

QPixmap QGLWidget::renderPixmap(int w, int h, bool useContext)
{
    Q_D(QGLWidget);
    QSize sz = size();
    if ((w > 0) && (h > 0))
        sz = QSize(w, h);

#if defined(Q_WS_X11)
    extern int qt_x11_preferred_pixmap_depth;
    int old_depth = qt_x11_preferred_pixmap_depth;
    qt_x11_preferred_pixmap_depth = x11Info().depth();
    QPixmap pm(sz);
    qt_x11_preferred_pixmap_depth = old_depth;
    QX11Info xinfo = x11Info();

    // make sure we use a pixmap with the same depth/visual as the widget
    if (xinfo.visual() != QX11Info::appVisual()) {
        QX11InfoData* xd = pm.x11Info().getX11Data(true);
        xd->depth = xinfo.depth();
        xd->visual = static_cast<Visual *>(xinfo.visual());
        const_cast<QX11Info &>(pm.x11Info()).setX11Data(xd);
    }

#else
    QPixmap pm(sz);
#endif

    d->glcx->doneCurrent();

    bool success = true;

    if (useContext && isValid() && d->renderCxPm(&pm))
        return pm;

    QGLFormat fmt = d->glcx->requestedFormat();
    fmt.setDirectRendering(false);                // Direct is unlikely to work
    fmt.setDoubleBuffer(false);                // We don't need dbl buf

    QGLContext* ocx = d->glcx;
    ocx->doneCurrent();
    d->glcx = new QGLContext(fmt, &pm);
    d->glcx->create();

    if (d->glcx->isValid())
        updateGL();
    else
        success = false;

    delete d->glcx;
    d->glcx = ocx;

    ocx->makeCurrent();

    if (success) {
#if defined(Q_WS_X11)
        if (xinfo.visual() != QX11Info::appVisual()) {
            QImage image = pm.toImage();
            QPixmap p = QPixmap::fromImage(image);
            return p;
        }
#endif
        return pm;
    }
    return QPixmap();
}



/*!
    Returns an image of the frame buffer. If \a withAlpha is true the
    alpha channel is included.

    Depending on your hardware, you can explicitly select which color
    buffer to grab with a glReadBuffer() call before calling this
    function.
*/
QImage QGLWidget::grabFrameBuffer(bool withAlpha)
{
    makeCurrent();
    QImage res;
    int w = width();
    int h = height();
    if (format().rgba()) {
        res = QImage(w, h, withAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, res.bits());
        if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            // OpenGL gives RGBA; Qt wants ARGB
            uint *p = (uint*)res.bits();
            uint *end = p + w*h;
            if (withAlpha && format().alpha()) {
                while (p < end) {
                    uint a = *p << 24;
                    *p = (*p >> 8) | a;
                    p++;
                }
            } else {
                while (p < end) {
                    *p = 0xFF000000 | (*p>>8);
                    ++p;
                }
            }
        } else {
            // OpenGL gives ABGR (i.e. RGBA backwards); Qt wants ARGB
            res = res.rgbSwapped();
        }
    } else {
#if defined (Q_WS_WIN)
        res = QImage(w, h, QImage::Format_Indexed8);
        glReadPixels(0, 0, w, h, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, res.bits());
        const QVector<QColor> pal = QColormap::instance().colormap();
        if (pal.size()) {
            res.setNumColors(pal.size());
            for (int i = 0; i < pal.size(); i++)
                res.setColor(i, pal.at(i).rgb());
        }
#endif
    }

    return res.mirrored();
}



/*!
    Initializes OpenGL for this widget's context. Calls the virtual
    function initializeGL().
*/

void QGLWidget::glInit()
{
    Q_D(QGLWidget);
    if (!isValid())
        return;
    makeCurrent();
    initializeGL();
    d->glcx->setInitialized(true);
}


/*!
    Executes the virtual function paintGL().

    The widget's rendering context will become the current context and
    initializeGL() will be called if it hasn't already been called.
*/

void QGLWidget::glDraw()
{
    Q_D(QGLWidget);
    if (!isValid())
        return;
    makeCurrent();
#ifndef Q_WS_QWS
    if (d->glcx->deviceIsPixmap())
        glDrawBuffer(GL_FRONT);
#endif
    if (!d->glcx->initialized()) {
        glInit();
        resizeGL(d->glcx->device()->width(), d->glcx->device()->height()); // New context needs this "resize"
    }
    paintGL();
    if (doubleBuffer()) {
        if (d->autoSwap)
            swapBuffers();
    } else {
        glFlush();
    }
}


/*!
    Convenience function for specifying a drawing color to OpenGL.
    Calls glColor4 (in RGBA mode) or glIndex (in color-index mode)
    with the color \a c. Applies to this widgets GL context.

    \sa qglClearColor(), QGLContext::currentContext(), QColor
*/

void QGLWidget::qglColor(const QColor& c) const
{
#ifdef Q_WS_QWS
    glColor4f(c.red()/255.0, c.green()/255.0, c.blue()/255.0, c.alpha()/255.0);
#else
    Q_D(const QGLWidget);
    const QGLContext *ctx = QGLContext::currentContext();
    if (ctx) {
        if (ctx->format().rgba())
            glColor4ub(c.red(), c.green(), c.blue(), c.alpha());
        else if (!d->cmap.isEmpty()) { // QGLColormap in use?
            int i = d->cmap.find(c.rgb());
            if (i < 0)
                i = d->cmap.findNearest(c.rgb());
            glIndexi(i);
        } else
            glIndexi(ctx->colorIndex(c));
    }
#endif
}

/*!
    Convenience function for specifying the clearing color to OpenGL.
    Calls glClearColor (in RGBA mode) or glClearIndex (in color-index
    mode) with the color \a c. Applies to this widgets GL context.

    \sa qglColor(), QGLContext::currentContext(), QColor
*/

void QGLWidget::qglClearColor(const QColor& c) const
{
#ifdef Q_WS_QWS
    glClearColor((GLfloat)c.red() / 255.0, (GLfloat)c.green() / 255.0,
                 (GLfloat)c.blue() / 255.0, (GLfloat) c.alpha() / 255.0);
#else
    Q_D(const QGLWidget);
    const QGLContext *ctx = QGLContext::currentContext();
    if (ctx) {
        if (ctx->format().rgba())
            glClearColor((GLfloat)c.red() / 255.0, (GLfloat)c.green() / 255.0,
                          (GLfloat)c.blue() / 255.0, (GLfloat) c.alpha() / 255.0);
        else if (!d->cmap.isEmpty()) { // QGLColormap in use?
            int i = d->cmap.find(c.rgb());
            if (i < 0)
                i = d->cmap.findNearest(c.rgb());
            glClearIndex(i);
        } else
            glClearIndex(ctx->colorIndex(c));
    }
#endif
}


/*!
    Converts the image \a img into the unnamed format expected by
    OpenGL functions such as glTexImage2D(). The returned image is not
    usable as a QImage, but QImage::width(), QImage::height() and
    QImage::bits() may be used with OpenGL.

    \omit ###

    \l opengl/texture example
    The following few lines are from the texture example. Most of the
    code is irrelevant, so we just quote the relevant bits:

    \quotefromfile opengl/texture/gltexobj.cpp
    \skipto tex1
    \printline tex1
    \printline gllogo.bmp

    We create \e tex1 (and another variable) for OpenGL, and load a real
    image into \e buf.

    \skipto convertToGLFormat
    \printline convertToGLFormat

    A few lines later, we convert \e buf into OpenGL format and store it
    in \e tex1.

    \skipto glTexImage2D
    \printline glTexImage2D
    \printline tex1.bits

    Note the dimension restrictions for texture images as described in
    the glTexImage2D() documentation. The width must be 2^m + 2*border
    and the height 2^n + 2*border where m and n are integers and
    border is either 0 or 1.

    Another function in the same example uses \e tex1 with OpenGL.

    \endomit
*/

QImage QGLWidget::convertToGLFormat(const QImage& img)
{
    QImage res = img.convertToFormat(QImage::Format_ARGB32);
    res = res.mirrored();

    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        // Qt has ARGB; OpenGL wants RGBA
        for (int i=0; i < res.height(); i++) {
            uint *p = (uint*)res.scanLine(i);
            uint *end = p + res.width();
            while (p < end) {
                *p = (*p << 8) | ((*p >> 24) & 0xFF);
                p++;
            }
        }
    }
    else {
        // Qt has ARGB; OpenGL wants ABGR (i.e. RGBA backwards)
        res = res.rgbSwapped();
    }
    return res;
}


/*!
    \fn QGLColormap & QGLWidget::colormap() const

    Returns the colormap for this widget.

    Usually it is only top-level widgets that can have different
    colormaps installed. Asking for the colormap of a child widget
    will return the colormap for the child's top-level widget.

    If no colormap has been set for this widget, the QColormap
    returned will be empty.

    \sa setColormap()
*/

/*!
    \fn void QGLWidget::setColormap(const QGLColormap & cmap)

    Set the colormap for this widget to \a cmap. Usually it is only
    top-level widgets that can have colormaps installed.

    \sa colormap()
*/


/*!
    Returns the value of the first display list that is generated for
    the characters in the given \a font. \a listBase indicates the base
    value used when generating the display lists for the font. The
    default value is 2000.
*/
int QGLWidget::fontDisplayListBase(const QFont & font, int listBase)
{
    Q_D(QGLWidget);
    int base;

    if (!d->glcx) { // this can't happen unless we run out of mem
        return 0;
    }

    // always regenerate font disp. lists for pixmaps - hw accelerated
    // contexts can't handle this otherwise
    bool regenerate = d->glcx->deviceIsPixmap();
#ifndef QT_NO_FONTCONFIG
    // font color needs to be part of the font cache key when using
    // antialiased fonts since one set of glyphs needs to be generated
    // for each font color
    QString color_key;
    if (font.styleStrategy() != QFont::NoAntialias) {
        GLfloat color[4];
        glGetFloatv(GL_CURRENT_COLOR, color);
        color_key.sprintf("%f_%f_%f",color[0], color[1], color[2]);
    }
    QString key = font.key() + color_key + QString::number((int) regenerate);
#else
    QString key = font.key() + QString::number((int) regenerate);
#endif
    if (!regenerate && (d->displayListCache.find(key) != d->displayListCache.end())) {
        base = d->displayListCache[key];
    } else {
        int maxBase = listBase - 256;
        QMap<QString,int>::ConstIterator it;
        for (it = d->displayListCache.constBegin(); it != d->displayListCache.constEnd(); ++it) {
            if (maxBase < it.value()) {
                maxBase = it.value();
            }
        }
        maxBase += 256;
        d->glcx->generateFontDisplayLists(font, maxBase);
        d->displayListCache[key] = maxBase;
        base = maxBase;
    }
    return base;
}

static void qt_drawFontLining(double x, double y, const QString &str, const QFont &font)
{
#ifndef Q_WS_QWS
    QFontMetrics fm(font);
    int h = fm.lineWidth();
    int w = fm.width(str);
    if (font.underline()) {
        int pos = fm.underlinePos();
        glRectd(x, pos + y, x + w, pos + y + h);
    }
    if (font.strikeOut()) {
        int pos = fm.strikeOutPos();
        glRectd(x, y - pos, x + w, y - pos + h);
    }
    if (font.overline()) {
        int pos = fm.overlinePos();
        glRectd(x, y - pos, x + w, y - pos + h);
    }
#else
    qWarning("drawFontLining not implemented on embedded");
#endif
}

/*!
   Renders the string \a str into the GL context of this widget.

   \a x and \a y are specified in window coordinates, with the origin
   in the upper left-hand corner of the window. If \a font is not
   specified, the currently set application font will be used to
   render the string. To change the color of the rendered text you can
   use the glColor() call (or the qglColor() convenience function),
   just before the renderText() call. Note that if you have
   GL_LIGHTING enabled, the string will not appear in the color you
   want. You should therefore switch lighting off before using
   renderText().

   \a listBase specifies the index of the first display list that is
   generated by this function. The default value is 2000. 256 display
   lists will be generated, one for each of the first 256 characters
   in the font that is used to render the string. If several fonts are
   used in the same widget, the display lists for these fonts will
   follow the last generated list. You would normally not have to
   change this value unless you are using lists in the same range. The
   lists are deleted when the widget is destroyed.

   \warning This function only works reliably with ASCII strings.
*/

void QGLWidget::renderText(int x, int y, const QString & str, const QFont & font, int listBase)
{
#ifndef Q_WS_QWS
    makeCurrent();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width(), height(), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.0);
    glEnable(GL_ALPHA_TEST);
    glRasterPos2i(0, 0);
    glBitmap(0, 0, 0, 0, x, -y, NULL);
    glListBase(fontDisplayListBase(font, listBase));
    QByteArray cstr(str.toLatin1());
    glCallLists(cstr.size(), GL_UNSIGNED_BYTE, cstr.constData());

    if (font.underline() || font.strikeOut() || font.overline())
        qt_drawFontLining(x, y, str, font);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
#endif
}

/*! \overload

    \a x, \a y and \a z are specified in scene or object coordinates
    relative to the currently set projection and model matrices. This
    can be useful if you want to annotate models with text labels and
    have the labels move with the model as it is rotated etc.
*/
void QGLWidget::renderText(double x, double y, double z, const QString & str, const QFont & font,
                           int listBase)
{
#ifndef Q_WS_QWS
    makeCurrent();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    glRasterPos3d(x, y, z);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.0);
    glEnable(GL_ALPHA_TEST);
    glListBase(fontDisplayListBase(font, listBase));
    QByteArray cstr(str.toLatin1());
    glCallLists(cstr.size(), GL_UNSIGNED_BYTE, cstr.constData());

    if (font.underline() || font.strikeOut() || font.overline()) {
        GLdouble model[4][4], proj[4][4];
        GLint view[4];
        glGetDoublev(GL_MODELVIEW_MATRIX, &model[0][0]);
        glGetDoublev(GL_PROJECTION_MATRIX, &proj[0][0]);
        glGetIntegerv(GL_VIEWPORT, &view[0]);

        GLdouble win_x = 0, win_y = 0, win_z = 0;
        qgluProject(x, y, z, &model[0][0], &proj[0][0], &view[0],
                    &win_x, &win_y, &win_z);
        win_y = height() - win_y; // y is inverted

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, width(), height(), 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        qt_drawFontLining(win_x, win_y, str, font);

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }
    glPopAttrib();
#endif
}

QGLFormat QGLWidget::format() const
{
    Q_D(const QGLWidget);
    return d->glcx->format();
}

const QGLContext *QGLWidget::context() const
{
    Q_D(const QGLWidget);
    return d->glcx;
}

bool QGLWidget::doubleBuffer() const
{
    Q_D(const QGLWidget);
    return d->glcx->format().doubleBuffer();
}

void QGLWidget::setAutoBufferSwap(bool on)
{
    Q_D(QGLWidget);
    d->autoSwap = on;
}

bool QGLWidget::autoBufferSwap() const
{
    Q_D(const QGLWidget);
    return d->autoSwap;
}

/*!
    Calls QGLContext:::bindTexture(\a image, \a target, \a format) on the currently
    set context.

    \sa deleteTexture()
*/
GLuint QGLWidget::bindTexture(const QImage &image, GLenum target, GLint format)
{
    Q_D(QGLWidget);
    return d->glcx->bindTexture(image, target, format);
}

/*!
    Calls QGLContext:::bindTexture(\a pixmap, \a target, \a format) on the currently
    set context.

    \sa deleteTexture()
*/
GLuint QGLWidget::bindTexture(const QPixmap &pixmap, GLenum target, GLint format)
{
    Q_D(QGLWidget);
    return d->glcx->bindTexture(pixmap, target, format);
}

/*! \overload

    Calls QGLContext::bindTexture(\a fileName) on the currently set context.

    \sa deleteTexture()
*/
GLuint QGLWidget::bindTexture(const QString &fileName)
{
    Q_D(QGLWidget);
    return d->glcx->bindTexture(fileName);
}

/*!
    Calls QGLContext::deleteTexture(\a id) on the currently set
    context.

    \sa bindTexture()
*/
void QGLWidget::deleteTexture(GLuint id)
{
    Q_D(QGLWidget);
    d->glcx->deleteTexture(id);
}

Q_GLOBAL_STATIC(QOpenGLPaintEngine, qt_gl_engine)

/*!
    \internal

    Returns the GL widget's paint engine. This is normally a
    QOpenGLPaintEngine.
*/
QPaintEngine *QGLWidget::paintEngine() const
{
    return qt_gl_engine();
}

#ifdef QT3_SUPPORT
/*!
    \overload
    \obsolete
 */
QGLWidget::QGLWidget(QWidget *parent, const char *name,
                      const QGLWidget* shareWidget, Qt::WindowFlags f)
    : QWidget(*(new QGLWidgetPrivate), parent, f | Qt::MSWindowsOwnDC)
{
    Q_D(QGLWidget);
    if (name)
        setObjectName(QString::fromAscii(name));
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(true); // for compatibility
    d->init(new QGLContext(QGLFormat::defaultFormat(), this), shareWidget);
}

/*!
    \overload
    \obsolete
 */
QGLWidget::QGLWidget(const QGLFormat &format, QWidget *parent,
                      const char *name, const QGLWidget* shareWidget,
                      Qt::WindowFlags f)
    : QWidget(*(new QGLWidgetPrivate), parent, f | Qt::MSWindowsOwnDC)
{
    Q_D(QGLWidget);
    if (name)
        setObjectName(QString::fromAscii(name));
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(true); // for compatibility
    d->init(new QGLContext(format, this), shareWidget);
}

/*!
    \overload
    \obsolete
 */
QGLWidget::QGLWidget(QGLContext *context, QWidget *parent,
                      const char *name, const QGLWidget *shareWidget, Qt::WindowFlags f)
    : QWidget(*(new QGLWidgetPrivate), parent, f | Qt::MSWindowsOwnDC)
{
    Q_D(QGLWidget);
    if (name)
        setObjectName(QString::fromAscii(name));
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(true); // for compatibility
    d->init(context, shareWidget);
}

#endif // QT3_SUPPORT

void QGLExtensions::init_extensions()
{
    QString extensions = QLatin1String(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    if (extensions.contains(QLatin1String("texture_rectangle")))
        glExtensions |= TextureRectangle;
    if (extensions.contains(QLatin1String("multisample")))
        glExtensions |= SampleBuffers;
    if (extensions.contains(QLatin1String("generate_mipmap")))
        glExtensions |= GenerateMipmap;
    if (extensions.contains(QLatin1String("texture_compression_s3tc")))
        glExtensions |= TextureCompression;
    if (extensions.contains(QLatin1String("ARB_fragment_program")))
        glExtensions |= FragmentProgram;
    if (extensions.contains(QLatin1String("mirrored_repeat")))
        glExtensions |= MirroredRepeat;
    if (extensions.contains(QLatin1String("EXT_framebuffer_object")))
        glExtensions |= FramebufferObject;
    if (extensions.contains(QLatin1String("EXT_stencil_two_side")))
        glExtensions |= StencilTwoSide;
    if (extensions.contains(QLatin1String("EXT_stencil_wrap")))
        glExtensions |= StencilWrap;

    QGLContext cx(QGLFormat::defaultFormat());
    if (glExtensions & TextureCompression) {
        qt_glCompressedTexImage2DARB = (pfn_glCompressedTexImage2DARB) cx.getProcAddress(QLatin1String("glCompressedTexImage2DARB"));
    }
}
