#include <qstyleinterface.h>
#include <qcdestyle.h>
#include <qcleanuphandler.h>

class CDEStyle : public QStyleInterface, public QLibraryInterface
{
public:
    CDEStyle();

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

CDEStyle::CDEStyle()
: ref( 0 )
{
}

QUnknownInterface *CDEStyle::queryInterface( const QUuid &uuid )
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

unsigned long CDEStyle::addRef()
{
    return ref++;
}

unsigned long CDEStyle::release()
{
    if ( !--ref ) {
	delete this;
	return 0;
    }

    return ref;
}

QStringList CDEStyle::featureList() const
{
    QStringList list;
    list << "CDE";
    return list;
}

QStyle* CDEStyle::create( const QString& s )
{
    if ( s.lower() == "cde" ) {
	QStyle *style = new QCDEStyle();
	styles.add( style );
	return style;
    }
    return 0;
}

bool CDEStyle::init()
{
    return TRUE;
}

void CDEStyle::cleanup() 
{
    styles.clear();;
}

bool CDEStyle::canUnload() const
{
    return styles.isEmpty();
}

Q_EXPORT_INTERFACE()
{
    Q_CREATE_INSTANCE( CDEStyle )
}
