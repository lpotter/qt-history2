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

#ifndef QTEXTDOCUMENT_P_H
#define QTEXTDOCUMENT_P_H

#include "qglobal.h"
#include <qstring.h>
#include <qvector.h>
#include <qlist.h>
#include <private/qobject_p.h>
#include "qfragmentmap_p.h"
#include "qtextlayout.h"
#include "qtextformat_p.h"
#include "qtextdocument.h"
#include "qtextobject.h"
#include <qmap.h>

// #define QT_QMAP_DEBUG

#ifdef QT_QMAP_DEBUG
#include <iostream>
#endif

class QTextFormatCollection;
class QTextFormat;
class QTextBlockFormat;
class QTextCursorPrivate;
class QAbstractTextDocumentLayout;
class QTextDocument;
class QTextFrame;

#define QTextBeginningOfFrame QChar(0xfdd0)
#define QTextEndOfFrame QChar(0xfdd1)

struct QTextDocumentConfig
{
    QString title;
};

class QTextFragmentData : public QFragment
{
public:
    inline void initialize() {}
    inline void invalidate() const {}
    inline void free() {}
    int stringPosition;
    int format;
};

class QTextBlockData : public QFragment
{
public:
    inline void initialize()
    { layout = 0; layoutDirty = true; textDirty = true; }
    inline void invalidate() const
    { layoutDirty = true; textDirty = true; }
    inline void free()
    { if (layoutDirty) delete layout; }

    // ##### probably store a QTextEngine * here!
    mutable QTextLayout *layout;
    mutable bool layoutDirty;
    mutable bool textDirty;
    mutable int format;
};


class QAbstractUndoItem;

class QTextUndoCommand
{
public:
    enum Command {
        Inserted = 0,
        Removed = 1,
        CharFormatChanged = 2,
        BlockFormatChanged = 3,
        BlockInserted = 4,
        BlockRemoved = 5,
        BlockAdded = 6,
        BlockDeleted = 7,
        GroupFormatChange = 8,
        Custom = 256
    };
    enum Operation {
        KeepCursor = 0,
        MoveCursor = 1
    };
    Q_UINT16 command;
    Q_UINT16 block : 1;
    Q_UINT16 operation : 2;
    int format;
    Q_UINT32 strPos;
    Q_UINT32 pos;
    union {
        int blockFormat;
        Q_UINT32 length;
        QAbstractUndoItem *custom;
        QTextObject *object;
    };

    bool tryMerge(const QTextUndoCommand &other);
};
Q_DECLARE_TYPEINFO(QTextUndoCommand, Q_PRIMITIVE_TYPE);

class QTextDocumentPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QTextDocument)
public:
    typedef QFragmentMap<QTextFragmentData> FragmentMap;
    typedef FragmentMap::ConstIterator FragmentIterator;
    typedef QFragmentMap<QTextBlockData> BlockMap;

    QTextDocumentPrivate();
    ~QTextDocumentPrivate();

    void init();

    void setLayout(QAbstractTextDocumentLayout *layout);

    void insert(int pos, const QString &text, int format);
    void insert(int pos, int strPos, int strLength, int format);
    void insertBlock(int pos, int blockFormat, int charFormat, QTextUndoCommand::Operation = QTextUndoCommand::MoveCursor);
    void insertBlock(const QChar &blockSeparator, int pos, int blockFormat, int charFormat,
                     QTextUndoCommand::Operation op = QTextUndoCommand::MoveCursor);

    void remove(int pos, int length, QTextUndoCommand::Operation = QTextUndoCommand::MoveCursor);

    QTextFrame *insertFrame(int start, int end, const QTextFrameFormat &format);
    void removeFrame(QTextFrame *frame);

    enum FormatChangeMode { MergeFormat, SetFormat };

    void setCharFormat(int pos, int length, const QTextCharFormat &newFormat, FormatChangeMode mode = SetFormat);
    void setBlockFormat(const QTextBlock &from, const QTextBlock &to,
			const QTextBlockFormat &newFormat, FormatChangeMode mode = SetFormat);

    void undoRedo(bool undo);
    inline void undo() { undoRedo(true); }
    inline void redo() { undoRedo(false); }
    void appendUndoItem(QAbstractUndoItem *);
    inline void beginEditBlock() { editBlock++; }
    void joinPreviousEditBlock();
    void endEditBlock();
    void enableUndoRedo(bool enable);
    inline bool isUndoRedoEnabled() const { return undoEnabled; }

    inline bool isUndoAvailable() const { return undoEnabled && undoPosition > 0; }
    inline bool isRedoAvailable() const { return undoEnabled && undoPosition < undoStack.size(); }

    inline QString buffer() const { return text; }
    QString plainText() const;
    inline int length() const { return fragments.length(); }

    inline QTextFormatCollection *formatCollection() { return &formats; }
    inline const QTextFormatCollection *formatCollection() const { return &formats; }
    inline QAbstractTextDocumentLayout *layout() const { return lout; }

    inline FragmentIterator find(int pos) const { return fragments.find(pos); }
    inline FragmentIterator begin() const { return fragments.begin(); }
    inline FragmentIterator end() const { return fragments.end(); }

    inline QTextBlock blocksBegin() const { return QTextBlock(const_cast<QTextDocumentPrivate *>(this), blocks.firstNode()); }
    inline QTextBlock blocksEnd() const { return QTextBlock(const_cast<QTextDocumentPrivate *>(this), 0); }
    inline QTextBlock blocksFind(int pos) const { return QTextBlock(const_cast<QTextDocumentPrivate *>(this), blocks.findNode(pos)); }
    inline int numBlocks() const { return blocks.numNodes(); }

    const BlockMap &blockMap() const { return blocks; }
    const FragmentMap &fragmentMap() const { return fragments; }

    static const QTextBlockData *block(const QTextBlock &it) { return it.p->blocks.fragment(it.n); }

    inline QTextDocumentConfig *config() { return &docConfig; }
    inline const QTextDocumentConfig *config() const { return &docConfig; }

    int nextCursorPosition(int position, QTextLayout::CursorMode mode) const;
    int previousCursorPosition(int position, QTextLayout::CursorMode mode) const;

    void changeObjectFormat(QTextObject *group, int format);

    void setModified(bool m);
    inline bool isModified() const { return modified; }

private:
    bool split(int pos);
    bool unite(uint f);
    void truncateUndoStack();

    void insert_string(int pos, uint strPos, uint length, int format, QTextUndoCommand::Operation op);
    void insert_block(int pos, uint strPos, int format, int blockformat, QTextUndoCommand::Operation op, int command);
    int remove_string(int pos, uint length, QTextUndoCommand::Operation op);
    int remove_block(int pos, int *blockformat, int command, QTextUndoCommand::Operation op);

    void insert_frame(QTextFrame *f);
    void scan_frames(int pos, int charsRemoved, int charsAddded);
    static void clearFrame(QTextFrame *f);

    void adjustDocumentChangesAndCursors(int from, int addedOrRemoved, QTextUndoCommand::Operation op);
    void documentChange(int from, int length);

public:
    inline void addCursor(QTextCursorPrivate *c) { cursors.append(c); }
    inline void removeCursor(QTextCursorPrivate *c) { cursors.removeAll(c); }

    QTextFrame *frameAt(int pos) const;
    QTextFrame *rootFrame() const { return frame; }

    QTextObject *objectForIndex(int objectIndex) const;
    QTextObject *objectForFormat(int formatIndex) const;
    QTextObject *objectForFormat(const QTextFormat &f) const;

    QTextObject *createObject(const QTextFormat &newFormat, int objectIndex = -1);

    QTextDocument *document() { return q_func(); }
    const QTextDocument *document() const { return q_func(); }

private:
    QTextDocumentPrivate(const QTextDocumentPrivate& m);
    QTextDocumentPrivate& operator= (const QTextDocumentPrivate& m);

    void appendUndoItem(const QTextUndoCommand &c);

    void contentsChanged();

    QString text;

    QVector<QTextUndoCommand> undoStack;
    bool undoEnabled;
    int undoPosition;

    int editBlock;
    int docChangeFrom;
    int docChangeOldLength;
    int docChangeLength;
    bool framesDirty;

    QTextFormatCollection formats;
    QTextFrame *frame;
    QAbstractTextDocumentLayout *lout;
    FragmentMap fragments;
    BlockMap blocks;

    QList<QTextCursorPrivate*> cursors;
    QMap<int, QTextObject *> objects;

    QTextDocumentConfig docConfig;

    bool modified;
    // position in undo stack of the last setModified(false) call
    int lastUnmodifiedUndoStackPos;
};

#endif // QTEXTDOCUMENT_P_H
