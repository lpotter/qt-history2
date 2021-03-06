/****************************************************************************
**
** Copyright (C) 2005-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef INTERFACES_H
#define INTERFACES_H

#include <QtPlugin>

QT_DECLARE_CLASS(QImage)
QT_DECLARE_CLASS(QPainter)
QT_DECLARE_CLASS(QWidget)
QT_DECLARE_CLASS(QPainterPath)
QT_DECLARE_CLASS(QPoint)
QT_DECLARE_CLASS(QRect)
QT_DECLARE_CLASS(QString)
QT_DECLARE_CLASS(QStringList)

class BrushInterface
{
public:
    virtual ~BrushInterface() {}

    virtual QStringList brushes() const = 0;
    virtual QRect mousePress(const QString &brush, QPainter &painter,
                             const QPoint &pos) = 0;
    virtual QRect mouseMove(const QString &brush, QPainter &painter,
                            const QPoint &oldPos, const QPoint &newPos) = 0;
    virtual QRect mouseRelease(const QString &brush, QPainter &painter,
                               const QPoint &pos) = 0;
};

class ShapeInterface
{
public:
    virtual ~ShapeInterface() {}

    virtual QStringList shapes() const = 0;
    virtual QPainterPath generateShape(const QString &shape,
                                       QWidget *parent) = 0;
};

class FilterInterface
{
public:
    virtual ~FilterInterface() {}

    virtual QStringList filters() const = 0;
    virtual QImage filterImage(const QString &filter, const QImage &image,
                               QWidget *parent) = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(BrushInterface,
                    "com.trolltech.PlugAndPaint.BrushInterface/1.0")
Q_DECLARE_INTERFACE(ShapeInterface,
                    "com.trolltech.PlugAndPaint.ShapeInterface/1.0")
Q_DECLARE_INTERFACE(FilterInterface,
                    "com.trolltech.PlugAndPaint.FilterInterface/1.0")
QT_END_NAMESPACE

#endif
