#include <qdir.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qglobal.h>
#include "configureapp.h"

#include <iostream>
#include <windows.h>

using namespace std;

std::ostream &operator<<( std::ostream &s, const QString &val ) {
    s << val.latin1();
    return s;
}

// Macros to simplify options marking, and WinCE only code
#define MARK_OPTION(x,y) ( dictionary[ #x ] == #y ? "*" : " " )
#define WCE(x) if ( dictionary[ "QMAKESPEC" ].startsWith( "wince-" ) ) { x }

class MakeItem
{
public:
    MakeItem( const QString &d, const QString &p, const QString &t, Configure::ProjectType qt )
	: directory( d ),
	  proFile( p ),
	  target( t ),
	  qmakeTemplate( qt )
    { }

    QString directory;
    QString proFile;
    QString target;
    Configure::ProjectType qmakeTemplate;
};

Configure::Configure( int& argc, char** argv )
{
    int i;

    /*
    ** Set up the initial state, the default
    */
    dictionary[ "CONFIGCMD" ] = argv[ 0 ];

    for ( i = 1; i < argc; i++ )
	configCmdLine += argv[ i ];

    for ( i=0; i<3; i++ )
	makeList[i].setAutoDelete( TRUE );

    dictionary[ "QCONFIG" ]	    = "full";
    dictionary[ "EMBEDDED" ]	    = "no";

    dictionary[ "BUILD_QMAKE" ]	    = "yes";
    dictionary[ "DSPFILES" ]	    = "yes";
    dictionary[ "VCPFILES" ]	    = "yes";
    dictionary[ "VCPROJFILES" ]	    = "yes";
    dictionary[ "QMAKESPEC" ]	    = getenv( "QMAKESPEC" );
    dictionary[ "QMAKE_INTERNAL" ]  = "no";
    dictionary[ "LEAN" ]	    = "no";
    dictionary[ "NOPROCESS" ]	    = "no";
    dictionary[ "STL" ]		    = "yes";
    dictionary[ "QWINEXPORT" ]	    = "no";
    dictionary[ "EXCEPTIONS" ]	    = "no";
    dictionary[ "RTTI" ]	    = "no";

    /*
	Figure out the version number doing:

	    QT_MAJOR = QT_VERSION >> 16;
	    QT_MINOR = ( QT_VERSION >> 8 ) & 0xff;
	    QT_PATCH = QT_VERSION & 0xff;
    */

    dictionary[ "VERSION" ]	    = QString("%1%2%3").arg(QT_VERSION>>16).arg(((QT_VERSION>>8)&0xff)).arg(QT_VERSION&0xff);
    dictionary[ "REDO" ]	    = "no";
    dictionary[ "FORCE_PROFESSIONAL" ] = getenv( "FORCE_PROFESSIONAL" );
    dictionary[ "DEPENDENCIES" ]    = "no";

    dictionary[ "DEBUG" ]	    = "no";
    dictionary[ "SHARED" ]	    = "yes";
    dictionary[ "THREAD" ]	    = "no";

    dictionary[ "GIF" ]		    = "no";
    dictionary[ "ZLIB" ]	    = "yes";
    dictionary[ "PNG" ]		    = "qt";
    dictionary[ "JPEG" ]	    = "qt";
    dictionary[ "MNG" ]		    = "no";

    dictionary[ "LIBPNG" ]	    = "qt";
    dictionary[ "LIBJPEG" ]	    = "qt";
    dictionary[ "LIBMNG" ]	    = "qt";

    dictionary[ "ACCESSIBILITY" ]   = "yes";
    dictionary[ "BIG_CODECS" ]	    = "yes";
    dictionary[ "TABLET" ]	    = "no";
    dictionary[ "OPENGL" ]	    = "yes";

    dictionary[ "STYLE_WINDOWS" ]   = "yes";
    dictionary[ "STYLE_MOTIF" ]	    = "yes";
    dictionary[ "STYLE_MOTIFPLUS" ] = "yes";
    dictionary[ "STYLE_PLATINUM" ]  = "yes";
    dictionary[ "STYLE_SGI" ]	    = "yes";
    dictionary[ "STYLE_CDE" ]	    = "yes";
    dictionary[ "STYLE_WINDOWSXP" ] = "no";
    dictionary[ "STYLE_POCKETPC" ]  = "no";

    dictionary[ "SQL_MYSQL" ]	    = "no";
    dictionary[ "SQL_ODBC" ]	    = "no";
    dictionary[ "SQL_OCI" ]	    = "no";
    dictionary[ "SQL_PSQL" ]	    = "no";
    dictionary[ "SQL_TDS" ]	    = "no";
    dictionary[ "SQL_DB2" ]	    = "no";

    dictionary[ "QT_SOURCE_TREE" ]  = QDir::convertSeparators( QDir::currentDirPath() );
    dictionary[ "QT_INSTALL_PREFIX" ] = QDir::convertSeparators( dictionary[ "QT_SOURCE_TREE" ] );
    dictionary[ "QT_INSTALL_TRANSLATIONS" ] = dictionary[ "QT_INSTALL_PREFIX" ] + "\\translations";

    QString tmp = dictionary[ "QMAKESPEC" ];
    tmp = tmp.mid( tmp.findRev( "\\" ) + 1 );
    dictionary[ "QMAKESPEC" ] = tmp;
    qmakeConfig += "nocrosscompiler";

#if !defined(EVAL)
    WCE( {
	// WinCE has different defaults than the desktop version
	dictionary[ "QMAKE_INTERNAL" ]	= "no";
	dictionary[ "BUILD_QMAKE" ]	= "no";
	dictionary[ "ACCESSIBILITY" ]	= "no"; // < CE 4.0
	dictionary[ "BIG_CODECS" ]	= "no"; // Keep small
	dictionary[ "STL" ]		= "no"; // < CE 4.0
	dictionary[ "OPENGL" ]		= "no"; // < CE 4.0
	dictionary[ "STYLE_MOTIF" ]	= "no";
	dictionary[ "STYLE_MOTIFPLUS" ] = "no";
	dictionary[ "STYLE_PLATINUM" ]  = "no";
	dictionary[ "STYLE_SGI" ]	= "no";
	dictionary[ "STYLE_CDE" ]	= "no";
	dictionary[ "STYLE_WINDOWSXP" ] = "no";
	dictionary[ "STYLE_POCKETPC" ]	= "yes";

	// Disabled modules for CE
	disabledModules += "opengl";		// < CE 4.0
	disabledModules += "sql";		// For now
    } );

    readLicense();

    buildModulesList();
#endif
}

#if !defined(EVAL)
void Configure::buildModulesList()
{
    QDir dir( dictionary[ "QT_SOURCE_TREE" ] + "/src" );
    const QFileInfoList* fiList = dir.entryInfoList();
    if ( !fiList )
	return;

    QFileInfoListIterator listIter( *fiList );
    QFileInfo* fi;

    licensedModules = QStringList::split( ' ', "compat styles tools kernel widgets dialogs iconview workspace" );

    QString products = licenseInfo[ "PRODUCTS" ];
    if( ( products == "qt-enterprise" || products == "qt-internal" ) && ( dictionary[ "FORCE_PROFESSIONAL" ] != "yes" ) )
	licensedModules += QStringList::split( ' ', "network canvas table xml opengl sql" );

    while( ( fi = listIter.current() ) ) {
	if( licensedModules.findIndex( fi->fileName() ) != -1 )
	    modules += fi->fileName();
	++listIter;
    }
}
#endif

void Configure::parseCmdLine()
{
    QStringList::Iterator args = configCmdLine.begin();

#if !defined(EVAL)
    if( (*args) == "-redo" ) {
	configCmdLine.clear();
	dictionary[ "REDO" ] = "yes";
	reloadCmdLine();
	args = configCmdLine.begin();	// We've got a new command line...
    }
    else if( (*args) == "-loadconfig" ) {
	++args;
	dictionary[ "REDO" ] = "yes";
	dictionary[ "CUSTOMCONFIG" ] = "_" + (*args);
	configCmdLine.clear();
	reloadCmdLine();
	args = configCmdLine.begin();
    }
#endif

    for( ; args != configCmdLine.end(); ++args ) {
	if( (*args) == "-help" )
	    dictionary[ "HELP" ] = "yes";
	else if( (*args) == "-?" )
	    dictionary[ "HELP" ] = "yes";

#if !defined(EVAL)
	else if( (*args) == "-qconfig" ) {
	    ++args;
	    if (args==configCmdLine.end())
		break;
	    dictionary[ "QCONFIG" ] = (*args);
	}

	else if( (*args) == "-release" )
	    dictionary[ "DEBUG" ] = "no";
	else if( (*args) == "-debug" )
	    dictionary[ "DEBUG" ] = "yes";

	else if( (*args) == "-shared" )
	    dictionary[ "SHARED" ] = "yes";
	else if( (*args) == "-static" )
	    dictionary[ "SHARED" ] = "no";

	else if( (*args) == "-no-thread" )
	    dictionary[ "THREAD" ] = "no";
	else if( (*args) == "-thread" )
	    dictionary[ "THREAD" ] = "yes";
#endif

	else if( (*args) == "-spec" ) {
	    ++args;
	    if (args==configCmdLine.end())
		break;
	    dictionary[ "QMAKESPEC" ] = (*args);
	}

#if !defined(EVAL)
	else if( (*args) == "-no-gif" )
	    dictionary[ "GIF" ] = "no";
	else if( (*args) == "-qt-gif" )
	    dictionary[ "GIF" ] = "yes";

	else if( (*args) == "-no-zlib" ) {
	    dictionary[ "ZLIB" ] = "no";
	    dictionary[ "PNG" ] = "no";
	} else if( (*args) == "-qt-zlib" ) {
	    dictionary[ "ZLIB" ] = "yes";
	} else if( (*args) == "-system-zlib" ) {
	    dictionary[ "ZLIB" ] = "system";
	}

	// ### remove -no-<format> options in Qt 4.0
	else if( (*args) == "-plugin-imgfmt-png" )
	    dictionary[ "PNG" ] = "plugin";
	else if( (*args) == "-qt-imgfmt-png" )
	    dictionary[ "PNG" ] = "qt";
	else if( (*args) == "-no-imgfmt-png" || (*args) == "-no-png" )
	    dictionary[ "PNG" ] = "no";
	else if( (*args) == "-qt-png" )
	    dictionary[ "LIBPNG" ] = "qt";
	else if( (*args) == "-system-png" )
	    dictionary[ "LIBPNG" ] = "system";

	else if( (*args) == "-plugin-imgfmt-mng" )
	    dictionary[ "MNG" ] = "plugin";
	else if( (*args) == "-qt-imgfmt-mng" )
	    dictionary[ "MNG" ] = "qt";
	else if( (*args) == "-no-imgfmt-mng" )
	    dictionary[ "MNG" ] = "no";
	else if( (*args) == "-qt-mng" )
	    dictionary[ "LIBMNG" ] = "qt";
	else if( (*args) == "-system-mng" )
	    dictionary[ "LIBMNG" ] = "system";

	else if( (*args) == "-plugin-imgfmt-jpeg" )
	    dictionary[ "JPEG" ] = "plugin";
	else if( (*args) == "-qt-imgfmt-jpeg" )
	    dictionary[ "JPEG" ] = "qt";
	else if( (*args) == "-no-imgfmt-jpeg" || (*args) == "-no-jpeg" )
	    dictionary[ "JPEG" ] = "no";
	else if( (*args) == "-qt-jpeg" )
	    dictionary[ "LIBJPEG" ] = "qt";
	else if( (*args) == "-system-jpeg" )
	    dictionary[ "LIBJPEG" ] = "system";

	else if( (*args) == "-qt-style-windows" )
	    dictionary[ "STYLE_WINDOWS" ] = "yes";
	else if( (*args) == "-plugin-style-windows" )
	    dictionary[ "STYLE_WINDOWS" ] = "plugin";
	else if( (*args) == "-no-style-windows" )
	    dictionary[ "STYLE_WINDOWS" ] = "no";

	else if( (*args) == "-qt-style-motif" )
	    dictionary[ "STYLE_MOTIF" ] = "yes";
	else if( (*args) == "-plugin-style-motif" )
	    dictionary[ "STYLE_MOTIF" ] = "plugin";
	else if( (*args) == "-no-style-motif" )
	    dictionary[ "STYLE_MOTIF" ] = "no";

	else if( (*args) == "-qt-style-platinum" )
	    dictionary[ "STYLE_PLATINUM" ] = "yes";
	else if( (*args) == "-plugin-style-platinum" )
	    dictionary[ "STYLE_PLATINUM" ] = "plugin";
	else if( (*args) == "-no-style-platinum" )
	    dictionary[ "STYLE_PLATINUM" ] = "no";

	else if( (*args) == "-qt-style-motifplus" )
	    dictionary[ "STYLE_MOTIFPLUS" ] = "yes";
	else if( (*args) == "-plugin-style-motifplus" )
	    dictionary[ "STYLE_MOTIFPLUS" ] = "plugin";
	else if( (*args) == "-no-style-motifplus" )
	    dictionary[ "STYLE_MOTIFPLUS" ] = "no";

	else if( (*args) == "-qt-style-cde" )
	    dictionary[ "STYLE_CDE" ] = "yes";
	else if( (*args) == "-plugin-style-cde" )
	    dictionary[ "STYLE_CDE" ] = "plugin";
	else if( (*args) == "-no-style-cde" )
	    dictionary[ "STYLE_CDE" ] = "no";

	else if( (*args) == "-qt-style-sgi" )
	    dictionary[ "STYLE_SGI" ] = "yes";
	else if( (*args) == "-plugin-style-sgi" )
	    dictionary[ "STYLE_SGI" ] = "plugin";
	else if( (*args) == "-no-style-sgi" )
	    dictionary[ "STYLE_SGI" ] = "no";

	else if( (*args) == "-qt-style-windowsxp" )
	    dictionary[ "STYLE_WINDOWSXP" ] = "yes";
	else if( (*args) == "-plugin-style-windowsxp" )
	    dictionary[ "STYLE_WINDOWSXP" ] = "plugin";
	else if( (*args) == "-no-style-windowsxp" )
	    dictionary[ "STYLE_WINDOWSXP" ] = "no";

	else if( (*args) == "-qt-style-pocketpc" )
	    dictionary[ "STYLE_POCKETPC" ] = "yes";
	else if( (*args) == "-plugin-style-pocketpc" )
	    dictionary[ "STYLE_POCKETPC" ] = "plugin";
	else if( (*args) == "-no-style-pocketpc" )
	    dictionary[ "STYLE_POCKETPC" ] = "no";

	else if( (*args) == "-qt-sql-mysql" )
	    dictionary[ "SQL_MYSQL" ] = "yes";
	else if( (*args) == "-plugin-sql-mysql" )
	    dictionary[ "SQL_MYSQL" ] = "plugin";
	else if( (*args) == "-no-sql-mysql" )
	    dictionary[ "SQL_MYSQL" ] = "no";

	else if( (*args) == "-qt-sql-odbc" )
	    dictionary[ "SQL_ODBC" ] = "yes";
	else if( (*args) == "-plugin-sql-odbc" )
	    dictionary[ "SQL_ODBC" ] = "plugin";
	else if( (*args) == "-no-sql-odbc" )
	    dictionary[ "SQL_ODBC" ] = "no";

	else if( (*args) == "-qt-sql-oci" )
	    dictionary[ "SQL_OCI" ] = "yes";
	else if( (*args) == "-plugin-sql-oci" )
	    dictionary[ "SQL_OCI" ] = "plugin";
	else if( (*args) == "-no-sql-oci" )
	    dictionary[ "SQL_OCI" ] = "no";

	else if( (*args) == "-qt-sql-psql" )
	    dictionary[ "SQL_PSQL" ] = "yes";
	else if( (*args) == "-plugin-sql-psql" )
	    dictionary[ "SQL_PSQL" ] = "plugin";
	else if( (*args) == "-no-sql-psql" )
	    dictionary[ "SQL_PSQL" ] = "no";

	else if( (*args) == "-qt-sql-tds" )
	    dictionary[ "SQL_TDS" ] = "yes";
	else if( (*args) == "-plugin-sql-tds" )
	    dictionary[ "SQL_TDS" ] = "plugin";
	else if( (*args) == "-no-sql-tds" )
	    dictionary[ "SQL_TDS" ] = "no";

	else if( (*args) == "-qt-sql-db2" )
	    dictionary[ "SQL_DB2" ] = "yes";
	else if( (*args) == "-plugin-sql-db2" )
	    dictionary[ "SQL_DB2" ] = "plugin";
	else if( (*args) == "-no-sql-db2" )
	    dictionary[ "SQL_DB2" ] = "no";

	else if( (*args) == "-internal" )
	    dictionary[ "QMAKE_INTERNAL" ] = "yes";

	else if( (*args) == "-no-qmake" )
	    dictionary[ "BUILD_QMAKE" ] = "no";

	else if( (*args) == "-dont-process" )
	    dictionary[ "NOPROCESS" ] = "yes";

	else if( (*args) == "-qmake-deps" )
	    dictionary[ "DEPENDENCIES" ] = "yes";

	else if( (*args) == "-D" ) {
	    ++args;
	    if (args==configCmdLine.end())
		break;
            qmakeDefines += (*args);
        } else if( (*args) == "-I" ) {
	    ++args;
	    if (args==configCmdLine.end())
		break;
	    qmakeIncludes += (*args);
	} else if( (*args) == "-L" ) {
	    ++args;
	    if (args==configCmdLine.end())
		break;
	    qmakeLibs += (*args);
	}
#endif

	else if( (*args) == "-no-dsp" )
	    dictionary[ "DSPFILES" ] = "no";
	else if( (*args) == "-dsp" )
	    dictionary[ "DSPFILES" ] = "yes";

	else if( (*args) == "-no-vcp" )
	    dictionary[ "VCPFILES" ] = "no";
	else if( (*args) == "-vcp" )
	    dictionary[ "VCPFILES" ] = "yes";

	else if( (*args) == "-no-vcproj" )
	    dictionary[ "VCPROJFILES" ] = "no";
	else if( (*args) == "-vcproj" )
	    dictionary[ "VCPROJFILES" ] = "yes";

#if !defined(EVAL)
	else if( (*args) == "-lean" )
	    dictionary[ "LEAN" ] = "yes";

	else if( (*args) == "-stl" )
	    dictionary[ "STL" ] = "yes";
	else if( (*args) == "-no-stl" )
	    dictionary[ "STL" ] = "no";

	else if( (*args) == "-qwinexport" )
	    dictionary[ "QWINEXPORT" ] = "yes";
	else if( (*args) == "-no-qwinexport" )
	    dictionary[ "QWINEXPORT" ] = "no";

	else if ( (*args) == "-exceptions" )
	    dictionary[ "EXCEPTIONS" ] = "yes";
	else if ( (*args) == "-no-exceptions" )
	    dictionary[ "EXCEPTIONS" ] = "no";

	else if ( (*args) == "-rtti" )
	    dictionary[ "RTTI" ] = "yes";
	else if ( (*args) == "-no-rtti" )
	    dictionary[ "RTTI" ] = "no";

	else if( (*args) == "-accessibility" )
	    dictionary[ "ACCESSIBILITY" ] = "yes";
	else if( (*args) == "-no-accessibility" )
	    dictionary[ "ACCESSIBILITY" ] = "no";

	else if( (*args) == "-no-big-codecs" )
	    dictionary[ "BIG_CODECS" ] = "no";
	else if( (*args) == "-big-codecs" )
	    dictionary[ "BIG_CODECS" ] = "yes";

	else if( (*args) == "-tablet" )
	    dictionary[ "TABLET" ] = "yes";
	else if( (*args) == "-no-tablet" )
	    dictionary[ "TABLET" ] = "no";

	else if( ( (*args) == "-override-version" ) || ( (*args) == "-version-override" ) ){
	    ++args;
	    if (args==configCmdLine.end())
		break;
	    dictionary[ "VERSION" ] = (*args);
	}

	else if( (*args) == "-saveconfig" ) {
	    ++args;
	    if (args==configCmdLine.end())
		break;
	    dictionary[ "CUSTOMCONFIG" ] = "_" + (*args);
	}

	else if( (*args) == "-prefix" ) {
	    ++args;
	    if(  args == configCmdLine.end() )
		break;
	    dictionary[ "QT_INSTALL_PREFIX" ] = (*args);
	}

	else if( (*args) == "-headerdir" ) {
	    ++args;
	    if(  args == configCmdLine.end() )
		break;
	    dictionary[ "QT_INSTALL_HEADERS" ] = (*args);
	}

	else if( (*args) == "-docdir" ) {
	    ++args;
	    if(  args == configCmdLine.end() )
		break;
	    dictionary[ "QT_INSTALL_DOCS" ] = (*args);
	}

	else if( (*args) == "-plugindir" ) {
	    ++args;
	    if(  args == configCmdLine.end() )
		break;
	    dictionary[ "QT_INSTALL_PLUGINS" ] = (*args);
	}

	else if( (*args) == "-libdir" ) {
	    ++args;
	    if(  args == configCmdLine.end() )
		break;
	    dictionary[ "QT_INSTALL_LIBS" ] = (*args);
	}

	else if( (*args) == "-bindir" ) {
	    ++args;
	    if(  args == configCmdLine.end() )
		break;
	    dictionary[ "QT_INSTALL_BINS" ] = (*args);
	}

	else if( (*args) == "-datadir" ) {
	    ++args;
	    if(  args == configCmdLine.end() )
		break;
	    dictionary[ "QT_INSTALL_DATA" ] = (*args);
	}

	else if( (*args) == "-translationdir" ) {
	    ++args;
	    if( args == configCmdLine.end() )
		break;
	    dictionary[ "QT_INSTALL_TRANSLATIONS" ] = (*args);
	}

	else if( (*args).find( QRegExp( "^-(en|dis)able-" ) ) != -1 ) {
	    // Scan to see if any specific modules and drivers are enabled or disabled
	    for( QStringList::Iterator module = modules.begin(); module != modules.end(); ++module ) {
		if( (*args) == QString( "-enable-" ) + (*module) ) {
		    enabledModules += (*module);
		    break;
		}
		else if( (*args) == QString( "-disable-" ) + (*module) ) {
		    disabledModules += (*module);
		    break;
		}
	    }
	}

	else {
	    dictionary[ "HELP" ] = "yes";
	    cout << "Unknown option " << (*args) << endl;
	    break;
	}

#endif
    }

    if( dictionary[ "QMAKESPEC" ].endsWith( "-msvc" ) ||
	dictionary[ "QMAKESPEC" ].endsWith( ".net" ) ||
	dictionary[ "QMAKESPEC" ].endsWith( "-icc" ) ) {
		dictionary[ "MAKE" ] = "nmake";
 		dictionary[ "QMAKEMAKEFILE" ] = "Makefile";
    } else if ( dictionary[ "QMAKESPEC" ] == QString( "win32-g++" ) ) {
	    dictionary[ "MAKE" ] = "mingw32-make";
    	dictionary[ "QMAKEMAKEFILE" ] = "Makefile.win32-g++";
    } else {
	    dictionary[ "MAKE" ] = "make";
	    dictionary[ "QMAKEMAKEFILE" ] = "Makefile";
    }

#if !defined(EVAL)
    for( QStringList::Iterator dis = disabledModules.begin(); dis != disabledModules.end(); ++dis ) {
	modules.remove( (*dis) );
    }
    for( QStringList::Iterator ena = enabledModules.begin(); ena != enabledModules.end(); ++ena ) {
	if( modules.findIndex( (*ena) ) == -1 )
	    modules += (*ena);
    }
    qmakeConfig += modules;

    for( QStringList::Iterator it = disabledModules.begin(); it != disabledModules.end(); ++it )
	qmakeConfig.remove( (*it) );

    if( !qmakeConfig.contains("opengl") )
	dictionary[ "OPENGL" ] = "no";

    if( ( dictionary[ "REDO" ] != "yes" ) && ( dictionary[ "HELP" ] != "yes" ) )
	saveCmdLine();
#endif
}

#if !defined(EVAL)
void Configure::validateArgs()
{
    QStringList configs;
    // Validate the specified config

    allConfigs = QStringList::split( ' ', "minimal small medium large full" );

    QStringList::Iterator config;
    for( config = allConfigs.begin(); config != allConfigs.end(); ++config ) {
	configs += (*config) + "-config";
	if( (*config) == dictionary[ "QCONFIG" ] )
	    break;
    }
    if( config == allConfigs.end() ) {
	dictionary[ "HELP" ] = "yes";
	cout << "No such configuration \"" << dictionary[ "QCONFIG" ].latin1() << "\"" << endl ;
    }
    else
	qmakeConfig += configs;
}
#endif

bool Configure::displayHelp()
{
    if( dictionary[ "HELP" ] == "yes" ) {
	cout << endl << endl;
	cout << "Command line arguments:  (* indicates default behaviour)" << endl << endl;
	cout << "-help                Bring up this help text." << endl << endl;

#if !defined(EVAL)
	cout << "-debug             " << MARK_OPTION(DEBUG,yes)	    << " Enable debug information." << endl;
	cout << "-release           " << MARK_OPTION(DEBUG,no)	    << " Disable debug information." << endl << endl;

	cout << "-shared            " << MARK_OPTION(SHARED,yes)    << " Build Qt as a shared library." << endl;
	cout << "-static            " << MARK_OPTION(SHARED,no)	    << " Build Qt as a static library." << endl << endl;

	cout << "-thread            " << MARK_OPTION(THREAD,yes)    << " Configure Qt with thread support." << endl;
	cout << "-no-thread         " << MARK_OPTION(THREAD,no)	    << " Configure Qt without thread support." << endl << endl;
#endif

	cout << "-spec                Specify a platform, uses %QMAKESPEC% as default." << endl;
#if !defined(EVAL)
	cout << "-qconfig             Specify config, available configs:" << endl;
	for( QStringList::Iterator config = allConfigs.begin(); config != allConfigs.end(); ++config )
	    cout << "                         " << (*config).latin1() << endl;

	cout << endl;
	cout << "-qt-gif            " << MARK_OPTION(GIF,yes)	    << " Enable GIF support." << endl;
	cout << "-no-gif            " << MARK_OPTION(GIF,no)	    << " Disable GIF support." << endl << endl;

	cout << "-no-zlib           " << MARK_OPTION(ZLIB,no)	    << " Disable zlib.  Implies -no-png." << endl;
	cout << "-qt-zlib           " << MARK_OPTION(ZLIB,yes)	    << " Compile in zlib." << endl;
	cout << "-system-zlib       " << MARK_OPTION(ZLIB,system)   << " Use existing zlib in system." << endl << endl;

	cout << "-plugin-imgfmt-<format> Enable format (png, jpeg, or mng) to" << endl;
	cout << "                        be linked to at runtime." << endl;
	cout << "-qt-imgfmt-<format>     Enable format (png, jpeg, or mng) to" << endl;
	cout << "                        be linked into Qt." << endl;
	cout << "-no-imgfmt-<format>     Fully disable format (png, jpeg, or mng)" << endl << endl;
	cout << "                        from Qt." << endl;

	cout << "-qt-png            " << MARK_OPTION(LIBPNG,qt)	    << " Use the libpng bundled with Qt." << endl;
	cout << "-system-png        " << MARK_OPTION(LIBPNG,system) << " Use existing libPNG in system." << endl  << endl;

	cout << "-qt-mng            " << MARK_OPTION(LIBMNG,qt)	    << " Use the libmng bundled with Qt." << endl;
	cout << "-system-mng        " << MARK_OPTION(LIBMNG,system) << " Use existing libMNG in system." << endl << endl;

	cout << "-qt-jpeg           " << MARK_OPTION(LIBJPEG,qt)    << " Use the libjpeg bundled with Qt." << endl;
	cout << "-system-jpeg       " << MARK_OPTION(LIBJPEG,system)<< " Use existing libJPEG in system" << endl << endl;

	cout << "-stl               " << MARK_OPTION(STL,yes)	    << " Enable STL support." << endl;
	cout << "-no-stl            " << MARK_OPTION(STL,no)	    << " Disable STL support." << endl  << endl;

	cout << "-qwinexport        " << MARK_OPTION(QWINEXPORT,yes)<< " Enable additional and centralized template exports." << endl;
	cout << "-no-qwinexport     " << MARK_OPTION(QWINEXPORT,no) << " Disable additional template exports." << endl  << endl;

	cout << "-exceptions        " << MARK_OPTION(EXCEPTIONS,yes)<< " Enable C++ exception support." << endl;
	cout << "-no-exceptions     " << MARK_OPTION(EXCEPTIONS,no) << " Disable C++ exception support." << endl  << endl;

	cout << "-rtti              " << MARK_OPTION(RTTI,yes)	    << " Enable runtime type information." << endl;
	cout << "-no-rtti           " << MARK_OPTION(RTTI,no)	    << " Disable runtime type information." << endl  << endl;

	cout << "-accessibility     " << MARK_OPTION(ACCESSIBILITY,yes) << " Enable Windows Active Accessibility." << endl;
	cout << "-no-accessibility  " << MARK_OPTION(ACCESSIBILITY,no)  << " Disable Windows Active Accessibility." << endl  << endl;

	cout << "-tablet            " << MARK_OPTION(TABLET,yes)    << " Enable tablet support." << endl;
	cout << "-no-tablet         " << MARK_OPTION(TABLET,no)	    << " Disable tablet support." << endl  << endl;

	cout << "-big-codecs        " << MARK_OPTION(BIG_CODECS,yes)<< " Enable the building of big codecs." << endl;
	cout << "-no-big-codecs     " << MARK_OPTION(BIG_CODECS,no) << " Disable the building of big codecs." << endl << endl;

	cout << "-dsp               " << MARK_OPTION(DSPFILES,yes)   << " Enable the generation of VC++ .DSP-files." << endl;
	cout << "-no-dsp            " << MARK_OPTION(DSPFILES,no)  << " Disable the generation of VC++ .DSP-files." << endl << endl;
#endif

	// Only show the VCP generation options for CE users for now
WCE( {	cout << "-vcp               " << MARK_OPTION(VCPFILES,yes)   << " Enable the generation of eMbedded VC++ .VCP-files." << endl;
	cout << "-no-vcp            " << MARK_OPTION(VCPFILES,no)  << " Disable the generation of eMbedded VC++ .VCP-files." << endl << endl; } );

	cout << "-vcproj            " <<MARK_OPTION(VCPROJFILES,yes) << " Enable the generation of VC++ .VCPROJ-files." << endl;
	cout << "-no-vcproj         " <<MARK_OPTION(VCPROJFILES,no)<< " Disable the generation of VC++ .VCPROJ-files." << endl << endl;

#if !defined(EVAL)
	cout << "-no-qmake            Do not build qmake." << endl;
	cout << "-lean                Only process the Qt core projects." << endl;
	cout << "                     (qt.pro, qtmain.pro)." << endl << endl;

	cout << "-D <define>          Add <define> to the list of defines." << endl;
	cout << "-I <includepath>     Add <includepath> to the include searchpath." << endl;
	cout << "-L <library>         Add <library> to the library list." << endl << endl;

	cout << "-enable-*            Enable the specified module" << endl;
	cout << "-disable-*           Disable the specified module" << endl;
	cout << "                     where module is one of" << endl;
	for( QStringList::Iterator module = modules.begin(); module != modules.end(); ++module )
	    cout << "                         " << (*module).latin1() << endl;
	cout << endl;

	cout << "-qt-sql-*            Build the specified Sql driver into Qt" << endl;
	cout << "-plugin-sql-*        Build the specified Sql driver into a plugin" << endl;
	cout << "-no-sql-*          * Don't build the specified Sql driver" << endl;
	cout << "                     where sql driver is one of" << endl;
	cout << "                         mysql" << endl;
	cout << "                         psql" << endl;
	cout << "                         oci" << endl;
	cout << "                         odbc" << endl;
	cout << "                         tds" << endl;
	cout << "                         db2" << endl << endl;

	cout << "-qt-style-*        * Build the specified style into Qt" << endl;
	cout << "-plugin-style-*      Build the specified style into a plugin" << endl;
	cout << "-no-style-*          Don't build the specified style" << endl;
	cout << "                     where style is one of" << endl;
	// Only show the PocketPC style option for CE users
WCE( {	cout << "                         pocketpc" << endl; } );
	cout << "                         windows" << endl;
	cout << "                         windowsxp" << endl;
	cout << "                         motif" << endl;
	cout << "                         cde" << endl;
	cout << "                         sgi" << endl;
	cout << "                         motifplus" << endl;
	cout << "                         platinum" << endl << endl;

	cout << "-prefix <prefix>      Install Qt to <prefix>, defaults to current directory." << endl;
	cout << "-docdir <docs>        Install documentation to <docs>, default <prefix>/doc." << endl;
	cout << "-headerdir <headers>  Install Qt headers to <header>, default <prefix>/include." << endl;
	cout << "-plugindir <plugins>  Install Qt plugins to <plugins>, default <prefix>/plugins." << endl;
	cout << "-datadir <data>       Data used by Qt programs will be installed to <data>." << endl;
	cout << "-translationdir <dir> Translations used by Qt programs will be installed to dir." << endl << "(default PREFIX/translations)" << endl;
	cout << "                     Default <prefix>." << endl;
	cout << "-libdir <libs>       Install Qt libraries to <libs>, default <prefix>/lib." << endl;
	cout << "-bindir <bins>       Install Qt binaries to <bins>, default <prefix>/bin." << endl << endl;

	cout << "-redo                Run configure with the same parameters as last time." << endl;
	cout << "-saveconfig <config> Run configure and save the parameters as <config>." << endl;
	cout << "-loadconfig <config> Run configure with the parameters from <config>." << endl;
#endif
	return true;
    }
    return false;
}

void Configure::generateOutputVars()
{
    // Generate variables for output
    if( dictionary[ "DEBUG" ] == "yes" ) {
	qmakeConfig += "debug";
	dictionary[ "QMAKE_OUTDIR" ] = "debug";
    } else {
	qmakeConfig += "release";
	dictionary[ "QMAKE_OUTDIR" ] = "release";
    }

    if( dictionary[ "THREAD" ] == "yes" ) {
	qmakeConfig += "thread";
	dictionary[ "QMAKE_OUTDIR" ] += "_mt";
    }

    if( dictionary[ "ACCESSIBILITY" ] == "yes" ) {
	qmakeConfig += "accessibility";
    }

    if( dictionary[ "SHARED" ] == "yes" ) {
	dictionary[ "QMAKE_OUTDIR" ] += "_shared";
    } else {
	dictionary[ "QMAKE_OUTDIR" ] += "_static";
    }

    if( !qmakeLibs.isEmpty() ) {
	qmakeVars += "LIBS += " + qmakeLibs.join( " " );
    }

    if( dictionary[ "GIF" ] == "yes" )
	qmakeConfig += "gif";
    else if( dictionary[ "GIF" ] == "no" )
	qmakeConfig += "no-gif";

    if( dictionary[ "ZLIB" ] == "yes" )
	qmakeConfig += "zlib";
    else if( dictionary[ "ZLIB" ] == "no" )
	qmakeConfig += "no-zlib";

    if( dictionary[ "JPEG" ] == "no" )
	qmakeConfig += "no-jpeg";
    else if( dictionary[ "JPEG" ] == "qt" )
	qmakeConfig += "jpeg";
    else if( dictionary[ "JPEG" ] == "plugin" )
	qmakeFormatPlugins += "jpeg";

    if( dictionary[ "LIBJPEG" ] == "system" )
	qmakeConfig += "system-jpeg";

    if( dictionary[ "MNG" ] == "no" )
	qmakeConfig += "no-mng";
    else if( dictionary[ "MNG" ] == "qt" )
	qmakeConfig += "mng";
    else if( dictionary[ "MNG" ] == "plugin" )
	qmakeFormatPlugins += "mng";

    if( dictionary[ "LIBMNG" ] == "system" )
	qmakeConfig += "system-mng";

    if( dictionary[ "PNG" ] == "no" )
	qmakeConfig += "no-png";
    else if( dictionary[ "PNG" ] == "qt" )
	qmakeConfig += "png";
    else if( dictionary[ "PNG" ] == "plugin" )
	qmakeFormatPlugins += "png";

    if( dictionary[ "LIBPNG" ] == "system" )
	qmakeConfig += "system-png";

    if( dictionary[ "BIG_CODECS" ] == "yes" )
	qmakeConfig += "bigcodecs";
    else if( dictionary[ "BIG_CODECS" ] == "no" )
	qmakeConfig += "no-bigcodecs";

    if( dictionary[ "TABLET" ] == "yes" )
	qmakeConfig += "tablet";
    else if( dictionary[ "TABLET" ] == "no" )
	qmakeConfig += "no-tablet";

    if ( dictionary[ "STYLE_WINDOWS" ] == "yes" )
	qmakeStyles += "windows";
    else if ( dictionary[ "STYLE_WINDOWS" ] == "plugin" )
	qmakeStylePlugins += "windows";

    if ( dictionary[ "STYLE_WINDOWSXP" ] == "yes" )
	qmakeStyles += "windowsxp";
    else if ( dictionary[ "STYLE_WINDOWSXP" ] == "plugin" )
	qmakeStylePlugins += "windowsxp";

    if ( dictionary[ "STYLE_MOTIF" ] == "yes" )
	qmakeStyles += "motif";
    else if ( dictionary[ "STYLE_MOTIF" ] == "plugin" )
	qmakeStylePlugins += "motif";

    if ( dictionary[ "STYLE_MOTIFPLUS" ] == "yes" )
	qmakeStyles += "motifplus";
    else if ( dictionary[ "STYLE_MOTIFPLUS" ] == "plugin" )
	qmakeStylePlugins += "motifplus";

    if ( dictionary[ "STYLE_PLATINUM" ] == "yes" )
	qmakeStyles += "platinum";
    else if ( dictionary[ "STYLE_PLATINUM" ] == "plugin" )
	qmakeStylePlugins += "platinum";

    if ( dictionary[ "STYLE_SGI" ] == "yes" )
	qmakeStyles += "sgi";
    else if ( dictionary[ "STYLE_SGI" ] == "plugin" )
	qmakeStylePlugins += "sgi";

    if ( dictionary[ "STYLE_POCKETPC" ] == "yes" )
	qmakeStyles += "pocketpc";
    else if ( dictionary[ "STYLE_POCKETPC" ] == "plugin" )
	qmakeStylePlugins += "pocketpc";

    if ( dictionary[ "STYLE_CDE" ] == "yes" )
	qmakeStyles += "cde";
    else if ( dictionary[ "STYLE_CDE" ] == "plugin" )
	qmakeStylePlugins += "cde";

    if ( dictionary[ "SQL_MYSQL" ] == "yes" )
	qmakeSql += "mysql";
    else if ( dictionary[ "SQL_MYSQL" ] == "plugin" )
	qmakeSqlPlugins += "mysql";

    if ( dictionary[ "SQL_ODBC" ] == "yes" )
	qmakeSql += "odbc";
    else if ( dictionary[ "SQL_ODBC" ] == "plugin" )
	qmakeSqlPlugins += "odbc";

    if ( dictionary[ "SQL_OCI" ] == "yes" )
	qmakeSql += "oci";
    else if ( dictionary[ "SQL_OCI" ] == "plugin" )
	qmakeSqlPlugins += "oci";

    if ( dictionary[ "SQL_PSQL" ] == "yes" )
	qmakeSql += "psql";
    else if ( dictionary[ "SQL_PSQL" ] == "plugin" )
	qmakeSqlPlugins += "psql";

    if ( dictionary[ "SQL_TDS" ] == "yes" )
	qmakeSql += "tds";
    else if ( dictionary[ "SQL_TDS" ] == "plugin" )
	qmakeSqlPlugins += "tds";

    if ( dictionary[ "SQL_DB2" ] == "yes" )
	qmakeSql += "db2";
    else if ( dictionary[ "SQL_DB2" ] == "plugin" )
	qmakeSqlPlugins += "db2";

    if ( dictionary[ "SHARED" ] == "yes" )
	qmakeVars += "QMAKE_QT_VERSION_OVERRIDE=" + dictionary[ "VERSION" ];

    if( !dictionary[ "QT_INSTALL_HEADERS" ] )
	dictionary[ "QT_INSTALL_HEADERS" ] = QDir::convertSeparators( dictionary[ "QT_INSTALL_PREFIX" ] + "/include" );

    if( !dictionary[ "QT_INSTALL_DOCS" ] )
	dictionary[ "QT_INSTALL_DOCS" ] = QDir::convertSeparators( dictionary[ "QT_INSTALL_PREFIX" ] + "/doc" );

    if( !dictionary[ "QT_INSTALL_PLUGINS" ] )
	dictionary[ "QT_INSTALL_PLUGINS" ] = QDir::convertSeparators( dictionary[ "QT_INSTALL_PREFIX" ] + "/plugins" );

    if( !dictionary[ "QT_INSTALL_LIBS" ] )
	dictionary[ "QT_INSTALL_LIBS" ] = QDir::convertSeparators( dictionary[ "QT_INSTALL_PREFIX" ] + "/lib" );

    if( !dictionary[ "QT_INSTALL_BINS" ] )
	dictionary[ "QT_INSTALL_BINS" ] = QDir::convertSeparators( dictionary[ "QT_INSTALL_PREFIX" ] + "/bin" );

    if( !dictionary[ "QT_INSTALL_DATA" ] )
	dictionary[ "QT_INSTALL_DATA" ] = QDir::convertSeparators( dictionary[ "QT_INSTALL_PREFIX" ] );

    qmakeVars += QString( "OBJECTS_DIR=" ) + QDir::convertSeparators( "tmp/obj/" + dictionary[ "QMAKE_OUTDIR" ] );
    qmakeVars += QString( "MOC_DIR=" ) + QDir::convertSeparators( "tmp/moc/" + dictionary[ "QMAKE_OUTDIR" ] );

    qmakeVars += QString( "DEFINES+=" ) + qmakeDefines.join( " " );
    qmakeVars += QString( "INCLUDEPATH+=" ) + qmakeIncludes.join( " " );
    qmakeVars += QString( "sql-drivers+=" ) + qmakeSql.join( " " );
    qmakeVars += QString( "sql-plugins+=" ) + qmakeSqlPlugins.join( " " );
    qmakeVars += QString( "styles+=" ) + qmakeStyles.join( " " );
    qmakeVars += QString( "style-plugins+=" ) + qmakeStylePlugins.join( " " );
    qmakeVars += QString( "imageformat-plugins+=" ) + qmakeFormatPlugins.join( " " );

    if( licenseInfo[ "PRODUCTS" ].length() )
	qmakeVars += QString( "QT_PRODUCT=" ) + licenseInfo[ "PRODUCTS" ];

    if( !dictionary[ "QMAKESPEC" ].length() ) {
	cout << "QMAKESPEC must either be defined as an environment variable, or specified" << endl;
	cout << "as an argument with -spec" << endl;
	dictionary[ "HELP" ] = "yes";

	QStringList winPlatforms;
	QDir mkspecsDir( dictionary[ "QT_SOURCE_TREE" ] + "\\mkspecs" );
	const QFileInfoList* specsList = mkspecsDir.entryInfoList();
	QFileInfoListIterator it( *specsList );
	QFileInfo* fi;

	while( ( fi = it.current() ) ) {
	    if( fi->fileName().left( 5 ) == "win32" ) {
		winPlatforms += fi->fileName();
	    }
	    ++it;
	}
	cout << "Available platforms are: " << winPlatforms.join( ", " ).latin1() << endl;
    }
}

#if !defined(EVAL)
void Configure::generateCachefile()
{
    // Generate .qmake.cache
    QFile cacheFile( dictionary[ "QT_SOURCE_TREE" ] + "\\.qmake.cache" );
    if( cacheFile.open( IO_WriteOnly | IO_Translate ) ) { // Truncates any existing file.
	QTextStream cacheStream( &cacheFile );
        for( QStringList::Iterator var = qmakeVars.begin(); var != qmakeVars.end(); ++var ) {
	    cacheStream << (*var) << endl;
	}
	cacheStream << "CONFIG+=" << qmakeConfig.join( " " ) << " incremental create_prl link_prl" << endl;
	cacheStream << "QMAKESPEC=" << dictionary[ "QMAKESPEC" ] << endl;
	cacheStream << "ARCH=i386" << endl; //### need to detect platform
	cacheStream << "QT_BUILD_TREE=" << dictionary[ "QT_SOURCE_TREE" ] << endl;
	cacheStream << "QT_SOURCE_TREE=" << dictionary[ "QT_SOURCE_TREE" ] << endl;
	cacheStream << "QT_INSTALL_PREFIX=" << dictionary[ "QT_INSTALL_PREFIX" ] << endl;
	cacheStream << "QT_INSTALL_TRANSLATIONS=" << dictionary[ "QT_INSTALL_TRANSLATIONS" ] << endl;
	cacheStream << "docs.path=" << dictionary[ "QT_INSTALL_DOCS" ] << endl;
	cacheStream << "headers.path=" << dictionary[ "QT_INSTALL_HEADERS" ] << endl;
	cacheStream << "plugins.path=" << dictionary[ "QT_INSTALL_PLUGINS" ] << "/plugins" << endl;
	cacheStream << "libs.path=" << dictionary[ "QT_INSTALL_LIBS" ] << "/lib" << endl;
	cacheStream << "bins.path=" << dictionary[ "QT_INSTALL_BINS" ] << "/bin" << endl;
	cacheStream << "data.path=" << dictionary[ "QT_INSTALL_DATA" ] << endl;
	cacheStream << "translations.path=" << dictionary[ "QT_INSTALL_TRANSLATIONS" ] << endl;

	cacheFile.close();
    }
    QFile configFile( dictionary[ "QT_SOURCE_TREE" ] + "\\.qtwinconfig" );
    if( configFile.open( IO_WriteOnly | IO_Translate ) ) { // Truncates any existing file.
	QTextStream configStream( &configFile );
	configStream << "CONFIG+=";
	if( dictionary[ "SHARED" ] == "yes" )
	    configStream << " shared";
	if ( dictionary[ "THREAD" ] == "yes" )
	    configStream << " thread";
	if ( dictionary[ "DEBUG" ] == "yes" )
	    configStream << " debug";
	else
	    configStream << " release";
	if( dictionary[ "STL" ] == "yes" )
	    configStream << " stl";
	if ( dictionary[ "EXCEPTIONS" ] == "yes" )
	    configStream << " exceptions";
	if ( dictionary[ "RTTI" ] == "yes" )
	    configStream << " rtti";
	configStream << endl;
	configFile.close();
    }
}
#endif

#if !defined(EVAL)
void Configure::generateConfigfiles()
{
    QString outDir( dictionary[ "QT_INSTALL_HEADERS" ] );

    if( dictionary[ "QMAKE_INTERNAL" ] == "yes" )
	outDir = dictionary[ "QT_SOURCE_TREE" ] + "/src/tools";

    QString outName( outDir + "/qconfig.h" );

    ::SetFileAttributesA( outName, FILE_ATTRIBUTE_NORMAL );
    QFile::remove( outName );
    QFile outFile( outName );

    if( outFile.open( IO_WriteOnly | IO_Translate ) ) {
	QTextStream outStream( &outFile );

	if( dictionary[ "QCONFIG" ] == "full" ) {
	    outStream << "// Everything" << endl << endl;
	    if( dictionary[ "SHARED" ] == "yes" ) {
		outStream << "#ifndef QT_DLL" << endl;
		outStream << "#define QT_DLL" << endl;
		outStream << "#endif" << endl;
	    }
	} else {
	    QString configName( "qconfig-" + dictionary[ "QCONFIG" ] + ".h" );
	    outStream << "// Copied from " << configName << endl;

	    QFile inFile( dictionary[ "QT_SOURCE_TREE" ] + "/src/tools/" + configName );
	    if( inFile.open( IO_ReadOnly ) ) {
		QByteArray buffer = inFile.readAll();
		outFile.writeBlock( buffer.data(), buffer.size() );
		inFile.close();
	    }
	}
	outStream << endl;
	outStream << "/* License information */" << endl;
	outStream << "#define QT_PRODUCT_LICENSEE \"" << licenseInfo[ "LICENSEE" ] << "\"" << endl;
	outStream << "#define QT_PRODUCT_LICENSE \"" << licenseInfo[ "PRODUCTS" ] << "\"" << endl;

	outStream << "// Compile time features" << endl;
	if( dictionary[ "STL" ] == "no" ) {
	    outStream << "#ifndef QT_NO_STL" << endl;
	    outStream << "#define QT_NO_STL" << endl;
	    outStream << "#endif" << endl;
	}
	if ( dictionary[ "STYLE_WINDOWS" ] == "no" ) {
	    outStream << "#ifndef QT_NO_STYLE_WINDOWS" << endl;
	    outStream << "#define QT_NO_STYLE_WINDOWS" << endl;
	    outStream << "#endif" << endl;
	}
	if ( dictionary[ "STYLE_WINDOWSXP" ] == "no" ) {
	    outStream << "#ifndef QT_NO_STYLE_WINDOWSXP" << endl;
	    outStream << "#define QT_NO_STYLE_WINDOWSXP" << endl;
	    outStream << "#endif" << endl;
	}
	if ( dictionary[ "STYLE_MOTIF" ] == "no" ) {
	    outStream << "#ifndef QT_NO_STYLE_MOTIF" << endl;
	    outStream << "#define QT_NO_STYLE_MOTIF" << endl;
	    outStream << "#endif" << endl;
	}
	if ( dictionary[ "STYLE_MOTIFPLUS" ] == "no" ) {
	    outStream << "#ifndef QT_NO_STYLE_MOTIFPLUS" << endl;
	    outStream << "#define QT_NO_STYLE_MOTIFPLUS" << endl;
	    outStream << "#endif" << endl;
	}
	if ( dictionary[ "STYLE_PLATINUM" ] == "no" ) {
	    outStream << "#ifndef QT_NO_STYLE_PLATINUM" << endl;
	    outStream << "#define QT_NO_STYLE_PLATINUM" << endl;
	    outStream << "#endif" << endl;
	}
	if ( dictionary[ "STYLE_SGI" ] == "no" ) {
	    outStream << "#ifndef QT_NO_STYLE_SGI" << endl;
	    outStream << "#define QT_NO_STYLE_SGI" << endl;
	    outStream << "#endif" << endl;
	}
	if ( dictionary[ "STYLE_CDE" ] == "no" ) {
	    outStream << "#ifndef QT_NO_STYLE_CDE" << endl;
	    outStream << "#define QT_NO_STYLE_CDE" << endl;
	    outStream << "#endif" << endl;
	}
	if( dictionary[ "QWINEXPORT" ] == "yes" ) {
	    outStream << "#ifndef QT_QWINEXPORT" << endl;
	    outStream << "#define QT_QWINEXPORT" << endl;
	    outStream << "#endif" << endl;
	}
	outFile.close();
	if( dictionary[ "QMAKE_INTERNAL" ] == "yes" ) {
	    if ( !CopyFileA( outName, dictionary[ "QT_INSTALL_HEADERS" ] + "/qconfig.h", FALSE ) )
		qDebug("Couldn't copy %s to include", outName.latin1() );
	    ::SetFileAttributesA( outName, FILE_ATTRIBUTE_READONLY );
	}
    }

    QString archFile = dictionary[ "QT_SOURCE_TREE" ] + "/arch/i386/arch/qatomic.h";
    QDir archhelper;
    archhelper.mkdir(dictionary[ "QT_INSTALL_HEADERS" ] + "/arch");
    if (!CopyFileA(archFile, dictionary[ "QT_INSTALL_HEADERS" ] + "/arch/qatomic.h", FALSE))
	qDebug("Couldn't copy %s to include/arch", archFile.latin1() );

    outName = outDir + "/qmodules.h";

    ::SetFileAttributesA( outName, FILE_ATTRIBUTE_NORMAL );
    QFile::remove( outName );
    outFile.setName( outName );

    if( outFile.open( IO_WriteOnly | IO_Translate ) ) {
	QTextStream outStream( &outFile );

	outStream << "// These modules are present in this configuration of Qt" << endl;
	for( QStringList::Iterator it = modules.begin(); it != modules.end(); ++it ) {
	    outStream << "#define QT_MODULE_" << (*it).upper() << endl;
	}
	outStream << endl;
	outFile.close();
	if( dictionary[ "QMAKE_INTERNAL" ] == "yes" ) {
	    if ( !CopyFileA( outName, dictionary[ "QT_INSTALL_HEADERS" ] + "/qmodules.h", FALSE ) )
		qDebug("Couldn't copy %s to include", outName.latin1() );
	    ::SetFileAttributesA( outName, FILE_ATTRIBUTE_READONLY );
	}
    }

    outDir = dictionary[ "QT_SOURCE_TREE" ];
    outName = outDir + "/src/tools/qconfig.cpp";
    ::SetFileAttributesA( outName, FILE_ATTRIBUTE_NORMAL );
    QFile::remove( outName );
    outFile.setName( outName );

    if( outFile.open( IO_WriteOnly | IO_Translate ) ) {
	QTextStream outStream( &outFile );
	outStream << "#include <qglobal.h>" << endl << endl;

	outStream << "/* Install paths from configure */" << endl;

	outStream << "static const char QT_INSTALL_PREFIX [267] = \"qt_nstpath="
		  << QString(dictionary["QT_INSTALL_PREFIX"]).replace( "\\", "\\\\" ) << "\";" << endl;
	outStream << "static const char QT_INSTALL_BINS   [267] = \"qt_binpath="
		  << QString(dictionary["QT_INSTALL_BINS"]).replace( "\\", "\\\\" )  << "\";" << endl;
	outStream << "static const char QT_INSTALL_DOCS   [267] = \"qt_docpath="
		  << QString(dictionary["QT_INSTALL_DOCS"]).replace( "\\", "\\\\" )  << "\";" << endl;
	outStream << "static const char QT_INSTALL_HEADERS[267] = \"qt_hdrpath="
		  << QString(dictionary["QT_INSTALL_HEADERS"]).replace( "\\", "\\\\" )  << "\";" << endl;
	outStream << "static const char QT_INSTALL_LIBS   [267] = \"qt_libpath="
		  << QString(dictionary["QT_INSTALL_LIBS"]).replace( "\\", "\\\\" )  << "\";" << endl;
	outStream << "static const char QT_INSTALL_PLUGINS[267] = \"qt_plgpath="
		  << QString(dictionary["QT_INSTALL_PLUGINS"]).replace( "\\", "\\\\" )  << "\";" << endl;
	outStream << "static const char QT_INSTALL_DATA   [267] = \"qt_datpath="
		  << QString(dictionary["QT_INSTALL_DATA"]).replace( "\\", "\\\\" )  << "\";" << endl;
	outStream << "static const char QT_INSTALL_TRANSLATIONS [267] = \"qt_trapath="
		  << QString(dictionary["QT_INSTALL_TRANSLATIONS"]).replace( "\\", "\\\\" )  << "\";" << endl;

	outStream << endl;
	outStream << "/* strlen( \"qt_xxxpath=\" ) == 11 */" << endl;
	outStream << "const char *qInstallPath()        { return QT_INSTALL_PREFIX  + 11; }" << endl;
	outStream << "const char *qInstallPathDocs()    { return QT_INSTALL_DOCS    + 11; }" << endl;
	outStream << "const char *qInstallPathHeaders() { return QT_INSTALL_HEADERS + 11; }" << endl;
	outStream << "const char *qInstallPathLibs()    { return QT_INSTALL_LIBS    + 11; }" << endl;
	outStream << "const char *qInstallPathBins()    { return QT_INSTALL_BINS    + 11;    }" << endl;
	outStream << "const char *qInstallPathPlugins() { return QT_INSTALL_PLUGINS + 11; }" << endl;
	outStream << "const char *qInstallPathData()    { return QT_INSTALL_DATA    + 11;    }" << endl;
	outStream << "const char *qInstallPathTranslations() { return QT_INSTALL_TRANSLATIONS + 11; }" << endl;

	outFile.close();
    }

    outDir = dictionary[ "QT_SOURCE_TREE" ];
    outName = outDir + "/src/qt.rc";
    ::SetFileAttributesA( outName, FILE_ATTRIBUTE_NORMAL );
    QFile::remove( outName );
    outFile.setName( outName );

    if( outFile.open( IO_WriteOnly | IO_Translate ) ) {
	QTextStream outStream( &outFile );

	int major = QT_VERSION >> 16;
	int minor = QT_VERSION >> 8 & 0xff;
	int patch = QT_VERSION&0xff;
	QString prodVer = "0x" + QString::number(major, 16) + ", " +
			  "0x" + QString::number(minor, 16) + ", " +
			  "0x" + QString::number(patch, 16) + ", 0";
	QString prodFile = "qt";
	if ( dictionary["THREAD"] == "yes" )
	    prodFile += "-mt";
	if ( dictionary["SHARED"] == "yes" )
	    prodFile += dictionary["VERSION"];
	prodFile += ".dll";

	QString internalName = licenseInfo["PRODUCTS"];

	outStream << "#ifndef Q_CC_BOR" << endl; 
	outStream << "# if defined(UNDER_CE) && UNDER_CE >= 400" << endl; 
	outStream << "#  include <winbase.h>" << endl; 
	outStream << "# else" << endl; 
	outStream << "#  include <winver.h>" << endl; 
	outStream << "# endif" << endl; 
	outStream << "#endif" << endl << endl;
	outStream << "VS_VERSION_INFO VERSIONINFO" << endl;
	outStream << "\tFILEVERSION 1,0,0,1" << endl;
	outStream << "\tPRODUCTVERSION " << prodVer << endl;
	outStream << "\tFILEFLAGSMASK 0x3fL" << endl;
	outStream << "#ifdef _DEBUG" << endl;
	outStream << "\tFILEFLAGS VS_FF_DEBUG" << endl;
	outStream << "#else" << endl;
	outStream << "\tFILEFLAGS 0x0L" << endl;
	outStream << "#endif" << endl;
	outStream << "\tFILEOS VOS__WINDOWS32" << endl;
	outStream << "\tFILETYPE VFT_DLL" << endl;
	outStream << "\tFILESUBTYPE 0x0L" << endl;
	outStream << "\tBEGIN" << endl;
	outStream << "\t\tBLOCK \"StringFileInfo\"" << endl;
	outStream << "\t\tBEGIN" << endl;
	outStream << "\t\t\tBLOCK \"040904B0\"" << endl;
	outStream << "\t\t\tBEGIN" << endl;
	outStream << "\t\t\t\tVALUE \"CompanyName\", \"Trolltech AS\\0\"" << endl;
	outStream << "\t\t\t\tVALUE \"FileDescription\", \"Qt\\0\"" << endl;
	outStream << "\t\t\t\tVALUE \"FileVersion\", \"1,0,0,1\\0\"" << endl;
	outStream << "\t\t\t\tVALUE \"InternalName\", \"" << internalName << "\\0\"" << endl;
	outStream << "\t\t\t\tVALUE \"LegalCopyright\", \"Copyright (C) 2003 Trolltech\\0\"" << endl;
	outStream << "\t\t\t\tVALUE \"LegalTrademarks\", \"\\0\"" << endl;
	outStream << "\t\t\t\tVALUE \"OriginalFilename\", \"" << prodFile << "\\0\"" << endl;
	outStream << "\t\t\t\tVALUE \"ProductName\", \"Qt\\0\"" << endl;
	outStream << "\t\t\t\tVALUE \"ProductVersion\", \""<< prodVer << "\\0\"" << endl;
	outStream << "\t\t\tEND" << endl;
	outStream << "\t\tEND" << endl;
	outStream << "\tEND" << endl;
	outStream << "/* End of Version info */" << endl << endl;
    }
}
#endif

#if !defined(EVAL)
void Configure::displayConfig()
{
    // Give some feedback
    if( QFile::exists( dictionary[ "QT_SOURCE_TREE" ] + "/LICENSE.TROLL" ) ) {
	cout << "Trolltech license file used." << dictionary[ "QT_SOURCE_TREE" ] + "/LICENSE.TROLL" << endl;
    } else if ( QFile::exists( QDir::homeDirPath() + "/.qt-license" ) ) {
	QString l1 = licenseInfo[ "LICENSEE" ];
	QString l2 = licenseInfo[ "LICENSEID" ];
	QString l3 = licenseInfo[ "PRODUCTS" ];
	QString l4 = licenseInfo[ "EXPIRYDATE" ];
	cout << "Licensee...................." << (l1.isNull() ? "" : l1) << endl;
	cout << "License ID.................." << (l2.isNull() ? "" : l2) << endl;
	cout << "Product license............." << (l3.isNull() ? "" : l3) << endl;
	cout << "Expiry Date................." << (l4.isNull() ? "" : l4) << endl << endl;
    }

    cout << "QMAKESPEC..................." << dictionary[ "QMAKESPEC" ] << endl;
    cout << "Maketool...................." << dictionary[ "MAKE" ] << endl;
    cout << "Configuration..............." << qmakeConfig.join( " " ) << endl;

    cout << "Debug symbols..............." << dictionary[ "DEBUG" ] << endl;
    cout << "Thread support.............." << dictionary[ "THREAD" ] << endl << endl;

    cout << "Accessibility support......." << dictionary[ "ACCESSIBILITY" ] << endl;
    cout << "Big Textcodecs.............." << dictionary[ "BIG_CODECS" ] << endl;
    cout << "Tablet support.............." << dictionary[ "TABLET" ] << endl;
    cout << "STL support................." << dictionary[ "STL" ] << endl;
    cout << "Additional exports.........." << dictionary[ "QWINEXPORT" ] << endl;
    cout << "Exception support..........." << dictionary[ "EXCEPTIONS" ] << endl;
    cout << "RTTI support................" << dictionary[ "RTTI" ] << endl;
    cout << "OpenGL support.............." << dictionary[ "OPENGL" ] << endl << endl;

    cout << "Image formats:" << endl;
    cout << "GIF support................." << dictionary[ "GIF" ] << endl;
    cout << "MNG support................." << dictionary[ "MNG" ] << endl;
    cout << "JPEG support................" << dictionary[ "JPEG" ] << endl;
    cout << "PNG support................." << dictionary[ "PNG" ] << endl << endl;

    cout << "Styles:" << endl;
WCE( { cout << "PocketPC...................." << dictionary[ "STYLE_POCKETPC" ] << endl; } );
    cout << "Windows....................." << dictionary[ "STYLE_WINDOWS" ] << endl;
    cout << "Windows XP.................." << dictionary[ "STYLE_WINDOWSXP" ] << endl;
    cout << "Motif......................." << dictionary[ "STYLE_MOTIF" ] << endl;
    cout << "Platinum...................." << dictionary[ "STYLE_PLATINUM" ] << endl;
    cout << "MotifPlus..................." << dictionary[ "STYLE_MOTIFPLUS" ] << endl;
    cout << "CDE........................." << dictionary[ "STYLE_CDE" ] << endl;
    cout << "SGI........................." << dictionary[ "STYLE_SGI" ] << endl << endl;
    // Only show the PocketPC style option for CE users

    cout << "Sql Drivers:" << endl;
    cout << "ODBC........................" << dictionary[ "SQL_ODBC" ] << endl;
    cout << "MySQL......................." << dictionary[ "SQL_MYSQL" ] << endl;
    cout << "OCI........................." << dictionary[ "SQL_OCI" ] << endl;
    cout << "PostgreSQL.................." << dictionary[ "SQL_PSQL" ] << endl;
    cout << "TDS........................." << dictionary[ "SQL_TDS" ] << endl;
    cout << "DB2........................." << dictionary[ "SQL_DB2" ] << endl << endl;

    cout << "Sources are in.............." << dictionary[ "QT_SOURCE_TREE" ] << endl;
    cout << "Install prefix.............." << dictionary[ "QT_INSTALL_PREFIX" ] << endl;
    cout << "Headers installed to........" << dictionary[ "QT_INSTALL_HEADERS" ] << endl;
    cout << "Libraries installed to......" << dictionary[ "QT_INSTALL_LIBS" ] << endl;
    cout << "Plugins installed to........" << dictionary[ "QT_INSTALL_PLUGINS" ] << endl;
    cout << "Binaries installed to......." << dictionary[ "QT_INSTALL_BINS" ] << endl;
    cout << "Docs installed to..........." << dictionary[ "QT_INSTALL_DOCS" ] << endl;
    cout << "Data installed to..........." << dictionary[ "QT_INSTALL_DATA" ] << endl;
    cout << "Translations installed to..." << dictionary[ "QT_INSTALL_TRANSLATIONS" ] << endl << endl;

    cout << endl;
    if( !qmakeDefines.isEmpty() ) {
	cout << "Defines.....................";
	for( QStringList::Iterator defs = qmakeDefines.begin(); defs != qmakeDefines.end(); ++defs )
	    cout << (*defs) << " ";
	cout << endl;
    }
    if( !qmakeIncludes.isEmpty() ) {
	cout << "Include paths...............";
	for( QStringList::Iterator incs = qmakeIncludes.begin(); incs != qmakeIncludes.end(); ++incs )
	    cout << (*incs) << " ";
	cout << endl;
    }
    if( !qmakeLibs.isEmpty() ) {
	cout << "Additional libraries........";
	for( QStringList::Iterator libs = qmakeLibs.begin(); libs != qmakeLibs.end(); ++libs )
	    cout << (*libs) << " ";
	cout << endl;
    }
    if( dictionary[ "FORCE_PROFESSIONAL" ] == "yes" ) {
	cout << "Licensing forced to professional edition.  If this is not what you want, unset" << endl;
	cout << "the FORCE_PROFESSIONAL environment variable." << endl;
    }
    if( dictionary[ "QMAKE_INTERNAL" ] == "yes" ) {
	cout << "Using internal configuration." << endl;
    }
    if( dictionary[ "SHARED" ] == "no" ) {
	cout << "WARNING: Using static linking will disable the use of plugins." << endl;
	cout << "         Make sure you compile ALL needed modules into the library." << endl;
    }
}
#endif

#if !defined(EVAL)
void Configure::buildQmake()
{
    if( dictionary[ "BUILD_QMAKE" ] == "yes" ) {
	QStringList args;

	// Build qmake
	cout << "Creating qmake..." << endl;
	QString pwd = QDir::currentDirPath();
	QDir::setCurrent( dictionary[ "QT_SOURCE_TREE" ] + "/qmake" );
	args += dictionary[ "MAKE" ];
	args += "-f";
	args += dictionary[ "QMAKEMAKEFILE" ];
	if( int r = system( args.join( " " ).latin1() ) ) {
	    cout << "Building qmake failed, return code " << r << endl << endl;
	    dictionary[ "DONE" ] = "yes";
	}
	QDir::setCurrent( pwd );
    }
}
#endif

void Configure::findProjects( const QString& dirName )
{
    QDir dir( dirName );
    QString entryName;
    const QFileInfoList* list = dir.entryInfoList();
    QFileInfoListIterator it( *list );
    QFileInfo* fi;
    int makeListNumber;
    ProjectType qmakeTemplate;

    if( dictionary[ "NOPROCESS" ] == "no" ) {
	while( ( fi = it.current() ) ) {
	    if( fi->fileName()[ 0 ] != '.' && fi->fileName() != "qmake.pro" ) {
		entryName = dirName + "/" + fi->fileName();
		if( fi->isDir() ) {
		    findProjects( entryName );
		} else {
		    if( fi->fileName().right( 4 ) == ".pro" ) {
			if ( fi->fileName() != "qtmain.pro" && fi->fileName() != "qt.pro" ) {
			    qmakeTemplate = projectType( fi->absFilePath() );
			    switch ( qmakeTemplate ) {
				case Lib:
				case Subdirs:
				    makeListNumber = 1;
				    break;
				default:
				    makeListNumber = 2;
				    break;
			    }
			    makeList[makeListNumber].append( new MakeItem(
					dirName,
					fi->fileName(),
					"Makefile",
					qmakeTemplate ) );
			    if( dictionary[ "DSPFILES" ] == "yes" ) {
				makeList[makeListNumber].append( new MakeItem(
					    dirName,
					    fi->fileName(),
					    fi->fileName().left( fi->fileName().length() - 4 ) + ".dsp",
					    qmakeTemplate ) );
			    }
			    if( dictionary[ "VCPFILES" ] == "yes" ) {
				makeList[makeListNumber].append( new MakeItem(
					    dirName,
					    fi->fileName(),
					    fi->fileName().left( fi->fileName().length() - 4 ) + ".vcp",
					    qmakeTemplate ) );
			    }
			    if( dictionary[ "VCPROJFILES" ] == "yes" ) {
				makeList[makeListNumber].append( new MakeItem(
					    dirName,
					    fi->fileName(),
					    fi->fileName().left( fi->fileName().length() - 4 ) + ".vcproj",
					    qmakeTemplate ) );
			    }
			}
		    }
		}
	    }
	    ++it;
	}
    }
}

void Configure::generateMakefiles()
{
    if( dictionary[ "NOPROCESS" ] == "no" ) {
#if !defined(EVAL)
	cout << "Creating makefiles in src..." << endl;
#endif

	QString spec = dictionary[ "QMAKESPEC" ];
	if( spec != "win32-msvc" )
	    dictionary[ "DSPFILES" ] = "no";

	if( !spec.startsWith( "wince-" ) )
	    dictionary[ "VCPFILES" ] = "no";

	if( spec != "win32-msvc.net" )
	    dictionary[ "VCPROJFILES" ] = "no";

#if !defined(EVAL)
	makeList[0].append( new MakeItem(
		    dictionary[ "QT_SOURCE_TREE" ] + "/src",
		    "qt.pro",
		    "Makefile",
		    Lib ) );
	if( dictionary[ "DSPFILES" ] == "yes" ) {
	    makeList[0].append( new MakeItem(
			dictionary[ "QT_SOURCE_TREE" ] + "/src",
			"qt.pro",
			"qt.dsp",
			Lib ) );
	}
	if( dictionary[ "VCPFILES" ] == "yes" ) {
	    makeList[0].append( new MakeItem(
			dictionary[ "QT_SOURCE_TREE" ] + "/src",
			"qt.pro",
			"qt.vcp",
			Lib ) );
	}
	if( dictionary[ "VCPROJFILES" ] == "yes" ) {
	    makeList[0].append( new MakeItem(
			dictionary[ "QT_SOURCE_TREE" ] + "/src",
			"qt.pro",
			"qt.vcproj",
			Lib ) );
	}
	makeList[0].append( new MakeItem(
		    dictionary[ "QT_SOURCE_TREE" ] + "/src",
		    "qtmain.pro",
		    "Makefile.main",
		    Lib ) );
	if( dictionary[ "DSPFILES" ] == "yes" ) {
	    makeList[0].append( new MakeItem(
			dictionary[ "QT_SOURCE_TREE" ] + "/src",
			"qtmain.pro",
			"qtmain.dsp",
			Lib ) );
	}
	if( dictionary[ "VCPFILES" ] == "yes" ) {
	    makeList[0].append( new MakeItem(
			dictionary[ "QT_SOURCE_TREE" ] + "/src",
			"qtmain.pro",
			"qtmain.vcp",
			Lib ) );
	}
	if( dictionary[ "VCPROJFILES" ] == "yes" ) {
	    makeList[0].append( new MakeItem(
			dictionary[ "QT_SOURCE_TREE" ] + "/src",
			"qtmain.pro",
			"qtmain.vcproj",
			Lib ) );
	}
#endif
	if( dictionary[ "LEAN" ] == "no" )
	    findProjects( dictionary[ "QT_SOURCE_TREE" ] );

	QString pwd = QDir::currentDirPath();
	for ( int i=0; i<3; i++ ) {
	    for ( MakeItem *it=makeList[i].first(); it; it=makeList[i].next() ) {
		QString dirPath = QDir::convertSeparators( it->directory + "/" );
		QString projectName = dirPath + it->proFile;
		QString makefileName = dirPath + it->target;
		QStringList args;

		args << QDir::convertSeparators( dictionary[ "QT_INSTALL_BINS" ] + "/qmake" );
		args << projectName;
		args << dictionary[ "QMAKE_ALL_ARGS" ];

		if ( makefileName.endsWith( ".dsp" ) ||
		     makefileName.endsWith( ".vcp" ) ||
		     makefileName.endsWith( ".vcproj" ) ) {
		    // We don't create projects for sub directories, continue
		    if ( it->qmakeTemplate == Subdirs )
			continue;
		    if( dictionary[ "DEPENDENCIES" ] == "no" )
			args << "-nodepend";
		    args << "-spec";
		    args << dictionary[ "QMAKESPEC" ];
		    args << "-tp vc";
		} else {
		    cout << "For " << projectName.latin1() << endl;
		    args << "-o";
		    args << makefileName;
		}

		QDir::setCurrent( QDir::convertSeparators( dirPath ) );
		if( int r = system( args.join( " " ).latin1() ) ) {
		    cout << "Qmake failed, return code " << r  << endl << endl;
		    dictionary[ "DONE" ] = "yes";
		}
	    }
	}
	QDir::setCurrent( pwd );
    } else {
	cout << "Processing of project files have been disabled." << endl;
	cout << "Only use this option if you really know what you're doing." << endl << endl;
	dictionary[ "DONE" ] = "yes";
	return;
    }
}

void Configure::showSummary()
{
    QString make = dictionary[ "MAKE" ];
    cout << endl << endl << "Qt is now configured for building. Just run " << make.latin1() << "." << endl;
    cout << "To reconfigure, run " << make.latin1() << " clean and configure." << endl << endl;
}

Configure::ProjectType Configure::projectType( const QString& proFileName )
{
    QFile proFile( proFileName );
    QString buffer;

    if( proFile.open( IO_ReadOnly ) ) {
	while( proFile.readLine( buffer, 1024 ) != -1 ) {
	    QStringList segments = QStringList::split( QRegExp( "\\s" ), buffer );
	    QStringList::Iterator it = segments.begin();

	    if(segments.size() >= 3) {
		QString keyword = (*it++);
		QString operation = (*it++);
		QString value = (*it++);

		if( keyword == "TEMPLATE" ) {
		    if( value == "lib" )
			return Lib;
		    else if( value == "subdirs" )
			return Subdirs;
		}
	    }
	}
	proFile.close();
    }
    // Default to app handling
    return App;
}

#if !defined(EVAL)
void Configure::readLicense()
{
    QFile licenseFile( QDir::homeDirPath() + "/.qt-license" );
    if( licenseFile.open( IO_ReadOnly ) ) {
	cout << "Reading license file in....." << QDir::homeDirPath().latin1() << endl;
	QString buffer;

	while( licenseFile.readLine( buffer, 1024 ) > 0 ) {
	    if( buffer[ 0 ] != '#' ) {
		QStringList components = QStringList::split( '=', buffer );
		if ( components.size() >= 2 ) {
		    QStringList::Iterator it = components.begin();
		    QString key = (*it++).stripWhiteSpace().replace( "\"", QString::null ).upper();
		    QString value = (*it++).stripWhiteSpace().replace( "\"", QString::null );
		    licenseInfo[ key ] = value;
		}
	    }
	}
	licenseFile.close();
    }
    if( QFile::exists( dictionary[ "QT_SOURCE_TREE" ] + "/LICENSE.TROLL" ) ) {
	licenseInfo[ "PRODUCTS" ] = "qt-internal";
	dictionary[ "QMAKE_INTERNAL" ] = "yes";
    } else if ( !licenseFile.exists() ) {
	cout << "License file not found in " << QDir::homeDirPath() << endl;
	cout << "Enterprise modules will not be available." << endl << endl;
	licenseInfo[ "PRODUCTS" ] = "qt-professional";
    }

    if( dictionary[ "FORCE_PROFESSIONAL" ] == "yes" )
        licenseInfo[ "PRODUCTS" ]= "qt-professional";
}
#endif

#if !defined(EVAL)
void Configure::reloadCmdLine()
{
    if( dictionary[ "REDO" ] == "yes" ) {
	QFile inFile( dictionary[ "QT_SOURCE_TREE" ] + "/configure" + dictionary[ "CUSTOMCONFIG" ] + ".cache" );
	if( inFile.open( IO_ReadOnly ) ) {
	    QTextStream inStream( &inFile );
	    QString buffer;
	    inStream >> buffer;
	    while( buffer.length() ) {
		configCmdLine += buffer;
		inStream >> buffer;
	    }
	    inFile.close();
	}
    }
}
#endif

#if !defined(EVAL)
void Configure::saveCmdLine()
{
    if( dictionary[ "REDO" ] != "yes" ) {
	QFile outFile( dictionary[ "QT_SOURCE_TREE" ] + "/configure" + dictionary[ "CUSTOMCONFIG" ] + ".cache" );
	if( outFile.open( IO_WriteOnly ) ) {
	    QTextStream outStream( &outFile );
	    for( QStringList::Iterator it = configCmdLine.begin(); it != configCmdLine.end(); ++it ) {
		outStream << (*it) << " " << endl;
	    }
	    outFile.close();
	}
    }
}
#endif

bool Configure::isDone()
{
    return( dictionary[ "DONE" ] == "yes" );
}
