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

#include "cppwriteicondeclaration.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"

#include <QtCore/QTextStream>

QT_BEGIN_NAMESPACE

namespace CPP {

WriteIconDeclaration::WriteIconDeclaration(Uic *uic)
    : driver(uic->driver()), output(uic->output()), option(uic->option())
{
}

void WriteIconDeclaration::acceptUI(DomUI *node)
{
    TreeWalker::acceptUI(node);
}

void WriteIconDeclaration::acceptImages(DomImages *images)
{
    TreeWalker::acceptImages(images);
}

void WriteIconDeclaration::acceptImage(DomImage *image)
{
    QString name = image->attributeName();
    if (name.isEmpty())
        return;

    driver->insertPixmap(name);
    output << option.indent << option.indent << name << "_ID,\n";
}

} // namespace CPP

QT_END_NAMESPACE
