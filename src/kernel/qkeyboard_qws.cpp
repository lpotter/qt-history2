/****************************************************************************
** $Id: //depot/qt/qws/util/qws/qkeyboard_qws.cpp#4 $
**
** Implementation of Qt/Embedded keyboard drivers
**
** Created : 991025
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Qt/Embedded may use this file in accordance with the
** Qt Embedded Commercial License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qwindowsystem_qws.h"
#include "qwsutils_qws.h"
#include "qgfx_qws.h"

#include "qapplication.h"
#include "qsocketnotifier.h"
#include "qnamespace.h"
#include "qtimer.h"

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <termios.h>
#include <sys/kd.h>
#include <sys/vt.h>




#ifndef QT_NO_QWS_KEYBOARD

static int xform_dirkey(int key)
{
    int xf = qt_screen->transformOrientation();
    return (key-Qt::Key_Left+xf)%4+Qt::Key_Left;
}

#define VTSWITCHSIG SIGUSR2

static QWSServer *server;

static bool vtActive = true;
static int  vtQws = 0;
static int  kbdFD = -1;

/*!
  \class QWSKeyboardHandler qkeyboard_qws.h
  \brief The QWSKeyboardHandler class implements the keyboard
  driver/handler for Qt/Embedded.

  The keyboard driver/handler handles events from system devices and
  generates key events.

  A QWSKeyboardHandler will usually open some system device in its
  constructor, create a QSocketNotifier on that opened device and when
  it receives data, it will call processKeyEvent() to send the event
  to Qt/Embedded for relaying to clients.
*/

/*!
  Subclasses call this to send a key event. The server may additionally
  filter it before sending it on to applications.

  <ul>
  <li>\a unicode is the Unicode value for the key, or 0xFFFF is none is appropriate.
  <li>\a keycode is the Qt keycode for the key (see Qt::Key).
       for the list of codes).
  <li>\a modifiers is the set of modifier keys (see Qt::Modifier).
  <li>\a isPress says whether this is a press or a release.
  <li>\a autoRepeat says whether this event was generated by an auto-repeat
	    mechanism, or an actual key press.
  </ul>
*/
void QWSKeyboardHandler::processKeyEvent(int unicode, int keycode, int modifiers,
			bool isPress, bool autoRepeat)
{
    server->processKeyEvent( unicode, keycode, modifiers, isPress, autoRepeat );
}

class QWSPC101KeyboardHandler : public QWSKeyboardHandler
{
    Q_OBJECT
public:
    QWSPC101KeyboardHandler();
    virtual ~QWSPC101KeyboardHandler();

    void doKey(uchar scancode);

private slots:
    void autoRepeat();
private:
    int shift;
    int alt;
    int ctrl;
    bool extended;
    int modifiers;
    int prevuni;
    int prevkey;

#ifdef QT_QWS_IPAQ
    // Could be used by other subclasses
    QTimer* repeater;
    int repeatdelay, repeatperiod;
#endif
};

void QWSPC101KeyboardHandler::autoRepeat()
{
#ifdef QT_QWS_IPAQ
    processKeyEvent( prevuni, prevkey, modifiers, FALSE, TRUE );
    processKeyEvent( prevuni, prevkey, modifiers, TRUE, TRUE );
    repeater->start(repeatperiod);
#endif
}


class QWSTtyKeyboardHandler : public QWSPC101KeyboardHandler
{
    Q_OBJECT
public:
    QWSTtyKeyboardHandler();
    virtual ~QWSTtyKeyboardHandler();

private slots:
    void readKeyboardData();

private:
    struct termios origTermData;
};

class QWSUsbKeyboardHandler : public QWSPC101KeyboardHandler
{
    Q_OBJECT
public:
    QWSUsbKeyboardHandler();
    virtual ~QWSUsbKeyboardHandler();

private slots:
    void readKeyboardData();

private:
    int fd;
};

class QWSVr41xxButtonsHandler : public QWSKeyboardHandler
{
    Q_OBJECT
public:
    QWSVr41xxButtonsHandler();
    virtual ~QWSVr41xxButtonsHandler();

    bool isOpen() { return buttonFD > 0; }

private slots:
    void readKeyboardData();

private:
    QString terminalName;
    int buttonFD;
    int kbdIdx;
    int kbdBufferLen;
    unsigned char *kbdBuffer;
    QSocketNotifier *notifier;
};

class QWSVFbKeyboardHandler : public QWSKeyboardHandler
{
    Q_OBJECT
public:
    QWSVFbKeyboardHandler();
    virtual ~QWSVFbKeyboardHandler();

    bool isOpen() { return fd > 0; }

private slots:
    void readKeyboardData();

private:
    QString terminalName;
    int fd;
    int kbdIdx;
    int kbdBufferLen;
    unsigned char *kbdBuffer;
    QSocketNotifier *notifier;
};


static void vtSwitchHandler(int /*sig*/)
{
    if (vtActive) {
	server->enablePainting(false);
	qt_screen->save();
	if (ioctl(kbdFD, VT_RELDISP, 1) == 0) {
	    vtActive = false;
	    server->closeMouse();
	}
	else {
	    server->enablePainting(true);
	}
	usleep(200000);
    }
    else {
	if (ioctl(kbdFD, VT_RELDISP, VT_ACKACQ) == 0) {
	    server->enablePainting(true);
	    vtActive = true;
	    qt_screen->restore();
	    server->openMouse();
	    server->refresh();
	}
    }

    signal(VTSWITCHSIG, vtSwitchHandler);
}

//
// PC-101 type keyboards
//

QWSPC101KeyboardHandler::QWSPC101KeyboardHandler()
{
    shift = 0;
    alt   = 0;
    ctrl  = 0;
    extended = false;
    prevuni = 0;
    prevkey = 0;
#ifdef QT_QWS_IPAQ
    repeatdelay = 400;
    repeatperiod = 80;
    repeater = new QTimer(this);
    connect(repeater, SIGNAL(timeout()), this, SLOT(autoRepeat()));
#endif
}

QWSPC101KeyboardHandler::~QWSPC101KeyboardHandler()
{
}

void QWSPC101KeyboardHandler::doKey(uchar code)
{
    int keyCode = Qt::Key_unknown;
    bool release = false;
    int keypad = 0;

    if (code == 224) {
	// extended
	extended = true;
	return;
    }

    if (code & 0x80) {
	release = true;
	code &= 0x7f;
    }

    if (extended) {
	switch (code) {
	case 72:
	    keyCode = Qt::Key_Up;
	    break;
	case 75:
	    keyCode = Qt::Key_Left;
	    break;
	case 77:
	    keyCode = Qt::Key_Right;
	    break;
	case 80:
	    keyCode = Qt::Key_Down;
	    break;
	case 82:
	    keyCode = Qt::Key_Insert;
	    break;
	case 71:
	    keyCode = Qt::Key_Home;
	    break;
	case 73:
	    keyCode = Qt::Key_Prior;
	    break;
	case 83:
	    keyCode = Qt::Key_Delete;
	    break;
	case 79:
	    keyCode = Qt::Key_End;
	    break;
	case 81:
	    keyCode = Qt::Key_Next;
	    break;
	case 28:
	    keyCode = Qt::Key_Enter;
	    break;
	case 53:
	    keyCode = Qt::Key_Slash;
	    break;
	}
    } else {
	if (code < 90) {
	    keyCode = QWSServer::keyMap()[code].key_code;
	}
#ifdef QT_QWS_IPAQ
	else {
	    bool repeatable = TRUE;
	    switch (code) {
		case 0x7a: case 0x7b: case 0x7c: case 0x7d:
		    keyCode = code - 0x7a + Key_F1;
		    repeatable = FALSE;
		    break;
		case 0x79:
		    keyCode = Key_SysReq;
		    repeatable = FALSE;
		    break;
		case 0x78:
		    keyCode = Key_Escape;
		    repeatable = FALSE;
		    break;
		case 0x60:
		    keyCode = Key_Return;
		    break;
		case 0x67:
		    keyCode = Key_Right;
		    break;
		case 0x69:
		    keyCode = Key_Up;
		    break;
		case 0x6a:
		    keyCode = Key_Down;
		    break;
		case 0x6c:
		    keyCode = Key_Left;
		    break;
	    }
	    if ( qt_screen->isTransformed()
		    && keyCode >= Qt::Key_Left && keyCode <= Qt::Key_Down )
	    {
		keyCode = xform_dirkey(keyCode);
	    }
	    if ( repeatable && !release )
		repeater->start(repeatdelay,TRUE);
	    else
		repeater->stop();

	}
#endif
    }

    /*
      Keypad consists of extended keys 53 and 28,
      and non-extended keys 55 and 71 through 83.
    */
    if ( extended ? (code == 53 || code == 28) :
	 (code == 55 || ( code >= 71 && code <= 83 )) )
	keypad = Qt::Keypad;



#if 0 //debug
    printf( "%d ", code );
    if (extended)
	printf(" (Extended) ");
    if (release)
	printf(" (Release) ");
    if (keypad)
	printf(" (Keypad) ");
    printf("\r\n");
#endif

    // Virtual console switching
    int term = 0;
    if (ctrl && alt && keyCode >= Qt::Key_F1 && keyCode <= Qt::Key_F10)
	term = keyCode - Qt::Key_F1 + 1;
    else if (ctrl && alt && keyCode == Qt::Key_Left)
	term = QMAX(vtQws - 1, 1);
    else if (ctrl && alt && keyCode == Qt::Key_Right)
	term = QMIN(vtQws + 1, 10);
    if (term && !release) {
	ctrl = 0;
	alt = 0;
	ioctl(kbdFD, VT_ACTIVATE, term);
	return;
    }

    // Ctrl-Alt-Backspace exits qws
    if (ctrl && alt && keyCode == Qt::Key_Backspace) {
	qApp->quit();
    }

    if (keyCode == Qt::Key_Alt) {
	alt = release ? 0 : AltButton;
    }
    else if (keyCode == Qt::Key_Control) {
	ctrl = release ? 0 : ControlButton;
    }
    else if (keyCode == Qt::Key_Shift) {
	shift = release ? 0 : ShiftButton;
    }
    if (keyCode != Qt::Key_unknown) {
	int unicode = 0;
	if (code < 90) {
	    if (!extended) {
		if (shift)
		    unicode =  QWSServer::keyMap()[code].shift_unicode ?  QWSServer::keyMap()[code].shift_unicode : 0xffff;
		else if (ctrl)
		    unicode =  QWSServer::keyMap()[code].ctrl_unicode ?  QWSServer::keyMap()[code].ctrl_unicode : 0xffff;
		else
		    unicode =  QWSServer::keyMap()[code].unicode ?  QWSServer::keyMap()[code].unicode : 0xffff;
		//printf("unicode: %c\r\n", unicode);
	    } else {
		if ( code == 53 )
		    unicode = '/';
	    }
	}

	modifiers = alt | ctrl | shift | keypad;

	// looks wrong -- WWA
	bool repeat = FALSE;
	if (prevuni == unicode && prevkey == keyCode && !release)
	    repeat = TRUE;

	processKeyEvent( unicode, keyCode, modifiers, !release, repeat );

	if (!release) {
	    prevuni = unicode;
	    prevkey = keyCode;
	} else {
	    prevkey = prevuni = 0;
	}
    }
    extended = false;
}


//
// Tty keyboard
//

QWSTtyKeyboardHandler::QWSTtyKeyboardHandler()
{
    kbdFD=open("/dev/tty0", O_RDWR | O_NDELAY, 0);

    if ( kbdFD >= 0 ) {
	QSocketNotifier *notifier;
	notifier = new QSocketNotifier( kbdFD, QSocketNotifier::Read, this );
	connect( notifier, SIGNAL(activated(int)),this,
		 SLOT(readKeyboardData()) );
    }

    // save for restore.
    tcgetattr( kbdFD, &origTermData );

    struct termios termdata;
    tcgetattr( kbdFD, &termdata );

    ioctl(kbdFD, KDSKBMODE, K_RAW);

    termdata.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
    termdata.c_oflag = 0;
    termdata.c_cflag = CREAD | CS8;
    termdata.c_lflag = 0;
    termdata.c_cc[VTIME]=0;
    termdata.c_cc[VMIN]=1;
    cfsetispeed(&termdata, 9600);
    cfsetospeed(&termdata, 9600);
    tcsetattr(kbdFD, TCSANOW, &termdata);

    signal(VTSWITCHSIG, vtSwitchHandler);

    struct vt_mode vtMode;
    ioctl(kbdFD, VT_GETMODE, &vtMode);

    // let us control VT switching
    vtMode.mode = VT_PROCESS;
    vtMode.relsig = VTSWITCHSIG;
    vtMode.acqsig = VTSWITCHSIG;
    ioctl(kbdFD, VT_SETMODE, &vtMode);

    struct vt_stat vtStat;
    ioctl(kbdFD, VT_GETSTATE, &vtStat);
    vtQws = vtStat.v_active;
}

QWSTtyKeyboardHandler::~QWSTtyKeyboardHandler()
{
    if (kbdFD >= 0)
    {
	ioctl(kbdFD, KDSKBMODE, K_XLATE);
	tcsetattr(kbdFD, TCSANOW, &origTermData);
	::close(kbdFD);
	kbdFD = -1;
    }
}

void QWSTtyKeyboardHandler::readKeyboardData()
{
    unsigned char buf[81];
    int n = read(kbdFD, buf, 80 );
    for ( int loop = 0; loop < n; loop++ )
	doKey(buf[loop]);
}


/* USB driver */

QWSUsbKeyboardHandler::QWSUsbKeyboardHandler()
{
    fd = open(getenv("QWS_USB_KEYBOARD"),O_RDONLY, 0);
    if ( fd >= 0 ) {
	QSocketNotifier *notifier;
	notifier = new QSocketNotifier( fd, QSocketNotifier::Read, this );
	connect( notifier, SIGNAL(activated(int)),this,
		 SLOT(readKeyboardData()) );
    }
}

QWSUsbKeyboardHandler::~QWSUsbKeyboardHandler()
{
    close(fd);
}


void QWSUsbKeyboardHandler::readKeyboardData()
{
    unsigned char buf[16];
    int n = read(kbdFD, buf, 16 );
    if ( n != 16 )
	return;
    doKey(buf[10]);
}

/*
 * vr41xx buttons driver
 */

QWSVr41xxButtonsHandler::QWSVr41xxButtonsHandler() : QWSKeyboardHandler()
{
    terminalName = "/dev/buttons";
    buttonFD = -1;
    notifier = 0;

    if ((buttonFD = open(terminalName, O_RDWR | O_NDELAY, 0)) < 0)
    {
	qWarning("Cannot open %s\n", terminalName.latin1());
    }

    if ( buttonFD >= 0 ) {
	notifier = new QSocketNotifier( buttonFD, QSocketNotifier::Read, this );
	connect( notifier, SIGNAL(activated(int)),this,
		 SLOT(readKeyboardData()) );
    }

    kbdBufferLen = 80;
    kbdBuffer = new unsigned char [kbdBufferLen];
    kbdIdx = 0;
}

QWSVr41xxButtonsHandler::~QWSVr41xxButtonsHandler()
{
    if ( buttonFD > 0 ) {
	::close( buttonFD );
	buttonFD = -1;
    }
    delete notifier;
    notifier = 0;
    delete [] kbdBuffer;
}

void QWSVr41xxButtonsHandler::readKeyboardData()
{
    int n = 0;
    do {
	n  = read(buttonFD, kbdBuffer+kbdIdx, kbdBufferLen - kbdIdx );
	if ( n > 0 )
	    kbdIdx += n;
    } while ( n > 0 );

    int idx = 0;
    while ( kbdIdx - idx >= 2 ) {
	unsigned char *next = kbdBuffer + idx;
	unsigned short *code = (unsigned short *)next;
	int keycode = Qt::Key_unknown;
	switch ( (*code) & 0x0fff ) {
	    case 0x7:
		keycode = Qt::Key_Up;
		break;
	    case 0x9:
		keycode = Qt::Key_Right;
		break;
	    case 0x8:
		keycode = Qt::Key_Down;
		break;
	    case 0xa:
		keycode = Qt::Key_Left;
		break;
	    case 0x3:
		keycode = Qt::Key_Up;
		break;
	    case 0x4:
		keycode = Qt::Key_Down;
		break;
	    case 0x1:
		keycode = Qt::Key_Backspace;
		break;
	    case 0x2:
		keycode = Qt::Key_Escape;
		break;
	    default:
		qDebug("Unrecognised key sequence %d", (int)code );
	}
//	if ( (*code) & 0x8000 )
	    processKeyEvent( 0, keycode, 0, TRUE, FALSE );
//	else
	    processKeyEvent( 0, keycode, 0, FALSE, FALSE );
/*
	unsigned short t = *code;
	for ( int i = 0; i < 16; i++ ) {
	    keycode = (t & 0x8000) ? Qt::Key_1 : Qt::Key_0;
	    int unicode = (t & 0x8000) ? '1' : '0';
	    processKeyEvent( unicode, keycode, 0, TRUE, FALSE );
	    processKeyEvent( unicode, keycode, 0, FALSE, FALSE );
	    t <<= 1;
	}
	keycode = Qt::Key_Space;
	processKeyEvent( ' ', keycode, 0, TRUE, FALSE );
	processKeyEvent( ' ', keycode, 0, FALSE, FALSE );
*/
	idx += 2;
    }

    int surplus = kbdIdx - idx;
    for ( int i = 0; i < surplus; i++ )
	kbdBuffer[i] = kbdBuffer[idx+i];
    kbdIdx = surplus;
}


/*
 * Virtual framebuffer keyboard driver
 */

#ifndef QT_NO_QWS_VFB
#include "qvfbhdr.h"
extern int qws_display_id;
#endif

QWSVFbKeyboardHandler::QWSVFbKeyboardHandler()
{
    kbdFD = -1;
#ifndef QT_NO_QWS_VFB
    kbdIdx = 0;
    kbdBufferLen = sizeof( QVFbKeyData ) * 5;
    kbdBuffer = new unsigned char [kbdBufferLen];

    terminalName = QString(QT_VFB_KEYBOARD_PIPE).arg(qws_display_id);

    if ((kbdFD = open( terminalName.local8Bit(), O_RDWR | O_NDELAY)) < 0) {
	qDebug( "Cannot open %s (%s)", terminalName.latin1(),
	strerror(errno));
    } else {
	// Clear pending input
	char buf[2];
	while (read(kbdFD, buf, 1) > 0) { }

	notifier = new QSocketNotifier( kbdFD, QSocketNotifier::Read, this );
	connect(notifier, SIGNAL(activated(int)),this, SLOT(readKeyboardData()));
    }
#endif
}

QWSVFbKeyboardHandler::~QWSVFbKeyboardHandler()
{
#ifndef QT_NO_QWS_VFB
    if ( kbdFD >= 0 )
	close( kbdFD );
    delete [] kbdBuffer;
#endif
}


void QWSVFbKeyboardHandler::readKeyboardData()
{
#ifndef QT_NO_QWS_VFB
    int n;
    do {
	n  = read(kbdFD, kbdBuffer+kbdIdx, kbdBufferLen - kbdIdx );
	if ( n > 0 )
	    kbdIdx += n;
    } while ( n > 0 );

    int idx = 0;
    while ( kbdIdx - idx >= (int)sizeof( QVFbKeyData ) ) {
	QVFbKeyData *kd = (QVFbKeyData *)(kbdBuffer + idx);
	if ( kd->unicode == 0 && kd->modifiers == 0 && kd->press ) {
	    // magic exit key
	    qWarning( "Instructed to quit by Virtual Keyboard" );
	    qApp->quit();
	}
	processKeyEvent( kd->unicode&0xffff, kd->unicode>>16,
				 kd->modifiers, kd->press, kd->repeat );
	idx += sizeof( QVFbKeyData );
    }

    int surplus = kbdIdx - idx;
    for ( int i = 0; i < surplus; i++ )
	kbdBuffer[i] = kbdBuffer[idx+i];
    kbdIdx = surplus;
#endif
}


/*
 * keyboard driver instantiation
 */

QWSKeyboardHandler *QWSServer::newKeyboardHandler( const QString &spec )
{
    server = this;

    QWSKeyboardHandler *handler = 0;
    
    if ( spec == "Buttons" ) {
	handler = new QWSVr41xxButtonsHandler();
    } else if ( spec == "QVFbKeyboard" ) {
	handler = new QWSVFbKeyboardHandler();
    } else if ( spec == "TTY" ) {
	if(getenv("QWS_USB_KEYBOARD")) {
	    handler = new QWSUsbKeyboardHandler();
	} else {
	    handler = new QWSTtyKeyboardHandler();
	}
    } else {
	qWarning( "Keyboard type %s unsupported", spec.latin1() );
    }

    return handler;
}

#include "qkeyboard_qws.moc"

#endif //QT_NO_QWS_KEYBOARD


static const QWSServer::KeyMap keyM[] = {
    {	Qt::Key_unknown,	0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_Escape,		27      , 27      , 0xffff  },
    {	Qt::Key_1,		'1'     , '!'     , 0xffff  },
    {	Qt::Key_2,		'2'     , '@'     , 0xffff  },
    {	Qt::Key_3,		'3'     , '#'     , 0xffff  },
    {	Qt::Key_4,		'4'     , '$'     , 0xffff  },
    {	Qt::Key_5,		'5'     , '%'     , 0xffff  },
    {	Qt::Key_6,		'6'     , '^'     , 0xffff  },
    {	Qt::Key_7,		'7'     , '&'     , 0xffff  },
    {	Qt::Key_8,		'8'     , '*'     , 0xffff  },
    {	Qt::Key_9,		'9'     , '('     , 0xffff  },	// 10
    {	Qt::Key_0,		'0'     , ')'     , 0xffff  },
    {	Qt::Key_Minus,		'-'     , '_'     , 0xffff  },
    {	Qt::Key_Equal,		'='     , '+'     , 0xffff  },
    {	Qt::Key_Backspace,	8       , 8       , 0xffff  },
    {	Qt::Key_Tab,		9       , 9       , 0xffff  },
    {	Qt::Key_Q,		'q'     , 'Q'     , 'Q'-64  },
    {	Qt::Key_W,		'w'     , 'W'     , 'W'-64  },
    {	Qt::Key_E,		'e'     , 'E'     , 'E'-64  },
    {	Qt::Key_R,		'r'     , 'R'     , 'R'-64  },
    {	Qt::Key_T,		't'     , 'T'     , 'T'-64  },  // 20
    {	Qt::Key_Y,		'y'     , 'Y'     , 'Y'-64  },
    {	Qt::Key_U,		'u'     , 'U'     , 'U'-64  },
    {	Qt::Key_I,		'i'     , 'I'     , 'I'-64  },
    {	Qt::Key_O,		'o'     , 'O'     , 'O'-64  },
    {	Qt::Key_P,		'p'     , 'P'     , 'P'-64  },
    {	Qt::Key_BraceLeft,	'['     , '{'     , 0xffff  },
    {	Qt::Key_Escape,		']'     , '}'     , 0xffff  },
    {	Qt::Key_Return,		13      , 13      , 0xffff  },
    {	Qt::Key_Control,	0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_A,		'a'     , 'A'     , 'A'-64  },  // 30
    {	Qt::Key_S,		's'     , 'S'     , 'S'-64  },
    {	Qt::Key_D,		'd'     , 'D'     , 'D'-64  },
    {	Qt::Key_F,		'f'     , 'F'     , 'F'-64  },
    {	Qt::Key_G,		'g'     , 'G'     , 'G'-64  },
    {	Qt::Key_H,		'h'     , 'H'     , 'H'-64  },
    {	Qt::Key_J,		'j'     , 'J'     , 'J'-64  },
    {	Qt::Key_K,		'k'     , 'K'     , 'K'-64  },
    {	Qt::Key_L,		'l'     , 'L'     , 'L'-64  },
    {	Qt::Key_Semicolon,	';'     , ':'     , 0xffff  },
    {	Qt::Key_Apostrophe,	'\''    , '"'     , 0xffff  },  // 40
    {	Qt::Key_QuoteLeft,	'`'     , '~'     , 0xffff  },
    {	Qt::Key_Shift,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_Backslash,	'\\'    , '|'     , 0xffff  },
    {	Qt::Key_Z,		'z'     , 'Z'     , 'Z'-64  },
    {	Qt::Key_X,		'x'     , 'X'     , 'X'-64  },
    {	Qt::Key_C,		'c'     , 'C'     , 'C'-64  },
    {	Qt::Key_V,		'v'     , 'V'     , 'V'-64  },
    {	Qt::Key_B,		'b'     , 'B'     , 'B'-64  },
    {	Qt::Key_N,		'n'     , 'N'     , 'N'-64  },
    {	Qt::Key_M,		'm'     , 'M'     , 'M'-64  },  // 50
    {	Qt::Key_Comma,		','     , '<'     , 0xffff  },
    {	Qt::Key_Period,		'.'     , '>'     , 0xffff  },
    {	Qt::Key_Slash,		'/'     , '?'     , 0xffff  },
    {	Qt::Key_Shift,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_Asterisk,	'*'     , '*'     , 0xffff  },
    {	Qt::Key_Alt,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_Space,		' '     , ' '     , 0xffff  },
    {	Qt::Key_CapsLock,	0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F1,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F2,		0xffff  , 0xffff  , 0xffff  },  // 60
    {	Qt::Key_F3,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F4,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F5,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F6,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F7,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F8,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F9,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F10,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_NumLock,	0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_ScrollLock,	0xffff  , 0xffff  , 0xffff  },  // 70
    {	Qt::Key_7,		'7'     , '7'     , 0xffff  },
    {	Qt::Key_8,		'8'     , '8'     , 0xffff  },
    {	Qt::Key_9,		'9'     , '9'     , 0xffff  },
    {	Qt::Key_Minus,		'-'     , '-'     , 0xffff  },
    {	Qt::Key_4,		'4'     , '4'     , 0xffff  },
    {	Qt::Key_5,		'5'     , '5'     , 0xffff  },
    {	Qt::Key_6,		'6'     , '6'     , 0xffff  },
    {	Qt::Key_Plus,		'+'     , '+'     , 0xffff  },
    {	Qt::Key_1,		'1'     , '1'     , 0xffff  },
    {	Qt::Key_2,		'2'     , '2'     , 0xffff  },  // 80
    {	Qt::Key_3,		'3'     , '3'     , 0xffff  },
    {	Qt::Key_0,		'0'     , '0'     , 0xffff  },
    {	Qt::Key_Period,		'.'     , '.'     , 0xffff  },
    {	Qt::Key_unknown,	0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_unknown,	0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_unknown,	0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F11,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_F12,		0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_unknown,	0xffff  , 0xffff  , 0xffff  },
    {	Qt::Key_unknown,	0xffff  , 0xffff  , 0xffff  },	// 90
    {	0,			0xffff  , 0xffff  , 0xffff  }
};


const QWSServer::KeyMap *QWSServer::keyMap()
{
    return keyM;
}
