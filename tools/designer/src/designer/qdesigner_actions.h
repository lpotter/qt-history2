/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QDESIGNER_ACTIONS_H
#define QDESIGNER_ACTIONS_H

#include <QtCore/QPointer>
#include <QtCore/QObject>

class QDesignerMainWindow;
class QDesignerWorkbench;

class QAction;
class QActionGroup;
class AbstractFormEditor;
class AbstractFormWindow;
class PreferenceDialog;

class QDesignerActions: public QObject
{
    Q_OBJECT
public:
    QDesignerActions(QDesignerMainWindow *mainWindow);
    virtual ~QDesignerActions();

    QDesignerMainWindow *mainWindow() const;
    QDesignerWorkbench *workbench() const;
    AbstractFormEditor *core() const;


    QActionGroup *fileActions() const;
    QActionGroup *editActions() const;
    QActionGroup *editModeActions() const;
    QActionGroup *formActions() const;
    QActionGroup *windowActions() const;

//
// file actions
//
    QAction *newFormAction() const;
    QAction *openFormAction() const;
    QAction *saveFormAction() const;
    QAction *saveFormAsAction() const;
    QAction *saveFormAsTemplateAction() const;
    QAction *closeFormAction() const;
    QAction *quitAction() const;

//
// edit actions
//
    QAction *undoAction() const;
    QAction *redoAction() const;
    QAction *cutAction() const;
    QAction *copyAction() const;
    QAction *pasteAction() const;
    QAction *deleteAction() const;
    QAction *selectAllAction() const;
    QAction *sendToBackAction() const;
    QAction *bringToFrontAction() const;

    QAction *preferencesSeparator() const;
    QAction *preferences() const;

//
// edit mode actions
//
    QAction *editWidgets() const;
    QAction *editConnections() const;
    QAction *editTabOrders() const;
    QAction *editBuddies() const;

//
// form actions
//
    QAction *layoutHorizontallyAction() const;
    QAction *layoutVerticallyAction() const;
    QAction *layoutHorizontallyInSplitterAction() const;
    QAction *layoutVerticallyInSplitterAction() const;
    QAction *layoutGridAction() const;
    QAction *breakLayoutAction() const;
    QAction *adjustSizeAction() const;
    QAction *previewFormAction() const;

//
// window actions
//
    QAction *showWorkbenchAction() const;

private slots:
    void updateEditMode(QAction *action);
    void setWorkbenchVisible(bool visible);
    void createForm();
    void openForm();
    void saveForm();
    void saveFormAs();
    void saveFormAsTemplate();
    void notImplementedYet();
    void editPreferences();

private:
    bool readInForm(const QString &fileName);
    bool writeOutForm(AbstractFormWindow *formWindow, const QString &fileName);
    bool saveForm(AbstractFormWindow *fw);
    bool saveFormAs(AbstractFormWindow *fw);

private:
    QDesignerMainWindow *m_mainWindow;
    QDesignerWorkbench *m_workbench;
    AbstractFormEditor *m_core;

    QActionGroup *m_fileActions;
    QActionGroup *m_editActions;
    QActionGroup *m_editModeActions;
    QActionGroup *m_formActions;
    QActionGroup *m_windowActions;

    QAction *m_newFormAction;
    QAction *m_openFormAction;
    QAction *m_saveFormAction;
    QAction *m_saveFormAsAction;
    QAction *m_saveFormAsTemplateAction;
    QAction *m_closeFormAction;

    QAction *m_quitAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_deleteAction;
    QAction *m_sendToBackAction;
    QAction *m_bringToFrontAction;
    QAction *m_selectAllAction;

    QAction *m_preferencesSeparator;
    QAction *m_preferences;

    QAction *m_editWidgets;
    QAction *m_editTabOrders;
    QAction *m_editConnections;
    QAction *m_editBuddies;


    QAction *m_layoutHorizontallyAction;
    QAction *m_layoutVerticallyAction;
    QAction *m_layoutHorizontallyInSplitterAction;
    QAction *m_layoutVerticallyInSplitterAction;
    QAction *m_layoutGridAction;
    QAction *m_breakLayoutAction;
    QAction *m_adjustSizeAction;
    QAction *m_previewFormAction;

    QAction *m_showWorkbenchAction;

    QPointer<PreferenceDialog> m_preferenceDialog;
};

#endif // QDESIGNER_ACTIONS_H

