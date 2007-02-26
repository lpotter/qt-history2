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

#include "formscriptrunner_p.h"
#include "ui4_p.h"
#include "private/qobject_p.h"

#include <QtScript/QScriptEngine>
#include <QtGui/QWidget>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

namespace {
    enum { debugFormScriptRunner = 0 };
}

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal {
#endif

class  QFormScriptRunner::QFormScriptRunnerPrivate {
public:
    QFormScriptRunnerPrivate() : m_options(DisableScripts) {}
    void clearErrors() { m_errors.clear(); }

    bool run(const QString &script, QWidget *widget, const WidgetList &children, QString *errorMessage);



    static void initializeEngine(QWidget *w, const WidgetList &children, QScriptEngine &scriptEngine);
    static QString engineError(QScriptEngine &scriptEngine);

    Options options() const { return m_options; }
    void setOptions(Options options) { m_options = options; }

    Errors errors() const { return m_errors; }
private:
    QScriptEngine m_scriptEngine;
    Options m_options;
    Errors m_errors;
};

bool QFormScriptRunner::QFormScriptRunnerPrivate::run(const QString &script, QWidget *widget, const WidgetList &children, QString *errorMessage) {
    bool rc = false;
    initializeEngine(widget, children, m_scriptEngine);

    do {
        if (!m_scriptEngine.canEvaluate(script)) {
            *errorMessage = QCoreApplication::tr("Syntax error.");
            break;
        }
        m_scriptEngine.evaluate(script);
        if (m_scriptEngine.hasUncaughtException ()) {
            *errorMessage = QCoreApplication::tr("Exception at line %1: %2").arg(m_scriptEngine.uncaughtExceptionLineNumber()).arg(engineError(m_scriptEngine));
            break;
        }
        rc = true;
    } while (false);
    if (!rc) {
        Error error;
        error.objectName = widget->objectName();
        error.script = script;
        error.errorMessage = *errorMessage;
        m_errors.push_back(error);
    }
    return rc;
}

void QFormScriptRunner::QFormScriptRunnerPrivate::initializeEngine(QWidget *w, const WidgetList &children, QScriptEngine &scriptEngine) {
    // Populate the script variables.
    QScriptValue widgetObject =  scriptEngine.newQObject(w);
    QScriptValue childrenArray = scriptEngine.newArray (children.size());

    for(int i = 0; i < children.size(); i++) {
        childrenArray.setProperty(i, scriptEngine.newQObject(children[i]));
    }

    scriptEngine.globalObject().setProperty(QLatin1String("widget"),   widgetObject);
    scriptEngine.globalObject().setProperty(QLatin1String("childWidgets"), childrenArray);
}

QString QFormScriptRunner::QFormScriptRunnerPrivate::engineError(QScriptEngine &scriptEngine) {
    QScriptValue error = scriptEngine.evaluate(QLatin1String("Error"));
    if (error.isValid())
        return error.toString();
    return QCoreApplication::tr("Unknown error");
}
// -- QFormScriptRunner

QFormScriptRunner::QFormScriptRunner() : m_impl(new QFormScriptRunnerPrivate)
{
}

QFormScriptRunner::~QFormScriptRunner()
{
    delete m_impl;
}

bool QFormScriptRunner::run(const DomWidget *domWidget,
                            const QString &customWidgetScript,
                            QWidget *widget, const WidgetList &children,
                            QString *errorMessage)
{
    typedef QList<DomScript*> DomScripts;

    const Options scriptOptions =  m_impl->options();
    if (scriptOptions & DisableScripts)
        return true;
    // get list
    const DomScripts domScripts = domWidget->elementScript();
    // Concatenate snippets, starting with custom widget script
    QString  script = customWidgetScript;
    if (script.isEmpty() && domScripts.empty())
        return true;

    foreach (const DomScript *scriptSnippet, domScripts) {
        // Ensure new line
        if (!script.isEmpty() && !script.endsWith(QLatin1Char('\n')))
            script += QLatin1Char('\n');
        script += scriptSnippet->text();
    }

    if (script.isEmpty())
        return true;

    const bool rc =  m_impl->run(script, widget, children, errorMessage);

    if (debugFormScriptRunner) {
        qDebug() << "For " << widget << " with " << children.size() << " children, ran: " << script;
        if (!rc)
            qDebug() << *errorMessage;
    }

    if (!rc) {
        if (!(scriptOptions & DisableWarnings)) {
            const QString message = QCoreApplication::tr("An error occured while running the script for %1: %2\nScript: %3").
                arg(widget->objectName()).arg(*errorMessage).arg(script);
            qWarning() <<  message;
        }
    }
    return rc;
}

QFormScriptRunner::Options QFormScriptRunner::options() const
{
    return  m_impl->options();
}

void QFormScriptRunner::setOptions(Options options)
{
     m_impl->setOptions(options);
}


QFormScriptRunner::Errors QFormScriptRunner::errors() const
{
    return m_impl->errors();
}

void QFormScriptRunner::clearErrors()
{
     m_impl->clearErrors();
}


#ifdef QFORMINTERNAL_NAMESPACE
} // namespace QFormInternal
#endif
