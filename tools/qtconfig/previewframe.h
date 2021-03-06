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

#ifndef PREVIEWFRAME_H
#define PREVIEWFRAME_H

#include "previewwidget.h"

#include <QMdiArea>

QT_BEGIN_NAMESPACE

class Workspace : public QMdiArea
{
    Q_OBJECT

public:
    Workspace( QWidget* parent = 0, const char* name = 0 );
    ~Workspace() {}

protected:
    void paintEvent( QPaintEvent* );
};

class PreviewFrame : public QFrame
{
    Q_OBJECT

public:
    PreviewFrame( QWidget *parent = 0, const char *name = 0 );
    void setPreviewPalette(QPalette);

private:
    PreviewWidget 	*previewWidget;
};

QT_END_NAMESPACE

#endif
