#include <qmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>

class MakeItem;

class Configure
{
public:
    Configure( int& argc, char** argv );

    void parseCmdLine();
#if !defined(EVAL)
    void buildModulesList();
    void validateArgs();
#endif
    bool displayHelp();
    void generateOutputVars();
#if !defined(EVAL)
    void generateCachefile();
    void displayConfig();
    void buildQmake();
#endif
    void generateMakefiles();
#if !defined(EVAL)
    void generateConfigfiles();
#endif
    void showSummary();
    void findProjects( const QString& dirName );

#if !defined(EVAL)
    void readLicense();
#endif

    enum ProjectType {
	App,
	Lib,
	Subdirs
    };

    ProjectType projectType( const QString& proFileName );
    bool isDone();
private:
    // Our variable dictionaries
    QMap<QString,QString> dictionary;
    QStringList licensedModules;
    QStringList allSqlDrivers;
    QStringList allConfigs;
    QStringList disabledModules;
    QStringList enabledModules;
    QStringList modules;
//    QStringList sqlDrivers;
    QStringList configCmdLine;
    QStringList qmakeConfig;

    QStringList qmakeSql;
    QStringList qmakeSqlPlugins;

    QStringList qmakeStyles;
    QStringList qmakeStylePlugins;

    QStringList qmakeFormatPlugins;

    QStringList qmakeVars;
    QStringList qmakeDefines;
    //  makeList[0] for qt and qtmain
    //  makeList[1] for subdirs and libs
    //  makeList[2] for the rest
    QPtrList<MakeItem> makeList[3];
    QStringList qmakeIncludes;
    QStringList qmakeLibs;

    QMap<QString,QString> licenseInfo;
    QString outputLine;

#if !defined(EVAL)
    void reloadCmdLine();
    void saveCmdLine();
#endif

};
