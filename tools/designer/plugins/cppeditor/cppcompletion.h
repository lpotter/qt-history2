#ifndef CPPCOMPLETION_H
#define CPPCOMPLETION_H

#include <completion.h>

class CppEditorCompletion : public EditorCompletion
{
    Q_OBJECT

public:
    CppEditorCompletion( Editor *e );

    bool doObjectCompletion( const QString &object );
    QStringList functionParameters( const QString &func, QChar & );
    void setContext( QObjectList *toplevels, QObject *this_ );

};

#endif
