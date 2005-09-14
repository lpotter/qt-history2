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

#ifndef QABSTRACTTESTLOGGER_H
#define QABSTRACTTESTLOGGER_H

class QAbstractTestLogger
{
public:
    enum IncidentTypes {
        Pass,
        XFail,
        Fail,
        XPass
    };


    enum MessageTypes {
        Warn,
        QWarning,
        QDebug,
        QSystem,
        QFatal,
        Skip,
        Info
    };


    QAbstractTestLogger() {}
    virtual ~QAbstractTestLogger() {}

    virtual void startLogging();
    virtual void stopLogging();

    virtual void enterTestFunction(const char *function) = 0;
    virtual void leaveTestFunction() = 0;

    virtual void addIncident(IncidentTypes type, const char *description,
                             const char *file = 0, int line = 0) = 0;

    virtual void addMessage(MessageTypes type, const char *message,
                            const char *file = 0, int line = 0) = 0;

    static void outputString(const char *msg);
};

#endif

