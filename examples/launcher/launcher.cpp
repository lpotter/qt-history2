#include <qapplication.h>
#include <qvbox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qimage.h>
#include <qtimer.h>
#include <qlistbox.h>
#include <qgroupbox.h>
#include <qwindowsystem_qws.h>

#include <stdlib.h>
#include <unistd.h>

struct {
    const char* label;
    const char* file;
} command[] = {
    { "Help Text Browser", "(cd ../helpviewer; exec ../helpviewer/helpviewer)" },
    { "Canvas - alpha-blending", "( cd ../canvas; exec ./canvas )" },
    { "Text Editor", "exec ../qwerty/qwerty ../qwerty/unicode.txt" },
    { "Scribble Editor", "exec ../scribble/scribble" },
    { "Internationalization", "( cd ../i18n; exec ./i18n all )" },
    { "Magnifier", "exec ../qmag/qmag" },
    { 0, 0 }
};

struct {
    const char* label;
    const char* file;
} other_command[] = {
    { "aclock", "( cd ../aclock; exec ./aclock; )" },
    { "addressbook", "( cd ../addressbook; exec ./addressbook; )" },
    { "buttongroups", "( cd ../buttongroups; exec ./buttongroups; )" },
    { "checklists", "( cd ../checklists; exec ./checklists; )" },
    { "cursor", "( cd ../cursor; exec ./cursor; )" },
    { "customlayout", "( cd ../customlayout; exec ./customlayout; )" },
    { "dclock", "( cd ../dclock; exec ./dclock; )" },
    { "dirview", "( cd ../dirview; exec ./dirview; )" },
    //{ "drawdemo", "( cd ../drawdemo; exec ./drawdemo; )" },
    { "drawlines", "( cd ../drawlines; exec ./drawlines; )" },
    //{ "forever", "( cd ../forever; exec ./forever; )" },
    { "hello", "( cd ../hello; exec ./hello; )" },
    { "layout", "( cd ../layout; exec ./layout; )" },
    { "life", "( cd ../life; exec ./life; )" },
    { "lineedits", "( cd ../lineedits; exec ./lineedits; )" },
    { "listbox", "( cd ../listbox; exec ./listbox; )" },
    { "listboxcombo", "( cd ../listboxcombo; exec ./listboxcombo; )" },
    { "mdi", "( cd ../mdi; exec ./mdi; )" },
    { "menu", "( cd ../menu; exec ./menu; )" },
    { "movies", "( cd ../movies; exec ./movies; )" },
    //{ "picture", "( cd ../picture; exec ./picture; )" },
    { "popup", "( cd ../popup; exec ./popup; )" },
    { "progress", "( cd ../progress; exec ./progress; )" },
    { "progressbar", "( cd ../progressbar; exec ./progressbar; )" },
    { "qfd", "( cd ../qfd; exec ./qfd; )" },
    { "rangecontrols", "( cd ../rangecontrols; exec ./rangecontrols; )" },
    { "richtext", "( cd ../richtext; exec ./richtext; )" },
    { "scrollview", "( cd ../scrollview; exec ./scrollview; )" },
    { "showimg", "( cd ../showimg; exec ./showimg; )" },
    //{ "sound", "( cd ../sound; exec ./sounds; )" },
    { "splitter", "( cd ../splitter; exec ./splitter; )" },
    { "tabdialog", "( cd ../tabdialog; exec ./tabdialog; )" },
    { "table", "( cd ../table; exec ./table; )" },
    { "tetrix", "( cd ../tetrix; exec ./tetrix; )" },
    { "tictac", "( cd ../tictac; exec ./tictac; )" },
    { "tooltip", "( cd ../tooltip; exec ./tooltip; )" },
    { "validator", "( cd ../validator; exec ./validator; )" },
    { "widgets", "( cd ../widgets; exec ./widgets; )" },
    { "wizard", "( cd ../wizard; exec ./wizard; )" },
    //{ "xform", "( cd ../xform; exec ./xform; )" },
    { 0, 0 }
};


class Launcher : public QHBox {
    Q_OBJECT
public:
    Launcher() :
	QHBox(0,0,WStyle_Tool | WStyle_Customize)
    {
	setSpacing(10);
	setMargin(10);
	QMimeSourceFactory::defaultFactory()
	    ->setImage("qtlogo",QImage("qtlogo.png"));
	QMimeSourceFactory::defaultFactory()
	    ->setImage("face",QImage("face.png"));
	QVBox* vb;
        vb = new QVBox(this);
	setStretchFactor(vb,1);
	QLabel* label = new QLabel(
	    "<center><h1><img src=qtlogo><img src=face><br>"
	    "Qt/Embedded</h1>"
	    "</center>"
"This display is a simple Qt/Embedded launcher application, running directly
on the Linux console. The buttons and listbox
to the right invoke additional applications.<p><br><hr>"
		, vb);
	info = new QLabel(vb);
	info->setFont(QFont("smoothtimes",17));
	info->setBackgroundColor(white);
	info->setAlignment(AlignTop);
	nextInfo();
	QTimer* infotimer = new QTimer(this);
	connect(infotimer,SIGNAL(timeout()),this,SLOT(nextInfo()));
	infotimer->start(20000);
	setBackgroundColor(white);
	label->setBackgroundColor(white);

	vb = new QVBox(this);
	vb->setBackgroundColor(white);
	QPushButton *pb=0;
	int i;
	for (i=0; command[i].label; i++) {
	    pb = new QPushButton(command[i].label,vb,command[i].file);
	    connect(pb, SIGNAL(clicked()), this, SLOT(execute()));
	}
	QListBox *lb = new QListBox(vb);
	lb->setFont(QFont("smoothtimes",17));
	for (i=0; other_command[i].label; i++) {
	    lb->insertItem(other_command[i].label);
	}
	lb->setMaximumHeight(pb->height()*8);
	connect(lb, SIGNAL(highlighted(int)), this, SLOT(executeOther(int)));
	connect(lb, SIGNAL(selected(int)), this, SLOT(executeOther(int)));
	
	QHBox* hb = new QHBox(vb);
	hb->setBackgroundColor(white);
	pb = new QPushButton("Restart",hb);
	connect(pb, SIGNAL(clicked()), this, SLOT(quit3()));
	pb = new QPushButton("Quit",hb);
	connect(pb, SIGNAL(clicked()), qApp, SLOT(quit()));
	hb->setSpacing(10);
	hb->setFixedHeight(hb->sizeHint().height());
    }

private slots:
    void nextInfo()
    {
	static int i = 0;
	static const char* infotext[] = {
	"<h2>What is it?</h2>

	The Qt/Embedded product provides you with all you need to
	create stunning graphical user interfaces for embedded devices.
	Qt/Embedded installs and runs with a very small memory footprint on any
	device running embedded Linux - without using X11.  ",

	"<h2>Trust the promises</h2>

	Qt/Embedded features the same API as the excellent
	Qt/Windows and Qt/X11 versions. Imagine writing your Qt application in
	your favourite desktop environment and just recompiling to move it to
	your embedded device. It saves you heaps of development effort and
	allows you to work productively in your favorite programming
	environment from day one. You can start today.  ",

	"<h2>Less is more</h2>

	Qt/Embedded is modular and scalable. You can assemble the
	Qt features you really need and leave the others out. Since Qt/Embedded
	is not based on X11 it has substantially lower memory requirements than
	X11. By picking and choosing features, the memory demands of
	Qt/Embedded can be tuned from 1 Mb to 3 Mb in ROM (Intel x86).
	Furthermore,applications written with Qt are known to have a small
	memory footprint compared to applications written with other toolkits.
	Qt scales from the smallest embedded device to high-end workstations.  ",

	"<h2>The source will be with you</h2>

	You know the value of source code availability. All Qt
	releases are delivered with source code. Qt/Embedded is no exception.
	You get a better understanding of how Qt works and it helps you debug
	and tune your code. Customer feedback allows us to continually improve
	the source code for better usability and performance.  ",

	"<h2>Add your touch</h2>

	Qt's clean object oriented design makes it easy to extend and
	enhance the standard widgets. You can create specialized widgets for the
	limited space offered by embedded screen devices. You can benefit from the
	wealth of available third-party Qt software. It is easy to add the killer
	feature you need.",

	"<h2>The beauty of it</h2>

	Qt/Embedded features some great additional functionality compared
	to X11. You will be stunned by the beauty of anti-aliased text and alpha
	blended pixmaps. These new features could add an additional touch of class to a
	user interface. Forget about embedded graphics which looks more like old
	alphanumeric terminals. Qt/Embedded can utilize hardware graphics acceleration
	and it is well suited for multimedia and web applications. Let Qt/Embedded
	impress you and your customers.",

	"<h2>Getting there faster</h2>

	Qt is one of the most popular GUI toolkits in the world.
	Programmers like the compact code,the powerful API,the ease of use, and the
	excellent support. It is easy to find existing developer skills with Qt and a
	lot of quality Qt based code has already been written. Qt/Embedded allows you
	and your programmers to move seamlessly into the exciting field of embedded
	systems. Your Qt experts don't need to be retrained. Our customers tell us that
	even programmers without Qt experience get up to speed sooner with Qt than with
	other toolkits.",

	"<h2>We support you</h2>

	Embedded devices have other requirements than conventional
	computers. The Qt/Embedded team at Trolltech is dedicated to support you with
	new features and widgets. We can offer training, partnerships and first class
	support. It is straightforward to port Qt/Embedded to new hardware. We can
	offer you a port to your special device or the embedded operating system of
	your choice. We look forward to working closely with you to help you succeed.",

	0
	};
	QString t=infotext[i];
	info->setText(
	    "<blockquote>"
	    +t+
	    "</blockquote>"
	);
	i++;
	if (!infotext[i]) i=0;
    }

    void quit3()
    {
	qApp->exit(3);
    }

    void run(const char* cmd)
    {
	QString c = cmd;
	// c += " 2>/dev/null >/dev/null";
	c += " &";
	system(c.latin1());
    }
    void execute()
    {
	run(sender()->name());
    }
    void executeOther(int i)
    {
	run(other_command[i].file);
    }

    void setAntiAliasedText(bool on)
    {
	if ( on ) {
	    unsetenv("QWS_NO_SMOOTH_FONTS");
	} else {
	    setenv("QWS_NO_SMOOTH_FONTS","1",1);
	}
    }
    void setAcceleration(bool on)
    {
	if ( on ) {
	    unsetenv("QWS_NOACCEL");
	} else {
	    setenv("QWS_NOACCEL","1",1);
	}
    }
private:
    QLabel* info;
};

#include "launcher.moc"



#ifdef _WS_QWS_

#include <qmap.h>
#include <qfile.h>
#include <qtextstream.h>

class SimpleIM : public QWSServer::KeyboardFilter
{
public:
    SimpleIM( const QString &fn );
    bool filter(int unicode, int keycode, int modifiers, bool isPress,
		      bool autoRepeat);

private:
    bool active;
    unsigned int maxlen;
    QMap<QString,QString> map;
    QString soFar;
};




SimpleIM::SimpleIM( const QString &fn )
{
    active = FALSE;
    maxlen = 0;
    QFile f( fn );
    if ( f.open(IO_ReadOnly) ) {
	
	QTextStream t( &f );        // use a text stream
	QString s;
	while ( !t.eof() ) {        // until end of file...
	    s = t.readLine();       // line of text excluding '\n'
	    int p = s.find( '\t' );
	    if ( p >= -1 ) {
		QString s1 = s.left( p );
		QString s2 = s.mid( p+1 );
		map[s1] = s2;
		maxlen = QMAX( maxlen, s1.length() );
	    }
	}
	f.close();
    }
    qDebug( "maxlen = %d", maxlen );
}



static void sendKey( int unicode, int keycode )
{
    QWSServer::sendKeyEvent( unicode, keycode, 0, TRUE, FALSE );
    QWSServer::sendKeyEvent( unicode, keycode, 0, FALSE, FALSE );
}

static void sendString( const QString &s )
{
    for ( int i = 0; i < int(s.length()); i++ )
	sendKey( s[i].unicode(), 0 );
}


/*
  Watch all keypresses. Toggle active mode when shift+space is seen.

  In active mode: Put each key pressed into the buffer soFar. When the
  contents of soFar match an entry in map, send backspace keypresses
  to remove the source characters and then send the mapped string.
  
  This is just a simple example. Production code would have to handle
  key releases as well. In some cases the backspace count will be wrong,
  if the substitution map contains multiple overlapping characters.
  (Eg. if "aa"->"A", "bb"->"B" and "aabb"->"XYZ".)
 */
bool SimpleIM::filter(int unicode, int keycode, int modifiers, bool isPress,
		      bool /*autoRepeat*/ )
{
    if ( modifiers == Qt::ShiftButton && keycode == Qt::Key_Space ) {
	if ( !isPress ) {
	    active = !active;
	}
	return TRUE; //filter
    }
    if ( active  && !(modifiers&~Qt::ShiftButton) ) {
	if ( isPress && keycode != Qt::Key_Shift ) {
	    soFar = soFar + QChar( unicode );
	    soFar = soFar.right( maxlen );
	
	    int n = soFar.length();
	    while ( n > 0 ) {
		QString candidate = soFar.right( n );
		if ( map.contains( candidate ) ) {
		    for ( int i = 0; i < n-1; i++ )
			sendKey( 0, Qt::Key_Backspace);
		    sendString( map[candidate] );
		    return TRUE; //filter
		}
		n--;
	    }
	}
	return FALSE; //don't filter
    }
    soFar = "";
    return FALSE; //don't filter
}

#endif

void silent(QtMsgType, const char *)
{
}


main(int argc, char** argv)
{
    QApplication app(argc,argv, QApplication::GuiServer);

    qInstallMsgHandler(silent);

    app.setFont(QFont("smoothtimes",22));
#ifdef _WS_QWS_
    if ( QString(argv[1]) == "-im" && argv[2] ) {
	SimpleIM* im = new SimpleIM( argv[2] );
	QWSServer::setKeyboardFilter( im );
    }
#endif

    Launcher l;
    app.setMainWidget(&l);
    l.showMaximized();
    return app.exec();
}
