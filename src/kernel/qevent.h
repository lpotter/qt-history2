/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qevent.h#70 $
**
** Definition of event classes
**
** Created : 931029
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

#ifndef QEVENT_H
#define QEVENT_H

#ifndef QT_H
#include "qwindowdefs.h"
#include "qregion.h"
#include "qnamespace.h"
#endif // QT_H


class Q_EXPORT QEvent: public Qt		// event base class
{
public:
    enum Type {
	None = 0,				// invalid event
	Timer = 1,				// timer event
	MouseButtonPress = 2,			// mouse button pressed
        MouseButtonRelease = 3,			// mouse button released
	MouseButtonDblClick= 4,			// mouse button double click
	MouseMove = 5,				// mouse move
	KeyPress = 6,				// key pressed
	KeyRelease = 7,				// key released
	FocusIn = 8,				// keyboard focus received
	FocusOut = 9,				// keyboard focus lost
	Enter = 10,				// mouse enters widget
	Leave = 11,				// mouse leaves widget
	Paint = 12,				// paint widget
	Move = 13,				// move widget
	Resize = 14,				// resize widget
	Create = 15,				// after object creation
	Destroy = 16,				// during object destruction
	Show = 17,				// widget is shown
	Hide = 18,				// widget is hidden
	Close = 19,				// request to close widget
	Quit = 20,				// request to quit application
	Accel = 30,				// accelerator event
	Wheel = 31,				// wheel event
	Clipboard = 40,				// internal clipboard event
	SockAct = 50,				// socket activation
	DragEnter = 60,				// drag moves into widget
	DragMove = 61,				// drag moves in widget
	DragLeave = 62,				// drag leaves or is cancelled
	Drop = 63,				// actual drop
	DragResponse = 64,			// drag accepted/rejected
	ChildInserted = 70,			// new child widget
	ChildRemoved = 71,			// deleted child widget
	LayoutHint = 72,			// child min/max size changed
	ActivateControl = 80,			// ActiveX activation
	DeactivateControl = 81,			// ActiveX deactivation
	User = 1000				// first user event id
    };

    QEvent( Type type )
	: t(type), posted(FALSE) {}
   ~QEvent()			{ if ( posted ) peErrMsg(); }
    Type  type()	const	{ return t; }
protected:
    Type  t;
    bool  posted;
private:
    void  peErrMsg();
};


class Q_EXPORT QTimerEvent : public QEvent
{
public:
    QTimerEvent( int timerId )
	: QEvent(Timer), id(timerId) {}
    int	  timerId()	const	{ return id; }
protected:
    int	  id;
};


class Q_EXPORT QMouseEvent : public QEvent
{
public:
    QMouseEvent( Type type, const QPoint &pos, int button, int state );

    QMouseEvent( Type type, const QPoint &pos, const QPoint&globalPos,
		 int button, int state )
	: QEvent(type), p(pos), g(globalPos), b(button),s((ushort)state) {};

    const QPoint &pos() const	{ return p; }
    const QPoint &globalPos() const { return g; }
    int	   x()		const	{ return p.x(); }
    int	   y()		const	{ return p.y(); }
    int	   globalX()	const	{ return g.x(); }
    int	   globalY()	const	{ return g.y(); }
    ButtonState button() const	{ return (ButtonState) b; }
    ButtonState state()	const	{ return (ButtonState) s; }
    ButtonState stateAfter() const;
protected:
    QPoint p;
    QPoint g;
    int	   b;
    ushort s;
};



class Q_EXPORT QWheelEvent : public QEvent
{
public:
    QWheelEvent( const QPoint &pos, int delta, int state )
	: QEvent(Wheel), p(pos), d(delta), s((ushort)state),
	  accpt(TRUE) {}
    int	   delta()	const	{ return d; }
    const QPoint &pos() const	{ return p; }
    int	   x()		const	{ return p.x(); }
    int	   y()		const	{ return p.y(); }
    ButtonState state()	const	{ return ButtonState(s); }
    bool   isAccepted() const	{ return accpt; }
    void   accept()		{ accpt = TRUE; }
    void   ignore()		{ accpt = FALSE; }
protected:
    QPoint p;
    int d;
    ushort s;
    bool   accpt;
};


class Q_EXPORT QKeyEvent : public QEvent
{
public:
    QKeyEvent( Type type, int key, int ascii, int state,
		const QString& text=QString::null, bool autorep=FALSE )
	: QEvent(type), txt(text), k((ushort)key), s((ushort)state),
	    a((uchar)ascii), accpt(TRUE), autor(autorep) {}
    int	   key()	const	{ return k; }
    int	   ascii()	const	{ return a; }
    ButtonState state()	const	{ return ButtonState(s); }
    ButtonState stateAfter() const;
    bool   isAccepted() const	{ return accpt; }
    QString text()      const   { return txt; }
    bool   isAutoRepeat() const	{ return autor; }
    void   accept()		{ accpt = TRUE; }
    void   ignore()		{ accpt = FALSE; }

protected:
    QString txt;
    ushort k, s;
    uchar  a;
    uint   accpt:1;
    uint   autor:1;
};


class Q_EXPORT QFocusEvent : public QEvent
{
public:
    QFocusEvent( Type type )
	: QEvent(type) {}
    bool   gotFocus()	const { return type() == FocusIn; }
    bool   lostFocus()	const { return type() == FocusOut; }
};


class Q_EXPORT QPaintEvent : public QEvent
{
public:
    QPaintEvent( const QRegion& paintRegion )
	: QEvent(Paint),
	  rec(paintRegion.boundingRect()),
	  reg(paintRegion) {}
    QPaintEvent( const QRect &paintRect )
	: QEvent(Paint),
	  rec(paintRect),
          reg(paintRect) {}
    const QRect &rect() const	  { return rec; }
    const QRegion &region() const { return reg; }
protected:
    QRect rec;
    QRegion reg;
};


class Q_EXPORT QMoveEvent : public QEvent
{
public:
    QMoveEvent( const QPoint &pos, const QPoint &oldPos )
	: QEvent(Move), p(pos), oldp(oldPos) {}
    const QPoint &pos()	  const { return p; }
    const QPoint &oldPos()const { return oldp;}
protected:
    QPoint p, oldp;
};


class Q_EXPORT QResizeEvent : public QEvent
{
public:
    QResizeEvent( const QSize &size, const QSize &oldSize )
	: QEvent(Resize), s(size), olds(oldSize) {}
    const QSize &size()	  const { return s; }
    const QSize &oldSize()const { return olds;}
protected:
    QSize s, olds;
};


class Q_EXPORT QCloseEvent : public QEvent
{
public:
    QCloseEvent()
	: QEvent(Close), accpt(FALSE) {}
    bool   isAccepted() const	{ return accpt; }
    void   accept()		{ accpt = TRUE; }
    void   ignore()		{ accpt = FALSE; }
protected:
    bool   accpt;
};


class Q_EXPORT QShowEvent : public QEvent
{
public:
    QShowEvent(bool spontaneous)
	: QEvent(Show), spont(spontaneous) {}
    bool spontaneous() const { return spont; }
protected:
    bool spont;
};


class Q_EXPORT QHideEvent : public QEvent
{
public:
    QHideEvent(bool spontaneous)
	: QEvent(Hide), spont(spontaneous) {}
    bool spontaneous() const { return spont; }
protected:
    bool spont;
};


// This class is rather closed at the moment.  If you need to create your
// own QDragMoveEvent objects, write to qt-bugs@troll.no and we'll try to
// find a way to extend it so it covers your needs.

class Q_EXPORT QDragMoveEvent : public QEvent
{
public:
    QDragMoveEvent( const QPoint& pos )
	: QEvent(DragMove), p(pos), accpt(FALSE), d(0),
	  rect( p,QSize( 1, 1 ) ) {}
    const QPoint& pos() const   { return p; }
    bool   isAccepted() const   { return accpt; }
    void   accept()		{ accpt = TRUE; }
    void   ignore()		{ accpt = FALSE; }
    void   accept( const QRect & r) { accpt = TRUE; rect = r; }
    void   ignore( const QRect & r) { accpt =FALSE; rect = r; }
    QRect  answerRect() const { return rect; }
    const char* format( int n = 0 );
    bool provides( const char* );
    QByteArray data( const char* );
protected:
    QDragMoveEvent( const QPoint& pos, Type type )
	: QEvent(type), p(pos), accpt(FALSE), d(0),
	  rect( p,QSize( 1, 1 ) ) {}
    QPoint p;
    bool   accpt;
    void * d;
    QRect rect;
};


class Q_EXPORT QDragEnterEvent : public QDragMoveEvent
{
public:
    QDragEnterEvent( const QPoint& pos ) :
	QDragMoveEvent(pos, DragEnter) { }
};


class Q_EXPORT QDragResponseEvent : public QEvent
{
public:
    QDragResponseEvent( bool accepted )
	: QEvent(DragResponse), a(accepted) {}
    bool   dragAccepted() const	{ return a; }
protected:
    bool a;
};


class Q_EXPORT QDragLeaveEvent : public QEvent
{
public:
    QDragLeaveEvent()
	: QEvent(DragLeave) {}
};


class Q_EXPORT QDropEvent : public QEvent
{
public:
    QDropEvent( const QPoint& pos )
	: QEvent(Drop), p(pos), accpt(FALSE) {}
    const QPoint &pos() const	{ return p; }
    bool   isAccepted() const	{ return accpt; }
    void   accept()		{ accpt = TRUE; }
    void   ignore()		{ accpt = FALSE; }
    QByteArray data( const char* );
protected:
    QPoint p;
    bool   accpt;
};


class Q_EXPORT QChildEvent : public QEvent
{
public:
    QChildEvent( Type type, QObject *child )
	: QEvent(type), c(child) {}
    QObject *child() const	{ return c; }
    bool inserted() const { return t == ChildInserted; }
    bool removed() const { return t == ChildRemoved; }
protected:
    QObject *c;
};


class Q_EXPORT QCustomEvent : public QEvent
{
public:
    QCustomEvent( Type type, void *data )
	: QEvent(type), d(data) {}
    void       *data()	const	{ return d; }
private:
    void       *d;
};


#endif // QEVENT_H
