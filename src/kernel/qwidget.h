/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qwidget.h#117 $
**
** Definition of QWidget class
**
** Created : 931029
**
** Copyright (C) 1993-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QWIDGET_H
#define QWIDGET_H

#include "qwindefs.h"
#include "qobject.h"
#include "qpaintd.h"
#include "qpalette.h"
#include "qcursor.h"
#include "qfont.h"
#include "qfontmet.h"
#include "qfontinf.h"


class QWidget : public QObject, public QPaintDevice
{						// base class for UI objects
    Q_OBJECT
public:
    QWidget( QWidget *parent=0, const char *name=0, WFlags f=0 );
   ~QWidget();

    WId		 winId() const;

  // GUI style setting

    GUIStyle	 style() const;
    virtual void setStyle( GUIStyle );

  // Widget types and states

    bool	 isTopLevel()	const;
    bool	 isModal()	const;
    bool	 isPopup()	const;
    bool	 isDesktop()	const;

    bool	 isEnabled()	const;
    virtual void setEnabled( bool );

  // Widget coordinates

    const QRect &frameGeometry() const;
    const QRect &geometry()	const;
    int		 x()		const;
    int		 y()		const;
    QPoint	 pos()		const;
    QSize	 size()		const;
    int		 width()	const;
    int		 height()	const;
    QRect	 rect()		const;
    QRect	 childrenRect() const;

    QSize	 minimumSize()	const;
    QSize	 maximumSize()	const;
    void	 setMinimumSize( const QSize & );
    void	 setMinimumSize( int w, int h );
    void	 setMaximumSize( const QSize & );
    void	 setMaximumSize( int w, int h );
    void	 setMinimumWidth( int w );
    void	 setMinimumHeight( int h );
    void	 setMaximumWidth( int w );
    void	 setMaximumHeight( int h );

    QSize	 sizeIncrement() const;
    void	 setSizeIncrement( const QSize & );
    void	 setSizeIncrement( int w, int h );

    void	 setFixedSize( const QSize & );
    void	 setFixedSize( int w, int h );
    void	 setFixedWidth( int w );
    void	 setFixedHeight( int h );

  // Widget coordinate mapping

    QPoint	 mapToGlobal( const QPoint & )	 const;
    QPoint	 mapFromGlobal( const QPoint & ) const;
    QPoint	 mapToParent( const QPoint & )	 const;
    QPoint	 mapFromParent( const QPoint & ) const;

    QWidget	*topLevelWidget()   const;

  // Widget attribute functions

    const QColor &backgroundColor() const;
    const QColor &foregroundColor() const;
    virtual void setBackgroundColor( const QColor & );

    const QPixmap *backgroundPixmap() const;
    virtual void setBackgroundPixmap( const QPixmap & );

    bool	 backgroundEmpty() const;
    void	 setBackgroundEmpty();

    const QColorGroup &colorGroup() const;
    const QPalette    &palette()    const;
    virtual void       setPalette( const QPalette & );

    const QFont &font()		const;
    virtual void setFont( const QFont & );
    QFontMetrics fontMetrics()	const;
    QFontInfo	 fontInfo()	const;

    const QCursor &cursor() const;
    virtual void setCursor( const QCursor & );

    const char	*caption()	const;
    void	 setCaption( const char * );
    const QPixmap *icon()	const;
    void	 setIcon( const QPixmap & );
    const char	*iconText()	const;
    void	 setIconText( const char * );

    bool	 hasMouseTracking() const;
    void	 setMouseTracking( bool enable );

  // Keyboard input focus functions

    enum FocusPolicy 
    { NoFocus = 0, TabFocus = 0x1, ClickFocus = 0x2, StrongFocus = 0x3 };

    bool	 isActiveWindow() const;
    void	 setActiveWindow();
    bool	 isFocusEnabled() const;
    FocusPolicy	 focusPolicy() const;
    void	 setFocusPolicy( FocusPolicy );
    bool	 hasFocus() const;
    void	 setFocus();
    void	 clearFocus();

  // Grab functions

    void	 grabMouse();
    void	 grabMouse( const QCursor & );
    void	 releaseMouse();
    void	 grabKeyboard();
    void	 releaseKeyboard();
    static QWidget *mouseGrabber();
    static QWidget *keyboardGrabber();

  // Update/refresh functions

    bool	 isUpdatesEnabled() const;
    void	 setUpdatesEnabled( bool enable );
    void	 update();
    void	 update( int x, int y, int w, int h);
    void	 repaint( bool erase=TRUE );
    void	 repaint( int x, int y, int w, int h, bool erase=TRUE );
    void	 repaint( const QRect &, bool erase=TRUE );

  // Widget management functions

    virtual void show();
    virtual void hide();
    void	 iconify();

    virtual bool close( bool forceKill=FALSE );
    bool	 isVisible()	const;
    bool	 isVisibleToTLW() const;

    void	 raise();
    void	 lower();
    virtual void move( int x, int y );
    void	 move( const QPoint & );
    virtual void resize( int w, int h );
    void	 resize( const QSize & );
    virtual void setGeometry( int x, int y, int w, int h );
    void	 setGeometry( const QRect & );

    virtual QSize sizeHint() const;
    virtual void  adjustSize();


    void	 recreate( QWidget *parent, WFlags, const QPoint &,
			   bool showIt=FALSE );

    void	 erase();
    void	 erase( int x, int y, int w, int h );
    void	 erase( const QRect & );
    void	 scroll( int dx, int dy );

    void	 drawText( int x, int y, const char * );
    void	 drawText( const QPoint &, const char * );

  // Misc. functions

public:
    QWidget	*parentWidget() const;
    bool	 testWFlags( WFlags n ) const;
    static QWidget	 *find( WId );
    static QWidgetMapper *wmapper();

  // Event handlers

protected:
    bool	 event( QEvent * );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseReleaseEvent( QMouseEvent * );
    virtual void mouseDoubleClickEvent( QMouseEvent * );
    virtual void mouseMoveEvent( QMouseEvent * );
    virtual void keyPressEvent( QKeyEvent * );
    virtual void keyReleaseEvent( QKeyEvent * );
    virtual void focusInEvent( QFocusEvent * );
    virtual void focusOutEvent( QFocusEvent * );
    virtual void enterEvent( QEvent * );
    virtual void leaveEvent( QEvent * );
    virtual void paintEvent( QPaintEvent * );
    virtual void moveEvent( QMoveEvent * );
    virtual void resizeEvent( QResizeEvent * );
    virtual void closeEvent( QCloseEvent * );

#if defined(_WS_MAC_)
    virtual bool macEvent( MSG * );		// Macintosh event
#elif defined(_WS_WIN_)
    virtual bool winEvent( MSG * );		// Windows event
#elif defined(_WS_PM_)
    virtual bool pmEvent( QMSG * );		// OS/2 PM event
#elif defined(_WS_X11_)
    virtual bool x11Event( XEvent * );		// X11 event
#endif

  // Misc. protected functions

protected:
    virtual void styleChange( GUIStyle );
    virtual void enabledChange( bool );
    virtual void backgroundColorChange( const QColor & );
    virtual void backgroundPixmapChange( const QPixmap & );
    virtual void paletteChange( const QPalette & );
    virtual void fontChange( const QFont & );

#if 1	/* OBSOLETE */
    bool	 acceptFocus()	const;
    void	 setAcceptFocus( bool );
#endif
    int		 metric( int )	const;

    void	 create( WId );
    void	 create( WId, bool initializeWindow, bool destroyOldWindow );
    void	 destroy( bool destroyOldWindow );
    WFlags	 getWFlags()	const;
    void	 setWFlags( WFlags );
    void	 clearWFlags( WFlags n );
    void	 setFRect( const QRect & );
    void	 setCRect( const QRect & );

    virtual bool focusNextPrevChild( bool next );

    QWExtra	*extraData();

#if defined(_WS_PM_)
    int		 convertYPos( int );
    void	 reposChildren();
    WId		 frm_wnd;
#endif

private:
    void	 setWinId( WId );
    bool	 create();
    bool	 destroy();
    void	 showWindow();
    void	 hideWindow();
    void	 createExtra();
    void	 deleteExtra();
    void	 internalMove( int, int );
    void	 internalResize( int, int );
    void	 internalSetGeometry( int, int, int, int );
    void	 deferMove( const QPoint & );
    void	 deferResize( const QSize & );
    void	 cancelMove();
    void	 cancelResize();
    void	 sendDeferredEvents();
    QWidget     *focusWidget() const;
    void 	 insertIntoFocusChain( QWidget * );
    void 	 reparentFocusWidgets( QWidget *parent );
    QFocusData  *focusData( bool create = FALSE );

    WId		 winid;
    WFlags	 flags;
    QRect	 frect;
    QRect	 crect;
    QColor	 bg_col;
    QPalette	 pal;
    QFont	 fnt;
    QCursor	 curs;
    QWExtra	*extra;
    QWidget	*focusChild; // ### unused now
    static void	 createMapper();
    static void	 destroyMapper();
    static QWidgetList   *tlwList();
    static QWidgetMapper *mapper;
    friend class QApplication;
    friend class QPainter;
    friend class QFontMetrics;
    friend class QFontInfo;
    friend class QETWidget;

private:	// Disabled copy constructor and operator=
    QWidget( const QWidget & ) :QPaintDevice(0) {}
    QWidget &operator=( const QWidget & ) { return *this; }
};


inline bool QWidget::testWFlags( WFlags f ) const
{ return (flags & f) != 0; }

inline WId QWidget::winId() const
{ return winid; }

inline bool QWidget::isTopLevel() const
{ return testWFlags(WType_TopLevel); }

inline bool QWidget::isModal() const
{ return testWFlags(WType_Modal); }

inline bool QWidget::isPopup() const
{ return testWFlags(WType_Popup); }

inline bool QWidget::isDesktop() const
{ return testWFlags(WType_Desktop); }

inline bool QWidget::isEnabled() const
{ return !testWFlags(WState_Disabled); }

inline const QRect &QWidget::frameGeometry() const
{ return frect; }

inline const QRect &QWidget::geometry() const
{ return crect; }

inline int QWidget::x() const
{ return frect.x(); }

inline int QWidget::y() const
{ return frect.y(); }

inline QPoint QWidget::pos() const
{ return frect.topLeft(); }

inline QSize QWidget::size() const
{ return crect.size(); }

inline int QWidget::width() const
{ return crect.width(); }

inline int QWidget::height() const
{ return crect.height(); }

inline QRect QWidget::rect() const
{ return QRect(0,0,crect.width(),crect.height()); }

inline void QWidget::setSizeIncrement( const QSize &s )
{ setSizeIncrement(s.width(),s.height()); }

inline void QWidget::setMinimumSize( const QSize &s )
{ setMinimumSize(s.width(),s.height()); }

inline void QWidget::setMaximumSize( const QSize &s )
{ setMaximumSize(s.width(),s.height()); }

inline const QColor &QWidget::backgroundColor() const
{ return bg_col; }

inline const QPalette &QWidget::palette() const
{ return pal; }

inline const QFont &QWidget::font() const
{ return fnt; }

inline QFontMetrics QWidget::fontMetrics() const
{ return QFontMetrics(this); }

inline QFontInfo QWidget::fontInfo() const
{ return QFontInfo(this); }

inline bool QWidget::hasMouseTracking() const
{ return testWFlags(WState_TrackMouse); }

inline bool  QWidget::isFocusEnabled() const
{ return testWFlags(WState_TabToFocus|WState_ClickToFocus); }

inline QWidget::FocusPolicy QWidget::focusPolicy() const
{ return (FocusPolicy)((testWFlags(WState_TabToFocus) ? (int)TabFocus : 0) +
		       (testWFlags(WState_ClickToFocus)?(int)ClickFocus:0)); }

inline bool QWidget::isUpdatesEnabled() const
{ return !testWFlags(WState_BlockUpdates); }

inline void QWidget::repaint( bool erase )
{ repaint( 0, 0, crect.width(), crect.height(), erase ); }

inline void QWidget::repaint( const QRect &r, bool erase )
{ repaint( r.x(), r.y(), r.width(), r.height(), erase ); }

inline void QWidget::erase()
{ erase( 0, 0, crect.width(), crect.height() ); }

inline void QWidget::erase( const QRect &r )
{ erase( r.x(), r.y(), r.width(), r.height() ); }

inline bool QWidget::isVisible() const
{ return testWFlags(WState_Visible); }

inline void QWidget::move( const QPoint &p )
{ move( p.x(), p.y() ); }

inline void QWidget::resize( const QSize &s )
{ resize( s.width(), s.height()); }

inline void QWidget::setGeometry( const QRect &r )
{ setGeometry( r.left(), r.top(), r.width(), r.height() ); }

inline void QWidget::drawText( const QPoint &p, const char *s )
{ drawText( p.x(), p.y(), s ); }

inline QWidget *QWidget::parentWidget() const
{ return (QWidget *)QObject::parent(); }

inline QWidgetMapper *QWidget::wmapper()
{ return mapper; }

inline bool QWidget::acceptFocus() const
{ return testWFlags(WState_ClickToFocus|WState_TabToFocus); }

inline WFlags QWidget::getWFlags() const
{ return flags; }

inline void QWidget::setWFlags( WFlags f )
{ flags |= f; }

inline void QWidget::clearWFlags( WFlags f )
{ flags &= ~f; }


#endif // QWIDGET_H
