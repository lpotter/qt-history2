/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "q3filedialog.h"

#ifndef QT_NO_FILEDIALOG

/*****************************************************************************
  Q3FileDialog debug facilities
 *****************************************************************************/
//#define DEBUG_FILEDIALOG_FILTERS

#include "qapplication.h"
#include <private/qapplication_p.h>
#include <private/qt_mac_p.h>
#include "qregexp.h"
#include "qbuffer.h"
#include "qstringlist.h"
#include "qtextcodec.h"
#include "qdesktopwidget.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  Externals
 *****************************************************************************/
extern WindowPtr qt_mac_window_for(const QWidget*); //qwidget_mac.cpp
extern const char qt3_file_dialog_filter_reg_exp[]; //qfiledialog.cpp

/*****************************************************************************
  Q3FileDialog utility functions
 *****************************************************************************/
static UInt8 *str_buffer = NULL;
static void cleanup_str_buffer()
{
    if(str_buffer) {
        free(str_buffer);
        str_buffer = NULL;
    }
}

// Returns the wildcard part of a filter.
struct qt_mac_filter_name {
    QString description, regxp, filter;
};
static qt_mac_filter_name *extractFilter(const QString& rawFilter)
{
    qt_mac_filter_name *ret = new qt_mac_filter_name;
    ret->filter = rawFilter;
    QString result = rawFilter;
    QRegExp r(QString::fromLatin1(qt3_file_dialog_filter_reg_exp));
    int index = r.indexIn(result);
    if(index >= 0) {
        ret->description = r.cap(1).trimmed();
        result = r.cap(2);
    }
    if(ret->description.isEmpty())
        ret->description = result;
    ret->regxp = result.replace(QLatin1Char(' '), QLatin1Char(';'));
    return ret;
}

// Makes a list of filters from ;;-separated text.
static QList<qt_mac_filter_name*> makeFiltersList(const QString &filter)
{
#ifdef DEBUG_FILEDIALOG_FILTERS
    qDebug("Q3FileDialog:%d - Got filter (%s)", __LINE__, filter.latin1());
#endif
    QString f(filter);
    if(f.isEmpty())
        f = Q3FileDialog::tr("All Files (*)");
    if(f.isEmpty())
        return QList<qt_mac_filter_name*>();
    QString sep(QLatin1String(";;"));
    int i = f.indexOf(sep, 0);
    if(i == -1) {
        sep = QLatin1String("\n");
        if(f.indexOf(sep, 0) != -1)
            i = f.indexOf(sep, 0);
    }

    QList<qt_mac_filter_name*> ret;
    QStringList filts = f.split(sep);
    for (QStringList::Iterator it = filts.begin(); it != filts.end(); ++it) {
        qt_mac_filter_name *filter = extractFilter((*it));
#ifdef DEBUG_FILEDIALOG_FILTERS
        qDebug("Q3FileDialog:%d Split out filter (%d) '%s' '%s'", __LINE__, ret.count(),
               filter->regxp.latin1(), filter->description.latin1());
#endif
        ret.append(filter);
    }
    return ret;
}

struct qt_mac_nav_filter_type {
    int index;
    QList<qt_mac_filter_name*> *filts;
};

static Boolean qt_mac_nav_filter(AEDesc *theItem, void *info,
                                             void *myd, NavFilterModes)
{
    qt_mac_nav_filter_type *t = (qt_mac_nav_filter_type *)myd;
    if(!t || !t->filts || t->index >= t->filts->count())
        return true;

    NavFileOrFolderInfo *theInfo = (NavFileOrFolderInfo *)info;
    QString file;
    qt_mac_filter_name *fn = t->filts->at(t->index);
    if(!fn)
        return true;
    if(theItem->descriptorType == typeFSRef) {
        FSRef ref;
        AEGetDescData(theItem, &ref, sizeof(ref));
        if(!str_buffer) {
            qAddPostRoutine(cleanup_str_buffer);
            str_buffer = (UInt8 *)malloc(1024);
        }
        FSRefMakePath(&ref, str_buffer, 1024);
        file = QString::fromUtf8((const char *)str_buffer);
        int slsh = file.lastIndexOf(QLatin1Char('/'));
        if(slsh != -1)
            file = file.right(file.length() - slsh - 1);
    }
    QStringList reg = fn->regxp.split(QLatin1String(";"));
    for(QStringList::Iterator it = reg.begin(); it != reg.end(); ++it) {
        QRegExp rg((*it), false, true);
#ifdef DEBUG_FILEDIALOG_FILTERS
        qDebug("Q3FileDialog:%d, asked to filter.. %s (%s)", __LINE__,
               file.latin1(), (*it).latin1());
#endif
        if(rg.exactMatch(file))
            return true;
    }
    return (theInfo->isFolder && !file.endsWith(QLatin1String(".app")));
}

//filter UPP stuff
static NavObjectFilterUPP mac_navFilterUPP = NULL;
static void cleanup_navFilterUPP()
{
    DisposeNavObjectFilterUPP(mac_navFilterUPP);
    mac_navFilterUPP = NULL;
}
static const NavObjectFilterUPP make_navFilterUPP()
{
    if(mac_navFilterUPP)
        return mac_navFilterUPP;
    qAddPostRoutine(cleanup_navFilterUPP);
    return mac_navFilterUPP = NewNavObjectFilterUPP(qt_mac_nav_filter);
}
//event UPP stuff
static NavEventUPP mac_navProcUPP = NULL;
static void cleanup_navProcUPP()
{
    DisposeNavEventUPP(mac_navProcUPP);
    mac_navProcUPP = NULL;
}
static bool g_nav_blocking=true;
static void qt_mac_filedialog_event_proc(const NavEventCallbackMessage msg,
                                                     NavCBRecPtr p, NavCallBackUserData myd)
{
    switch(msg) {
    case kNavCBPopupMenuSelect: {
        qt_mac_nav_filter_type *t = (qt_mac_nav_filter_type *)myd;
        NavMenuItemSpec *s = (NavMenuItemSpec*)p->eventData.eventDataParms.param;
        t->index = s->menuType;
#ifdef DEBUG_FILEDIALOG_FILTERS
        qDebug("Q3FileDialog:%d - Selected a filter: %ld", __LINE__, s->menuType);
#endif
        break; }
    case kNavCBStart:
        g_nav_blocking=true;
        break;
    case kNavCBUserAction:
        g_nav_blocking=false;
        break;
    }
}
static const NavEventUPP make_navProcUPP()
{
    if(mac_navProcUPP)
        return mac_navProcUPP;
    qAddPostRoutine(cleanup_navProcUPP);
    return mac_navProcUPP = NewNavEventUPP(qt_mac_filedialog_event_proc);
}


extern OSErr qt_mac_create_fsref(const QString &, FSRef *); //qglobal.cpp

QStringList Q3FileDialog::macGetOpenFileNames(const QString &filter, QString *pwd,
                                             QWidget *parent, const char* /*name*/,
                                             const QString& caption, QString *selectedFilter,
                                             bool multi, bool directory)
{
    OSErr err;
    QStringList retstrl;

    NavDialogCreationOptions options;
    NavGetDefaultDialogCreationOptions(&options);
    options.modality = kWindowModalityAppModal;
    options.optionFlags |= kNavDontConfirmReplacement | kNavSupportPackages;
    if (!multi)
        options.optionFlags &= ~kNavAllowMultipleFiles;
    if(!caption.isEmpty())
        options.windowTitle = CFStringCreateWithCharacters(NULL, (UniChar *)caption.unicode(),
                                                           caption.length());

    static const int w = 450, h = 350;
    options.location.h = options.location.v = -1;
    if(parent && parent->isVisible()) {
        Qt::WindowType wt = parent->window()->windowType();
        if (wt != Qt::Desktop && wt != Qt::Sheet && wt != Qt::Drawer) {
            options.modality = kWindowModalityWindowModal;
            options.parentWindow = qt_mac_window_for(parent);
        } else {
            parent = parent->window();
            QString s = parent->windowTitle();
            options.clientName = CFStringCreateWithCharacters(NULL, (UniChar *)s.unicode(), s.length());
            options.location.h = (parent->x() + (parent->width() / 2)) - (w / 2);
            options.location.v = (parent->y() + (parent->height() / 2)) - (h / 2);

            QRect r = QApplication::desktop()->screenGeometry(
                QApplication::desktop()->screenNumber(parent));
            if(options.location.h + w > r.right())
                options.location.h -= (options.location.h + w) - r.right() + 10;
            if(options.location.v + h > r.bottom())
                options.location.v -= (options.location.v + h) - r.bottom() + 10;
        }
    } else if(QWidget *p = qApp->mainWidget()) {
        static int last_screen = -1;
        int scr = QApplication::desktop()->screenNumber(p);
        if(last_screen != scr) {
            QRect r = QApplication::desktop()->screenGeometry(scr);
            options.location.h = (r.x() + (r.width() / 2)) - (w / 2);
            options.location.v = (r.y() + (r.height() / 2)) - (h / 2);
        }
    }

    QList<qt_mac_filter_name*> filts = makeFiltersList(filter);
    qt_mac_nav_filter_type t;
    t.index = 0;
    t.filts = &filts;
    if(filts.count() > 1) {
        int i = 0;
        CFStringRef *arr = (CFStringRef *)malloc(sizeof(CFStringRef) * filts.count());
        for (QList<qt_mac_filter_name*>::Iterator it = filts.begin(); it != filts.end(); ++it) {
            QString rg = (*it)->description;
            arr[i++] = CFStringCreateWithCharacters(NULL, (UniChar *)rg.unicode(), rg.length());
        }
        options.popupExtension = CFArrayCreate(NULL, (const void **)arr, filts.count(), NULL);
    }

    NavDialogRef dlg;
    if(directory) {
        if(NavCreateChooseFolderDialog(&options, make_navProcUPP(), NULL, NULL, &dlg)) {
            qDebug("Shouldn't happen %s:%d", __FILE__, __LINE__);
            return retstrl;
        }
    } else {
        if(NavCreateGetFileDialog(&options, NULL, make_navProcUPP(), NULL,
                                  make_navFilterUPP(), (void *) (filts.isEmpty() ? NULL : &t),
                                  &dlg)) {
            qDebug("Shouldn't happen %s:%d", __FILE__, __LINE__);
            return retstrl;
        }
    }
    if(pwd && !pwd->isEmpty()) {
        FSRef fsref;
        if(qt_mac_create_fsref(*pwd, &fsref) == noErr) {
            AEDesc desc;
            if(AECreateDesc(typeFSRef, &fsref, sizeof(FSRef), &desc) == noErr)
                NavCustomControl(dlg, kNavCtlSetLocation, (void*)&desc);
        }
    }

    NavDialogRun(dlg);
    if (selectedFilter) {
        NavMenuItemSpec navSpec;
        bzero(&navSpec, sizeof(NavMenuItemSpec));
        qt_mac_filter_name *sel_filt_name = makeFiltersList(*selectedFilter).at(0);
        for (int i = 0; i < filts.count(); ++i) {
            const qt_mac_filter_name *filter = filts.at(i);
            if (sel_filt_name->description == filter->description
                    && sel_filt_name->regxp == filter->regxp
                    && sel_filt_name->filter == filter->filter) {
                navSpec.menuType = i;
                break;
            }
        }
        NavCustomControl(dlg, kNavCtlSelectCustomType, &navSpec);
    }
    if(options.modality == kWindowModalityWindowModal) { //simulate modality
        QWidget modal_widg(parent, __FILE__ "__modal_dlg",
                           Qt::WType_TopLevel | Qt::WStyle_Customize | Qt::WStyle_DialogBorder);
        modal_widg.createWinId();
        QApplicationPrivate::enterModal(&modal_widg);
        while(g_nav_blocking)
            qApp->processEvents(QEventLoop::WaitForMoreEvents);
        QApplicationPrivate::leaveModal(&modal_widg);
    }

    if(!(NavDialogGetUserAction(dlg) &
          (kNavUserActionOpen | kNavUserActionChoose | kNavUserActionNewFolder))) {
        NavDialogDispose(dlg);
        return retstrl;
    }
    NavReplyRecord ret;
    NavDialogGetReply(dlg, &ret);
    NavDialogDispose(dlg);

    long count;
    err = AECountItems(&(ret.selection), &count);
    if(!ret.validRecord || err != noErr || !count) {
        NavDisposeReply(&ret);
        return retstrl;
    }

    for(long index = 1; index <= count; index++) {
        FSRef ref;
        err = AEGetNthPtr(&(ret.selection), index, typeFSRef, 0, 0, &ref, sizeof(ref), 0);
        if(err != noErr)
            break;

        if(!str_buffer) {
            qAddPostRoutine(cleanup_str_buffer);
            str_buffer = (UInt8 *)malloc(1024);
        }
        FSRefMakePath(&ref, str_buffer, 1024);
        retstrl.append(QString::fromUtf8((const char *)str_buffer));
    }
    NavDisposeReply(&ret);
    if(selectedFilter)
        *selectedFilter = filts.at(t.index)->filter;
    while (!filts.isEmpty())
        delete filts.takeFirst();
    return retstrl;
}

// Copious copy and paste from qfiledialog.cpp. Fix in 4.0.
static QString encodeFileName(const QString &fName)
{
    QString newStr;
    QByteArray cName = fName.utf8();
    const QByteArray sChars("<>#@\"&%$:,;?={}|^~[]\'`\\*");

    int len = cName.length();
    if (!len)
        return QString();
    for (int i = 0; i < len ;++i) {
        uchar inCh = (uchar)cName[i];
        if (inCh >= 128 || sChars.contains(inCh))
        {
            newStr += QLatin1Char('%');
            ushort c = inCh / 16;
            c += c > 9 ? 'A' - 10 : '0';
            newStr += QLatin1Char((char)c);
            c = inCh % 16;
            c += c > 9 ? 'A' - 10 : '0';
            newStr += QLatin1Char((char)c);
        } else {
            newStr += QLatin1Char((char)inCh);
        }
    }
    return newStr;
}

QString Q3FileDialog::macGetSaveFileName(const QString &start, const QString &filter,
                                         QString *, QWidget *parent, const char* /*name*/,
                                         const QString& caption, QString *selectedFilter)
{
    OSErr err;
    QString retstr;
    NavDialogCreationOptions options;
    NavGetDefaultDialogCreationOptions(&options);
    static const int w = 450, h = 350;
    options.optionFlags |= kNavDontConfirmReplacement;
    options.modality = kWindowModalityAppModal;
    options.location.h = options.location.v = -1;
    QString workingDir;
    QString initialSelection;
    if (!start.isEmpty()) {
        Q3UrlOperator u(encodeFileName(start));
        if (u.isLocalFile() && QFileInfo(u.path()).isDir()) {
            workingDir = start;
        } else {
            if (u.isLocalFile()) {
                QFileInfo fi(u.dirPath());
                if (fi.exists()) {
                    workingDir = u.dirPath();
                    initialSelection = u.fileName();
                }
            } else {
                workingDir = u.toString();
            }
        }
        if (!initialSelection.isEmpty())
            options.saveFileName = CFStringCreateWithCharacters(0,
                                                            (UniChar *)initialSelection.unicode(),
                                                            initialSelection.length());
    }
    if(!caption.isEmpty())
        options.windowTitle = CFStringCreateWithCharacters(NULL, (UniChar *)caption.unicode(),
                                                           caption.length());
    if(parent && parent->isVisible()) {
        Qt::WindowType wt = parent->window()->windowType();
        if (wt != Qt::Desktop && wt != Qt::Sheet && wt != Qt::Drawer) {
            options.modality = kWindowModalityWindowModal;
            options.parentWindow = qt_mac_window_for(parent);
        } else {
            parent = parent->window();
            QString s = parent->windowTitle();
            options.clientName = CFStringCreateWithCharacters(NULL, (UniChar *)s.unicode(), s.length());
            options.location.h = (parent->x() + (parent->width() / 2)) - (w / 2);
            options.location.v = (parent->y() + (parent->height() / 2)) - (h / 2);

            QRect r = QApplication::desktop()->screenGeometry(
                QApplication::desktop()->screenNumber(parent));
            if(options.location.h + w > r.right())
                options.location.h -= (options.location.h + w) - r.right() + 10;
            if(options.location.v + h > r.bottom())
                options.location.v -= (options.location.v + h) - r.bottom() + 10;
        }
    } else if(QWidget *p = qApp->mainWidget()) {
        static int last_screen = -1;
        int scr = QApplication::desktop()->screenNumber(p);
        if(last_screen != scr) {
            QRect r = QApplication::desktop()->screenGeometry(scr);
            options.location.h = (r.x() + (r.width() / 2)) - (w / 2);
            options.location.v = (r.y() + (r.height() / 2)) - (h / 2);
        }
    }

    QList<qt_mac_filter_name*> filts = makeFiltersList(filter);
    qt_mac_nav_filter_type t;
    t.index = 0;
    t.filts = &filts;
    if(filts.count() > 1) {
        int i = 0;
        CFStringRef *arr = (CFStringRef *)malloc(sizeof(CFStringRef) * filts.count());
        for (QList<qt_mac_filter_name*>::Iterator it = filts.begin(); it != filts.end(); ++it) {
            QString rg = (*it)->description;
            arr[i++] = CFStringCreateWithCharacters(NULL, (UniChar *)rg.unicode(), rg.length());
        }
        options.popupExtension = CFArrayCreate(NULL, (const void **)arr, filts.count(), NULL);
    }

    NavDialogRef dlg;
    if(NavCreatePutFileDialog(&options, 'cute', kNavGenericSignature, make_navProcUPP(),
                              (void *) (filts.isEmpty() ? NULL : &t), &dlg)) {
        qDebug("Shouldn't happen %s:%d", __FILE__, __LINE__);
        return retstr;
    }
    if (!workingDir.isEmpty()) {
        FSRef fsref;
        if (qt_mac_create_fsref(workingDir, &fsref) == noErr) {
            AEDesc desc;
            if (AECreateDesc(typeFSRef, &fsref, sizeof(FSRef), &desc) == noErr)
                NavCustomControl(dlg, kNavCtlSetLocation, (void*)&desc);
        }
    }
    NavDialogRun(dlg);
    if (selectedFilter) {
        NavMenuItemSpec navSpec;
        bzero(&navSpec, sizeof(NavMenuItemSpec));
        qt_mac_filter_name *sel_filt_name = makeFiltersList(*selectedFilter).at(0);
        for (int i = 0; i < filts.count(); ++i) {
            const qt_mac_filter_name *filter = filts.at(i);
            if (sel_filt_name->description == filter->description
                    && sel_filt_name->regxp == filter->regxp
                    && sel_filt_name->filter == filter->filter) {
                navSpec.menuType = i;
                break;
            }
        }
        NavCustomControl(dlg, kNavCtlSelectCustomType, &navSpec);
    }
    if(options.modality == kWindowModalityWindowModal) { //simulate modality
        QWidget modal_widg(parent, __FILE__ "__modal_dlg",
                           Qt::WType_TopLevel | Qt::WStyle_Customize | Qt::WStyle_DialogBorder);
        modal_widg.createWinId();
        QApplicationPrivate::enterModal(&modal_widg);
        while(g_nav_blocking)
            qApp->processEvents(QEventLoop::WaitForMoreEvents);
        QApplicationPrivate::leaveModal(&modal_widg);
    }

    if(NavDialogGetUserAction(dlg) != kNavUserActionSaveAs) {
        NavDialogDispose(dlg);
        return retstr;
    }
    NavReplyRecord ret;
    NavDialogGetReply(dlg, &ret);
    NavDialogDispose(dlg);

    long count;
    err = AECountItems(&(ret.selection), &count);
    if(!ret.validRecord || err != noErr || !count) {
        NavDisposeReply(&ret);
        return retstr;
    }

    AEKeyword        keyword;
    DescType    type;
    Size        size;
    FSRef ref;
    err = AEGetNthPtr(&(ret.selection), 1, typeFSRef, &keyword,
                      &type, &ref, sizeof(ref), &size);
    if(err == noErr) {
        if(!str_buffer) {
            qAddPostRoutine(cleanup_str_buffer);
            str_buffer = (UInt8 *)malloc(1024);
        }
        FSRefMakePath(&ref, str_buffer, 1024);
        retstr = QString::fromUtf8((const char *)str_buffer);
        //now filename
        CFStringGetCString(ret.saveFileName, (char *)str_buffer, 1024, kCFStringEncodingUTF8);
        retstr += QLatin1String("/") + QString::fromUtf8((const char *)str_buffer);
    }
    NavDisposeReply(&ret);
    if(selectedFilter)
        *selectedFilter = filts.at(t.index)->filter;
    while (!filts.isEmpty())
        delete filts.takeFirst();
    return retstr;
}

QT_END_NAMESPACE

#endif
