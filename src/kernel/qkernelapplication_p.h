#ifndef QKERNELAPPLICATION_P_H
#define QKERNELAPPLICATION_P_H

#include "qobject_p.h"
#include "qkernelapplication.h"
#include "qtranslator.h"
#include "qmetaobject.h"

class Q_KERNEL_EXPORT QTranslatorList : private QList<QTranslator*>
{
public:
    using QList<QTranslator*>::prepend;
    using QList<QTranslator*>::remove;
    using QList<QTranslator*>::isEmpty;
    using QList<QTranslator*>::constBegin;
    using QList<QTranslator*>::constEnd;
};

class Q_KERNEL_EXPORT QKernelApplicationPrivate : public QObjectPrivate
{
    Q_DECL_PUBLIC(QKernelApplication);
public:
    QKernelApplicationPrivate(int &aargc,  char **aargv);
    ~QKernelApplicationPrivate() {}

    int &argc;
    char **argv;
#ifndef QT_NO_TRANSLATION
    QTranslatorList translators;
#endif
#ifndef QT_NO_COMPONENT
    QStringList *app_libpaths;
#endif
};


#endif
