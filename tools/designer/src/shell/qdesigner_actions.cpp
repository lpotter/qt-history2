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

#include "qdesigner_actions.h"
#include "qdesigner_mainwindow.h"

// sdk
#include <abstractformeditor.h>
#include <abstractformwindowmanager.h>
#include <qtundo.h>

#include <QAction>
#include <QActionGroup>

QDesignerActions::QDesignerActions(QDesignerMainWindow *mainWindow)
    : QObject(mainWindow),
      m_mainWindow(mainWindow)
{
    Q_ASSERT(m_mainWindow != 0);
    Q_ASSERT(m_mainWindow->core() != 0);

    AbstractFormWindowManager *formWindowManager = m_mainWindow->core()->formWindowManager();
    Q_ASSERT(formWindowManager != 0);

    m_fileActions = new QActionGroup(this);
    m_fileActions->setExclusive(false);

    m_editActions = new QActionGroup(this);
    m_editActions->setExclusive(false);

    m_formActions = new QActionGroup(this);
    m_formActions->setExclusive(false);

    m_newFormAction = new QAction(this);
    m_fileActions->addAction(m_newFormAction);

    m_openFormAction = new QAction(this);
    m_fileActions->addAction(m_openFormAction);

    m_saveFormAction = new QAction(this);
    m_fileActions->addAction(m_saveFormAction);

    m_saveFormAsAction = new QAction(this);
    m_fileActions->addAction(m_saveFormAsAction);

    m_closeFormAction = new QAction(this);
    m_fileActions->addAction(m_closeFormAction);

    m_quitAction = new QAction(this);
    m_fileActions->addAction(m_quitAction);

    m_undoAction = QtUndoManager::manager()->createUndoAction(this);
    m_editActions->addAction(m_undoAction);

    m_redoAction = QtUndoManager::manager()->createRedoAction(this);
    m_editActions->addAction(m_redoAction);

    m_cutAction = formWindowManager->actionCut();
    m_editActions->addAction(m_cutAction);

    m_copyAction = formWindowManager->actionCopy();
    m_editActions->addAction(m_copyAction);

    m_pasteAction = formWindowManager->actionPaste();
    m_editActions->addAction(m_pasteAction);

    m_deleteAction = formWindowManager->actionDelete();
    m_editActions->addAction(m_deleteAction);

    m_sendToBackAction = formWindowManager->actionLower();
    m_editActions->addAction(m_sendToBackAction);

    m_bringToFrontAction = formWindowManager->actionRaise();
    m_editActions->addAction(m_bringToFrontAction);

    m_selectAllAction = formWindowManager->actionSelectAll();
    m_editActions->addAction(m_selectAllAction);

    m_layoutHorizontallyAction = formWindowManager->actionHorizontalLayout();
    m_formActions->addAction(m_layoutHorizontallyAction);

    m_layoutVerticallyAction = formWindowManager->actionVerticalLayout();
    m_formActions->addAction(m_layoutVerticallyAction);

    m_layoutHorizontallyInSplitterAction = formWindowManager->actionSplitHorizontal();
    m_formActions->addAction(m_layoutHorizontallyInSplitterAction);

    m_layoutVerticallyInSplitterAction = formWindowManager->actionSplitVertical();
    m_formActions->addAction(m_layoutVerticallyInSplitterAction);

    m_layoutGridAction = formWindowManager->actionGridLayout();
    m_formActions->addAction(m_layoutGridAction);

    m_breakLayoutAction = formWindowManager->actionBreakLayout();
    m_formActions->addAction(m_breakLayoutAction);

    m_adjustSizeAction = formWindowManager->actionAdjustSize();
    m_formActions->addAction(m_adjustSizeAction);

    m_previewFormAction = new QAction(this);
    m_formActions->addAction(m_previewFormAction);
}

QDesignerActions::~QDesignerActions()
{
}

QDesignerMainWindow *QDesignerActions::mainWindow() const
{ return m_mainWindow; }

QActionGroup *QDesignerActions::fileActions() const
{ return m_fileActions; }

QActionGroup *QDesignerActions::editActions() const
{ return m_editActions; }

QActionGroup *QDesignerActions::formActions() const
{ return m_formActions; }

QAction *QDesignerActions::newFormAction() const
{ return m_newFormAction; }

QAction *QDesignerActions::openFormAction() const
{ return m_openFormAction; }

QAction *QDesignerActions::saveFormAction() const
{ return m_saveFormAction; }

QAction *QDesignerActions::saveFormAsAction() const
{ return m_saveFormAsAction; }

QAction *QDesignerActions::closeFormAction() const
{ return m_closeFormAction; }

QAction *QDesignerActions::quitAction() const
{ return m_quitAction; }

QAction *QDesignerActions::undoAction() const
{ return m_undoAction; }

QAction *QDesignerActions::redoAction() const
{ return m_redoAction; }

QAction *QDesignerActions::cutAction() const
{ return m_cutAction; }

QAction *QDesignerActions::copyAction() const
{ return m_copyAction; }

QAction *QDesignerActions::pasteAction() const
{ return m_pasteAction; }

QAction *QDesignerActions::selectAllAction() const
{ return m_selectAllAction; }

QAction *QDesignerActions::deleteAction() const
{ return m_deleteAction; }

QAction *QDesignerActions::sendToBackAction() const
{ return m_sendToBackAction; }

QAction *QDesignerActions::bringToFrontAction() const
{ return m_bringToFrontAction; }

QAction *QDesignerActions::layoutHorizontallyAction() const
{ return m_layoutHorizontallyAction; }

QAction *QDesignerActions::layoutVerticallyAction() const
{ return m_layoutVerticallyAction; }

QAction *QDesignerActions::layoutHorizontallyInSplitterAction() const
{ return m_layoutHorizontallyInSplitterAction; }

QAction *QDesignerActions::layoutVerticallyInSplitterAction() const
{ return m_layoutVerticallyInSplitterAction; }

QAction *QDesignerActions::layoutGridAction() const
{ return m_layoutGridAction; }

QAction *QDesignerActions::breakLayoutAction() const
{ return m_breakLayoutAction; }

QAction *QDesignerActions::adjustSizeAction() const
{ return m_adjustSizeAction; }

QAction *QDesignerActions::previewFormAction() const
{ return m_previewFormAction; }

