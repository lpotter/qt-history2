/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qlistbox.h#44 $
**
** Definition of QListBox widget class
**
** Created : 941121
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
***********************************************************************/

#ifndef QLISTBOX_H
#define QLISTBOX_H

#include "qtablevw.h"
#include "qpixmap.h"


#define LBI_Undefined	0			// list box item types
#define LBI_Text	1
#define LBI_Pixmap	2
#define LBI_UserDefined 1000


class QStrList;
class QLBItemList;

class QListBox;


class QListBoxItem
{
public:
    QListBoxItem();
    virtual ~QListBoxItem();

    virtual const char	  *text()   const { return txt; }
    virtual const QPixmap *pixmap() const { return 0; }

    virtual int	 height( const QListBox * ) const = 0;
    virtual int	 width( const QListBox * )  const = 0;

protected:
    virtual void paint( QPainter * ) = 0;
    void	 setText( const char *text ) { txt = text; }

private:
    QString txt;
    bool selected;

    friend class QListBox;

private:	// Disabled copy constructor and operator=
    QListBoxItem( const QListBoxItem & ) {}
    QListBoxItem &operator=( const QListBoxItem & ) { return *this; }
};


class QListBoxText : public QListBoxItem
{
public:
    QListBoxText( const char * = 0 );
   ~QListBoxText();
    void  paint( QPainter * );
    int	  height( const QListBox * ) const;
    int	  width( const QListBox * )  const;
private:	// Disabled copy constructor and operator=
    QListBoxText( const QListBoxText & ) {}
    QListBoxText &operator=( const QListBoxText & ) { return *this; }
};


class QListBoxPixmap : public QListBoxItem
{
public:
    QListBoxPixmap( const QPixmap & );
   ~QListBoxPixmap();
    const QPixmap *pixmap() const { return &pm; }
protected:
    void paint( QPainter * );
    int height( const QListBox * ) const;
    int width( const QListBox * ) const;
private:
    QPixmap pm;
private:	// Disabled copy constructor and operator=
    QListBoxPixmap( const QListBoxPixmap & ) {}
    QListBoxPixmap &operator=( const QListBoxPixmap & ) { return *this; }
};


class QListBox : public QTableView		// list box widget
{
    Q_OBJECT
public:
    QListBox( QWidget *parent=0, const char *name=0, WFlags f=0  );
   ~QListBox();

    void	setFont( const QFont & );

    uint	count() const;

    void	insertStrList( const QStrList *, int index=-1 );
    void	insertStrList( const char**, int numStrings=-1, int index=-1 );

    void	insertItem( const QListBoxItem *, int index=-1 );
    void	insertItem( const char *text, int index=-1 );
    void	insertItem( const QPixmap &pixmap, int index=-1 );
    void	inSort( const QListBoxItem * );
    void	inSort( const char *text );

    void	removeItem( int index );
    void	clear();

    const char *text( int index )	const;
    const QPixmap *pixmap( int index )	const;

    void	changeItem( const QListBoxItem *, int index );
    void	changeItem( const char *text, int index );
    void	changeItem( const QPixmap &pixmap, int index );

    bool	autoUpdate()	const;
    void	setAutoUpdate( bool );

    int		numItemsVisible() const;

    int		currentItem()	const;
    void	setCurrentItem( int index );
    void	centerCurrentItem();
    int		topItem()	const;
    void	setTopItem( int index );

    bool	dragSelect()		const;
    void	setDragSelect( bool );
    bool	autoScroll()		const;
    void	setAutoScroll( bool );
    bool	autoScrollBar()		const;
    void	setAutoScrollBar( bool );
    bool	scrollBar()		const;
    void	setScrollBar( bool );
    bool	autoBottomScrollBar()	const;
    void	setAutoBottomScrollBar( bool );
    bool	bottomScrollBar()	const;
    void	setBottomScrollBar( bool );
    bool	smoothScrolling()	const;
    void	setSmoothScrolling( bool );

    int		itemHeight()		const;
    int		itemHeight( int index ) const;

    long	maxItemWidth();

signals:
    void	highlighted( int index );
    void	selected( int index );

protected:
    QListBoxItem *item( int index ) const;
    bool	itemVisible( int index );

    int		cellHeight( int index = 0 );
    void	paintCell( QPainter *, int row, int col );

    void	mousePressEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseDoubleClickEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );
    void	keyPressEvent( QKeyEvent *e );
    void	focusInEvent( QFocusEvent *e );
    void	resizeEvent( QResizeEvent * );
    void	timerEvent( QTimerEvent * );

    int		findItem( int yPos ) const;
    bool	itemYPos( int index, int *yPos ) const;
    void	updateItem( int index, bool clear = TRUE );
    void	clearList();
    void	updateCellWidth();

private:
    void	updateNumRows( bool );
    void	insert( const QListBoxItem *, int, bool );
    void	change( const QListBoxItem *lbi, int );
    void	setMaxItemWidth( int );

    uint	doDrag		: 1;
    uint	doAutoScroll	: 1;
    uint	isTiming	: 1;
    uint	scrollDown	: 1;
    uint	stringsOnly	: 1;
    uint	multiSelect	: 1;
    uint	goingDown	: 1;
    int		current;
    QLBItemList *itemList;

private:	// Disabled copy constructor and operator=
    QListBox( const QListBox & ) {}
    QListBox &operator=( const QListBox & ) { return *this; }
};


#endif // QLISTBOX_H
