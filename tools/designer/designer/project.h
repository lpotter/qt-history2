/**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef PROJECT_H
#define PROJECT_H

#include <qfeatures.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qmap.h>
#include <qpluginmanager.h>
#include "../interfaces/projectsettingsiface.h"
#include "sourcefile.h"

class FormWindow;
class QObjectList;
struct DesignerProject;
struct DesignerDatabase;
class PixmapCollection;
class Project;

#ifndef QT_NO_SQL
class QSqlDatabase;

class DatabaseConnection
{
public:
    DatabaseConnection( Project *p ) :
#ifndef QT_NO_SQL
	conn( 0 ),
#endif
	project( p ), loaded( FALSE ), iface( 0 ) {}
    ~DatabaseConnection();

    bool refreshCatalog();
    bool open( bool suppressDialog = TRUE );
    void close();
    DesignerDatabase *iFace();

    bool isLoaded() const { return loaded; }
    void setName( const QString& n ) { nm = n; }
    QString name() const { return nm; }
    void setDriver( const QString& d ) { drv = d; }
    QString driver() const { return drv; }
    void setDatabase( const QString& db ) { dbName = db; }
    QString database() const { return dbName; }
    void setUsername( const QString& u ) { uname = u; }
    QString username() const { return uname; }
    void setPassword( const QString& p ) { pword = p; }
    QString password() const { return pword; }
    void setHostname( const QString& h ) { hname = h; }
    QString hostname() const { return hname; }
    void setPort( int p ) { prt = p; }
    int port() const { return prt; }
    QString lastError() const { return dbErr; }
    void addTable( const QString& t ) { tbls.append(t); }
    void setFields( const QString& t, const QStringList& f ) { flds[t] = f; }
    QStringList tables() const { return tbls; }
    QStringList fields( const QString& t ) { return flds[t]; }
    QMap<QString, QStringList> fields() { return flds; }
#ifndef QT_NO_SQL
    QSqlDatabase* connection() const { return conn; }
    void remove();
#endif

private:
    QString nm;
    QString drv, dbName, uname, pword, hname;
    QString dbErr;
    int prt;
    QStringList tbls;
    QMap<QString, QStringList> flds;
#ifndef QT_NO_SQL
    QSqlDatabase *conn;
#endif
    Project *project;
    bool loaded;
    DesignerDatabase *iface;
};

#endif

class Project
{
public:
    Project( const QString &fn, const QString &pName = QString::null, QPluginManager<ProjectSettingsInterface> *pm = 0 );
    ~Project();

    void setFileName( const QString &fn, bool doClear = TRUE );
    QString fileName() const;
    void setProjectName( const QString &n );
    QString projectName() const;
    QString fixedProjectName() const;

    void setDatabaseDescription( const QString &db );
    QString databaseDescription() const;

    void setDescription( const QString &s );
    QString description() const;

    void setLanguage( const QString &l );
    QString language() const;

    QStringList uiFiles() const;
    void addUiFile( const QString &f, FormWindow *fw );
    void removeUiFile( const QString &f, FormWindow *fw );
    void removeSourceFile( const QString &f, SourceFile *sf );
    void setUiFiles( const QStringList &lst );
    void formClosed( FormWindow *fw );
    FormWindow *formWindow( const QString &filename );

    bool isValid() const;

    bool hasFormWindow( FormWindow* fw ) const;
    bool hasUiFile( const QString &filename ) const;
    void setFormWindow( const QString &f, FormWindow *fw );
    void setFormWindowFileName( FormWindow *fw, const QString &f );

    QString makeAbsolute( const QString &f );
    QString makeRelative( const QString &f );

    void save();

#ifndef QT_NO_SQL
    QPtrList<DatabaseConnection> databaseConnections() const;
    void setDatabaseConnections( const QPtrList<DatabaseConnection> &lst );
    void addDatabaseConnection( DatabaseConnection *conn );
    void removeDatabaseConnection( const QString &conn );
    DatabaseConnection *databaseConnection( const QString &name );
    QStringList databaseConnectionList();
    QStringList databaseTableList( const QString &connection );
    QStringList databaseFieldList( const QString &connection, const QString &table );
#endif
    void saveConnections();
    void loadConnections();
    void saveImages();
    void loadImages();

    bool openDatabase( const QString &connection, bool suppressDialog = TRUE );
    void closeDatabase( const QString &connection );

    QObjectList *formList() const;

    DesignerProject *iFace();

    QString formName( const QString &uifile );

    void setCustomSetting( const QString &key, const QString &value );
    QString customSetting( const QString &key ) const;

    void setImageFile( const QString &f );
    QString imageFile() const;

    PixmapCollection *pixmapCollection() const { return pixCollection; }

    void setActive( bool b );

    QPtrList<SourceFile> sourceFiles() const { return sources; }
    void addSourceFile( SourceFile *sf );

    QPtrList<FormWindow> unnamedForms() const;
    QPtrList<FormWindow> forms() const;

    void setIncludePath( const QString &platform, const QString &path );
    void setLibs( const QString &platform, const QString &path );
    void setDefines( const QString &platform, const QString &path );
    void setConfig( const QString &platform, const QString &config );
    void setTemplate( const QString &t );

    QString config( const QString &platform ) const;
    QString libs( const QString &platform ) const;
    QString defines( const QString &platform ) const;
    QString includePath( const QString &platform ) const;
    QString templte() const;

private:
    void parse();
    void clear();
    void updateCustomSettings();
    void readPlatformSettings( const QString &contents,
			       const QString &setting,
			       QMap<QString, QString> &res );
    void removePlatformSettings( QString &contents, const QString &setting );
    void writePlatformSettings( QString &contents, const QString &setting,
				const QMap<QString, QString> &input );

private:
    QString filename;
    QStringList uifiles;
    QString proName;
    QStringList loadedForms;
    QString desc;
    QMap<FormWindow*, QString> formWindows;
    QString dbFile;
#ifndef QT_NO_SQL
    QPtrList<DatabaseConnection> dbConnections;
#endif
    QString lang;
    DesignerProject *iface;
    QMap<QString, QString> customSettings;
    QStringList csList;
    QPluginManager<ProjectSettingsInterface> *projectSettingsPluginManager;
    QString imgFile;
    PixmapCollection *pixCollection;
    QPtrList<SourceFile> sources;
    QMap<QString, QString> inclPath, defs, lbs, cfg;
    QString templ;

};

#endif
