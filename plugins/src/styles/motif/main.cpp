#include <qstyleplugin.h>
#include <qmotifstyle.h>

class MotifStyle : public QStylePlugin
{
public:
    MotifStyle();

    QStringList featureList() const;
    QStyle *create( const QString& );
};

MotifStyle::MotifStyle()
: QStylePlugin()
{
}

QStringList MotifStyle::featureList() const
{
    QStringList list;
    list << "Motif";
    return list;
}

QStyle* MotifStyle::create( const QString& s )
{
    if ( s.lower() == "motif" )
	return new QMotifStyle();

    return 0;
}

Q_EXPORT_PLUGIN( MotifStyle )

