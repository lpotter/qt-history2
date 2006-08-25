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

#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include "qtcolorline.h"
#include "qdrawutil.h"

#include "qdebug.h"

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtColorLinePrivate
{
    QtColorLine *q_ptr;
    Q_DECLARE_PUBLIC(QtColorLine)
public:
    QtColorLinePrivate();

    QColor color() const;
    void setColor(const QColor &color);

    QtColorLine::ColorComponent colorComponent() const;
    void setColorComponent(QtColorLine::ColorComponent component);

    void setIndicatorSize(int size);
    int indicatorSize() const;

    void setIndicatorSpace(int space);
    int indicatorSpace() const;

    void setFlip(bool flip);
    bool flip() const;

    void setBackgroundTransparent(bool transparent);
    bool backgroundTransparent() const;

    void setOrientation(Qt::Orientation orientation);
    Qt::Orientation orientation() const;

    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
private:
    void checkColor();
    bool isMainPixmapValid() const;
    void validate();
    void recreateMainPixmap();
    QSize pixmapSizeFromGeometrySize(const QSize &geometrySize) const;
    QPixmap gradientPixmap(int size, Qt::Orientation orientation, const QColor &begin, const QColor &end, bool flipped = false) const;
    QPixmap gradientPixmap(Qt::Orientation orientation, const QColor &begin, const QColor &end, bool flipped = false) const;
    QPixmap hueGradientPixmap(int size, Qt::Orientation orientation, bool flipped = false,
                int saturation = 0xFF, int value = 0xFF, int alpha = 0xFF) const;
    QPixmap hueGradientPixmap(Qt::Orientation orientation, bool flipped = false,
                int saturation = 0xFF, int value = 0xFF, int alpha = 0xFF) const;

    QVector<QRect> rects(const QPointF &point) const;

    QColor colorFromPoint(const QPointF &point) const;
    QPointF pointFromColor(const QColor &color) const;

    QColor m_color;
    QtColorLine::ColorComponent m_component;
    bool m_flipped;
    bool m_backgroundTransparent;
    Qt::Orientation m_orientation;
    bool m_dragging;
    int m_indicatorSize;
    int m_indicatorSpace;
    QPointF m_point;
    QPoint m_clickOffset;

    QPixmap m_mainPixmap;
    QSize m_pixmapSize;

    struct PixData {
        QSize size;
        QColor color;
        QtColorLine::ColorComponent component;
        bool flipped;
        Qt::Orientation orientation;
    };

    PixData m_lastValidMainPixmapData;
};

}

QtColorLinePrivate::QtColorLinePrivate()
    : m_color(Qt::black), m_component(QtColorLine::Value),
        m_flipped(false), m_backgroundTransparent(true), m_orientation(Qt::Horizontal), m_dragging(false)
{
    m_indicatorSize = 22;
    m_indicatorSpace = 0;
    m_pixmapSize = QSize(0, 0);
    m_point = pointFromColor(m_color);
}

void QtColorLinePrivate::setColor(const QColor &color)
{
    if (m_color == color)
        return;
    if (!color.isValid())
        return;
    if (m_dragging) // Warning perhaps here, recursive call
        return;
    m_color = color;
    checkColor();
    QColor c = colorFromPoint(m_point);
    m_point = pointFromColor(m_color);
    q_ptr->update();
}

QColor QtColorLinePrivate::color() const
{
    return m_color;
}

void QtColorLinePrivate::setColorComponent(QtColorLine::ColorComponent component)
{
    if (m_component == component)
        return;
    if (m_dragging) // Warning perhaps here, recursive call
        return;
    m_component = component;
    checkColor();
    m_point = pointFromColor(m_color);
    q_ptr->update();
}

QtColorLine::ColorComponent QtColorLinePrivate::colorComponent() const
{
    return m_component;
}

void QtColorLinePrivate::setIndicatorSize(int size)
{
    if (size <= 0)
        return;
    if (m_dragging) // Warning perhaps here, recursive call
        return;
    if (m_indicatorSize == size)
        return;
    m_indicatorSize = size;
    m_pixmapSize = pixmapSizeFromGeometrySize(q_ptr->contentsRect().size());
    q_ptr->update();
    q_ptr->updateGeometry();
}

int QtColorLinePrivate::indicatorSize() const
{
    return m_indicatorSize;
}

void QtColorLinePrivate::setIndicatorSpace(int space)
{
    if (space < 0)
        return;
    if (m_dragging) // Warning perhaps here, recursive call
        return;
    if (m_indicatorSpace == space)
        return;
    m_indicatorSpace = space;
    m_pixmapSize = pixmapSizeFromGeometrySize(q_ptr->contentsRect().size());
    q_ptr->update();
}

int QtColorLinePrivate::indicatorSpace() const
{
    return m_indicatorSpace;
}

void QtColorLinePrivate::setFlip(bool flip)
{
    if (m_dragging) // Warning perhaps here, recursive call
        return;
    if (m_flipped == flip)
        return;
    m_flipped = flip;
    m_point = pointFromColor(m_color);
    q_ptr->update();
}

bool QtColorLinePrivate::flip() const
{
    return m_flipped;
}

void QtColorLinePrivate::setBackgroundTransparent(bool transparent)
{
    if (m_backgroundTransparent == transparent)
        return;
    m_backgroundTransparent = transparent;
    q_ptr->update();
}

bool QtColorLinePrivate::backgroundTransparent() const
{
    return m_backgroundTransparent;
}

void QtColorLinePrivate::setOrientation(Qt::Orientation orientation)
{
    if (m_dragging) // Warning perhaps here, recursive call
        return;
    if (m_orientation == orientation)
        return;

    m_orientation = orientation;
    if (!q_ptr->testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        QSizePolicy sp = q_ptr->sizePolicy();
        sp.transpose();
        q_ptr->setSizePolicy(sp);
        q_ptr->setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
    m_point = pointFromColor(m_color);
    q_ptr->update();
    q_ptr->updateGeometry();
}

Qt::Orientation QtColorLinePrivate::orientation() const
{
    return m_orientation;
}

void QtColorLinePrivate::checkColor()
{
    switch (m_component) {
        case QtColorLine::Red:
        case QtColorLine::Green:
        case QtColorLine::Blue:
            if (m_color.spec() != QColor::Rgb)
                m_color = m_color.toRgb();
            break;
        case QtColorLine::Hue:
        case QtColorLine::Saturation:
        case QtColorLine::Value:
            if (m_color.spec() != QColor::Hsv)
                m_color = m_color.toHsv();
            break;
        default:
            break;
    }
    if (m_color.spec() == QColor::Hsv) {
        if (m_color.hue() == 360 || m_color.hue() == -1) {
            m_color.setHsvF(0.0, m_color.saturationF(), m_color.valueF(), m_color.alphaF());
        }
    }
}

bool QtColorLinePrivate::isMainPixmapValid() const
{
    if (m_mainPixmap.isNull()) {
        if (m_pixmapSize.isEmpty())
            return true;
        else
            return false;
    }
    if (m_lastValidMainPixmapData.component != m_component)
        return false;
    if (m_lastValidMainPixmapData.size != m_pixmapSize)
        return false;
    if (m_lastValidMainPixmapData.flipped != m_flipped)
        return false;
    if (m_lastValidMainPixmapData.orientation != m_orientation)
        return false;
    if (m_lastValidMainPixmapData.color == m_color)
        return true;
    switch (m_component) {
        case QtColorLine::Red:
            if (m_color.green() == m_lastValidMainPixmapData.color.green() &&
                m_color.blue() == m_lastValidMainPixmapData.color.blue() &&
                m_color.alpha() == m_lastValidMainPixmapData.color.alpha())
                return true;
            break;
        case QtColorLine::Green:
            if (m_color.red() == m_lastValidMainPixmapData.color.red() &&
                m_color.blue() == m_lastValidMainPixmapData.color.blue() &&
                m_color.alpha() == m_lastValidMainPixmapData.color.alpha())
                return true;
            break;
        case QtColorLine::Blue:
            if (m_color.red() == m_lastValidMainPixmapData.color.red() &&
                m_color.green() == m_lastValidMainPixmapData.color.green() &&
                m_color.alpha() == m_lastValidMainPixmapData.color.alpha())
                return true;
            break;
        case QtColorLine::Hue:
            if (m_color.saturation() == m_lastValidMainPixmapData.color.saturation() &&
                m_color.value() == m_lastValidMainPixmapData.color.value() &&
                m_color.alpha() == m_lastValidMainPixmapData.color.alpha())
                return true;
            break;
        case QtColorLine::Saturation:
            if (m_color.hue() == m_lastValidMainPixmapData.color.hue() &&
                m_color.value() == m_lastValidMainPixmapData.color.value() &&
                m_color.alpha() == m_lastValidMainPixmapData.color.alpha())
                return true;
            break;
        case QtColorLine::Value:
            if (m_color.hue() == m_lastValidMainPixmapData.color.hue() &&
                m_color.saturation() == m_lastValidMainPixmapData.color.saturation() &&
                m_color.alpha() == m_lastValidMainPixmapData.color.alpha())
                return true;
            break;
        case QtColorLine::Alpha:
            if (m_color.hue() == m_lastValidMainPixmapData.color.hue() &&
                m_color.saturation() == m_lastValidMainPixmapData.color.saturation() &&
                m_color.value() == m_lastValidMainPixmapData.color.value())
                return true;
    }
    return false;
}

void QtColorLinePrivate::validate()
{
    recreateMainPixmap();
}

QPixmap QtColorLinePrivate::gradientPixmap(Qt::Orientation orientation, const QColor &begin, const QColor &end, bool flipped) const
{
    int size = m_pixmapSize.width();
    if (orientation == Qt::Vertical)
        size = m_pixmapSize.height();
    return gradientPixmap(size, orientation, begin, end, flipped);
}

QPixmap QtColorLinePrivate::gradientPixmap(int size, Qt::Orientation orientation,
            const QColor &begin, const QColor &end, bool flipped) const
{
    int gradW = size;
    int gradH = size;
    int w = size;
    int h = size;
    if (orientation == Qt::Horizontal) {
        gradH = 0;
        h = 1;
    } else {
        gradW = 0;
        w = 1;
    }
    QColor c1 = begin;
    QColor c2 = end;
    if (flipped) {
        c1 = end;
        c2 = begin;
    }
    QLinearGradient lg(0, 0, gradW, gradH);
    lg.setColorAt(0, c1);
    lg.setColorAt(1, c2);
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(QRect(0, 0, w, h), lg);
    return QPixmap::fromImage(img);
}

QPixmap QtColorLinePrivate::hueGradientPixmap(Qt::Orientation orientation, bool flipped,
                int saturation, int value, int alpha) const
{
    int size = m_pixmapSize.width();
    if (orientation == Qt::Vertical)
        size = m_pixmapSize.height();
    return hueGradientPixmap(size, orientation, flipped, saturation, value, alpha);
}

QPixmap QtColorLinePrivate::hueGradientPixmap(int size, Qt::Orientation orientation, bool flipped,
                int saturation, int value, int alpha) const
{
    int gradW = size + 1;
    int gradH = size + 1;
    int w = size;
    int h = size;
    if (orientation == Qt::Horizontal) {
        gradH = 0;
        h = 1;
    } else {
        gradW = 0;
        w = 1;
    }
    QList<QColor> colorList;
    colorList << QColor::fromHsv(0, saturation, value, alpha);
    colorList << QColor::fromHsv(60, saturation, value, alpha);
    colorList << QColor::fromHsv(120, saturation, value, alpha);
    colorList << QColor::fromHsv(180, saturation, value, alpha);
    colorList << QColor::fromHsv(240, saturation, value, alpha);
    colorList << QColor::fromHsv(300, saturation, value, alpha);
    colorList << QColor::fromHsv(0, saturation, value, alpha);
    QLinearGradient lg(0, 0, gradW, gradH);
    for (int i = 0; i <= 6; i++)
        lg.setColorAt((double)i / 6.0, flipped ? colorList.at(6 - i) : colorList.at(i));
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(QRect(0, 0, w, h), lg);
    return QPixmap::fromImage(img);
}

void QtColorLinePrivate::recreateMainPixmap()
{
    if (isMainPixmapValid())
        return;

    m_lastValidMainPixmapData.size = m_pixmapSize;
    m_lastValidMainPixmapData.component = m_component;
    m_lastValidMainPixmapData.color = m_color;
    m_lastValidMainPixmapData.flipped = m_flipped;
    m_lastValidMainPixmapData.orientation = m_orientation;

    if (m_pixmapSize.isEmpty()) {
        m_mainPixmap = QPixmap();
        return;
    }

    if (m_mainPixmap.size() != m_pixmapSize)
        m_mainPixmap = QPixmap(m_pixmapSize);

    Qt::Orientation orient = m_orientation;
    bool flip = m_flipped;
    QPixmap pix;

    int r = m_color.red();
    int g = m_color.green();
    int b = m_color.blue();
    int h = m_color.hue();
    int s = m_color.saturation();
    int v = m_color.value();
    int a = m_color.alpha();

    if (m_component == QtColorLine::Hue)
        pix = hueGradientPixmap(orient, flip, s, v, a);
    else if (m_component == QtColorLine::Saturation)
        pix = gradientPixmap(orient, QColor::fromHsv(h, 0, v, a), QColor::fromHsv(h, 0xFF, v, a), flip);
    else if (m_component == QtColorLine::Value)
        pix = gradientPixmap(orient, QColor::fromRgb(0, 0, 0, a), QColor::fromHsv(h, s, 0xFF, a), flip);
    else if (m_component == QtColorLine::Red)
        pix = gradientPixmap(orient, QColor::fromRgb(0, g, b, a), QColor::fromRgb(0xFF, g, b, a), flip);
    else if (m_component == QtColorLine::Green)
        pix = gradientPixmap(orient, QColor::fromRgb(r, 0, b, a), QColor::fromRgb(r, 0xFF, b, a), flip);
    else if (m_component == QtColorLine::Blue)
        pix = gradientPixmap(orient, QColor::fromRgb(r, g, 0, a), QColor::fromRgb(r, g, 0xFF, a), flip);
    else if (m_component == QtColorLine::Alpha)
        pix = gradientPixmap(orient, QColor::fromRgb(r, g, b, 0), QColor::fromRgb(r, g, b, 0xFF), flip);

    m_mainPixmap = pix;
    return;
/*
    QPixmap pm(20, 20);
    QPainter pmp(&pm);
    pmp.fillRect(0, 0, 10, 10, Qt::lightGray);
    pmp.fillRect(10, 10, 10, 10, Qt::lightGray);
    pmp.fillRect(0, 10, 10, 10, Qt::darkGray);
    pmp.fillRect(10, 0, 10, 10, Qt::darkGray);
    pmp.end();

    QPainter p(&m_mainPixmap);
    p.fillRect(QRect(QPoint(0, 0), m_pixmapSize), pm);
    p.fillRect(QRect(QPoint(0, 0), m_pixmapSize), pix);
*/
    QImage img(m_pixmapSize, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(QRect(QPoint(0, 0), m_pixmapSize), pix);
    m_mainPixmap = QPixmap::fromImage(img);
//    QPainter p(&m_mainPixmap);
//    p.fillRect(QRect(QPoint(0, 0), m_pixmapSize), pix);
}

QSize QtColorLinePrivate::pixmapSizeFromGeometrySize(
        const QSize &geometrySize) const
{
    QSize size(m_indicatorSize + 2 * m_indicatorSpace - 1,
                m_indicatorSize + 2 * m_indicatorSpace - 1);
    if (m_orientation == Qt::Horizontal)
        size.setHeight(0);
    else
        size.setWidth(0);
    return geometrySize - size;
}

QColor QtColorLinePrivate::colorFromPoint(const QPointF &point) const
{
    QPointF p = point;
    if (p.x() < 0)
        p.setX(0.0);
    else if (p.x() > 1)
        p.setX(1.0);
    if (p.y() < 0)
        p.setY(0.0);
    else if (p.y() > 1)
        p.setY(1.0);

    double pos = p.x();
    if (m_orientation == Qt::Vertical)
        pos = p.y();
    if (m_flipped)
        pos = 1.0 - pos;
    QColor c;
    qreal hue;
    switch (m_component) {
        case QtColorLine::Red:
            c.setRgbF(pos, m_color.greenF(), m_color.blueF(), m_color.alphaF());
            break;
        case QtColorLine::Green:
            c.setRgbF(m_color.redF(), pos, m_color.blueF(), m_color.alphaF());
            break;
        case QtColorLine::Blue:
            c.setRgbF(m_color.redF(), m_color.greenF(), pos, m_color.alphaF());
            break;
        case QtColorLine::Hue:
            hue = pos;
            hue *= 35999.0 / 36000.0;
            c.setHsvF(hue, m_color.saturationF(), m_color.valueF(), m_color.alphaF());
            break;
        case QtColorLine::Saturation:
            c.setHsvF(m_color.hueF(), pos, m_color.valueF(), m_color.alphaF());
            break;
        case QtColorLine::Value:
            c.setHsvF(m_color.hueF(), m_color.saturationF(), pos, m_color.alphaF());
            break;
        case QtColorLine::Alpha:
            c.setHsvF(m_color.hueF(), m_color.saturationF(), m_color.valueF(), pos);
            break;
    }
    return c;
}

QPointF QtColorLinePrivate::pointFromColor(const QColor &color) const
{
    qreal hue = color.hueF();
    if (color.hue() == 360)
        hue = 0.0;
    else
        hue *= 36000.0 / 35999.0;

    double pos = 0.0;
    switch (m_component) {
        case QtColorLine::Red:
            pos = color.redF();
            break;
        case QtColorLine::Green:
            pos = color.greenF();
            break;
        case QtColorLine::Blue:
            pos = color.blueF();
            break;
        case QtColorLine::Hue:
            pos = hue;
            break;
        case QtColorLine::Saturation:
            pos = color.saturationF();
            break;
        case QtColorLine::Value:
            pos = color.valueF();
            break;
        case QtColorLine::Alpha:
            pos = color.alphaF();
            break;
    }
    if (m_flipped)
        pos = 1.0 - pos;
    QPointF p(pos, pos);
    if (m_orientation == Qt::Horizontal)
        p.setY(0);
    else
        p.setX(0);
    return p;
}

QVector<QRect> QtColorLinePrivate::rects(const QPointF &point) const
{
    QRect r = q_ptr->geometry();
    r.moveTo(0, 0);

    int x1 = (int)((r.width() - m_indicatorSize - 2 * m_indicatorSpace) * point.x() + 0.5);
    int x2 = x1 + m_indicatorSize + 2 * m_indicatorSpace;
    int y1 = (int)((r.height() - m_indicatorSize - 2 * m_indicatorSpace) * point.y() + 0.5);
    int y2 = y1 + m_indicatorSize + 2 * m_indicatorSpace;

    QVector<QRect> rects;
    if (m_orientation == Qt::Horizontal) {
        // r0 r1 r2
        QRect r0(0, 0, x1, r.height());
        QRect r1(x1 + m_indicatorSpace, 0, m_indicatorSize, r.height());
        QRect r2(x2, 0, r.width() - x2, r.height());

        rects << r0 << r1 << r2;
    } else {
        // r0
        // r1
        // r2
        QRect r0(0, 0, r.width(), y1);
        QRect r1(0, y2, r.width(), r.height() - y2);
        QRect r2(0, y1 + m_indicatorSpace, r.width(), m_indicatorSize);

        rects << r0 << r1 << r2;
    }
    return rects;
}

void QtColorLinePrivate::resizeEvent(QResizeEvent *event)
{
    m_pixmapSize = pixmapSizeFromGeometrySize(event->size());
}

void QtColorLinePrivate::paintEvent(QPaintEvent *)
{
    QRect rect = q_ptr->rect();

    QVector<QRect> r = rects(m_point);

    QColor cBack = q_ptr->palette().color(QPalette::Active, QPalette::Window);
    QColor c = colorFromPoint(m_point);

    QPixmap pix(rect.size());

    QPainter p(q_ptr);
    if (!m_backgroundTransparent) {
        int pixSize = 20;
        QPixmap pm(2 * pixSize, 2 * pixSize);
        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);
        pmp.end();

        p.end();
        p.begin(&pix);
        p.setBrushOrigin((rect.width() % pixSize + pixSize) / 2, (rect.height() % pixSize + pixSize) / 2);
        p.fillRect(rect, pm);
        p.setBrushOrigin(0, 0);
    }

    if (q_ptr->isEnabled()) {
        validate();

        QSize fieldSize = pixmapSizeFromGeometrySize(q_ptr->geometry().size());

        QPoint posOnField = r[1].topLeft() - QPoint(m_indicatorSpace, m_indicatorSpace);
        int x = posOnField.x();
        int y = posOnField.y();
        int w = fieldSize.width();
        int h = fieldSize.height();

        QRect r0, r2;
        if (m_orientation == Qt::Horizontal) {
            r0 = QRect(0, 0, x, m_pixmapSize.height());
            r2 = QRect(x + 1, 0, w - x - 1, m_pixmapSize.height());
        } else {
            r0 = QRect(0, 0, m_pixmapSize.width(), y);
            r2 = QRect(0, y + 1, m_pixmapSize.width(), h - y - 1);
        }

        p.setBrush(m_mainPixmap);
        p.setPen(Qt::NoPen);
        if (r[0].isValid()) {
            p.drawRect(r[0]);
        }
        if (r[2].isValid()) {
            p.setBrushOrigin(r[2].topLeft() - r2.topLeft());
            p.drawRect(r[2]);
        }
        QPen pen(c);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        if (r[1].isValid()) {
            p.drawRect(r[1].adjusted(0, 0, -1, -1));
            p.drawRect(r[1].adjusted(1, 1, -2, -2));
        }
        p.setPen(Qt::NoPen);

        if (!m_backgroundTransparent)
            p.fillRect(rect, pix);
    }

    p.setBrush(Qt::NoBrush);
    int lw = 2;
    int br = 1;
    r[1].adjust(br, br, -br, -br);
    if (r[1].adjusted(lw, lw, -lw, -lw).isValid()) {
        QStyleOptionFrame opt;
        opt.init(q_ptr);
        opt.rect = r[1];
        opt.lineWidth = 2;
        opt.midLineWidth = 1;
        if (m_dragging)
            opt.state |= QStyle::State_Sunken;
        else
            opt.state |= QStyle::State_Raised;
        q_ptr->style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, q_ptr);
        if (q_ptr->isEnabled())
            p.fillRect(r[1].adjusted(lw, lw, -lw, -lw), c);
    }
}

void QtColorLinePrivate::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    QVector<QRect> r = rects(m_point);
    QPoint clickPos = event->pos();

    QSize fieldSize = q_ptr->geometry().size() -
            QSize(m_indicatorSize + 2 * m_indicatorSpace - 1, m_indicatorSize + 2 * m_indicatorSpace - 1);
    QPoint posOnField = r[1].topLeft() - QPoint(m_indicatorSpace, m_indicatorSpace);
    m_clickOffset = posOnField - clickPos;

    if (!r[1].contains(clickPos))
        return;
    m_dragging = true;
    q_ptr->update();
}

void QtColorLinePrivate::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging)
        return;
    QPoint newPos = event->pos();

    QSize fieldSize = q_ptr->geometry().size() -
            QSize(m_indicatorSize + 2 * m_indicatorSpace - 1, m_indicatorSize + 2 * m_indicatorSpace - 1);
    QPoint newPosOnField = newPos + m_clickOffset;
    if (newPosOnField.x() < 0)
        newPosOnField.setX(0);
    else if (newPosOnField.x() > fieldSize.width())
        newPosOnField.setX(fieldSize.width());
    if (newPosOnField.y() < 0)
        newPosOnField.setY(0);
    else if (newPosOnField.y() > fieldSize.height())
        newPosOnField.setY(fieldSize.height());

    double x = (double)newPosOnField.x() / fieldSize.width();
    double y = (double)newPosOnField.y() / fieldSize.height();
    m_point = QPointF(x, y);
    QColor color = colorFromPoint(m_point);
    if (m_color == color)
        return;
    m_color = color;
    emit q_ptr->colorChanged(color); // maybe before internal set, 1 line above
    q_ptr->update();
}

void QtColorLinePrivate::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    m_dragging = false;
    q_ptr->update();
}

////////////////////////////////////////////////////

QtColorLine::QtColorLine(QWidget *parent)
    : QWidget(parent)
{
    d_ptr = new QtColorLinePrivate;
    d_ptr->q_ptr = this;

    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
}

QtColorLine::~QtColorLine()
{
    delete d_ptr;
}

QSize QtColorLine::minimumSizeHint() const
{
    return QSize(d_ptr->m_indicatorSize, d_ptr->m_indicatorSize);
}

QSize QtColorLine::sizeHint() const
{
    return QSize(d_ptr->m_indicatorSize, d_ptr->m_indicatorSize);
}

void QtColorLine::setColor(const QColor &color)
{
    d_ptr->setColor(color);
}

QColor QtColorLine::color() const
{
    return d_ptr->color();
}

void QtColorLine::setColorComponent(QtColorLine::ColorComponent component)
{
    d_ptr->setColorComponent(component);
}

QtColorLine::ColorComponent QtColorLine::colorComponent() const
{
    return d_ptr->colorComponent();
}

void QtColorLine::setIndicatorSize(int size)
{
    d_ptr->setIndicatorSize(size);
}

int QtColorLine::indicatorSize() const
{
    return d_ptr->indicatorSize();
}

void QtColorLine::setIndicatorSpace(int space)
{
    d_ptr->setIndicatorSpace(space);
}

int QtColorLine::indicatorSpace() const
{
    return d_ptr->indicatorSpace();
}

void QtColorLine::setFlip(bool flip)
{
    d_ptr->setFlip(flip);
}

bool QtColorLine::flip() const
{
    return d_ptr->flip();
}

void QtColorLine::setBackgroundTransparent(bool transparent)
{
    d_ptr->setBackgroundTransparent(transparent);
}

bool QtColorLine::backgroundTransparent() const
{
    return d_ptr->backgroundTransparent();
}

void QtColorLine::setOrientation(Qt::Orientation orientation)
{
    d_ptr->setOrientation(orientation);
}

Qt::Orientation QtColorLine::orientation() const
{
    return d_ptr->orientation();
}
void QtColorLine::resizeEvent(QResizeEvent *event)
{
    d_ptr->resizeEvent(event);
}

void QtColorLine::paintEvent(QPaintEvent *event)
{
    d_ptr->paintEvent(event);
}

void QtColorLine::mousePressEvent(QMouseEvent *event)
{
    d_ptr->mousePressEvent(event);
}

void QtColorLine::mouseMoveEvent(QMouseEvent *event)
{
    d_ptr->mouseMoveEvent(event);
}

void QtColorLine::mouseReleaseEvent(QMouseEvent *event)
{
    d_ptr->mouseReleaseEvent(event);
}

