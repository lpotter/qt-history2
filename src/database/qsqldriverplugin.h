#ifndef QSQLDRIVERPLUGIN_H
#define QSQLDRIVERPLUGIN_H

#include <qplugin.h>
#include <qpluginmanager.h>
#include "qsqldriverinterface.h"

class QSqlDriverPlugIn : public QSqlDriverInterface, public QPlugIn
{
public:
    QSqlDriverPlugIn( const QString& filename, LibraryPolicy = Default, const char* fn=0 );
    QCString queryPlugInInterface() const { return "QSqlDriverInterface"; }
    QStringList featureList();

    QSqlDriver* create( const QString& name );
};

class QSqlDriverPlugInManager : public QPlugInManager< QSqlDriverPlugIn >
{
public:
    QSqlDriverPlugInManager( const QString& path = QString::null, const QString& filter = "*.dll; *.so",
			     QPlugIn::LibraryPolicy pol = QPlugIn::Default, const char* fn = 0 );
    ~QSqlDriverPlugInManager()
    {
	qDebug("~QSqlDriverPlugInManager");
    }
};

#endif

