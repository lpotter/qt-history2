#include "qws.h"
#include "qwsevent.h"
#include "qwscommand.h"
#include "qwsutils.h"

#include <qapplication.h>

#include <stdlib.h>
#include <stdio.h>


#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

//keyboard
#include <termios.h>
#include <sys/kd.h>

static const char *mouseDev = "/dev/mouse";
static const int mouseBufSize = 100;

    static const int screen_width = 1024; //#####
    static const int screen_height = 768; //#####


/*
  
  int xf86KbdOn()
{
	struct termios nTty;

#if USE_MEDIUMRAW_KBD
	ioctl(xf86Info.consoleFd, KDSKBMODE, K_MEDIUMRAW);
#else
	ioctl(xf86Info.consoleFd, KDSKBMODE, K_RAW);
#endif
	nTty = kbdtty;
	nTty.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
	nTty.c_oflag = 0;
	nTty.c_cflag = CREAD | CS8;
	nTty.c_lflag = 0;
	nTty.c_cc[VTIME]=0;
	nTty.c_cc[VMIN]=1;
	cfsetispeed(&nTty, 9600);
	cfsetospeed(&nTty, 9600);
	tcsetattr(xf86Info.consoleFd, TCSANOW, &nTty);
	return(xf86Info.consoleFd);
}
*/


//#define EXPERIMENTAL_LINUX_KBD



void QWSServer::initIO()
{
    //mouse:
    if ((mouseFD = open( mouseDev, O_RDWR | O_NDELAY)) < 0) {
	printf( "Cannot open %s (%s)\n", (const char*)mouseDev,
		strerror(errno));
	exit(1);
    }
    mouseBuf = new uchar[mouseBufSize];
    mouseIdx = 0;
    mousePos = QPoint(500,300);
    QSocketNotifier *sn = new QSocketNotifier( mouseFD,
					       QSocketNotifier::Read,
					       this );
    connect( sn, SIGNAL(activated(int)),this, SLOT(readMouseData()) );

#ifdef EXPERIMENTAL_LINUX_KBD
    //keyboard

    kbdFD = 0;
    struct termios termdata;
    tcgetattr( kbdFD, &termdata );

#if 1
    termdata.c_lflag &= ~ECHO;
    termdata.c_lflag &= ~ICANON;
    termdata.c_iflag &= ~ICRNL;
    termdata.c_lflag |= ISIG;
    termdata.c_cc[VMIN] = 1;
    termdata.c_cc[VTIME] = 0;         

    tcsetattr( kbdFD, TCSANOW, &termdata );    
#else
    // the X way, seriously screws up keyboard handling

#if USE_MEDIUMRAW_KBD
    ioctl(kbdFD, KDSKBMODE, K_MEDIUMRAW);
#else
    ioctl(kbdFD, KDSKBMODE, K_RAW);
#endif

    termdata.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
    termdata.c_oflag = 0;
    termdata.c_cflag = CREAD | CS8;
    termdata.c_lflag = 0;
    termdata.c_cc[VTIME]=0;
    termdata.c_cc[VMIN]=1;
    cfsetispeed(&termdata, 9600);
    cfsetospeed(&termdata, 9600);
    tcsetattr(kbdFD, TCSANOW, &termdata);
#endif


    QSocketNotifier *ksn = new QSocketNotifier( kbdFD,
						QSocketNotifier::Read,
						this );
    connect( ksn, SIGNAL(activated(int)),this, SLOT(readKeyboardData()) );

#endif
}


void QWSServer::readKeyboardData()
{
    unsigned char buf[81];
    int n;
    n = read(kbdFD, buf, 80 );
    if ( n > 0 ) {
#if 0 //debug	
	int i;
	for ( i = 0; i < n; i++ ) {
	    unsigned char c = buf[i];
	    if ( c >= ' ' )
		printf( "%d[%c] ", c, c );
	    else
		printf( "%d ", c );
	}
	printf( "\n" );
#endif
	sendKeyEvent( buf[0], 0, TRUE, FALSE );  //###
	sendKeyEvent( buf[0], 0, FALSE, FALSE ); //###
    }
}

/*
  mouseIdx is the number of bytes in the buffer (aka the first free
  position). handleMouseData() moves any data it doesn't use to
  the beginning of the buffer, and updates mouseIdx.
 */

void QWSServer::readMouseData()
{
    int n;
    do {
	n = read(mouseFD, mouseBuf+mouseIdx, mouseBufSize-mouseIdx );
	if ( n > 0 ) {
	    mouseIdx += n;
	    handleMouseData();
	}
    } while ( n > 0 );
}



/*
  This implements the Logitech MouseMan(Plus) protocol,
  wheel not yet supported.
*/

void QWSServer::handleMouseData()
{
    static const int accel_limit = 5;
    static const int accel = 2;


    //    printf( "handleMouseData mouseIdx=%d\n", mouseIdx );


    int idx = 0;

    while ( mouseIdx-idx >= 3 ) {
	int bstate = 0;
	uchar *mb = mouseBuf+idx;

	if (mb[0] & 0x01)
	    bstate |= Qt::LeftButton;
	if (mb[0] & 0x02)
	    bstate |= Qt::RightButton;
	if (mb[0] & 0x04)
	    bstate |= Qt::MidButton;

	int overflow = (mb[0]>>6 )& 0x03;
	//### wheel events signalled with overflow bit, ignore for now
	int dx,dy;
	if ( !overflow ) {
	    bool xs = mb[0] & 0x10;
	    bool ys = mb[0] & 0x20;

	    dx = xs ? mb[1]-256 : mb[1];
	    dy = ys ? mb[2]-256 : mb[2];
	    if ( QABS(dx) > accel_limit || QABS(dy) > accel_limit ) {
		dx *= accel;
		dy *= accel;
	    }
	    int mx = mousePos.x() + dx;
	    int my = mousePos.y() - dy; // swap coordinate system

	    mousePos.setX( QMIN( QMAX( mx, 0 ), screen_width ) );
	    mousePos.setY( QMIN( QMAX( my, 0 ), screen_height ) );

	    sendMouseEvent( mousePos, bstate );
	}
	idx += 3;


#if 0 //debug
	const char *b1 = (mb[0] & 0x01) ? "b1":"  ";//left
	const char *b2 = (mb[0] & 0x02) ? "b2":"  ";//right
	const char *b3 = (mb[0] & 0x04) ? "b3":"  ";//mid


	printf( "(%2d) %02x %02x %02x ", idx, mb[0],mb[1],mb[2] );


	if ( overflow )
	    printf( "Overflow%d %s %s %s  (%4d,%4d)\n", overflow,
		    b1, b2, b3, mouseX, mouseY );
	else
	    printf( "%s %s %s (%+3d,%+3d)  (%4d,%4d)\n",
		    b1, b2, b3, dx, dy, mouseX, mouseY );
#endif
    }

    int surplus = mouseIdx - idx;
    for ( int i = 0; i < surplus; i++ )
	mouseBuf[i] = mouseBuf[idx+i];
    mouseIdx = surplus;


    //printf( "exit handleMouseData mouseIdx=%d\n", mouseIdx );


}


#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>


static struct fb_var_screeninfo fb_vinfo;
static struct fb_fix_screeninfo fb_finfo;
static struct {
  int fbfd;
  long int screensize;
  uchar *fbp;
} qfb;

static bool fb_open = FALSE;

static void open_fb()
{
  fb_open = TRUE;
  qfb.fbp=0;
 /* Open the file for reading and writing */
  qfb.fbfd = open("/dev/fb0", O_RDWR);
  if (!qfb.fbfd) {
    printf("Error: cannot open framebuffer device.\n");
    exit(1);
  }
  printf("The framebuffer device was opened successfully.\n");

  /* Get fixed screen information */
  if (ioctl(qfb.fbfd, FBIOGET_FSCREENINFO, &fb_finfo)) {
    printf("Error reading fixed information.\n");
    exit(2);
  }

  /* Get variable screen information */
  if (ioctl(qfb.fbfd, FBIOGET_VSCREENINFO, &fb_vinfo)) {
    printf("Error reading variable information.\n");
    exit(3);
  }

  /* Figure out the size of the screen in bytes */
  qfb.screensize = fb_vinfo.xres * fb_vinfo.yres * fb_vinfo.bits_per_pixel / 8;

  /* Map the device to memory */
  qfb.fbp = (uchar *)mmap(0, qfb.screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
		     qfb.fbfd, 0);
  if ((int)qfb.fbp == -1) {
    printf("Error: failed to map framebuffer device to
memory.\n");
    exit(4);
  }
  printf("The framebuffer device was mapped to memory successfully.\n");

}


static void close_fb()
{
  munmap(qfb.fbp, qfb.screensize);
  close(qfb.fbfd);
}


void QWSServer::paintServerRegion()
{

    if ( shmid == -1 ) {
    
	if ( !fb_open )
	    open_fb();

#if 1
	//16bpp
	ushort *fbp = (ushort*)qfb.fbp;
	ushort col = 0x0200;
#else	
	//32bpp
	uint *fbp = (uint*)qfb.fbp;
	uint col = 0x003000;
#endif
	//### testcode - should paint properly

	
	QRegion sr = serverRegion.intersect( QRegion(0,0,fb_vinfo.xres,
						     fb_vinfo.yres ));
	
	QArray<QRect> reg = sr.rects();
	
	for ( int i = 0;  i < int(reg.count());  i++ ) {
	    QRect r = reg[i];
	    for ( int y = r.top(); y <= r.bottom() ; y++ )
	      for ( int x = r.left(); x <= r.right() ; x++ ) {
		int l = (x+fb_vinfo.xoffset) * (fb_vinfo.bits_per_pixel/8) +
			(y+fb_vinfo.yoffset) * fb_finfo.line_length;
#if 1
		//16bpp
		*(fbp+l/sizeof(ushort)) = col;
#else
		//32bpp
		*(fbp+l/sizeof(uint)) = col;
		col +=0x000200;
#endif
	      }
	}
    }
}


