/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qpopupmenu.h#43 $
**
** Definition of QPopupMenu class
**
** Created : 941128
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

#ifndef QPOPUPMENU_H
#define QPOPUPMENU_H

#ifndef QT_H
#include "qtableview.h"
#include "qmenudata.h"
#endif // QT_H


class QPopupMenu : public QTableView, public QMenuData
{
friend class QMenuData;
friend class QMenuBar;
    Q_OBJECT
public:
    QPopupMenu( QWidget *parent=0, const char *name=0 );
   ~QPopupMenu();

    void	popup( const QPoint & pos, int indexAtPoint = 0 );// open popup
    void	updateItem( int id );

    void	setCheckable( bool );
    bool	isCheckable() const;

    void	setFont( const QFont & );	// reimplemented set font
    void	show();				// reimplemented show
    void	hide();				// reimplemented hide

    int		exec();
    int 	exec( const QPoint & pos, int indexAtPoint = 0 );// modal popup

    void	setActiveItem( int );

signals:
    void	activated( int itemId );
    void	highlighted( int itemId );
    void	activatedRedirect( int itemId );// to parent menu
    void	highlightedRedirect( int itemId );
    void	aboutToShow();

protected:
    int		cellHeight( int );
    int		cellWidth( int );
    void	paintCell( QPainter *, int, int );

    void	paintEvent( QPaintEvent * );
    void	mousePressEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );
    void	keyPressEvent( QKeyEvent * );
    void	timerEvent( QTimerEvent * );

private slots:
    void	subActivated( int itemId );
    void	subHighlighted( int itemId );
    void	accelActivated( int itemId );
    void	accelDestroyed();
    void	modalActivation( int );

    void	subMenuTimer();

private:
    void	menuContentsChanged();
    void	menuStateChanged();
    void	menuInsPopup( QPopupMenu * );
    void	menuDelPopup( QPopupMenu * );
    void	frameChanged();

    void	paintAll();
    void	actSig( int );
    void	hilitSig( int );
    void	setFirstItemActive();
    void	hideAllPopups();
    void	hidePopups();
    bool	tryMenuBar( QMouseEvent * );
    void	byeMenuBar();

    int		itemAtPos( const QPoint & );
    int		itemPos( int index );
    void	updateSize();
    void	updateRow( int row );
    void	updateAccel( QWidget * );
    void	enableAccel( bool );

    void	setTabMark( int );
    int		tabMark();
    void	setCheckableFlag( bool );

    QMenuItem  *selfItem;
    QAccel     *autoaccel;
    bool	accelDisabled;
    int		popupActive;
    int		tabCheck;
    bool	hasDoubleItem;
    int		maxPMWidth;

private:	// Disabled copy constructor and operator=
    QPopupMenu( const QPopupMenu & );
    QPopupMenu &operator=( const QPopupMenu & );
};


#endif // QPOPUPMENU_H
