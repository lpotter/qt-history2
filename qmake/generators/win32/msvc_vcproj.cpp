/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_COMMERCIAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "msvc_vcproj.h"
#include "option.h"
#include "qtmd5.h" // SG's MD5 addon
#include "xmloutput.h"
#include <qdir.h>
#include <qregexp.h>
#include <qhash.h>
#include <quuid.h>
#include <stdlib.h>

//#define DEBUG_SOLUTION_GEN
//#define DEBUG_PROJECT_GEN

// Filter GUIDs (Do NOT change these!) ------------------------------
const char _GUIDSourceFiles[]          = "{4FC737F1-C7A5-4376-A066-2A32D752A2FF}";
const char _GUIDHeaderFiles[]          = "{93995380-89BD-4b04-88EB-625FBE52EBFB}";
const char _GUIDGeneratedFiles[]       = "{71ED8ED8-ACB9-4CE9-BBE1-E00B30144E11}";
const char _GUIDResourceFiles[]        = "{D9D6E242-F8AF-46E4-B9FD-80ECBC20BA3E}";
const char _GUIDLexYaccFiles[]         = "{E12AE0D2-192F-4d59-BD23-7D3FA58D3183}";
const char _GUIDTranslationFiles[]     = "{639EADAA-A684-42e4-A9AD-28FC9BCB8F7C}";
const char _GUIDFormFiles[]            = "{99349809-55BA-4b9d-BF79-8FDBB0286EB3}";
const char _GUIDExtraCompilerFiles[]   = "{E0D8C965-CC5F-43d7-AD63-FAEF0BBC0F85}";

#ifdef Q_OS_WIN32
#include <qt_windows.h>

struct {
    DotNET version;
    const char *versionStr;
    const char *regKey;
} dotNetCombo[] = {
#ifdef Q_OS_WIN64
    {NET2005, "MSVC.NET 2005 (8.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\ProductDir"},
    {NET2003, "MSVC.NET 2003 (7.1)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\7.1\\Setup\\VC\\ProductDir"},
    {NET2002, "MSVC.NET 2002 (7.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\7.0\\Setup\\VC\\ProductDir"},
#else
    {NET2005, "MSVC.NET 2005 (8.0)", "Software\\Microsoft\\VisualStudio\\8.0\\Setup\\VC\\ProductDir"},
    {NET2005, "MSVC 2005 Express Edition(8.0)", "Software\\Microsoft\\VCExpress\\8.0\\Setup\\VC\\ProductDir"},
    {NET2003, "MSVC.NET 2003 (7.1)", "Software\\Microsoft\\VisualStudio\\7.1\\Setup\\VC\\ProductDir"},
    {NET2002, "MSVC.NET 2002 (7.0)", "Software\\Microsoft\\VisualStudio\\7.0\\Setup\\VC\\ProductDir"},
#endif
    {NETUnknown, "", ""},
};

static QString keyPath(const QString &rKey)
{
    int idx = rKey.lastIndexOf(QLatin1Char('\\'));
    if (idx == -1)
        return QString();
    return rKey.left(idx + 1);
}

static QString keyName(const QString &rKey)
{
    int idx = rKey.lastIndexOf(QLatin1Char('\\'));
    if (idx == -1)
        return rKey;

    QString res(rKey.mid(idx + 1));
    if (res == "Default" || res == ".")
        res = "";
    return res;
}

static QString readRegistryKey(HKEY parentHandle, const QString &rSubkey)
{

    QString rSubkeyName = keyName(rSubkey);
    QString rSubkeyPath = keyPath(rSubkey);

    HKEY handle = 0;
    LONG res;
    QT_WA( {
        res = RegOpenKeyExW(parentHandle, (WCHAR*)rSubkeyPath.utf16(),
                            0, KEY_READ, &handle);
    } , {
        res = RegOpenKeyExA(parentHandle, rSubkeyPath.toLocal8Bit(),
                            0, KEY_READ, &handle);
    } );

    if (res != ERROR_SUCCESS)
        return QString();

    // get the size and type of the value
    DWORD dataType;
    DWORD dataSize;
    QT_WA( {
        res = RegQueryValueExW(handle, (WCHAR*)rSubkeyName.utf16(), 0, &dataType, 0, &dataSize);
    }, {
        res = RegQueryValueExA(handle, rSubkeyName.toLocal8Bit(), 0, &dataType, 0, &dataSize);
    } );
    if (res != ERROR_SUCCESS) {
        RegCloseKey(handle);
        return QString();
    }

    // get the value
    QByteArray data(dataSize, 0);
    QT_WA( {
        res = RegQueryValueExW(handle, (WCHAR*)rSubkeyName.utf16(), 0, 0,
                               reinterpret_cast<unsigned char*>(data.data()), &dataSize);
    }, {
        res = RegQueryValueExA(handle, rSubkeyName.toLocal8Bit(), 0, 0,
                               reinterpret_cast<unsigned char*>(data.data()), &dataSize);
    } );
    if (res != ERROR_SUCCESS) {
        RegCloseKey(handle);
        return QString();
    }

    QString result;
    switch (dataType) {
        case REG_EXPAND_SZ:
        case REG_SZ: {
            QT_WA( {
                result = QString::fromUtf16(((const ushort*)data.constData()));
            }, {
                result = QString::fromLatin1(data.constData());
            } );
            break;
        }

        case REG_MULTI_SZ: {
            QStringList l;
            int i = 0;
            for (;;) {
                QString s;
                QT_WA( {
                    s = QString::fromUtf16((const ushort*)data.constData() + i);
                }, {
                    s = QString::fromLatin1(data.constData() + i);
                } );
                i += s.length() + 1;

                if (s.isEmpty())
                    break;
                l.append(s);
            }
	    result = l.join(", ");
            break;
        }

        case REG_NONE:
        case REG_BINARY: {
            QT_WA( {
                result = QString::fromUtf16((const ushort*)data.constData(), data.size()/2);
            }, {
                result = QString::fromLatin1(data.constData(), data.size());
            } );
            break;
        }

        case REG_DWORD_BIG_ENDIAN:
        case REG_DWORD: {
            Q_ASSERT(data.size() == sizeof(int));
            int i;
            memcpy((char*)&i, data.constData(), sizeof(int));
	    result = QString::number(i);
            break;
        }

        default:
            qWarning("QSettings: unknown data %d type in windows registry", dataType);
            break;
    }

    RegCloseKey(handle);
    return result;
}
#endif

DotNET which_dotnet_version()
{
#ifndef Q_OS_WIN32
    return NET2002; // Always generate 7.0 versions on other platforms
#else
    // Only search for the version once
    static DotNET current_version = NETUnknown;
    if(current_version != NETUnknown)
        return current_version;

    // Fallback to .NET 2002
    current_version = NET2002;

    QStringList warnPath;
    int installed = 0;
    int i = 0;
    for(; dotNetCombo[i].version; ++i) {
        QString path = readRegistryKey(HKEY_LOCAL_MACHINE, dotNetCombo[i].regKey);
        if(!path.isEmpty()) {
            ++installed;
            current_version = dotNetCombo[i].version;
			warnPath += QString("%1").arg(dotNetCombo[i].versionStr);
        }
    }

    if (installed < 2)
        return current_version;

    // More than one version installed, search directory path
    QString paths = qgetenv("PATH");
    QStringList pathlist = paths.toLower().split(";");

    i = installed = 0;
    for(; dotNetCombo[i].version; ++i) {
        QString productPath = readRegistryKey(HKEY_LOCAL_MACHINE, dotNetCombo[i].regKey).toLower();
		if (productPath.isEmpty())
			continue;
        QStringList::iterator it;
        for(it = pathlist.begin(); it != pathlist.end(); ++it) {
            if((*it).contains(productPath)) {
                ++installed;
                current_version = dotNetCombo[i].version;
                warnPath += QString("%1 in path").arg(dotNetCombo[i].versionStr);
				break;
            }
        }
    }
	switch(installed) {
	case 1:
		break;
	case 0:
		warn_msg(WarnLogic, "Generator: MSVC.NET: Found more than one version of Visual Studio, but"
				 " none in your path! Fallback to lowest version (%s)", warnPath.join(", ").toLatin1().data());
		break;
	default:
		warn_msg(WarnLogic, "Generator: MSVC.NET: Found more than one version of Visual Studio in"
				 " your path! Fallback to lowest version (%s)", warnPath.join(", ").toLatin1().data());
		break;
	}

    return current_version;
#endif
};

// Flatfile Tags ----------------------------------------------------
const char _slnHeader70[]       = "Microsoft Visual Studio Solution File, Format Version 7.00";
const char _slnHeader71[]       = "Microsoft Visual Studio Solution File, Format Version 8.00";
const char _slnHeader80[]       = "Microsoft Visual Studio Solution File, Format Version 9.00";
                                  // The following UUID _may_ change for later servicepacks...
                                  // If so we need to search through the registry at
                                  // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\7.0\Projects
                                  // to find the subkey that contains a "PossibleProjectExtension"
                                  // containing "vcproj"...
                                  // Use the hardcoded value for now so projects generated on other
                                  // platforms are actually usable.
const char _slnMSVCvcprojGUID[] = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
const char _slnProjectBeg[]     = "\nProject(\"";
const char _slnProjectMid[]     = "\") = ";
const char _slnProjectEnd[]     = "\nEndProject";
const char _slnGlobalBeg[]      = "\nGlobal";
const char _slnGlobalEnd[]      = "\nEndGlobal";
const char _slnSolutionConf[]   = "\n\tGlobalSection(SolutionConfiguration) = preSolution"
                                  "\n\t\tConfigName.0 = Debug"
                                  "\n\t\tConfigName.1 = Release"
                                  "\n\tEndGlobalSection";
const char _slnProjDepBeg[]     = "\n\tGlobalSection(ProjectDependencies) = postSolution";
const char _slnProjDepEnd[]     = "\n\tEndGlobalSection";
const char _slnProjConfBeg[]    = "\n\tGlobalSection(ProjectConfiguration) = postSolution";
const char _slnProjRelConfTag1[]= ".Release.ActiveCfg = Release|Win32";
const char _slnProjRelConfTag2[]= ".Release.Build.0 = Release|Win32";
const char _slnProjDbgConfTag1[]= ".Debug.ActiveCfg = Debug|Win32";
const char _slnProjDbgConfTag2[]= ".Debug.Build.0 = Debug|Win32";
const char _slnProjConfEnd[]    = "\n\tEndGlobalSection";
const char _slnExtSections[]    = "\n\tGlobalSection(ExtensibilityGlobals) = postSolution"
                                  "\n\tEndGlobalSection"
                                  "\n\tGlobalSection(ExtensibilityAddIns) = postSolution"
                                  "\n\tEndGlobalSection";
// ------------------------------------------------------------------

VcprojGenerator::VcprojGenerator() : Win32MakefileGenerator(), init_flag(false)
{
}
bool VcprojGenerator::writeMakefile(QTextStream &t)
{
    initProject(); // Fills the whole project with proper data

    // Generate solution file
    if(project->first("TEMPLATE") == "vcsubdirs") {
        if (!project->isActiveConfig("build_pass")) {
            debug_msg(1, "Generator: MSVC.NET: Writing solution file");
            writeSubDirs(t);
        } else {
            debug_msg(1, "Generator: MSVC.NET: Not writing solution file for build_pass configs");
        }
        return true;
    } else
    // Generate single configuration project file
    if((project->first("TEMPLATE") == "vcapp" ||
        project->first("TEMPLATE") == "vclib")) {
        if(!project->isActiveConfig("build_pass")) {
            debug_msg(1, "Generator: MSVC.NET: Writing single configuration project file");
            XmlOutput xmlOut(t);
            xmlOut << vcProject;
        }
        return true;
    }
    return project->isActiveConfig("build_pass");
}

bool VcprojGenerator::writeProjectMakefile()
{
    usePlatformDir();
    QTextStream t(&Option::output);

    // Check if all requirements are fullfilled
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        fprintf(stderr, "Project file not generated because all requirements not met:\n\t%s\n",
                var("QMAKE_FAILED_REQUIREMENTS").toLatin1().constData());
        return true;
    }

    // Generate project file
    if(project->first("TEMPLATE") == "vcapp" ||
       project->first("TEMPLATE") == "vclib") {
        if (!mergedProjects.count()) {
            warn_msg(WarnLogic, "Generator: MSVC.NET: no single configuration created, cannot output project!");
            return false;
        }

        debug_msg(1, "Generator: MSVC.NET: Writing project file");
        VCProject mergedProject;
        for (int i = 0; i < mergedProjects.count(); ++i) {
            VCProjectSingleConfig *singleProject = &(mergedProjects.at(i)->vcProject);
            mergedProject.SingleProjects += *singleProject;
            for (int j = 0; j < singleProject->ExtraCompilersFiles.count(); ++j) {
                const QString &compilerName = singleProject->ExtraCompilersFiles.at(j).Name;
                if (!mergedProject.ExtraCompilers.contains(compilerName))
                    mergedProject.ExtraCompilers += compilerName;
            }
        }

        if(mergedProjects.count() > 1 &&
	   mergedProjects.at(0)->vcProject.Name ==
	   mergedProjects.at(1)->vcProject.Name)
	    mergedProjects.at(0)->writePrlFile();
        mergedProject.Name = unescapeFilePath(project->first("QMAKE_ORIG_TARGET"));
        mergedProject.Version = mergedProjects.at(0)->vcProject.Version;
        mergedProject.ProjectGUID = getProjectUUID().toString().toUpper();
        mergedProject.Keyword = project->first("VCPROJ_KEYWORD");
        mergedProject.SccProjectName = mergedProjects.at(0)->vcProject.SccProjectName;
        mergedProject.SccLocalPath = mergedProjects.at(0)->vcProject.SccLocalPath;
        mergedProject.PlatformName = mergedProjects.at(0)->vcProject.PlatformName;

        XmlOutput xmlOut(t);
        xmlOut << mergedProject;
        return true;
    } else if(project->first("TEMPLATE") == "vcsubdirs") {
        return writeMakefile(t);
    }
    return false;
}

struct VcsolutionDepend {
    QString uuid;
    QString vcprojFile, orig_target, target;
    ::target targetType;
    QStringList dependencies;
};

QUuid VcprojGenerator::getProjectUUID(const QString &filename)
{
    bool validUUID = true;

    // Read GUID from variable-space
    QUuid uuid = project->first("GUID");

    // If none, create one based on the MD5 of absolute project path
    if(uuid.isNull() || !filename.isEmpty()) {
        QString abspath = filename.isEmpty()?project->first("QMAKE_MAKEFILE"):filename;
        qtMD5(abspath.toUtf8(), (unsigned char*)(&uuid));
        validUUID = !uuid.isNull();
        uuid.data4[0] = (uuid.data4[0] & 0x3F) | 0x80; // UV_DCE variant
        uuid.data3 = (uuid.data3 & 0x0FFF) | (QUuid::Name<<12);
    }

    // If still not valid, generate new one, and suggest adding to .pro
    if(uuid.isNull() || !validUUID) {
        uuid = QUuid::createUuid();
        fprintf(stderr,
                "qmake couldn't create a GUID based on filepath, and we couldn't\nfind a valid GUID in the .pro file (Consider adding\n'GUID = %s'  to the .pro file)\n",
                uuid.toString().toUpper().toLatin1().constData());
    }

    // Store GUID in variable-space
    project->values("GUID") = QStringList(uuid.toString().toUpper());
    return uuid;
}

QUuid VcprojGenerator::increaseUUID(const QUuid &id)
{
    QUuid result(id);
    qint64 dataFirst = (result.data4[0] << 24) +
                       (result.data4[1] << 16) +
                       (result.data4[2] << 8) +
                        result.data4[3];
    qint64 dataLast =  (result.data4[4] << 24) +
                       (result.data4[5] << 16) +
                       (result.data4[6] <<  8) +
                        result.data4[7];

    if(!(dataLast++))
        dataFirst++;

    result.data4[0] = uchar((dataFirst >> 24) & 0xff);
    result.data4[1] = uchar((dataFirst >> 16) & 0xff);
    result.data4[2] = uchar((dataFirst >>  8) & 0xff);
    result.data4[3] = uchar(dataFirst         & 0xff);
    result.data4[4] = uchar((dataLast  >> 24) & 0xff);
    result.data4[5] = uchar((dataLast  >> 16) & 0xff);
    result.data4[6] = uchar((dataLast  >>  8) & 0xff);
    result.data4[7] = uchar(dataLast          & 0xff);
    return result;
}

void VcprojGenerator::writeSubDirs(QTextStream &t)
{
    // Check if all requirements are fullfilled
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        fprintf(stderr, "Project file not generated because all requirements not met:\n\t%s\n",
                var("QMAKE_FAILED_REQUIREMENTS").toLatin1().constData());
        return;
    }

    switch(which_dotnet_version()) {
    case NET2005:
        t << _slnHeader80;
        break;
    case NET2003:
        t << _slnHeader71;
        break;
    case NET2002:
        t << _slnHeader70;
        break;
    default:
        t << _slnHeader70;
        warn_msg(WarnLogic, "Generator: MSVC.NET: Unknown version (%d) of MSVC detected for .sln", which_dotnet_version());
        break;
    }

    QHash<QString, VcsolutionDepend*> solution_depends;
    QList<VcsolutionDepend*> solution_cleanup;

    QStringList subdirs = project->values("SUBDIRS");
    QString oldpwd = qmake_getpwd();

    // Make sure that all temp projects are configured
    // for release so that the depends are created
    // without the debug <lib>dxxx.lib name mangling
    QStringList old_after_vars = Option::after_user_vars;
    Option::after_user_vars.append("CONFIG+=release");

    for(int i = 0; i < subdirs.size(); ++i) {
        QString tmp = subdirs.at(i);
        if(!project->isEmpty(tmp + ".file")) {
            if(!project->isEmpty(tmp + ".subdir"))
                warn_msg(WarnLogic, "Cannot assign both file and subdir for subdir %s",
                         tmp.toLatin1().constData());
            tmp = project->first(tmp + ".file");
        } else if(!project->isEmpty(tmp + ".subdir")) {
            tmp = project->first(tmp + ".subdir");
        }
        QFileInfo fi(fileInfo(Option::fixPathToLocalOS(tmp, true)));
        if(fi.exists()) {
            if(fi.isDir()) {
                QString profile = tmp;
                if(!profile.endsWith(Option::dir_sep))
                    profile += Option::dir_sep;
                profile += fi.baseName() + ".pro";
                subdirs.append(profile);
            } else {
                QMakeProject tmp_proj;
                QString dir = fi.path(), fn = fi.fileName();
                if(!dir.isEmpty()) {
                    if(!qmake_setpwd(dir))
                        fprintf(stderr, "Cannot find directory: %s\n", dir.toLatin1().constData());
                }
                if(tmp_proj.read(fn)) {
                    // Check if all requirements are fullfilled
                    if(!tmp_proj.variables()["QMAKE_FAILED_REQUIREMENTS"].isEmpty()) {
                        fprintf(stderr, "Project file(%s) not added to Solution because all requirements not met:\n\t%s\n",
                                fn.toLatin1().constData(), tmp_proj.values("QMAKE_FAILED_REQUIREMENTS").join(" ").toLatin1().constData());
                        continue;
                    }
                    if(tmp_proj.first("TEMPLATE") == "vcsubdirs") {
                        QStringList tmp_proj_subdirs = tmp_proj.variables()["SUBDIRS"];
                        for(int x = 0; x < tmp_proj_subdirs.size(); ++x) {
                            QString tmpdir = tmp_proj_subdirs.at(x);
                            if(!tmp_proj.isEmpty(tmpdir + ".file")) {
                                if(!tmp_proj.isEmpty(tmpdir + ".subdir"))
                                    warn_msg(WarnLogic, "Cannot assign both file and subdir for subdir %s",
                                            tmpdir.toLatin1().constData());
                                tmpdir = tmp_proj.first(tmpdir + ".file");
                            } else if(!tmp_proj.isEmpty(tmpdir + ".subdir")) {
                                tmpdir = tmp_proj.first(tmpdir + ".subdir");
                            }
                            subdirs += fileFixify(tmpdir);
                        }
                    } else if(tmp_proj.first("TEMPLATE") == "vcapp" || tmp_proj.first("TEMPLATE") == "vclib") {
                        // Initialize a 'fake' project to get the correct variables
                        // and to be able to extract all the dependencies
                        VcprojGenerator tmp_vcproj;
                        tmp_vcproj.setNoIO(true);
                        tmp_vcproj.setProjectFile(&tmp_proj);
                        if(Option::debug_level) {
                            QMap<QString, QStringList> &vars = tmp_proj.variables();
                            for(QMap<QString, QStringList>::Iterator it = vars.begin();
                                it != vars.end(); ++it) {
                                if(it.key().left(1) != "." && !it.value().isEmpty())
                                    debug_msg(1, "%s: %s === %s", fn.toLatin1().constData(), it.key().toLatin1().constData(),
                                                it.value().join(" :: ").toLatin1().constData());
                            }
                        }

                        // We assume project filename is [QMAKE_ORIG_TARGET].vcproj
                        QString vcproj = unescapeFilePath(fixFilename(tmp_vcproj.project->first("QMAKE_ORIG_TARGET")) + project->first("VCPROJ_EXTENSION"));

                        // If file doesn't exsist, then maybe the users configuration
                        // doesn't allow it to be created. Skip to next...
                        if(!exists(qmake_getpwd() + Option::dir_sep + vcproj)) {
                            warn_msg(WarnLogic, "Ignored (not found) '%s'", QString(qmake_getpwd() + Option::dir_sep + vcproj).toLatin1().constData());
                            goto nextfile; // # Dirty!
                        }

                        VcsolutionDepend *newDep = new VcsolutionDepend;
                        newDep->vcprojFile = fileFixify(vcproj);
                        newDep->orig_target = unescapeFilePath(tmp_proj.first("QMAKE_ORIG_TARGET"));
                        newDep->target = tmp_proj.first("MSVCPROJ_TARGET").section(Option::dir_sep, -1);
                        newDep->targetType = tmp_vcproj.projectTarget;
                        newDep->uuid = getProjectUUID(Option::fixPathToLocalOS(qmake_getpwd() + QDir::separator() + vcproj)).toString().toUpper();

                        // We want to store it as the .lib name.
                        if(newDep->target.endsWith(".dll"))
                            newDep->target = newDep->target.left(newDep->target.length()-3) + "lib";

                        // All projects having mocable sourcefiles are dependent on moc.exe
                        if(tmp_proj.variables()["CONFIG"].contains("moc"))
                            newDep->dependencies << "moc.exe";

                        // All extra compilers which has valid input are considered dependencies
                        const QStringList &quc = tmp_proj.variables()["QMAKE_EXTRA_COMPILERS"];
                        for(QStringList::ConstIterator it = quc.constBegin(); it != quc.constEnd(); ++it) {
                            const QStringList &invar = tmp_proj.variables().value((*it) + ".input");
                            for(QStringList::ConstIterator iit = invar.constBegin(); iit != invar.constEnd(); ++iit) {
                                const QStringList fileList = tmp_proj.variables().value(*iit);
                                if (!fileList.isEmpty()) {
                                    QString dep = tmp_proj.first((*it) + ".commands").section('/', -1).section('\\', -1);
                                    if (!newDep->dependencies.contains(dep))
                                        newDep->dependencies << dep;
                                }
                            }
                        }

                        // Add all unknown libs to the deps
                        QStringList where("QMAKE_LIBS");
                        if(!tmp_proj.isEmpty("QMAKE_INTERNAL_PRL_LIBS"))
                            where = tmp_proj.variables()["QMAKE_INTERNAL_PRL_LIBS"];
                        for(QStringList::iterator wit = where.begin();
                            wit != where.end(); ++wit) {
                            QStringList &l = tmp_proj.variables()[(*wit)];
                            for(QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
                                QString opt = (*it);
                                if(!opt.startsWith("/") &&   // Not a switch
                                    opt != newDep->target && // Not self
                                    opt != "opengl32.lib" && // We don't care about these libs
                                    opt != "glu32.lib" &&    // to make depgen alittle faster
                                    opt != "kernel32.lib" &&
                                    opt != "user32.lib" &&
                                    opt != "gdi32.lib" &&
                                    opt != "comdlg32.lib" &&
                                    opt != "advapi32.lib" &&
                                    opt != "shell32.lib" &&
                                    opt != "ole32.lib" &&
                                    opt != "oleaut32.lib" &&
                                    opt != "uuid.lib" &&
                                    opt != "imm32.lib" &&
                                    opt != "winmm.lib" &&
                                    opt != "wsock32.lib" &&
                                    opt != "ws2_32.lib" &&
                                    opt != "winspool.lib" &&
                                    opt != "delayimp.lib")
                                {
                                    newDep->dependencies << opt.section(Option::dir_sep, -1);
                                }
                            }
                        }
#ifdef DEBUG_SOLUTION_GEN
                        qDebug("Deps for %20s: [%s]", newDep->target.toLatin1().constData(), newDep->dependencies.join(" :: ").toLatin1().constData());
#endif
                        solution_cleanup.append(newDep);
                        solution_depends.insert(newDep->target, newDep);
                        t << _slnProjectBeg << _slnMSVCvcprojGUID << _slnProjectMid
                            << "\"" << newDep->orig_target << "\", \"" << newDep->vcprojFile
                            << "\", \"" << newDep->uuid << "\"";
                        t << _slnProjectEnd;
                    }
                }
nextfile:
                qmake_setpwd(oldpwd);
            }
        }
    }
    t << _slnGlobalBeg;
    t << _slnSolutionConf;
    t << _slnProjDepBeg;

    // Restore previous after_user_var options
    Option::after_user_vars = old_after_vars;

    // Figure out dependencies
    for(QList<VcsolutionDepend*>::Iterator it = solution_cleanup.begin(); it != solution_cleanup.end(); ++it) {
        int cnt = 0;
        for(QStringList::iterator dit = (*it)->dependencies.begin();  dit != (*it)->dependencies.end(); ++dit) {
            if(VcsolutionDepend *vc = solution_depends[*dit])
                t << "\n\t\t" << (*it)->uuid << "." << cnt++ << " = " << vc->uuid;
        }
    }
    t << _slnProjDepEnd;
    t << _slnProjConfBeg;
    for(QList<VcsolutionDepend*>::Iterator it = solution_cleanup.begin(); it != solution_cleanup.end(); ++it) {
        t << "\n\t\t" << (*it)->uuid << _slnProjDbgConfTag1;
        t << "\n\t\t" << (*it)->uuid << _slnProjDbgConfTag2;
        t << "\n\t\t" << (*it)->uuid << _slnProjRelConfTag1;
        t << "\n\t\t" << (*it)->uuid << _slnProjRelConfTag2;
    }
    t << _slnProjConfEnd;
    t << _slnExtSections;
    t << _slnGlobalEnd;


    while (!solution_cleanup.isEmpty())
        delete solution_cleanup.takeFirst();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

bool VcprojGenerator::hasBuiltinCompiler(const QString &file)
{
    // Source files
    for (int i = 0; i < Option::cpp_ext.count(); ++i)
        if (file.endsWith(Option::cpp_ext.at(i)))
            return true;
    for (int i = 0; i < Option::c_ext.count(); ++i)
        if (file.endsWith(Option::c_ext.at(i)))
            return true;
    if (file.endsWith(".rc"))
        return true;
    return false;
}

void VcprojGenerator::init()
{
    if(init_flag)
        return;
    if(project->first("TEMPLATE") == "vcsubdirs") { //too much work for subdirs
        init_flag = true;
        return;
    }

    debug_msg(1, "Generator: MSVC.NET: Initializing variables");

    // this should probably not be here, but I'm using it to wrap the .t files
    if(project->first("TEMPLATE") == "vcapp")
        project->values("QMAKE_APP_FLAG").append("1");
    else if(project->first("TEMPLATE") == "vclib")
        project->values("QMAKE_LIB_FLAG").append("1");
    if(project->values("QMAKESPEC").isEmpty())
        project->values("QMAKESPEC").append(qgetenv("QMAKESPEC"));

    processVars();
    initOld();           // Currently calling old DSP code to set variables. CLEAN UP!

    // Figure out what we're trying to build
    if(project->first("TEMPLATE") == "vcapp") {
        projectTarget = Application;
    } else if(project->first("TEMPLATE") == "vclib") {
        if(project->isActiveConfig("staticlib"))
            projectTarget = StaticLib;
        else
            projectTarget = SharedLib;
    }

    // Setup PCH variables
    precompH = project->first("PRECOMPILED_HEADER");
    precompCPP = project->first("PRECOMPILED_SOURCE");
    usePCH = !precompH.isEmpty() && project->isActiveConfig("precompile_header");
    if (usePCH) {
        precompHFilename = fileInfo(precompH).fileName();
        // Created files
        QString origTarget = unescapeFilePath(project->first("QMAKE_ORIG_TARGET"));
        precompObj = origTarget + Option::obj_ext;
        precompPch = origTarget + ".pch";
        // Add PRECOMPILED_HEADER to HEADERS
        if (!project->values("HEADERS").contains(precompH))
            project->values("HEADERS") += precompH;
        // Return to variable pool
        project->values("PRECOMPILED_OBJECT") = QStringList(precompObj);
        project->values("PRECOMPILED_PCH")    = QStringList(precompPch);

        autogenPrecompCPP = precompCPP.isEmpty() && project->isActiveConfig("autogen_precompile_source");
        if (autogenPrecompCPP) {
            precompCPP = precompH
                + (Option::cpp_ext.count() ? Option::cpp_ext.at(0) : QLatin1String(".cpp"));
            project->values("GENERATED_SOURCES") += precompCPP;
        } else if (!precompCPP.isEmpty()) {
            project->values("SOURCES") += precompCPP;
        }
    }

    // Add all input files for a custom compiler into a map for uniqueness,
    // unless the compiler is configure as a combined stage, then use the first one
    const QStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for(QStringList::ConstIterator it = quc.constBegin(); it != quc.constEnd(); ++it) {
        const QStringList &invar = project->variables().value((*it) + ".input");
        const QString compiler_out = project->first((*it) + ".output");
        for(QStringList::ConstIterator iit = invar.constBegin(); iit != invar.constEnd(); ++iit) {
            QStringList fileList = project->variables().value(*iit);
            if (!fileList.isEmpty()) {
                if (project->values((*it) + ".CONFIG").indexOf("combine") != -1)
                    fileList = QStringList(fileList.first());
                for(QStringList::ConstIterator fit = fileList.constBegin(); fit != fileList.constEnd(); ++fit) {
                    QString file = (*fit);
                    if (verifyExtraCompiler((*it), file)) {
                        if (!hasBuiltinCompiler(file)) {
                            extraCompilerSources[file] += *it;
                        } else {
                            QString out = Option::fixPathToTargetOS(replaceExtraCompilerVariables(
                                            compiler_out, file, QString()), false);
                            extraCompilerSources[out] += *it;
                            extraCompilerOutputs[out] = QStringList(file); // Can only have one
                        }
                    }
                }
            }
        }
    }

#if 0 // Debuging
    Q_FOREACH(QString aKey, extraCompilerSources.keys()) {
        qDebug("Extracompilers for %s are (%s)", aKey.toLatin1().constData(), extraCompilerSources.value(aKey).join(", ").toLatin1().constData());
    }
    Q_FOREACH(QString aKey, extraCompilerOutputs.keys()) {
        qDebug("Object mapping for %s is (%s)", aKey.toLatin1().constData(), extraCompilerOutputs.value(aKey).join(", ").toLatin1().constData());
    }
    qDebug("");
#endif
}

bool VcprojGenerator::mergeBuildProject(MakefileGenerator *other)
{
    VcprojGenerator *otherVC = static_cast<VcprojGenerator*>(other);
    if (!otherVC) {
        warn_msg(WarnLogic, "VcprojGenerator: Cannot merge other types of projects! (ignored)");
        return false;
    }
    mergedProjects += otherVC;
    return true;
}

void VcprojGenerator::initProject()
{
    // Initialize XML sub elements
    // - Do this first since project elements may need
    // - to know of certain configuration options
    initConfiguration();
    initRootFiles();
    initSourceFiles();
    initHeaderFiles();
    initGeneratedFiles();
    initLexYaccFiles();
    initTranslationFiles();
    initFormFiles();
    initResourceFiles();
    initExtraCompilerOutputs();

    // Own elements -----------------------------
    vcProject.Name = unescapeFilePath(project->first("QMAKE_ORIG_TARGET"));
    switch(which_dotnet_version()) {
    case NET2005:
		//### using ',' because of a bug in 2005 B2
		//### VS uses '.' or ',' depending on the regional settings! Using ',' always works.
        vcProject.Version = "8,00";
        break;
    case NET2003:
        vcProject.Version = "7.10";
        break;
    case NET2002:
        vcProject.Version = "7.00";
        break;
    default:
        vcProject.Version = "7.00";
        warn_msg(WarnLogic, "Generator: MSVC.NET: Unknown version (%d) of MSVC detected for .vcproj", which_dotnet_version());
        break;
    }

    vcProject.Keyword = project->first("VCPROJ_KEYWORD");
    vcProject.PlatformName = (vcProject.Configuration.idl.TargetEnvironment == midlTargetWin64 ? "Win64" : "Win32");
    // These are not used by Qt, but may be used by customers
    vcProject.SccProjectName = project->first("SCCPROJECTNAME");
    vcProject.SccLocalPath = project->first("SCCLOCALPATH");
    vcProject.flat_files = project->isActiveConfig("flat");
}

void VcprojGenerator::initConfiguration()
{
    // Initialize XML sub elements
    // - Do this first since main configuration elements may need
    // - to know of certain compiler/linker options
    VCConfiguration &conf = vcProject.Configuration;
    conf.CompilerVersion = which_dotnet_version();

    initCompilerTool();

    // Only on configuration per build
    bool isDebug = project->isActiveConfig("debug");

    if(projectTarget == StaticLib)
        initLibrarianTool();
    else {
        conf.linker.GenerateDebugInformation = isDebug ? _True : _False;
        initLinkerTool();
    }
    initResourceTool();
    initIDLTool();

    // Own elements -----------------------------
    QString temp = project->first("BuildBrowserInformation");
    switch (projectTarget) {
    case SharedLib:
        conf.ConfigurationType = typeDynamicLibrary;
        break;
    case StaticLib:
        conf.ConfigurationType = typeStaticLibrary;
        break;
    case Application:
    default:
        conf.ConfigurationType = typeApplication;
        break;
    }

    conf.Name = project->values("BUILD_NAME").join(" ");
    if (conf.Name.isEmpty())
        conf.Name = isDebug ? "Debug" : "Release";
    conf.Name += (conf.idl.TargetEnvironment == midlTargetWin64 ? "|Win64" : "|Win32");
    conf.ATLMinimizesCRunTimeLibraryUsage = (project->first("ATLMinimizesCRunTimeLibraryUsage").isEmpty() ? _False : _True);
    conf.BuildBrowserInformation = triState(temp.isEmpty() ? (short)unset : temp.toShort());
    temp = project->first("CharacterSet");
    conf.CharacterSet = charSet(temp.isEmpty() ? (short)charSetNotSet : temp.toShort());
    conf.DeleteExtensionsOnClean = project->first("DeleteExtensionsOnClean");
    conf.ImportLibrary = conf.linker.ImportLibrary;
    conf.IntermediateDirectory = project->first("OBJECTS_DIR");
    conf.OutputDirectory = ".";
    conf.PrimaryOutput = project->first("PrimaryOutput");
    conf.WholeProgramOptimization = conf.compiler.WholeProgramOptimization;
    temp = project->first("UseOfATL");
    if(!temp.isEmpty())
        conf.UseOfATL = useOfATL(temp.toShort());
    temp = project->first("UseOfMfc");
    if(!temp.isEmpty())
        conf.UseOfMfc = useOfMfc(temp.toShort());

    // Configuration does not need parameters from
    // these sub XML items;
    initCustomBuildTool();
    initPreBuildEventTools();
    initPostBuildEventTools();
    initPreLinkEventTools();

    // Set definite values in both configurations
    if (isDebug) {
        conf.compiler.PreprocessorDefinitions.removeAll("NDEBUG");
    } else {
        conf.compiler.PreprocessorDefinitions += "NDEBUG";
    }

}

void VcprojGenerator::initCompilerTool()
{
    QString placement = project->first("OBJECTS_DIR");
    if(placement.isEmpty())
        placement = ".\\";

    VCConfiguration &conf = vcProject.Configuration;
    conf.compiler.AssemblerListingLocation = placement ;
    conf.compiler.ProgramDataBaseFileName = ".\\" ;
    conf.compiler.ObjectFile = placement ;
    // PCH
    if (usePCH) {
        conf.compiler.UsePrecompiledHeader     = pchUseUsingSpecific;
        conf.compiler.PrecompiledHeaderFile    = "$(IntDir)\\" + precompPch;
        conf.compiler.PrecompiledHeaderThrough = project->first("PRECOMPILED_HEADER");
        conf.compiler.ForcedIncludeFiles       = project->values("PRECOMPILED_HEADER");
        // Minimal build option triggers an Internal Compiler Error
        // when used in conjunction with /FI and /Yu, so remove it
        project->values("QMAKE_CFLAGS_DEBUG").removeAll("-Gm");
        project->values("QMAKE_CFLAGS_DEBUG").removeAll("/Gm");
        project->values("QMAKE_CXXFLAGS_DEBUG").removeAll("-Gm");
        project->values("QMAKE_CXXFLAGS_DEBUG").removeAll("/Gm");
    }

    conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS"));
    if(project->isActiveConfig("debug")){
        // Debug version
        conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS"));
        conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS_DEBUG"));
        if((projectTarget == Application) || (projectTarget == StaticLib))
            conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS_MT_DBG"));
        else
            conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS_MT_DLLDBG"));
    } else {
        // Release version
        conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS"));
        conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS_RELEASE"));
        conf.compiler.PreprocessorDefinitions += "QT_NO_DEBUG";
        conf.compiler.PreprocessorDefinitions += "NDEBUG";
        if((projectTarget == Application) || (projectTarget == StaticLib))
            conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS_MT"));
        else
            conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS_MT_DLL"));
    }

    // Common for both release and debug
    if(project->isActiveConfig("warn_off"))
        conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS_WARN_OFF"));
    else if(project->isActiveConfig("warn_on"))
        conf.compiler.parseOptions(project->values("QMAKE_CXXFLAGS_WARN_ON"));
    if(project->isActiveConfig("windows"))
        conf.compiler.PreprocessorDefinitions += project->values("MSVCPROJ_WINCONDEF");

    // Can this be set for ALL configs?
    // If so, use qmake.conf!
    if(projectTarget == SharedLib)
        conf.compiler.PreprocessorDefinitions += "_WINDOWS";

    conf.compiler.PreprocessorDefinitions += project->values("DEFINES");
    conf.compiler.PreprocessorDefinitions += project->values("PRL_EXPORT_DEFINES");
    conf.compiler.parseOptions(project->values("MSVCPROJ_INCPATH"));
}

void VcprojGenerator::initLibrarianTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    conf.librarian.OutputFile = project->first("DESTDIR");
    if(conf.librarian.OutputFile.isEmpty())
        conf.librarian.OutputFile = ".\\";

    if(!conf.librarian.OutputFile.endsWith("\\"))
        conf.librarian.OutputFile += '\\';

    conf.librarian.OutputFile += project->first("MSVCPROJ_TARGET");
}

void VcprojGenerator::initLinkerTool()
{
    findLibraries(); // Need to add the highest version of the libs
    VCConfiguration &conf = vcProject.Configuration;
    conf.linker.parseOptions(project->values("MSVCPROJ_LFLAGS"));
    conf.linker.AdditionalDependencies += project->values("MSVCPROJ_LIBS");

    switch (projectTarget) {
    case Application:
        conf.linker.OutputFile = project->first("DESTDIR");
        break;
    case SharedLib:
        conf.linker.parseOptions(project->values("MSVCPROJ_LIBOPTIONS"));
        conf.linker.OutputFile = project->first("DESTDIR");
        break;
    case StaticLib: //unhandled - added to remove warnings..
        break;
    }

    if(conf.linker.OutputFile.isEmpty())
        conf.linker.OutputFile = ".\\";

    if(!conf.linker.OutputFile.endsWith("\\"))
        conf.linker.OutputFile += '\\';

    conf.linker.OutputFile += project->first("MSVCPROJ_TARGET");

    if(project->isActiveConfig("debug")){
        conf.linker.parseOptions(project->values("QMAKE_LFLAGS_DEBUG"));
    } else {
        conf.linker.parseOptions(project->values("QMAKE_LFLAGS_RELEASE"));
    }

    if(project->isActiveConfig("dll")){
        conf.linker.parseOptions(project->values("QMAKE_LFLAGS_QT_DLL"));
    }

    if(project->isActiveConfig("console")){
        conf.linker.parseOptions(project->values("QMAKE_LFLAGS_CONSOLE"));
    } else {
        conf.linker.parseOptions(project->values("QMAKE_LFLAGS_WINDOWS"));
    }

}

void VcprojGenerator::initResourceTool()
{
    VCConfiguration &conf = vcProject.Configuration;
    conf.resource.PreprocessorDefinitions = conf.compiler.PreprocessorDefinitions;
}


void VcprojGenerator::initIDLTool()
{
}

void VcprojGenerator::initCustomBuildTool()
{
}

void VcprojGenerator::initPreBuildEventTools()
{
}

QString VcprojGenerator::fixCommandLine(DotNET version, const QString &input) const
{
    QString result = input;

    if (version == NET2005)
        result = result.replace(QLatin1Char('\n'), QLatin1String("&#x000D;&#x000A;"));
    
    return result;
}

void VcprojGenerator::initPostBuildEventTools()
{
    VCConfiguration &conf = vcProject.Configuration;
    if(!project->values("QMAKE_POST_LINK").isEmpty()) {
        QString cmdline = fixCommandLine(conf.CompilerVersion, var("QMAKE_POST_LINK"));
        conf.postBuild.Description = cmdline;
        conf.postBuild.CommandLine = cmdline;
    }
    if(!project->values("MSVCPROJ_COPY_DLL").isEmpty()) {
        if(!conf.postBuild.CommandLine.isEmpty())
            conf.postBuild.CommandLine += " && ";
        conf.postBuild.Description += var("MSVCPROJ_COPY_DLL_DESC");
        conf.postBuild.CommandLine += var("MSVCPROJ_COPY_DLL");
    }
}

void VcprojGenerator::initPreLinkEventTools()
{
}

void VcprojGenerator::initRootFiles()
{
    // Note: Root files do _not_ have any filter name, filter nor GUID!
    vcProject.RootFiles.addFiles(project->values("RC_FILE"));

    vcProject.RootFiles.Project = this;
    vcProject.RootFiles.Config = &(vcProject.Configuration);
    vcProject.RootFiles.CustomBuild = none;
}

void VcprojGenerator::initSourceFiles()
{
    vcProject.SourceFiles.Name = "Source Files";
    vcProject.SourceFiles.Filter = "cpp;c;cxx;def;odl;idl;hpj;bat;asm;asmx";
    vcProject.SourceFiles.Guid = _GUIDSourceFiles;

    vcProject.SourceFiles.addFiles(project->values("SOURCES"));

    vcProject.SourceFiles.Project = this;
    vcProject.SourceFiles.Config = &(vcProject.Configuration);
    vcProject.SourceFiles.CustomBuild = none;
}

void VcprojGenerator::initHeaderFiles()
{
    vcProject.HeaderFiles.Name = "Header Files";
    vcProject.HeaderFiles.Filter = "h;hpp;hxx;hm;inl;inc;xsd";
    vcProject.HeaderFiles.Guid = _GUIDHeaderFiles;

    vcProject.HeaderFiles.addFiles(project->values("HEADERS"));
    if (usePCH) // Generated PCH cpp file
        vcProject.HeaderFiles.addFile(precompH);

    vcProject.HeaderFiles.Project = this;
    vcProject.HeaderFiles.Config = &(vcProject.Configuration);
//    vcProject.HeaderFiles.CustomBuild = mocHdr;
//    addMocArguments(vcProject.HeaderFiles);
}

void VcprojGenerator::initGeneratedFiles()
{
    vcProject.GeneratedFiles.Name = "Generated Files";
    vcProject.GeneratedFiles.Filter = "cpp;c;cxx;moc;h;def;odl;idl;res;";
    vcProject.GeneratedFiles.Guid = _GUIDGeneratedFiles;

    // ### These cannot have CustomBuild (mocSrc)!!
    vcProject.GeneratedFiles.addFiles(project->values("GENERATED_SOURCES"));
    vcProject.GeneratedFiles.addFiles(project->values("GENERATED_FILES"));
    vcProject.GeneratedFiles.addFiles(project->values("IDLSOURCES"));
    vcProject.GeneratedFiles.addFiles(project->values("RES_FILE"));
    vcProject.GeneratedFiles.addFiles(project->values("QMAKE_IMAGE_COLLECTION"));   // compat
    if(!extraCompilerOutputs.isEmpty())
        vcProject.GeneratedFiles.addFiles(extraCompilerOutputs.keys());

    vcProject.GeneratedFiles.Project = this;
    vcProject.GeneratedFiles.Config = &(vcProject.Configuration);
//    vcProject.GeneratedFiles.CustomBuild = mocSrc;
}

void VcprojGenerator::initLexYaccFiles()
{
    vcProject.LexYaccFiles.Name = "Lex / Yacc Files";
    vcProject.LexYaccFiles.ParseFiles = _False;
    vcProject.LexYaccFiles.Filter = "l;y";
    vcProject.LexYaccFiles.Guid = _GUIDLexYaccFiles;

    vcProject.LexYaccFiles.addFiles(project->values("LEXSOURCES"));
    vcProject.LexYaccFiles.addFiles(project->values("YACCSOURCES"));

    vcProject.LexYaccFiles.Project = this;
    vcProject.LexYaccFiles.Config = &(vcProject.Configuration);
    vcProject.LexYaccFiles.CustomBuild = lexyacc;
}

void VcprojGenerator::initTranslationFiles()
{
    vcProject.TranslationFiles.Name = "Translation Files";
    vcProject.TranslationFiles.ParseFiles = _False;
    vcProject.TranslationFiles.Filter = "ts";
    vcProject.TranslationFiles.Guid = _GUIDTranslationFiles;

    vcProject.TranslationFiles.addFiles(project->values("TRANSLATIONS"));

    vcProject.TranslationFiles.Project = this;
    vcProject.TranslationFiles.Config = &(vcProject.Configuration);
    vcProject.TranslationFiles.CustomBuild = none;
}


void VcprojGenerator::initFormFiles()
{
    vcProject.FormFiles.Name = "Form Files";
    vcProject.FormFiles.ParseFiles = _False;
    vcProject.FormFiles.Filter = "ui";
    vcProject.FormFiles.Guid = _GUIDFormFiles;

    vcProject.FormFiles.addFiles(project->values("FORMS"));
    vcProject.FormFiles.addFiles(project->values("FORMS3"));

    vcProject.FormFiles.Project = this;
    vcProject.FormFiles.Config = &(vcProject.Configuration);
    vcProject.FormFiles.CustomBuild = none;
}


void VcprojGenerator::initResourceFiles()
{
    vcProject.ResourceFiles.Name = "Resource Files";
    vcProject.ResourceFiles.ParseFiles = _False;
    vcProject.ResourceFiles.Filter = "qrc;*"; //"rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;ts;qrc";
    vcProject.ResourceFiles.Guid = _GUIDResourceFiles;

    // Bad hack, please look away -------------------------------------
    QString rcc_dep_cmd = project->values("rcc.depend_command").join(" ");
    if(!rcc_dep_cmd.isEmpty()) {
        QString argv0 = Option::fixPathToLocalOS(rcc_dep_cmd.split(' ').first());
        if(exists(argv0)) {
            QStringList qrc_files = project->values("RESOURCES");
            QStringList deps;
            if(!qrc_files.isEmpty()) {
                for (int i = 0; i < qrc_files.count(); ++i) {
        	    char buff[256];
                    QString dep_cmd = replaceExtraCompilerVariables(rcc_dep_cmd, qrc_files.at(i),"");

                    dep_cmd = Option::fixPathToLocalOS(dep_cmd);
                    if(FILE *proc = QT_POPEN(dep_cmd.toLatin1().constData(), "r")) {
        	        QString indeps;
                        while(!feof(proc)) {
                            int read_in = (int)fread(buff, 1, 255, proc);
                            if(!read_in)
                                break;
                            indeps += QByteArray(buff, read_in);
                        }
                        fclose(proc);
                        if(!indeps.isEmpty())
                            deps += fileFixify(indeps.replace('\n', ' ').simplified().split(' '));
                    }
                }
            }
            vcProject.ResourceFiles.addFiles(deps);
        }
    }
    // You may look again --------------------------------------------

    vcProject.ResourceFiles.addFiles(project->values("RESOURCES"));
    vcProject.ResourceFiles.addFiles(project->values("IMAGES"));

    vcProject.ResourceFiles.Project = this;
    vcProject.ResourceFiles.Config = &(vcProject.Configuration);
    vcProject.ResourceFiles.CustomBuild = none;
}

void VcprojGenerator::initExtraCompilerOutputs()
{
    const QStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for(QStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
        // Create an extra compiler filter and add the files
        VCFilter extraCompile;
        extraCompile.Name = (*it);
        extraCompile.ParseFiles = _False;
        extraCompile.Filter = "";
        extraCompile.Guid = QString(_GUIDExtraCompilerFiles) + "-" + (*it);


        // If the extra compiler has a variable_out set the output file
        // is added to an other file list, and does not need its own..
        QString tmp_other_out = project->first((*it) + ".variable_out");
        if (!tmp_other_out.isEmpty())
            continue;

        QString tmp_out = project->first((*it) + ".output");
        if (project->values((*it) + ".CONFIG").indexOf("combine") != -1) {
            // Combined output, only one file result
            extraCompile.addFile(
                Option::fixPathToTargetOS(replaceExtraCompilerVariables(tmp_out, QString(), QString()), false));
        } else {
            // One output file per input
            QStringList tmp_in = project->values(project->first((*it) + ".input"));
            for (int i = 0; i < tmp_in.count(); ++i) {
                const QString &filename = tmp_in.at(i);
                if (extraCompilerSources.contains(filename))
                    extraCompile.addFile(
                        Option::fixPathToTargetOS(replaceExtraCompilerVariables(tmp_out, filename, QString()), false));
            }
        }
        extraCompile.Project = this;
        extraCompile.Config = &(vcProject.Configuration);
        extraCompile.CustomBuild = none;

        vcProject.ExtraCompilersFiles.append(extraCompile);
    }
}

/* \internal
    Sets up all needed variables from the environment and all the different caches and .conf files
*/

void VcprojGenerator::initOld()
{
    if(init_flag)
        return;

    init_flag = true;
    QStringList::Iterator it;

    // Decode version, and add it to $$MSVCPROJ_VERSION --------------
    if(!project->values("VERSION").isEmpty()) {
        QString version = project->values("VERSION")[0];
        int firstDot = version.indexOf(".");
        QString major = version.left(firstDot);
        QString minor = version.right(version.length() - firstDot - 1);
        minor.replace(QRegExp("\\."), "");
        project->values("MSVCPROJ_VERSION").append("/VERSION:" + major + "." + minor);
        project->values("QMAKE_LFLAGS").append("/VERSION:" + major + "." + minor);
    }

    project->values("QMAKE_LIBS") += project->values("LIBS");

     // Get filename w/o extention -----------------------------------
    QString msvcproj_project = "";
    QString targetfilename = "";
    if(!project->isEmpty("TARGET")) {
        project->values("TARGET") = unescapeFilePaths(project->values("TARGET"));
        targetfilename = msvcproj_project = project->first("TARGET");
    }

    // Init base class too -------------------------------------------
    MakefileGenerator::init();

    if(msvcproj_project.isEmpty())
        msvcproj_project = Option::output.fileName();

    msvcproj_project = msvcproj_project.right(msvcproj_project.length() - msvcproj_project.lastIndexOf("\\") - 1);
    msvcproj_project = msvcproj_project.left(msvcproj_project.lastIndexOf("."));
    msvcproj_project.replace(QRegExp("-"), "");

    project->values("MSVCPROJ_PROJECT").append(msvcproj_project);
    QStringList &proj = project->values("MSVCPROJ_PROJECT");

    for(it = proj.begin(); it != proj.end(); ++it)
        (*it).replace(QRegExp("\\.[a-zA-Z0-9_]*$"), "");

    // SUBSYSTEM -----------------------------------------------------
    if(!project->values("QMAKE_APP_FLAG").isEmpty()) {
            project->values("MSVCPROJ_TEMPLATE").append("win32app" + project->first("VCPROJ_EXTENSION"));
            if(project->isActiveConfig("console")) {
                project->values("MSVCPROJ_CONSOLE").append("CONSOLE");
                project->values("MSVCPROJ_WINCONDEF").append("_CONSOLE");
                project->values("MSVCPROJ_VCPROJTYPE").append("0x0103");
                project->values("MSVCPROJ_SUBSYSTEM").append("CONSOLE");
            } else {
                project->values("MSVCPROJ_CONSOLE").clear();
                project->values("MSVCPROJ_WINCONDEF").append("_WINDOWS");
                project->values("MSVCPROJ_VCPROJTYPE").append("0x0101");
                project->values("MSVCPROJ_SUBSYSTEM").append("WINDOWS");
            }
    }

    // $$QMAKE.. -> $$MSVCPROJ.. -------------------------------------
    project->values("MSVCPROJ_LIBS") += project->values("QMAKE_LIBS");
    project->values("MSVCPROJ_LFLAGS") += project->values("QMAKE_LFLAGS");
    if(!project->values("QMAKE_LIBDIR").isEmpty()) {
        QStringList strl = project->values("QMAKE_LIBDIR");
        QStringList::iterator stri;
        for(stri = strl.begin(); stri != strl.end(); ++stri) {
            if(!(*stri).startsWith("/LIBPATH:"))
                (*stri).prepend("/LIBPATH:");
        }
        project->values("MSVCPROJ_LFLAGS") += strl;
    }
    project->values("MSVCPROJ_CXXFLAGS") += project->values("QMAKE_CXXFLAGS");
    QStringList &incs = project->values("INCLUDEPATH");
    for(QStringList::Iterator incit = incs.begin(); incit != incs.end(); ++incit) {
        QString inc = (*incit);
        if (!inc.startsWith('"') && !inc.endsWith('"'))
            inc = QString("\"%1\"").arg(inc); // Quote all paths if not quoted already
        project->values("MSVCPROJ_INCPATH").append("-I" + inc);
    }
    project->values("MSVCPROJ_INCPATH").append("-I" + specdir());

    QString dest;
    project->values("MSVCPROJ_TARGET") = QStringList(project->first("TARGET"));
    Option::fixPathToTargetOS(project->first("TARGET"));
    dest = project->first("TARGET") + project->first("TARGET_EXT");
    project->values("MSVCPROJ_TARGET") = QStringList(dest);

    // DLL COPY ------------------------------------------------------
    if(project->isActiveConfig("dll") && !project->values("DLLDESTDIR").isEmpty()) {
        QStringList dlldirs = project->values("DLLDESTDIR");
        QString copydll("");
        QStringList::Iterator dlldir;
        for(dlldir = dlldirs.begin(); dlldir != dlldirs.end(); ++dlldir) {
            if(!copydll.isEmpty())
                copydll += " && ";
            copydll += "copy  \"$(TargetPath)\" \"" + *dlldir + "\"";
        }

        QString deststr("Copy " + dest + " to ");
        for(dlldir = dlldirs.begin(); dlldir != dlldirs.end();) {
            deststr += *dlldir;
            ++dlldir;
            if(dlldir != dlldirs.end())
                deststr += ", ";
        }

        project->values("MSVCPROJ_COPY_DLL").append(copydll);
        project->values("MSVCPROJ_COPY_DLL_DESC").append(deststr);
    }

    if (!project->values("DEF_FILE").isEmpty())
        project->values("MSVCPROJ_LFLAGS").append("/DEF:"+project->first("DEF_FILE"));

    project->values("QMAKE_INTERNAL_PRL_LIBS") << "MSVCPROJ_LIBS";

    // Verbose output if "-d -d"...
    outputVariables();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

QString VcprojGenerator::replaceExtraCompilerVariables(const QString &var, const QString &in, const QString &out)
{
    QString ret = MakefileGenerator::replaceExtraCompilerVariables(var, in, out);

    QStringList &defines = project->values("VCPROJ_MAKEFILE_DEFINES");
    if(defines.isEmpty())
        defines.append(varGlue("PRL_EXPORT_DEFINES"," -D"," -D","") +
                       varGlue("DEFINES"," -D"," -D",""));
    ret.replace("$(DEFINES)", defines.first());

    QStringList &incpath = project->values("VCPROJ_MAKEFILE_INCPATH");
    if(incpath.isEmpty() && !this->var("MSVCPROJ_INCPATH").isEmpty())
        incpath.append(this->var("MSVCPROJ_INCPATH"));
    ret.replace("$(INCPATH)", incpath.join(" "));

    return ret;
}



bool VcprojGenerator::openOutput(QFile &file, const QString &build) const
{
    QString outdir;
    if(!file.fileName().isEmpty()) {
        QFileInfo fi(fileInfo(file.fileName()));
        if(fi.isDir())
            outdir = file.fileName() + QDir::separator();
    }
    if(!outdir.isEmpty() || file.fileName().isEmpty()) {
        QString ext = project->first("VCPROJ_EXTENSION");
        if(project->first("TEMPLATE") == "vcsubdirs")
            ext = project->first("VCSOLUTION_EXTENSION");
        QString outputName = unescapeFilePath(project->first("TARGET"));
        if (!project->first("MAKEFILE").isEmpty())
            outputName = project->first("MAKEFILE");
        file.setFileName(outdir + outputName + ext);
    }
    if(QDir::isRelativePath(file.fileName()))
        file.setFileName(Option::fixPathToLocalOS(qmake_getpwd() + Option::dir_sep + fixFilename(file.fileName())));
    return Win32MakefileGenerator::openOutput(file, build);
}

QString VcprojGenerator::fixFilename(QString ofile) const
{
    ofile = Option::fixPathToLocalOS(ofile);
    int slashfind = ofile.lastIndexOf(Option::dir_sep);
    if(slashfind == -1) {
        ofile = ofile.replace('-', '_');
    } else {
        int hypenfind = ofile.indexOf('-', slashfind);
        while (hypenfind != -1 && slashfind < hypenfind) {
            ofile = ofile.replace(hypenfind, 1, '_');
            hypenfind = ofile.indexOf('-', hypenfind + 1);
        }
    }
    return ofile;
}

QString VcprojGenerator::findTemplate(QString file)
{
    QString ret;
    if(!exists((ret = file)) &&
       !exists((ret = QString(Option::mkfile::qmakespec + "/" + file))) &&
       !exists((ret = QString(QLibraryInfo::location(QLibraryInfo::DataPath) + "/win32-msvc.net/" + file))) &&
       !exists((ret = (QString(qgetenv("HOME")) + "/.tmake/" + file))))
        return "";
    debug_msg(1, "Generator: MSVC.NET: Found template \'%s\'", ret.toLatin1().constData());
    return ret;
}


void VcprojGenerator::processPrlVariable(const QString &var, const QStringList &l)
{
    if(var == "QMAKE_PRL_DEFINES") {
        QStringList &out = project->values("MSVCPROJ_DEFINES");
        for(QStringList::ConstIterator it = l.begin(); it != l.end(); ++it) {
            if(out.indexOf((*it)) == -1)
                out.append((" /D " + *it));
        }
    } else {
        MakefileGenerator::processPrlVariable(var, l);
    }
}

void VcprojGenerator::outputVariables()
{
#if 0
    qDebug("Generator: MSVC.NET: List of current variables:");
    for(QMap<QString, QStringList>::ConstIterator it = project->variables().begin(); it != project->variables().end(); ++it)
        qDebug("Generator: MSVC.NET: %s => %s", it.key().toLatin1().constData(), it.data().join(" | ").toLatin1().constData());
#endif
}

