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

#ifndef QDND_P_H
#define QDND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qobject.h"
#include "QtCore/qmap.h"
#include "QtGui/qmime.h"
#include "QtGui/qdrag.h"
#include "QtGui/qpixmap.h"
#include "QtCore/qpoint.h"
#include "private/qobject_p.h"
#ifdef Q_WS_MAC
# include "private/qt_mac_p.h"
#endif

#if defined(Q_WS_WIN)
# include <windows.h>
# include <objidl.h>
#endif

#ifndef QT_NO_DRAGANDDROP

QT_BEGIN_NAMESPACE

class QEventLoop;

class QDragPrivate : public QObjectPrivate
{
public:
    QWidget *source;
    QWidget *target;
    QMimeData *data;
    QPixmap pixmap;
    QPoint hotspot;
    Qt::DropActions possible_actions;
    Qt::DropAction executed_action;
    QMap<Qt::DropAction, QPixmap> customCursors;
    Qt::DropAction defaultDropAction;
};

class QInternalMimeData : public QMimeData
{
    Q_OBJECT
public:
    QInternalMimeData();
    ~QInternalMimeData();

    bool hasFormat(const QString &mimeType) const;
    QStringList formats() const;
    static bool canReadData(const QString &mimeType);


    static QStringList formatsHelper(const QMimeData *data);
    static bool hasFormatHelper(const QString &mimeType, const QMimeData *data);
    static QByteArray renderDataHelper(const QString &mimeType, const QMimeData *data);

protected:
    QVariant retrieveData(const QString &mimeType, QVariant::Type type) const;

    virtual bool hasFormat_sys(const QString &mimeType) const = 0;
    virtual QStringList formats_sys() const = 0;
    virtual QVariant retrieveData_sys(const QString &mimeType, QVariant::Type type) const = 0;
};

class QDropData : public QInternalMimeData
{
    Q_OBJECT
public:
    QDropData();
    ~QDropData();

protected:
    bool hasFormat_sys(const QString &mimeType) const;
    QStringList formats_sys() const;
    QVariant retrieveData_sys(const QString &mimeType, QVariant::Type type) const;

#if defined(Q_WS_WIN)
public:
    LPDATAOBJECT currentDataObject;
#endif
};

class Q_GUI_EXPORT QDragManager: public QObject {
    Q_OBJECT

    QDragManager();
    ~QDragManager();
    // only friend classes can use QDragManager.
    friend class QDrag;
    friend class QDragMoveEvent;
    friend class QDropEvent;
    friend class QApplication;
#ifdef Q_WS_MAC
    friend class QWidgetPrivate; //dnd is implemented here
#endif

    bool eventFilter(QObject *, QEvent *);
    void timerEvent(QTimerEvent*);

public:
    Qt::DropAction drag(QDrag *);

    void cancel(bool deleteSource = true);
    void move(const QPoint &);
    void drop();
    void updatePixmap();
    QWidget *source() const { return object ? object->d_func()->source : 0; }
    QDragPrivate *dragPrivate() const { return object ? object->d_func() : 0; }
    static QDragPrivate *dragPrivate(QDrag *drag) { return drag ? drag->d_func() : 0; }

    static QDragManager *self();
    Qt::DropAction defaultAction(Qt::DropActions possibleActions,
                                 Qt::KeyboardModifiers modifiers) const;

    QDrag *object;

    void updateCursor();

    bool beingCancelled;
    bool restoreCursor;
    bool willDrop;
    QEventLoop *eventLoop;

    QPixmap dragCursor(Qt::DropAction action) const;

    bool hasCustomDragCursors() const;

    QDropData *dropData;

    void emitActionChanged(Qt::DropAction newAction) { if (object) emit object->actionChanged(newAction); }

    void setCurrentTarget(QWidget *target, bool dropped = false);
    QWidget *currentTarget();

#ifdef Q_WS_X11
    QPixmap xdndMimeTransferedPixmap[2];
    int xdndMimeTransferedPixmapIndex;
#endif

private:
    QPixmap *pm_cursor;
    int n_cursor;
#ifdef Q_WS_QWS
    Qt::DropAction currentActionForOverrideCursor;
#endif

    QWidget *currentDropTarget;

    static QDragManager *instance;
    Q_DISABLE_COPY(QDragManager)
};


#if defined(Q_WS_WIN)

class QOleDataObject : public IDataObject
{
public:
    explicit QOleDataObject(QMimeData *mimeData);
    virtual ~QOleDataObject();

    void releaseQt();
    const QMimeData *mimeData() const;
    DWORD reportedPerformedEffect() const;

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IDataObject methods
    STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHOD(GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHOD(QueryGetData)(LPFORMATETC pformatetc);
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut);
    STDMETHOD(SetData)(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,
                       BOOL fRelease);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc);
    STDMETHOD(DAdvise)(FORMATETC FAR* pFormatetc, DWORD advf,
                      LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
    STDMETHOD(DUnadvise)(DWORD dwConnection);
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA FAR* ppenumAdvise);

private:
    ULONG m_refs;
    QPointer<QMimeData> data;
    int CF_PERFORMEDDROPEFFECT;
    DWORD performedEffect;
};


class QOleEnumFmtEtc : public IEnumFORMATETC
{
public:
    explicit QOleEnumFmtEtc(const QVector<FORMATETC> &fmtetcs);
    explicit QOleEnumFmtEtc(const QVector<LPFORMATETC> &lpfmtetcs);
    virtual ~QOleEnumFmtEtc();

    bool isNull() const;

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IEnumFORMATETC methods
    STDMETHOD(Next)(ULONG celt, LPFORMATETC rgelt, ULONG FAR* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(LPENUMFORMATETC FAR* newEnum);

private:
    bool copyFormatEtc(LPFORMATETC dest, LPFORMATETC src) const;

    ULONG m_dwRefs;
    ULONG m_nIndex;
    QVector<LPFORMATETC> m_lpfmtetcs;
    bool m_isNull;
};

class QOleDropTarget : public IDropTarget
{
public:
    QOleDropTarget(QWidget* w);

    void releaseQt();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IDropTarget methods
    STDMETHOD(DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

private:
    ULONG m_refs;
    QWidget* widget;
    QPointer<QWidget> currentWidget;
    QRect answerRect;
    QPoint lastPoint;
    DWORD choosenEffect;
    DWORD lastKeyState;

    void sendDragEnterEvent(QWidget *to, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
};

#endif

QT_END_NAMESPACE

#endif // QT_NO_DRAGANDDROP

#endif // QDND_P_H
