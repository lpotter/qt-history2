/****************************************************************************
** $Id$
**
** Definition of ________ class.
**
** Created : 970521
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the network module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "projectgenerator.h"
#include "option.h"
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>


ProjectGenerator::ProjectGenerator(QMakeProject *p) : MakefileGenerator(p), init_flag(FALSE)
{
}

void
ProjectGenerator::init()
{
    if(init_flag)
	return;
    int file_count = 0;
    init_flag = TRUE;

    QMap<QString, QStringList> &v = project->variables();
    QString templ = Option::user_template.isEmpty() ? QString("app") : Option::user_template;
    if(!Option::user_template_prefix.isEmpty())
	templ.prepend(Option::user_template_prefix);
    v["TEMPLATE_ASSIGN"] += templ;

    //figure out target
    if(Option::output.name() == "-" || Option::output.name().isEmpty())
	v["TARGET"] = QStringList("unknown");

    //the scary stuff
    if(project->first("TEMPLATE_ASSIGN") != "subdirs") {
	QString builtin_regex;
	{ //calculate the builtin regular expression..
	    QStringList builtin_exts(".c");
	    builtin_exts << Option::ui_ext << Option::yacc_ext << Option::lex_ext;
	    builtin_exts += Option::h_ext + Option::cpp_ext;
	    for(QStringList::Iterator ext_it = builtin_exts.begin(); 
		ext_it != builtin_exts.end(); ++ext_it) {
		if(!builtin_regex.isEmpty())
		    builtin_regex += "; ";
		builtin_regex += QString("*") + (*ext_it);
	    }
	}
	QStringList dirs = Option::projfile::project_dirs;
	if(Option::projfile::do_pwd)
	    dirs.prepend(QDir::currentDirPath());

	for(QStringList::Iterator pd = dirs.begin(); pd != dirs.end(); pd++) {
	    QString dir, regex;
	    bool add_depend = FALSE;
	    if(QFile::exists((*pd))) {
		QFileInfo fi((*pd));
		if(fi.isDir()) {
		    dir = (*pd);
		    add_depend = TRUE;
		    if(dir.right(1) != Option::dir_sep)
			dir += Option::dir_sep;
		    if(Option::projfile::do_recursive) {
			QDir d(dir);
			d.setFilter(QDir::Dirs);
			for(int i = 0; i < (int)d.count(); i++) {
			    if(d[i] != "." && d[i] != "..") 
				dirs.append(dir + d[i] + QDir::separator() + builtin_regex);
			}
		    }
		    regex = builtin_regex;
		} else {
		    QString file = (*pd);
		    int s = file.findRev(Option::dir_sep);
		    if(s != -1)
			dir = file.left(s+1);
		    if(addFile(file)) {
			add_depend = TRUE;
			file_count++;
		    }
		}
	    } else { //regexp
		regex = (*pd);
	    }
	    if(!regex.isEmpty()) {
		int s = regex.findRev(Option::dir_sep);
		if(s != -1) {
		    dir = regex.left(s+1);
		    regex = regex.right(regex.length() - (s+1));
		}
		if(Option::projfile::do_recursive) {
		    QDir d(dir);
		    d.setFilter(QDir::Dirs);
		    for(int i = 0; i < (int)d.count(); i++) {
			if(d[i] != "." && d[i] != "..") 
			    dirs.append(dir + d[i] + QDir::separator() + regex);
		    }
		}
		QDir d(dir, regex);
		for(int i = 0; i < (int)d.count(); i++) {
		    QString file = dir + d[i];
		    if (addFile(file)) {
			add_depend = TRUE;
			file_count++;
		    }
		}
	    }
	    if(add_depend && !dir.isEmpty() && !v["DEPENDPATH"].contains(dir)) {
		QFileInfo fi(dir);
		if(fi.absFilePath() != QDir::currentDirPath()) {
		    fileFixify(dir);
		    v["DEPENDPATH"] += dir;
		}
	    }
	}
    }
    if(!file_count) { //shall we try a subdir?
	QStringList dirs = Option::projfile::project_dirs;
	if(Option::projfile::do_pwd)
	    dirs.prepend(".");
	for(QStringList::Iterator pd = dirs.begin(); pd != dirs.end(); pd++) {
	    if(QFile::exists((*pd))) {
		QString newdir = (*pd);
		QFileInfo fi(newdir);
		if(fi.isDir()) {
		    fileFixify(newdir);
		    QStringList &subdirs = v["SUBDIRS"];
		    if(QFile::exists(fi.filePath() + QDir::separator() + fi.fileName() + ".pro") &&
		       !subdirs.contains(newdir)) {
			subdirs.append(newdir);
		    } else {
			QDir d(newdir, "*.pro");
			d.setFilter(QDir::Files);
			for(int i = 0; i < (int)d.count(); i++) {
			    QString nd = newdir + QDir::separator() + d[i];
			    fileFixify(nd);
			    if(d[i] != "." && d[i] != ".." && !subdirs.contains(nd)) {
				if(newdir + d[i] != Option::output_dir + Option::output.name())
				    subdirs.append(nd);
			    }
			}
		    }
		    if(Option::projfile::do_recursive) {
			QDir d(newdir);
			d.setFilter(QDir::Dirs);
			for(int i = 0; i < (int)d.count(); i++) {
			    QString nd = newdir + QDir::separator() + d[i];
			    fileFixify(nd);
			    if(d[i] != "." && d[i] != ".." && !dirs.contains(nd))
				dirs.append(nd);
			}
		    }
		}
	    } else { //regexp
		QString regx = (*pd), dir;
		int s = regx.findRev(Option::dir_sep);
		if(s != -1) {
		    dir = regx.left(s+1);
		    regx = regx.right(regx.length() - (s+1));
		}
		QDir d(dir, regx);
		d.setFilter(QDir::Dirs);
		QStringList &subdirs = v["SUBDIRS"];
		for(int i = 0; i < (int)d.count(); i++) {
		    QString newdir(dir + d[i]);
		    QFileInfo fi(newdir);
		    if(fi.fileName() != "." && fi.fileName() != "..") {
			fileFixify(newdir);
			if(QFile::exists(fi.filePath() + QDir::separator() + fi.fileName() + ".pro") &&
			   !subdirs.contains(newdir)) {
			   subdirs.append(newdir);
			} else {
			    QDir d(newdir, "*.pro");
			    d.setFilter(QDir::Files);
			    for(int i = 0; i < (int)d.count(); i++) {
				QString nd = newdir + QDir::separator() + d[i];
				fileFixify(nd);
				if(d[i] != "." && d[i] != ".." && !subdirs.contains(nd)) {
				    if(newdir + d[i] != Option::output_dir + Option::output.name())
					subdirs.append(nd);
				}
			    }
			}
			if(Option::projfile::do_recursive && !dirs.contains(newdir))
			    dirs.append(newdir);
		    }
		}
	    }
	}
	v["TEMPLATE_ASSIGN"] = "subdirs";
	return;
    }

    QPtrList<MakefileDependDir> deplist;
    deplist.setAutoDelete(TRUE);
    {
	QStringList &d = v["DEPENDPATH"];
	for(QStringList::Iterator it = d.begin(); it != d.end(); ++it) {
	    QString r = (*it), l = Option::fixPathToLocalOS((*it));
	    deplist.append(new MakefileDependDir(r, l));
	}
    }
    QStringList &h = v["HEADERS"];
    bool no_qt_files = TRUE;
    QString srcs[] = { "SOURCES", "YACCSOURCES", "LEXSOURCES", "INTERFACES", QString::null };
    for(int i = 0; !srcs[i].isNull(); i++) {
	QStringList &l = v[srcs[i]];
	for(QStringList::Iterator val_it = l.begin(); val_it != l.end(); ++val_it) {
	    if(generateDependencies(deplist, (*val_it), TRUE)) {
		QStringList &tmp = findDependencies((*val_it));
		if(!tmp.isEmpty()) {
		    for(QStringList::Iterator dep_it = tmp.begin(); dep_it != tmp.end(); ++dep_it) {
			QString file_no_path = (*dep_it).right(
			    (*dep_it).length() - ((*dep_it).findRev(Option::dir_sep)+1));
			if(no_qt_files && file_no_path.find(QRegExp("^q[a-z_0-9].h$")) != -1)
			    no_qt_files = FALSE;
			QString h_ext;
			for(QStringList::Iterator hit = Option::h_ext.begin(); hit != Option::h_ext.end(); ++hit) {
			    if((*dep_it).right((*hit).length()) == (*hit)) {
				h_ext = (*hit);
				break;
			    }
			}
			if(!h_ext.isEmpty()) {
			    if((*dep_it).left(1).lower() == "q") {
				QString qhdr = (*dep_it).lower();
				if(file_no_path == "qthread.h")
				    addConfig("thread");
			    }
			    for(QStringList::Iterator cppit = Option::cpp_ext.begin();
				cppit != Option::cpp_ext.end(); ++cppit) {
				QString src((*dep_it).left((*dep_it).length() - h_ext.length()) + (*cppit));
				if(QFile::exists(src)) {
				    bool exists = FALSE;
				    QStringList &srcl = v["SOURCES"];
				    for(QStringList::Iterator src_it = srcl.begin(); src_it != srcl.end(); ++src_it) {
					if((*src_it).lower() == src.lower()) {
					    exists = TRUE;
					    break;
					}
				    }
				    if(!exists)
					srcl.append(src);
				}
			    }
			} else if((*dep_it).right(2) == Option::lex_ext &&
				  file_no_path.left(Option::lex_mod.length()) == Option::lex_mod) {
			    addConfig("lex_included");
			}
			if(!h.contains((*dep_it))) {
			    if(generateMocList((*dep_it)) && !findMocDestination((*dep_it)).isEmpty())
				h += (*dep_it);
			}
		    }
		}
	    }
	}
    }
    if(h.isEmpty())
	addConfig("moc", FALSE);

    //if we find a file that matches an forms it needn't be included in the project
    QStringList &u = v["INTERFACES"];
    QString no_ui[] = { "SOURCES", "HEADERS", QString::null };
    {
	for(int i = 0; !no_ui[i].isNull(); i++) {
	    QStringList &l = v[no_ui[i]];
	    for(QStringList::Iterator val_it = l.begin(); val_it != l.end(); ) {
		bool found = FALSE;
		for(QStringList::Iterator ui_it = u.begin(); ui_it != u.end(); ++ui_it) {
		    QString s1 = (*val_it).right((*val_it).length() - ((*val_it).findRev(Option::dir_sep) + 1));
		    if(s1.findRev('.') != -1)
			s1 = s1.left(s1.findRev('.')) + Option::ui_ext;
		    QString u1 = (*ui_it).right((*ui_it).length() - ((*ui_it).findRev(Option::dir_sep) + 1));
		    if(s1 == u1) {
			found = TRUE;
			break;
		    }
		}
		if(!found && (*val_it).right(Option::moc_ext.length()) == Option::moc_ext)
		    found = TRUE;
		if(found)
		    val_it = l.remove(val_it);
		else
		    ++val_it;
	    }
	}
    }
}


bool
ProjectGenerator::writeMakefile(QTextStream &t)
{
    t << "######################################################################" << endl;
    t << "# Automatically generated by qmake (" << qmake_version() << ") " << QDateTime::currentDateTime().toString() << endl;
    t << "######################################################################" << endl << endl;
    QStringList::Iterator it;
    for(it = Option::before_user_vars.begin(); it != Option::before_user_vars.end(); ++it)
	t << (*it) << endl;
    t << getWritableVar("TEMPLATE_ASSIGN", FALSE);
    if(project->first("TEMPLATE_ASSIGN") == "subdirs") {
	t << endl << "# Directories" << "\n"
	  << getWritableVar("SUBDIRS");
    } else {
	t << getWritableVar("TARGET")
	  << getWritableVar("CONFIG", FALSE)
	  << getWritableVar("CONFIG_REMOVE", FALSE)
	  << getWritableVar("DEPENDPATH") << endl;

	t << "# Input" << "\n";
	t << getWritableVar("HEADERS") 
	  << getWritableVar("INTERFACES") 
	  << getWritableVar("LEXSOURCES") 
	  << getWritableVar("YACCSOURCES") 
	  << getWritableVar("SOURCES");
    }
    for(it = Option::after_user_vars.begin(); it != Option::after_user_vars.end(); ++it)
	t << (*it) << endl;
    return TRUE;
}

bool
ProjectGenerator::addConfig(const QString &cfg, bool add)
{
    QString where = "CONFIG";
    if(!add)
	where = "CONFIG_REMOVE";
    if(!project->variables()[where].contains(cfg)) {
	project->variables()[where] += cfg;
	return TRUE;
    }
    return FALSE;
}


bool
ProjectGenerator::addFile(QString file)
{
    fileFixify(file, QDir::currentDirPath());
    QString dir;
    int s = file.findRev(Option::dir_sep);
    if(s != -1)
	dir = file.left(s+1);
    if(file.mid(dir.length(), Option::moc_mod.length()) == Option::moc_mod)
	return FALSE;

    QString where;
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit) {
	if(file.right((*cppit).length()) == (*cppit)) {
	    if(QFile::exists(file.left(file.length() - (*cppit).length()) + Option::ui_ext))
		return FALSE;
	    else
		where = "SOURCES";
	    break;
	}
    }
    if(where.isEmpty()) {
	for(QStringList::Iterator hit = Option::h_ext.begin(); hit != Option::h_ext.end(); ++hit) {
	    if(file.right((*hit).length()) == (*hit)) {
		where = "HEADERS";
		break;
	    }
	}
    }
    if(where.isEmpty()) {
	if(file.right(Option::ui_ext.length()) == Option::ui_ext) {
	    where = "INTERFACES";
	} else if(file.right(2) == ".c") {
	    where = "SOURCES";
	} else if(file.right(Option::lex_ext.length()) == Option::lex_ext) {
	    where = "LEXSOURCES";
	} else if(file.right(Option::yacc_ext.length()) == Option::yacc_ext) {
	    where = "YACCSOURCES";
	}
    }

    QString newfile = file;
    fileFixify(newfile);
    if(!where.isEmpty() && !project->variables()[where].contains(file)) {
	project->variables()[where] += newfile;
	return TRUE;
    }
    return FALSE;
}


QString
ProjectGenerator::getWritableVar(const QString &v, bool /*fixPath*/)
{
    QStringList &vals = project->variables()[v];
    if(vals.isEmpty())
	return "";

    QString ret;
    if(v.right(7) == "_REMOVE")
	ret = v.left(v.length() - 7) + " -= ";
    else if(v.right(7) == "_ASSIGN")
	ret = v.left(v.length() - 7) + " = ";
    else
	ret = v + " += ";
    QString join = vals.join(" ");
    if(ret.length() + join.length() > 80) {
	QString spaces;
	for(unsigned int i = 0; i < ret.length(); i++)
	    spaces += " ";
	join = vals.join(" \\\n" + spaces);
    } 
    // ### Commented out for now so that project generation works.
    // Sam: can you look at why this was needed?
    /*    if(fixPath)
	join = join.replace("\\", "/");*/
    return ret + join + "\n";
}

QString
ProjectGenerator::defaultMakefile() const
{
    QString dir = QDir::currentDirPath();
    int s = dir.findRev('/');
    if(s != -1)
	dir = dir.right(dir.length() - (s + 1));
    return dir + ".pro";
}
