/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qlistbox.h#104 $
**
** Definition of QListBox widget class
**
** Created : 941121
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QLISTBOX_H
#define QLISTBOX_H

#ifndef QT_H
#include "qscrollview.h"
#include "qpixmap.h"
#include "qtimer.h"
#endif // QT_H


class QListBoxPrivate;
class QListBoxItem;
class QString;
class QStrList;
class QStringList;


class Q_EXPORT QListBox : public QScrollView
{
    friend class QListBoxItem;

    Q_OBJECT
public:
    QListBox( QWidget *parent=0, const char *name=0, WFlags f=0  );
   ~QListBox();

    virtual void setFont( const QFont & );

    uint count() const;

    void insertStringList( const QStringList&, int index=-1 );
    void insertStrList( const QStrList *, int index=-1 );
    void insertStrList( const QStrList &, int index=-1 );
    void insertStrList( const char **,
			int numStrings=-1, int index=-1 );

    void insertItem( const QListBoxItem *, int index=-1 );
    void insertItem( const QListBoxItem *, const QListBoxItem *after );
    void insertItem( const QString &text, int index=-1 );
    void insertItem( const QPixmap &pixmap, int index=-1 );
    void insertItem( const QPixmap &pixmap, const QString &text, int index=-1 );

    void removeItem( int index );
    void clear();

    QString text( int index )	const;
    const QPixmap *pixmap( int index )	const;

    void changeItem( const QListBoxItem *, int index );
    void changeItem( const QString &text, int index );
    void changeItem( const QPixmap &pixmap, int index );
    void changeItem( const QPixmap &pixmap, const QString &text, int index );

    void takeItem( const QListBoxItem * );

    int numItemsVisible() const;

    int currentItem() const;
    QString currentText() const { return text(currentItem()); }
    virtual void setCurrentItem( int index );
    virtual void setCurrentItem( QListBoxItem * );
    void centerCurrentItem() { ensureCurrentVisible(); }
    int topItem() const;
    virtual void setTopItem( int index );
    virtual void setBottomItem( int index );

    long maxItemWidth() const;

    enum SelectionMode { Single, Multi, Extended, NoSelection };
    virtual void setSelectionMode( SelectionMode );
    SelectionMode selectionMode() const;

    void setMultiSelection( bool multi );
    bool isMultiSelection() const;

    virtual void setSelected( QListBoxItem *, bool );
    void setSelected( int, bool );
    bool isSelected( int ) const;
    bool isSelected( const QListBoxItem * ) const;

    QSize sizeHint() const;
    QSize	minimumSizeHint() const;

    QListBoxItem *item( int index ) const;
    int index( const QListBoxItem * ) const;
    QListBoxItem *findItem( const QString &text ) const;

    void triggerUpdate( bool doLayout );

    bool itemVisible( int index );
    bool itemVisible( const QListBoxItem * );

    enum LayoutMode { FixedNumber,
		      FitToWidth, FitToHeight = FitToWidth,
		      Variable };
    virtual void setColumnMode( LayoutMode );
    virtual void setColumnMode( int );
    virtual void setRowMode( LayoutMode );
    virtual void setRowMode( int );

    LayoutMode columnMode() const;
    LayoutMode rowMode() const;

    int numColumns() const;
    int numRows() const;

    bool variableWidth() const;
    virtual void setVariableWidth( bool );

    bool variableHeight() const;
    virtual void setVariableHeight( bool );

    void viewportPaintEvent( QPaintEvent * );

#ifndef QT_NO_COMPAT // obsolete, provided for source compatibility
    bool dragSelect() const { return TRUE; }
    void setDragSelect( bool ) {}
    bool autoScroll() const { return TRUE; }
    void setAutoScroll( bool ) {}
    bool autoScrollBar() const { return vScrollBarMode() == Auto; }
    void setAutoScrollBar( bool enable ) { setVScrollBarMode( enable ? Auto : AlwaysOff ); }
    bool scrollBar() const { return vScrollBarMode() != AlwaysOff; }
    void setScrollBar( bool enable ) { setVScrollBarMode( enable ? AlwaysOn : AlwaysOff ); }
    bool autoBottomScrollBar() const { return hScrollBarMode() == Auto; }
    void setAutoBottomScrollBar( bool enable ) { setHScrollBarMode( enable ? Auto : AlwaysOff ); }
    bool bottomScrollBar() const { return hScrollBarMode() != AlwaysOff; }
    void setBottomScrollBar( bool enable ) { setHScrollBarMode( enable ? AlwaysOn : AlwaysOff ); }
    bool smoothScrolling() const { return FALSE; }
    void setSmoothScrolling( bool ) {}
    bool autoUpdate() const { return TRUE; }
    void setAutoUpdate( bool ) {}
    void setFixedVisibleLines( int lines ) { setRowMode( lines ); }
    void inSort( const QListBoxItem * );
    void inSort( const QString& text );
    int cellHeight( int i ) const { return itemHeight(i); }
    int cellHeight() const { return itemHeight(); }
    int cellWidth() const { return maxItemWidth(); }
    int cellWidth(int i) const { ASSERT(i==0); return maxItemWidth(); }
    int	numCols() const { return numColumns(); }
#endif

    int itemHeight( int index = 0 ) const;
    QListBoxItem * itemAt( QPoint ) const;

    QRect itemRect( QListBoxItem *item ) const;

public slots:
    virtual void ensureCurrentVisible();
    virtual void clearSelection();
    void invertSelection(); // ######### make virtual

signals:
    void highlighted( int index );
    void selected( int index );
    void highlighted( const QString &);
    void selected( const QString &);
    void highlighted( QListBoxItem * );
    void selected( QListBoxItem * );

    void selectionChanged();
    void selectionChanged( QListBoxItem * );
    void currentChanged( QListBoxItem * );
    void clicked( QListBoxItem * );
    void clicked( QListBoxItem *, const QPoint & );
    void pressed( QListBoxItem * );
    void pressed( QListBoxItem *, const QPoint & );

    void doubleClicked( QListBoxItem * );
    void returnPressed( QListBoxItem * );
    void rightButtonClicked( QListBoxItem *, const QPoint & );
    void rightButtonPressed( QListBoxItem *, const QPoint & );
    void mouseButtonPressed( int, QListBoxItem*, const QPoint& );
    void mouseButtonClicked( int, QListBoxItem*, const QPoint& );

    void onItem( QListBoxItem *item );
    void onViewport();

protected:
    void viewportMousePressEvent( QMouseEvent * );
    void viewportMouseReleaseEvent( QMouseEvent * );
    void viewportMouseDoubleClickEvent( QMouseEvent * );
    void viewportMouseMoveEvent( QMouseEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseDoubleClickEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );
    void keyPressEvent( QKeyEvent *e );
    void focusInEvent( QFocusEvent *e );
    void focusOutEvent( QFocusEvent *e );
    void resizeEvent( QResizeEvent * );
    void showEvent( QShowEvent * );

    void updateItem( int index );
    void updateItem( QListBoxItem * );

#ifndef QT_NO_COMPAT
    void updateCellWidth() { }
    int totalWidth() const { return contentsWidth(); }
    int totalHeight() const { return contentsHeight(); }
#endif

    virtual void paintCell( QPainter *, int row, int col );

    void toggleCurrentItem();

    void doLayout() const;

#ifndef QT_NO_COMPAT // obsolete, provided for source compatibility
    bool itemYPos( int index, int *yPos ) const;
    int findItem( int yPos ) const {return index(itemAt(QPoint(0,yPos)));}
#endif

protected slots:
    void clearInputString();

private slots:
    void refreshSlot();
    void doAutoScroll();
    void adjustItems();

private:
    void tryGeometry( int, int ) const;
    int currentRow() const;
    int currentColumn() const;
    void updateSelection();

    void emitChangedSignal( bool );

    int columnAt( int ) const;
    int rowAt( int ) const;

    QListBoxPrivate * d;

    static QListBox * changedListBox;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QListBox( const QListBox & );
    QListBox &operator=( const QListBox & );
#endif
};


class Q_EXPORT QListBoxItem
{
public:
    QListBoxItem( QListBox* listbox = 0);
    QListBoxItem( QListBox* listbox, QListBoxItem *after);
    virtual ~QListBoxItem();

    virtual QString text() const;
    virtual const QPixmap *pixmap() const;

    virtual int	 height( const QListBox * ) const;
    virtual int	 width( const QListBox * )  const;

    bool selected() const { return s; }
    bool current() const;

    QListBox *listBox() const;

protected:
    virtual void paint( QPainter * ) = 0;
    virtual void setText( const QString &text ) { txt = text; }
    void setCustomHighlighting( bool );

private:
    QString txt;
    uint s:1;
    uint dirty:1;
    uint custom_highlight : 1;
    int x, y;
    QListBoxItem * p, * n;
    QListBox* lbox;
    friend class QListBox;
    friend class QComboBox;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QListBoxItem( const QListBoxItem & );
    QListBoxItem &operator=( const QListBoxItem & );
#endif
};


class Q_EXPORT QListBoxText : public QListBoxItem
{
public:
    QListBoxText( QListBox* listbox, const QString & text=QString::null );
    QListBoxText( const QString & text=QString::null );
    QListBoxText( QListBox* listbox, const QString & text, QListBoxItem *after );
   ~QListBoxText();

    int	 height( const QListBox * ) const;
    int	 width( const QListBox * )  const;

protected:
    void  paint( QPainter * );

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QListBoxText( const QListBoxText & );
    QListBoxText &operator=( const QListBoxText & );
#endif
};


class Q_EXPORT QListBoxPixmap : public QListBoxItem
{
public:
    QListBoxPixmap( QListBox* listbox, const QPixmap & );
    QListBoxPixmap( const QPixmap & );
    QListBoxPixmap( QListBox* listbox, const QPixmap & pix, QListBoxItem *after );
    QListBoxPixmap( QListBox* listbox, const QPixmap &, const QString& );
    QListBoxPixmap( const QPixmap &, const QString& );
    QListBoxPixmap( QListBox* listbox, const QPixmap & pix, const QString&, QListBoxItem *after );
   ~QListBoxPixmap();

    const QPixmap *pixmap() const { return &pm; }

    int	 height( const QListBox * ) const;
    int	 width( const QListBox * )  const;

protected:
    void paint( QPainter * );

private:
    QPixmap pm;
private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QListBoxPixmap( const QListBoxPixmap & );
    QListBoxPixmap &operator=( const QListBoxPixmap & );
#endif
};


#endif // QLISTBOX_H
