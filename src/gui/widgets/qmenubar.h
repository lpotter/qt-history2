/****************************************************************************
**
** Definition of QMenuBar class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QMENUBAR_H
#define QMENUBAR_H

#include "qmenu.h"

class QMenuBarPrivate;
#ifdef QT_COMPAT
class QMenuItem;
#endif

class Q_GUI_EXPORT QMenuBar : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QMenuBar);
    Q_PROPERTY(bool defaultUp READ isDefaultUp WRITE setDefaultUp)

public:
    QMenuBar(QWidget *parent = 0);
    ~QMenuBar();

    QAction *addMenu(const QString &title, QMenu *menu);
    QAction *insertMenu(QAction *before, const QString &title, QMenu *menu);

    void clear();

    QAction *activeAction() const;

    void setDefaultUp(bool);
    bool isDefaultUp() const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    int heightForWidth(int) const;

    QRect actionGeometry(QAction *) const;
    QAction *actionAtPos(const QPoint &) const;

#ifdef Q_WS_MAC
    MenuRef macMenu();
#endif

signals:
    void activated(QAction *action);
    void highlighted(QAction *action);

protected:
    void contextMenuEvent(QContextMenuEvent *);
    void changeEvent(QEvent *);
    void keyPressEvent(QKeyEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void leaveEvent(QEvent *);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void actionEvent(QActionEvent *);
    void focusOutEvent(QFocusEvent *);
    void focusInEvent(QFocusEvent *);
    bool eventFilter(QObject *, QEvent *);
    bool event(QEvent *);

private slots:
    void internalShortcutActivated(int);

protected:
    void setLeftWidget(QWidget *);
    void setRightWidget(QWidget *);

#ifdef QT_COMPAT
public:
    inline QT_COMPAT uint count() const { return actions().count(); }
    inline QT_COMPAT int insertItem(const QString &text, const QObject *receiver, const char* member,
                                    const QKeySequence& accel = 0, int id = -1, int index = -1) {
        return insertAny(0, &text, receiver, member, &accel, 0, id, index);
    }
    inline QT_COMPAT int insertItem(const QIconSet& icon, const QString &text,
                                    const QObject *receiver, const char* member,
                                    const QKeySequence& accel = 0, int id = -1, int index = -1) {
        return insertAny(&icon, &text, receiver, member, &accel, 0, id, index);
    }
    inline QT_COMPAT int insertItem(const QPixmap &pixmap, const QObject *receiver, const char* member,
                                    const QKeySequence& accel = 0, int id = -1, int index = -1) {
        QIconSet icon(pixmap);
        return insertAny(&icon, 0, receiver, member, &accel, 0, id, index);
    }
    inline QT_COMPAT int insertItem(const QString &text, int id=-1, int index=-1) {
        return insertAny(0, &text, 0, 0, 0, 0, id, index);
    }
    inline QT_COMPAT int insertItem(const QIconSet& icon, const QString &text, int id=-1, int index=-1) {
        return insertAny(&icon, &text, 0, 0, 0, 0, id, index);
    }
    inline QT_COMPAT int insertItem(const QString &text, QMenu *popup, int id=-1, int index=-1) {
        return insertAny(0, &text, 0, 0, 0, popup, id, index);
    }
    inline QT_COMPAT int insertItem(const QIconSet& icon, const QString &text, QMenu *popup, int id=-1, int index=-1) {
        return insertAny(&icon, &text, 0, 0, 0, popup, id, index);
    }
    inline QT_COMPAT int insertItem(const QPixmap &pixmap, int id=-1, int index=-1) {
        QIconSet icon(pixmap);
        return insertAny(&icon, 0, 0, 0, 0, 0, id, index);
    }
    inline QT_COMPAT int insertItem(const QPixmap &pixmap, QMenu *popup, int id=-1, int index=-1) {
        QIconSet icon(pixmap);
        return insertAny(&icon, 0, 0, 0, 0, popup, id, index);
    }
    QT_COMPAT int insertSeparator(int index=-1);
    inline QT_COMPAT void removeItem(int id) {
        if(QAction *act = findActionForId(id)) 
            removeAction(act); }
    inline QT_COMPAT void removeItemAt(int index) { 
        if(QAction *act = actions().value(index))
            removeAction(act); }
#ifndef QT_NO_ACCEL
    inline QT_COMPAT QKeySequence accel(int id) const { 
        if(QAction *act = findActionForId(id))
            return act->accel();
        return QKeySequence(); }
    inline QT_COMPAT void setAccel(const QKeySequence& key, int id) { 
        if(QAction *act = findActionForId(id))
            act->setAccel(key);
    }
#endif
    inline QT_COMPAT QIconSet iconSet(int id) const { 
        if(QAction *act = findActionForId(id))
            return act->icon();
        return QIconSet(); }
    inline QT_COMPAT QString text(int id) const { 
        if(QAction *act = findActionForId(id))
            return act->text();
        return QString(); }
    inline QT_COMPAT QPixmap pixmap(int id) const { 
        if(QAction *act = findActionForId(id))
            return act->icon().pixmap();
        return QString(); }
    inline QT_COMPAT void setWhatsThis(int id, const QString &w) { 
        if(QAction *act = findActionForId(id))
            act->setWhatsThis(w); }
    inline QT_COMPAT QString whatsThis(int id) const { 
        if(QAction *act = findActionForId(id))
            return act->whatsThis(); 
        return QString(); }

    inline QT_COMPAT void changeItem(int id, const QString &text) { 
        if(QAction *act = findActionForId(id))
            act->setText(text); }
    inline QT_COMPAT void changeItem(int id, const QPixmap &pixmap) { 
        if(QAction *act = findActionForId(id))
            act->setIcon(QIconSet(pixmap)); }
    inline QT_COMPAT void changeItem(int id, const QIconSet &icon, const QString &text) {
        if(QAction *act = findActionForId(id)) {
            act->setIcon(icon);
            act->setText(text);
        }
    }
    inline QT_COMPAT bool isItemActive(int id) const { return findActionForId(id) == activeAction(); }
    inline QT_COMPAT bool isItemEnabled(int id) const { 
        if(QAction *act = findActionForId(id)) 
            return act->isEnabled(); 
        return false; }
    inline QT_COMPAT void setItemEnabled(int id, bool enable) { 
        if(QAction *act = findActionForId(id)) 
            act->setEnabled(enable); }
    inline QT_COMPAT bool isItemChecked(int id) const { 
        if(QAction *act = findActionForId(id)) 
            return act->isChecked(); 
        return false; }
    inline QT_COMPAT void setItemChecked(int id, bool check) { 
        if(QAction *act = findActionForId(id)) 
            act->setChecked(check); }
    inline QT_COMPAT bool isItemVisible(int id) const { 
        if(QAction *act = findActionForId(id)) 
            return act->isVisible();
        return false; }
    inline QT_COMPAT void setItemVisible(int id, bool visible) { 
        if(QAction *act = findActionForId(id)) 
            act->setVisible(visible); }
    inline QT_COMPAT QRect itemGeometry(int index) {
        if(QAction *act = actions().value(index)) 
            return actionGeometry(act);
        return QRect();
    }
    inline QT_COMPAT int indexOf(int id) const { return actions().indexOf(findActionForId(id)); }
    inline QT_COMPAT int idAt(int index) const {
        return findIdForAction(actions().value(index));
    }
    inline QT_COMPAT void activateItemAt(int index) {
        if(QAction *ret = actions().value(index))
            ret->activate(QAction::Trigger);
    }
    inline QT_COMPAT bool connectItem(int id, const QObject *receiver, const char* member) {
        if(QAction *act = findActionForId(id)) {
            QObject::connect(act, SIGNAL(triggered()), receiver, member);
            return true;
        }
        return false;
    }
    inline QT_COMPAT bool disconnectItem(int id,const QObject *receiver, const char* member) {
        if(QAction *act = findActionForId(id)) {
            QObject::disconnect(act, SIGNAL(triggered()), receiver, member);
            return true;
        } 
        return false;
    }
    inline QT_COMPAT QMenuItem *findItem(int id) const {
        return (QMenuItem*)findActionForId(id);
    }
    QT_COMPAT bool setItemParameter(int id, int param);
    QT_COMPAT int itemParameter(int id) const;

    //frame
    QT_COMPAT int frameWidth() const;

signals:
    QT_COMPAT void activated(int itemId);
    QT_COMPAT void highlighted(int itemId);

private slots:
    void compatActivated(QAction *);
    void compatHighlighted(QAction *);

protected:
    inline QT_COMPAT int itemAtPos(const QPoint &p) {
        return findIdForAction(actionAtPos(p));
    }

private:
    QAction *findActionForId(int id) const;
    int insertAny(const QIconSet *icon, const QString *text, const QObject *receiver, const char *member,
                  const QKeySequence *accel, const QMenu *popup, int id, int index);
    int findIdForAction(QAction*) const;
#endif

private:
    friend class QMenu;
    friend class QWorkspacePrivate;
    friend class QMenuPrivate;

#ifdef Q_WS_MAC
    friend class QApplication;
    static bool macUpdateMenuBar();
    friend bool qt_mac_activate_action(MenuRef, uint, QAction::ActionEvent, bool);
#endif
#if defined(Q_DISABLE_COPY)  // Disabled copy constructor and operator=
    QMenuBar(const QMenuBar &);
    QMenuBar &operator=(const QMenuBar &);
#endif
};
#endif // QMENUBAR_H
