/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qlistview.h#6 $
**
** Definition of 
**
** Copyright (C) 1996 by Troll Tech AS.  All rights reserved.
**
****************************************************************************/

#ifndef QLISTVIEW_H
#define QLISTVIEW_H

class QStrList;
class QPixmap;

class QListView;
struct QListViewPrivate;

#include "qscrollview.h"


class QListViewItem
{
public:
    QListViewItem( QListView * parent );
    QListViewItem( QListViewItem * parent );
    QListViewItem( QListViewItem * parent,
		   const char * firstLabel, ... );
    QListViewItem( QListViewItem * parent,
		   const QPixmap, const char * firstLabel, ... );
    virtual ~QListViewItem();

    virtual void insertItem( QListViewItem * );
    virtual void removeItem( QListViewItem * );

    virtual int compare( int column, const QListViewItem * with ) const;

    int height() const { return ownHeight; }
    virtual void setHeight( int );
    virtual void invalidateHeight();
    int totalHeight() const;

    virtual const char * text( int ) const;

    int children() const { return childCount; }

    bool isOpen() const { return open && childCount>0; } // ###
    virtual void setOpen( bool );

    virtual void paintCell( QPainter *,  const QColorGroup & cg,
			    int column, int width ) const;
    virtual void paintTreeBranches( QPainter * p, const QColorGroup & cg,
				    int w, int y, int h, GUIStyle s ) const;

    const QListViewItem * firstChild() const { return childItem; }
    const QListViewItem * nextSibling() const { return siblingItem; }

    virtual QListView *listView() const;

private:
    void init();

    int ownHeight;
    int maybeTotalHeight;
    int childCount;
    bool open;

    QListViewItem * parentItem;
    QListViewItem * siblingItem;
    QListViewItem * childItem;

    QStrList * columnTexts;
    QPixmap * icon;

    friend QListView;
};


class QListView: public QScrollView
{
    Q_OBJECT
public:
    QListView( QWidget * parent = 0, const char * name = 0 );
    ~QListView();

    int treeStepSize() const; 
    virtual void setTreeStepSize( int );

    virtual void insertItem( QListViewItem * );
    virtual void clear();

    virtual void setColumn( const char * label, int size, int column=-1 );

    void show();

public slots:
    void triggerUpdate();

signals:
    void sizeChanged();

protected:
    bool eventFilter( QObject * o, QEvent * );
    void drawContentsOffset( QPainter *, int ox, int oy,
			     int cx, int cy, int cw, int ch );

protected slots:
    void updateContents();

private:
    QListViewPrivate * d;
};


#endif
