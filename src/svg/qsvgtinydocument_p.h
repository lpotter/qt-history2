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

#ifndef QSVGTINYDOCUMENT_P_H
#define QSVGTINYDOCUMENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsvgstructure_p.h"

#include "QtCore/qrect.h"
#include "QtCore/qlist.h"
#include "QtCore/qhash.h"
#include "QtCore/qdatetime.h"
#include "qsvgstyle_p.h"
#include "qsvgfont_p.h"

QT_BEGIN_NAMESPACE

class QPainter;
class QByteArray;
class QSvgFont;

class Q_SVG_EXPORT QSvgTinyDocument : public QSvgStructureNode
{
public:
    static QSvgTinyDocument * load(const QString &file);
    static QSvgTinyDocument * load(const QByteArray &contents);
public:
    QSvgTinyDocument();
    ~QSvgTinyDocument();
    Type type() const;

    QSize size() const;
    void setWidth(int len, bool percent);
    void setHeight(int len, bool percent);
    int width() const;
    int height() const;
    bool widthPercent() const;
    bool heightPercent() const;

    bool preserveAspectRatio() const;

    QRectF viewBox() const;
    void setViewBox(const QRectF &rect);

    virtual void draw(QPainter *p);//from the QSvgNode
    
    void draw(QPainter *p, const QRectF &bounds);
    void draw(QPainter *p, const QString &id,
              const QRectF &bounds=QRectF());

    QMatrix matrixForElement(const QString &id) const;
    QRectF boundsOnElement(const QString &id) const;
    bool   elementExists(const QString &id) const;

    void addSvgFont(QSvgFont *);
    QSvgFont *svgFont(const QString &family) const;

    void restartAnimation();
    int currentElapsed() const;
    bool animated() const;
    void setAnimated(bool a);
    int animationDuration() const;
    int currentFrame() const;
    void setCurrentFrame(int);
    void setFramesPerSecond(int num);
private:
    void adjustWindowBounds(QPainter *p, 
                            const QRectF &desired,
                            const QRectF &current);
private:
    mutable QSize  m_size;
    bool   m_widthPercent;
    bool   m_heightPercent;

    mutable QRectF m_viewBox;

    QHash<QString, QSvgRefCounter<QSvgFont> > m_fonts;

    QTime m_time;
    bool  m_animated;
    int   m_animationDuration;
    int   m_fps;
};

inline QSize QSvgTinyDocument::size() const
{
    //if the size is busted we need to resolve it
    if (m_size.width() <= 0 ||
        m_size.height() <= 0) {
        QMatrix matx = QMatrix();
        QRectF rect = transformedBounds(matx);
        if (m_viewBox.isNull())
            m_viewBox = rect;
        m_size = rect.size().toSize();
    }
        
    return m_size;
}

inline int QSvgTinyDocument::width() const
{
    return m_size.width();
}

inline int QSvgTinyDocument::height() const
{
    return m_size.height();
}

inline bool QSvgTinyDocument::widthPercent() const
{
    return m_widthPercent;
}

inline bool QSvgTinyDocument::heightPercent() const
{
    return m_heightPercent;
}

inline QRectF QSvgTinyDocument::viewBox() const
{
    return m_viewBox;
}

inline bool QSvgTinyDocument::preserveAspectRatio() const
{
    return false;
}

inline int QSvgTinyDocument::currentElapsed() const
{
    return m_time.elapsed();
}

inline int QSvgTinyDocument::animationDuration() const
{
    return m_animationDuration;
}

QT_END_NAMESPACE

#endif // QSVGTINYDOCUMENT_P_H
