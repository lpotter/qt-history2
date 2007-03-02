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

#ifndef QWINDOWSURFACE_EGL_P_H
#define QWINDOWSURFACE_EGL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include "private/qwindowsurface_qws_p.h"
#include "GLES/egl.h"

class QPaintDevice;
class QPoint;
class QRegion;
class QSize;
class QWidget;
class QGLContext;

class QEGLWindowSurfacePrivate;

class QEGLWindowSurface : public QWSWindowSurface, public QPaintDevice
{
    Q_DECLARE_PRIVATE(QEGLWindowSurface)
public:
    QEGLWindowSurface(QWidget *widget);
    ~QEGLWindowSurface();

    QPaintDevice *paintDevice();
    QPaintEngine *paintEngine() const;

    void scroll(const QRegion &area, int dx, int dy);

    int metric(PaintDeviceMetric m) const;

    bool isValidFor(const QWidget*) const { return true; }
    const QImage image() const { return QImage(); }

    // want to replace swapBuffers with a reimp of flush
    virtual void swapBuffers() = 0;

    virtual bool chooseContext(QGLContext *context, const QGLContext *shareContext) = 0;
    QGLContext *context() const;

    // remove virtual when we have a better data() function
    virtual GLuint textureId() const = 0;

protected:
    // remove virtual when we have a better data() function
    virtual void setTextureId(GLuint texId) = 0;

    void setContext(QGLContext *context);

private:
    QEGLWindowSurfacePrivate *d_ptr;
};


#endif // QWINDOWSURFACE_EGL_P_H