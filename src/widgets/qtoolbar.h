/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qtoolbar.h#14 $
**
** Definition of QToolBar class
**
** Created : 980306
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

#ifndef QTOOLBAR_H
#define QTOOLBAR_H

#ifndef QT_H
#include "qwidget.h"
#include "qmainwindow.h"
#endif // QT_H

class QButton;
class QBoxLayout;
class QToolBarPrivate;


class QToolBar: public QWidget
{
    Q_OBJECT
public:
    QToolBar( const QString &label,
	      QMainWindow *, QMainWindow::ToolBarDock = QMainWindow::Top,
	      bool newLine = FALSE, const char * name = 0 );
    QToolBar( const QString &label, QMainWindow *, QWidget *,
	      bool newLine = FALSE, const char * name = 0, WFlags f = 0 );
    QToolBar( QMainWindow * parent = 0, const char * name = 0 );
    ~QToolBar();

    void addSeparator();

    enum Orientation { Horizontal, Vertical };
    virtual void setOrientation( Orientation );
    Orientation orientation() const { return o; }

    void show();

    QMainWindow * mainWindow();

    virtual void setStretchableWidget( QWidget * );
    
    bool event( QEvent * e );

protected:
    void paintEvent( QPaintEvent * );

private:
    virtual void setUpGM();

    QBoxLayout * b;
    QToolBarPrivate * d;
    Orientation o;
    QMainWindow * mw;
    QWidget * sw;
};


#endif
