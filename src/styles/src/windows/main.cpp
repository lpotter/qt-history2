#include <qstyleinterface.h>
#include <qwindowsstyle.h>
#include <qcleanuphandler.h>

class WindowsStyle : public QStyleInterface, public QLibraryInterface
{
public:
    WindowsStyle();

    QUnknownInterface *queryInterface( const QUuid& );
    unsigned long addRef();
    unsigned long release();

    QStringList featureList() const;
    QStyle *create( const QString& );

    bool init();
    void cleanup();
    bool canUnload() const;

private:
    QGuardedCleanupHandler<QStyle> styles;

    unsigned long ref;
};

WindowsStyle::WindowsStyle()
: ref( 0 )
{
}

QUnknownInterface *WindowsStyle::queryInterface( const QUuid &uuid )
{
    QUnknownInterface *iface = 0;
    if ( uuid == IID_QUnknownInterface )
	iface = (QUnknownInterface*)(QStyleInterface*)this;
    else if ( uuid == IID_QFeatureListInterface )
	iface = (QFeatureListInterface*)this;
    else if ( uuid == IID_QStyleInterface )
	iface = (QStyleInterface*)this;
    else if ( uuid == IID_QLibraryInterface )
	iface = (QLibraryInterface*)this;

    if ( iface )
	iface->addRef();
    return iface;
}

unsigned long WindowsStyle::addRef()
{
    return ref++;
}

unsigned long WindowsStyle::release()
{
    if ( !--ref ) {
	delete this;
	return 0;
    }

    return ref;
}

QStringList WindowsStyle::featureList() const
{
    QStringList list;
    list << "Windows";
    return list;
}

QStyle* WindowsStyle::create( const QString& s )
{
    if ( s.lower() == "windows" ) {
	QStyle *style = new QWindowsStyle();
	styles.add( style );
	return style;
    }
    return 0;
}

bool WindowsStyle::init()
{
    return TRUE;
}

void WindowsStyle::cleanup()
{
    styles.clear();
}

bool WindowsStyle::canUnload() const
{
    return styles.isEmpty();
}

Q_EXPORT_INTERFACE()
{
    Q_CREATE_INSTANCE( WindowsStyle )
}
