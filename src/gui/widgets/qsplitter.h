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

#ifndef QSPLITTER_H
#define QSPLITTER_H

#include "QtGui/qframe.h"
#include "QtGui/qsizepolicy.h"

#ifndef QT_NO_SPLITTER

class QSplitterPrivate;
class QTextStream;
template <typename T> class QList;

class QSplitterHandle;

class Q_GUI_EXPORT QSplitter : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(bool opaqueResize READ opaqueResize WRITE setOpaqueResize)
    Q_PROPERTY(int handleWidth READ handleWidth WRITE setHandleWidth)
    Q_PROPERTY(bool childrenCollapsible READ childrenCollapsible WRITE setChildrenCollapsible)

public:
    QSplitter(QWidget* parent = 0, const char* name = 0);
    QSplitter(Qt::Orientation, QWidget* parent = 0, const char* name = 0);
    ~QSplitter();

    void addWidget(QWidget *w);
    void insertWidget(int index, QWidget *w);

    void setOrientation(Qt::Orientation);
    Qt::Orientation orientation() const;

    void setChildrenCollapsible(bool);
    bool childrenCollapsible() const;

    void setCollapsible(QWidget *w, bool);
    void setOpaqueResize(bool = true);
    bool opaqueResize() const;
    void refresh();

    void moveToFirst(QWidget *);
    void moveToLast(QWidget *);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    QList<int> sizes() const;
    void setSizes(const QList<int> &list);

    int handleWidth() const;
    void setHandleWidth(int);

    int indexOf(QWidget *w) const;
    QWidget *widget(int index) const;
    int count() const;

    void moveSplitter(int pos, int index);
    void getRange(int index, int *, int *) const;
    void setRubberband(int position);
    int closestLegalPosition(int, int);
    int indexOfHandle(QSplitterHandle *handle) const;
    QSplitterHandle *handle(int index) const;

protected:
    virtual QSplitterHandle *createHandle();

    void childEvent(QChildEvent *);

    bool event(QEvent *);
    void resizeEvent(QResizeEvent *);

    void changeEvent(QEvent *);

#ifdef QT_COMPAT
public:
    enum ResizeMode { Stretch, KeepSize, FollowSizeHint, Auto };
    QT_COMPAT void setResizeMode(QWidget *w, ResizeMode mode);
protected:
    QT_COMPAT int idAfter(QWidget*) const;
#endif

private:
    Q_DISABLE_COPY(QSplitter)
    Q_DECLARE_PRIVATE(QSplitter)
#ifndef QT_NO_TEXTSTREAM
    friend Q_GUI_EXPORT QTextStream& operator<<(QTextStream&, const QSplitter&);
    friend Q_GUI_EXPORT QTextStream& operator>>(QTextStream&, QSplitter&);
#endif
};

#ifndef QT_NO_TEXTSTREAM
Q_GUI_EXPORT QTextStream& operator<<(QTextStream&, const QSplitter&);
Q_GUI_EXPORT QTextStream& operator>>(QTextStream&, QSplitter&);
#endif

class QSplitterHandlePrivate;
class QSplitterHandle : public QWidget
{
    Q_OBJECT
public:
    QSplitterHandle(Qt::Orientation o, QSplitter *parent);
    void setOrientation(Qt::Orientation o);
    Qt::Orientation orientation() const;
    bool opaque() const;
    QSplitter *splitter() const;

    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

    void moveSplitter(int p);
    int closestLegalPosition(int p);

private:
    Q_DISABLE_COPY(QSplitterHandle)
    Q_DECLARE_PRIVATE(QSplitterHandle)
};


#endif // QT_NO_SPLITTER

#endif // QSPLITTER_H
