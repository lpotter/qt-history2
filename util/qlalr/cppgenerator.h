/****************************************************************************
**
** Copyright (C) 2007-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the QLALR project on Trolltech Labs.
**
** $TROLLTECH_GPL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef CPPGENERATOR_H
#define CPPGENERATOR_H

#include "lalr.h"
#include "compress.h"

class Grammar;
class Automaton;
class Recognizer;

class CppGenerator
{
public:
  CppGenerator(const Recognizer &p, Grammar &grammar, Automaton &aut, bool verbose):
    p (p),
    grammar (grammar),
    aut (aut),
    verbose (verbose),
    debug_info (false),
    troll_copyright (false) {}

  void operator () ();

  bool debugInfo () const { return debug_info; }
  void setDebugInfo (bool d) { debug_info = d; }

  bool trollCopyright () const { return troll_copyright; }
  void setTrollCopyright (bool t) { troll_copyright = t; }

private:
  void generateDecl (QTextStream &out);
  void generateImpl (QTextStream &out);

  QString debugInfoProt() const;
  QString trollCopyrightHeader() const;
  QString trollPrivateCopyrightHeader() const;

private:
  static QString startIncludeGuard(const QString &fileName);
  static QString endIncludeGuard(const QString &fileName);

  const Recognizer &p;
  Grammar &grammar;
  Automaton &aut;
  bool verbose;
  int accept_state;
  int state_count;
  int terminal_count;
  int non_terminal_count;
  bool debug_info;
  bool troll_copyright;
  Compress compressed_action;
  Compress compressed_goto;
  QVector<int> count;
  QVector<int> defgoto;
};

#endif // CPPGENERATOR_H

// kate: indent-width 2;
