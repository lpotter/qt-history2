/****************************************************************************
**
** Copyright (C) 2005-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtDesigner>
#include <QtGui>
#include <QtPlugin>

#include "tictactoe.h"
#include "tictactoeplugin.h"
#include "tictactoetaskmenu.h"

TicTacToePlugin::TicTacToePlugin(QObject *parent)
    : QObject(parent)
{
    initialized = false;
}

QString TicTacToePlugin::name() const
{
    return "TicTacToe";
}

QString TicTacToePlugin::group() const
{
    return "Display Widgets [Examples]";
}

QString TicTacToePlugin::toolTip() const
{
    return "";
}

QString TicTacToePlugin::whatsThis() const
{
    return "";
}

QString TicTacToePlugin::includeFile() const
{
    return "tictactoe.h";
}

QIcon TicTacToePlugin::icon() const
{
    return QIcon();
}

bool TicTacToePlugin::isContainer() const
{
    return false;
}

QWidget *TicTacToePlugin::createWidget(QWidget *parent)
{
    TicTacToe *ticTacToe = new TicTacToe(parent);
    ticTacToe->setState("-X-XO----");
    return ticTacToe;
}

bool TicTacToePlugin::isInitialized() const
{
    return initialized;
}

void TicTacToePlugin::initialize(QDesignerFormEditorInterface *formEditor)
{
    if (initialized)
        return;

    QExtensionManager *manager = formEditor->extensionManager();
    Q_ASSERT(manager != 0);

    manager->registerExtensions(new TicTacToeTaskMenuFactory(manager),
                                Q_TYPEID(QDesignerTaskMenuExtension));

    initialized = true;
}

Q_EXPORT_PLUGIN2(taskmenuextension, TicTacToePlugin)
