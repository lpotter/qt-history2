/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of Qt Designer.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "writeincludes.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"
#include "databaseinfo.h"

#include <qtextstream.h>

WriteIncludes::WriteIncludes(Uic *uic)
    : driver(uic->driver()), output(uic->output()), option(uic->option())
{
    this->uic = uic;
}

void WriteIncludes::accept(DomUI *node)
{
    m_includes.clear();
    m_customWidgets.clear();

    if (node->elementIncludes())
        accept(node->elementIncludes());

    if (node->elementCustomWidgets())
        TreeWalker::accept(node->elementCustomWidgets());

    m_includes.insert("qapplication.h", true);
    m_includes.insert("qvariant.h", true);

    if (uic->databaseInfo()->connections().size()) {
        m_includes.insert("qsqldatabase.h", true);
        m_includes.insert("qsqlcursor.h", true);
        m_includes.insert("qsqlrecord.h", true);
        m_includes.insert("qsqlform.h", true);
    }

    TreeWalker::accept(node);

    QHashIterator<QString, bool> it(m_includes);
    while (it.hasNext()) {
        it.next();

        if (it.value())
            output << "#include <" << it.key() << ">\n";
        else
            output << "#include \"" << it.key() << "\"\n";
    }
    output << "\n";
}

void WriteIncludes::accept(DomWidget *node)
{
    if (node->attributeClass() == QLatin1String("Line"))
        add(QLatin1String("QFrame"));
    else
        add(node->attributeClass());

    TreeWalker::accept(node);
}

void WriteIncludes::accept(DomLayout *node)
{
    add(node->attributeClass());
    TreeWalker::accept(node);
}

void WriteIncludes::accept(DomSpacer *node)
{
    add(QLatin1String("QSpacerItem"));
    TreeWalker::accept(node);
}

void WriteIncludes::add(const QString &className)
{
    if (className.isEmpty())
        return;

    QString header = className.toLower() + ".h";

    if (className == QLatin1String("QVBoxLayout")  // special case for layouts
            || className == QLatin1String("QHBoxLayout")
            || className == QLatin1String("QBoxLayout")
            || className == QLatin1String("QLayout")
            || className == QLatin1String("QGridLayout")
            || className == QLatin1String("QSpacerItem")) {
        m_includes.insert("qlayout.h", true);
    } else if (className == QLatin1String("Line")) {
        m_includes.insert("qframe.h", true);
    } else if (className == QLatin1String("QDateEdit") // special case for datetime
            || className == QLatin1String("QTimeEdit")) {
        m_includes.insert("qdatetimeedit.h", true);
    } else if (className == QLatin1String("Q3DateEdit") // special case for d3atetime
            || className == QLatin1String("Q3TimeEdit")) {
        m_includes.insert("q3datetimeedit.h", true);
    } else if (!m_includes.contains(header)
            && !m_customWidgets.contains(className)) {
        m_includes.insert(header, true);
    }

    if (uic->customWidgetsInfo()->extends(className, "Q3ListView")) {
        m_includes.insert("q3header.h", true);
    }
}

void WriteIncludes::accept(DomCustomWidget *node)
{
    if (node->elementClass().isEmpty())
        return;

    bool global = node->elementHeader()->attributeLocation().toLower() == QLatin1String("global");
    m_includes.insert(node->elementHeader()->text(), global);
    m_customWidgets.insert(node->elementClass(), true);
}

void WriteIncludes::accept(DomCustomWidgets *node)
{
    Q_UNUSED(node);
}

void WriteIncludes::accept(DomIncludes *node)
{
    TreeWalker::accept(node);
}

void WriteIncludes::accept(DomInclude *node)
{
    bool global = true;
    if (node->hasAttributeLocation())
        global = node->attributeLocation() == QLatin1String("global");
    m_includes.insert(node->text(), global);
}
