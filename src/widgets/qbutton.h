/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qbutton.h#7 $
**
** Definition of QButton class
**
** Author  : Haavard Nord
** Created : 940206
**
** Copyright (C) 1994 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QBUTTON_H
#define QBUTTON_H

#include "qwidget.h"


class QPixMapDict;


class QButton : public QWidget			// button class
{
    Q_OBJECT
public:
    QButton( QWidget *parent=0, const char *name=0 );

    const char *label()		const;		// get/set button text
    void setLabel( const char *label, bool resize=FALSE );

    virtual void resizeFitLabel();

signals:
    void    pressed();
    void    released();
    void    clicked();

protected:
    bool    isDown() const { return buttonDown; }
    bool    isUp()   const { return !buttonDown; }

    void    switchOn();				// switch button on
    void    switchOff();			// switch button off
    bool    isOn()   const { return buttonOn; }

    void    setOnOffButton( bool );		// set to on/off type button
    bool    isOnOffButton()  const { return onOffButton; }

    virtual bool hitButton( const QPoint &pos ) const;
    virtual void drawButton( QPainter * );

    void    mousePressEvent( QMouseEvent * );
    void    mouseReleaseEvent( QMouseEvent * );
    void    mouseMoveEvent( QMouseEvent * );
    void    paintEvent( QPaintEvent * );
    bool    focusInEvent( QEvent * );

    static bool	    acceptPixmap( int, int );
    static QPixMap *findPixmap( const char * );
    static void	    savePixmap( const char *, const QPixMap * );

private:
    static void	    delPixmaps();
    static QPixMapDict *pmdict;
    static long	    pmsize;
    QString btext;
    uint    onOffButton	: 1;
    uint    buttonDown	: 1;
    uint    buttonOn	: 1;
    uint    mlbDown	: 1;
};


#endif // QBUTTON_H
