/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qscrollview.h#23 $
**
** Definition of QScrollView class
**
** Created : 970523
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/
#ifndef QVIEWP_H
#define QVIEWP_H

#ifndef QT_H
#include "qframe.h"
#include "qscrollbar.h"
#endif // QT_H

struct QScrollViewData;

class QScrollView : public QFrame
{
    Q_OBJECT
public:
    QScrollView(QWidget *parent=0, const char *name=0, WFlags f=0);
    ~QScrollView();

    enum ResizePolicy { Default, Manual, AutoOne };
    virtual void setResizePolicy( ResizePolicy );
    ResizePolicy resizePolicy() const;

    void addChild(QWidget* child);
    void removeChild(QWidget* child);
    virtual void addChild(QWidget* child, int x, int y);
    virtual void moveChild(QWidget* child, int x, int y);
    int childX(QWidget* child);
    int childY(QWidget* child);
    bool childIsVisible(QWidget* child);
    void showChild(QWidget* child, bool yes=TRUE);

    enum ScrollBarMode { Auto, AlwaysOff, AlwaysOn };

    ScrollBarMode vScrollBarMode() const;
    virtual void  setVScrollBarMode( ScrollBarMode );

    ScrollBarMode hScrollBarMode() const;
    virtual void  setHScrollBarMode( ScrollBarMode );

    QWidget*     cornerWidget() const;
    virtual void setCornerWidget(QWidget*);

    QScrollBar*  horizontalScrollBar();
    QScrollBar*  verticalScrollBar();
    QWidget*	 viewport();

    int		contentsWidth() const;
    int		contentsHeight() const;
    int		contentsX() const;
    int		contentsY() const;

    void	resize( int w, int h );
    void	resize( const QSize& );
    void	show();

signals:
    void	contentsMoving(int x, int y);

public slots:
    virtual void resizeContents( int w, int h );
    void	scrollBy( int dx, int dy );
    void        setContentsPos( int x, int y );
    void	ensureVisible(int x, int y);
    void	ensureVisible(int x, int y, int xmargin, int ymargin);
    void	center(int x, int y);
    void	center(int x, int y, float xmargin, float ymargin);

    void	updateScrollBars();

protected:
    void	resizeEvent(QResizeEvent*);
    void 	wheelEvent( QWheelEvent * );
    bool	eventFilter( QObject *, QEvent *e );

    virtual void viewportPaintEvent( QPaintEvent* );
    virtual void viewportMousePressEvent( QMouseEvent* );
    virtual void viewportMouseReleaseEvent( QMouseEvent* );
    virtual void viewportMouseDoubleClickEvent( QMouseEvent* );
    virtual void viewportMouseMoveEvent( QMouseEvent* );

    virtual void drawContentsOffset(QPainter*, int ox, int oy,
		    int cx, int cy, int cw, int ch);
    void	frameChanged();

    void setMargins(int left, int top, int right, int bottom);
    int leftMargin() const;
    int topMargin() const;
    int rightMargin() const;
    int bottomMargin() const;

    bool focusNextPrevChild( bool next );

private:
    void moveContents(int x, int y);

    QScrollViewData* d;

private slots:
    void hslide(int);
    void vslide(int);

private:	// Disabled copy constructor and operator=
    QScrollView( const QScrollView & );
    QScrollView &operator=( const QScrollView & );
    void changeFrameRect(const QRect&);
};

inline void QScrollView::addChild(QWidget* child)
{
    addChild(child,0,0);
}

#endif
