#ifndef CINDENT_H
#define CINDENT_H

#include <private/qrichtext_p.h>
#include "dlldefs.h"

class EDITOR_EXPORT CIndent : public QTextIndent
{
public:
    CIndent();
    virtual ~CIndent() {}
    void indent( QTextDocument *doc, QTextParagraph *parag, int *oldIndent, int *newIndent );

    void setTabSize( int ts );
    void setIndentSize( int is );
    void setAutoIndent( bool ai ) { autoIndent = ai; reindent(); }
    void setKeepTabs( bool kt ) { keepTabs = kt; }

private:
    void reindent();
    void indentLine( QTextParagraph *p, int &oldIndent, int &newIndent );
    void tabify( QString &s );

public:
    int tabSize, indentSize;
    bool autoIndent, keepTabs;
    QTextDocument *lastDoc;

};

#endif
