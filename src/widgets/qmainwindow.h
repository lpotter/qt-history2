/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qmainwindow.h#35 $
**
** Definition of QMainWindow class
**
** Created : 980316
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QMAINWINDOW_H
#define QMAINWINDOW_H

#ifndef QT_H
#include "qwidget.h"
#include "qtoolbar.h"
#include "qlist.h"
#endif // QT_H

#ifndef QT_NO_COMPLEXWIDGETS

class QMenuBar;
class QStatusBar;
class QToolTipGroup;

class QMainWindowPrivate;

class Q_EXPORT QMainWindow: public QWidget
{
    Q_OBJECT
    Q_PROPERTY( bool rightJustification READ rightJustification WRITE setRightJustification )
    Q_PROPERTY( bool usesBigPixmaps READ usesBigPixmaps WRITE setUsesBigPixmaps )
    Q_PROPERTY( bool usesTextLabel READ usesTextLabel WRITE setUsesTextLabel )
    Q_PROPERTY( bool toolBarsMovable READ toolBarsMovable WRITE setToolBarsMovable )
    Q_PROPERTY( bool dockWidgetsMovable READ dockWidgetsMovable WRITE setDockWidgetsMovable )
    Q_PROPERTY( bool opaqueMoving READ opaqueMoving WRITE setOpaqueMoving )

public:
    QMainWindow( QWidget * parent = 0, const char * name = 0, WFlags f = WType_TopLevel );
    ~QMainWindow();

#ifndef QT_NO_MENUBAR
    QMenuBar * menuBar() const;
#endif
    QStatusBar * statusBar() const;
    QToolTipGroup * toolTipGroup() const;

    virtual void setCentralWidget( QWidget * );
    QWidget * centralWidget() const;

    virtual void setDockEnabled( Dock dock, bool enable );
    bool isDockEnabled( Dock dock ) const;
    bool isDockEnabled( QDockArea *area ) const;
    virtual void setDockEnabled( QDockWidget *tb, Dock dock, bool enable );
    bool isDockEnabled( QDockWidget *tb, Dock dock ) const;
    bool isDockEnabled( QDockWidget *tb, QDockArea *area ) const;

    void addDockWidget( QDockWidget *, Dock = Top, bool newLine = FALSE );
    void addDockWidget( QDockWidget *, const QString &label,
		     Dock = Top, bool newLine = FALSE );
    void moveDockWidget( QDockWidget *, Dock = Top );
    void moveDockWidget( QDockWidget *, Dock, bool nl, int index, int extraOffset = -1 );
    void removeDockWidget( QDockWidget * );

    void show();
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    bool rightJustification() const;
    bool usesBigPixmaps() const;
    bool usesTextLabel() const;
    bool dockWidgetsMovable() const;
    bool opaqueMoving() const;

    bool eventFilter( QObject*, QEvent* );

    bool getLocation( QDockWidget *tb, Dock &dock, int &index, bool &nl, int &extraOffset ) const;

    QList<QDockWidget> dockWidgets( Dock dock ) const;
    void lineUpDockWidgets( bool keepNewLines = FALSE );

    bool isDockMenuEnabled() const;

    // compatibility stuff
    void addToolBar( QDockWidget *, Dock = Top, bool newLine = FALSE );
    void addToolBar( QDockWidget *, const QString &label,
		     Dock = Top, bool newLine = FALSE );
    void moveToolBar( QDockWidget *, Dock = Top );
    void moveToolBar( QDockWidget *, Dock, bool nl, int index, int extraOffset = -1 );
    void removeToolBar( QDockWidget * );
    bool hasDockWidget( QDockWidget *dw );
    
    bool toolBarsMovable() const;
    QList<QToolBar> toolBars( Dock dock ) const;
    void lineUpToolBars( bool keepNewLines = FALSE );

    QDockArea *dockingArea( const QPoint &p );

public slots:
    virtual void setRightJustification( bool );
    virtual void setUsesBigPixmaps( bool );
    virtual void setUsesTextLabel( bool );
    virtual void setDockWidgetsMovable( bool );
    virtual void setOpaqueMoving( bool );
    virtual void setDockMenuEnabled( bool );
    virtual void whatsThis();

    // compatibility stuff
    void setToolBarsMovable( bool );

signals:
    void pixmapSizeChanged( bool );
    void usesTextLabelChanged( bool );
    void dockWidgetPositionChanged( QDockWidget * );

    // compatibility stuff
    void toolBarPositionChanged( QToolBar * );

protected slots:
    virtual void setUpLayout();
    bool showDockMenu( const QPoint &globalPos );

protected:
    void paintEvent( QPaintEvent * );
    void childEvent( QChildEvent * );
    bool event( QEvent * );
    void styleChange( QStyle& );

private slots:
    void slotPositionChanged();

private:
    QMainWindowPrivate * d;
    void triggerLayout( bool deleteLayout = TRUE);

#ifndef QT_NO_MENUBAR
    virtual void setMenuBar( QMenuBar * );
#endif
    virtual void setStatusBar( QStatusBar * );
    virtual void setToolTipGroup( QToolTipGroup * );

    friend class QDockWidget;
    friend class QMenuBar;
    friend class QHideDock;
    friend class QToolBar;
private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QMainWindow( const QMainWindow & );
    QMainWindow& operator=( const QMainWindow & );
#endif
};

inline void QMainWindow::addToolBar( QDockWidget *w, ToolBarDock d, bool newLine )
{
    addDockWidget( w, d, newLine );
}

inline void QMainWindow::addToolBar( QDockWidget *w, const QString &label,
			      ToolBarDock d, bool newLine )
{
    addDockWidget( w, label, d, newLine );
}

inline void QMainWindow::moveToolBar( QDockWidget *w, ToolBarDock d )
{
    moveDockWidget( w, d );
}

inline void QMainWindow::moveToolBar( QDockWidget *w, ToolBarDock d, bool nl, int index, int extraOffset )
{
    moveDockWidget( w, d, nl, index, extraOffset );
}

inline void QMainWindow::removeToolBar( QDockWidget *w )
{
    removeDockWidget( w );
}

inline bool QMainWindow::toolBarsMovable() const
{
    return dockWidgetsMovable();
}

inline void QMainWindow::lineUpToolBars( bool keepNewLines )
{
    lineUpDockWidgets( keepNewLines );
}

inline void QMainWindow::setToolBarsMovable( bool b )
{
    setDockWidgetsMovable( b );
}

#endif // QT_NO_COMPLEXWIDGETS

#endif // QMAINWINDOW_H
