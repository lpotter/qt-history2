#ifndef QTTOOLBARDIALOG_H
#define QTTOOLBARDIALOG_H

#include <QDialog>

#if defined(Q_WS_WIN)
#  if !defined(QT_QTTOOLBARDIALOG_EXPORT) && !defined(QT_QTTOOLBARDIALOG_IMPORT)
#    define QT_QTTOOLBARDIALOG_EXPORT
#  elif defined(QT_QTTOOLBARDIALOG_IMPORT)
#    if defined(QT_QTTOOLBARDIALOG_EXPORT)
#      undef QT_QTTOOLBARDIALOG_EXPORT
#    endif
#    define QT_QTTOOLBARDIALOG_EXPORT __declspec(dllimport)
#  elif defined(QT_QTTOOLBARDIALOG_EXPORT)
#    undef QT_QTTOOLBARDIALOG_EXPORT
#    define QT_QTTOOLBARDIALOG_EXPORT __declspec(dllexport)
#  endif
#else
#  define QT_QTTOOLBARDIALOG_EXPORT
#endif

class QMainWindow;
class QAction;
class QToolBar;

class QtToolBarManagerPrivate;

class QT_QTTOOLBARDIALOG_EXPORT QtToolBarManager : public QObject
{
    Q_OBJECT
public:

    QtToolBarManager(QObject *parent = 0);
    ~QtToolBarManager();

    void setMainWindow(QMainWindow *mainWindow);
    QMainWindow *mainWindow() const;

    void addAction(QAction *action, const QString &category);
    void removeAction(QAction *action);

    void addToolBar(QToolBar *toolBar, const QString &category);
    void removeToolBar(QToolBar *toolBar);

    QList<QToolBar *> toolBars() const;

    QByteArray saveState(int version = 0) const;
    bool restoreState(const QByteArray &state, int version = 0);

private:

    friend class QtToolBarDialog;
    QtToolBarManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtToolBarManager)
    Q_DISABLE_COPY(QtToolBarManager)
};

class QtToolBarDialogPrivate;

class QT_QTTOOLBARDIALOG_EXPORT QtToolBarDialog : public QDialog
{
    Q_OBJECT
public:

    QtToolBarDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~QtToolBarDialog();

    void setToolBarManager(QtToolBarManager *toolBarManager);

protected:

    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private:

    QtToolBarDialogPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QtToolBarDialog)
    Q_DISABLE_COPY(QtToolBarDialog)

    Q_PRIVATE_SLOT(d_func(), void newClicked())
    Q_PRIVATE_SLOT(d_func(), void removeClicked())
    Q_PRIVATE_SLOT(d_func(), void defaultClicked())
    Q_PRIVATE_SLOT(d_func(), void okClicked())
    Q_PRIVATE_SLOT(d_func(), void applyClicked())
    Q_PRIVATE_SLOT(d_func(), void cancelClicked())
    Q_PRIVATE_SLOT(d_func(), void upClicked())
    Q_PRIVATE_SLOT(d_func(), void downClicked())
    Q_PRIVATE_SLOT(d_func(), void leftClicked())
    Q_PRIVATE_SLOT(d_func(), void rightClicked())
    Q_PRIVATE_SLOT(d_func(), void renameClicked())
    Q_PRIVATE_SLOT(d_func(), void toolBarRenamed(QListWidgetItem *))
    Q_PRIVATE_SLOT(d_func(), void currentActionChanged(QTreeWidgetItem *))
    Q_PRIVATE_SLOT(d_func(), void currentToolBarChanged(QListWidgetItem *))
    Q_PRIVATE_SLOT(d_func(), void currentToolBarActionChanged(QListWidgetItem *))
};

#endif
