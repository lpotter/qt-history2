/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qmenubar.h#45 $
**
** Definition of QMenuBar class
**
** Created : 941209
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

#ifndef QMENUBAR_H
#define QMENUBAR_H

#ifndef QT_H
#include "qpopupmenu.h"
#endif // QT_H

class QDomElement;

class Q_EXPORT QMenuBar : public QFrame, public QMenuData
{
    Q_OBJECT
public:
    QMenuBar( QWidget *parent=0, const char *name=0 );
   ~QMenuBar();

    void	updateItem( int id );

    void	show();				// reimplemented show
    void	hide();				// reimplemented hide

    bool	eventFilter( QObject *, QEvent * );

    int		heightForWidth(int) const;

    enum	Separator { Never=0, InWindowsStyle=1 };
    Separator 	separator() const;
    virtual void	setSeparator( Separator when );

    bool customWhatsThis() const;

#ifdef QT_BUILDER
    bool setConfiguration( const QDomElement& element );
#endif

signals:
    void	activated( int itemId );
    void	highlighted( int itemId );

protected:
    void	drawContents( QPainter * );
    void	fontChange( const QFont & );
    void	mousePressEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );
    void	keyPressEvent( QKeyEvent * );
    void	focusInEvent( QFocusEvent * );
    void	focusOutEvent( QFocusEvent * );
    void	resizeEvent( QResizeEvent * );
    void	enterEvent( QEvent * );
    void	leaveEvent( QEvent * );
    void	menuContentsChanged();
    void	menuStateChanged();
    void 	styleChange( QStyle& );

private slots:
    void	subActivated( int itemId );
    void	subHighlighted( int itemId );
    void	accelActivated( int itemId );
    void	accelDestroyed();

private:
    void	menuInsPopup( QPopupMenu * );
    void	menuDelPopup( QPopupMenu * );
    void	frameChanged();

    bool	tryMouseEvent( QPopupMenu *, QMouseEvent * );
    void	tryKeyEvent( QPopupMenu *, QKeyEvent * );
    void	goodbye( bool cancelled = FALSE );
    void	openActPopup();
    void	hidePopups();

    virtual void	setActItem( int, bool = FALSE );
    virtual void	setWindowsAltMode( bool, int = 0 );
    
    void setActiveItem( int index, bool show = TRUE, bool activate_first_item = TRUE );
    void setAltMode( bool );

    int		calculateRects( int max_width = -1 );
    int		itemAtPos( const QPoint & );
    QRect	itemRect( int item );

    void	setupAccelerators();
    QAccel     *autoaccel;
    QRect      *irects;
    int		rightSide;

    uint	mseparator	: 1;
    uint	waitforalt : 1;
    uint	popupvisible  : 1;
    uint	hasmouse : 1;
    
    friend class QPopupMenu;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QMenuBar( const QMenuBar & );
    QMenuBar &operator=( const QMenuBar & );
#endif
};


#endif // QMENUBAR_H
