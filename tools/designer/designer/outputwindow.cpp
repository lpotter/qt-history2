#include "outputwindow.h"
#include "designerappiface.h"

#include <qlistview.h>
#include <qtextedit.h>
#include <qapplication.h>
#include <qheader.h>

static QTextEdit *debugoutput = 0;
bool debugToStderr = FALSE;

OutputWindow::OutputWindow( QWidget *parent )
    : QTabWidget( parent, "output_window" ), debugView( 0 ), errorView( 0 )
{
    setupDebug();
    setupError();
    iface = new DesignerOutputDockImpl( this );
}

OutputWindow::~OutputWindow()
{
    debugoutput = debugView = 0;
    errorView = 0;
    qInstallMsgHandler( oldMsgHandler );
    delete iface;
}

void OutputWindow::setupError()
{
    errorView = new QListView( this, "OutputWindow::errorView" );

    addTab( errorView, tr( "Error Messages" ) );
    errorView->addColumn( tr( "Message" ) );
    errorView->addColumn( tr( "Line" ) );
    errorView->header()->setStretchEnabled( TRUE );
    errorView->setAllColumnsShowFocus( TRUE );
}

void debugMessageOutput( QtMsgType type, const char *msg )
{
    QString s( msg );

    if ( type != QtFatalMsg ) {
	if( debugToStderr )
	else if ( debugoutput )
	    debugoutput->append( s + "\n" );
    } else {
	abort();
    }

    qApp->flush();
}

void OutputWindow::setupDebug()
{
    debugoutput = debugView = new QTextEdit( this, "OutputWindow::debugView" );
    addTab( debugView, "Debug Output" );

    oldMsgHandler = qInstallMsgHandler( debugMessageOutput );
}

void OutputWindow::setErrorMessages( const QStringList &errors, const QValueList<int> &lines, bool clear )
{
    if ( clear )
	errorView->clear();
    QStringList::ConstIterator mit = errors.begin();
    QValueList<int>::ConstIterator lit = lines.begin();
    for ( ; lit != lines.end() && mit != errors.end(); ++lit, ++mit )
	(void)new QListViewItem( errorView, *mit, QString::number( *lit ) );
    setCurrentPage( 1 );
}

DesignerOutputDock *OutputWindow::iFace()
{
    return iface;
}

void OutputWindow::appendDebug( const QString &text )
{
    debugView->append( text + "\n" );
}

void OutputWindow::clearErrorMessages()
{
    errorView->clear();
}

void OutputWindow::clearDebug()
{
    debugView->clear();
}
