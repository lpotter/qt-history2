#include <QtTest/QtTest>
#include <QAction>
#include <QUndoStack>

/******************************************************************************
** Commands
*/

class InsertCommand : public QUndoCommand
{
public:
    InsertCommand(QString *str, int idx, const QString &text,
                    QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();

private:
    QString *m_str;
    int m_idx;
    QString m_text;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(QString *str, int idx, int len, QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();

private:
    QString *m_str;
    int m_idx;
    QString m_text;
};

class AppendCommand : public QUndoCommand
{
public:
    AppendCommand(QString *str, const QString &text, bool _fail_merge = false,
                    QUndoCommand *parent = 0);
    ~AppendCommand();

    virtual void undo();
    virtual void redo();
    virtual int id() const;
    virtual bool mergeWith(const QUndoCommand *other);

    bool merged;
    bool fail_merge;
    static int delete_cnt;

private:
    QString *m_str;
    QString m_text;
};

InsertCommand::InsertCommand(QString *str, int idx, const QString &text,
                            QUndoCommand *parent)
    : QUndoCommand(parent)
{
    QVERIFY(str->length() >= idx);

    setText("insert");

    m_str = str;
    m_idx = idx;
    m_text = text;
}

void InsertCommand::redo()
{
    QVERIFY(m_str->length() >= m_idx);

    m_str->insert(m_idx, m_text);
}

void InsertCommand::undo()
{
    QCOMPARE(m_str->mid(m_idx, m_text.length()), m_text);

    m_str->remove(m_idx, m_text.length());
}

RemoveCommand::RemoveCommand(QString *str, int idx, int len, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    QVERIFY(str->length() >= idx + len);

    setText("remove");

    m_str = str;
    m_idx = idx;
    m_text = m_str->mid(m_idx, len);
}

void RemoveCommand::redo()
{
    QCOMPARE(m_str->mid(m_idx, m_text.length()), m_text);

    m_str->remove(m_idx, m_text.length());
}

void RemoveCommand::undo()
{
    QVERIFY(m_str->length() >= m_idx);

    m_str->insert(m_idx, m_text);
}

int AppendCommand::delete_cnt = 0;

AppendCommand::AppendCommand(QString *str, const QString &text, bool _fail_merge,
                                QUndoCommand *parent)
    : QUndoCommand(parent)
{
    setText("append");

    m_str = str;
    m_text = text;
    merged = false;
    fail_merge = _fail_merge;
}

AppendCommand::~AppendCommand()
{
    ++delete_cnt;
}

void AppendCommand::redo()
{
    m_str->append(m_text);
}

void AppendCommand::undo()
{
    QCOMPARE(m_str->mid(m_str->length() - m_text.length()), m_text);

    m_str->truncate(m_str->length() - m_text.length());
}

int AppendCommand::id() const
{
    return 1;
}

bool AppendCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id())
        return false;
    if (fail_merge)
        return false;
    m_text += static_cast<const AppendCommand*>(other)->m_text;
    merged = true;
    return true;
}

/******************************************************************************
** tst_QUndoStack
*/

class tst_QUndoStack : public QObject
{
    Q_OBJECT
public:
    tst_QUndoStack();

private slots:
    void undoRedo();
    void setIndex();
    void setClean();
    void clear();
    void childCommand();
    void macroBeginEnd();
    void compression();
    void undoLimit();
};

tst_QUndoStack::tst_QUndoStack()
{
}

static QString glue(const QString &s1, const QString &s2)
{
    QString result;

    result.append(s1);
    if (!s1.isEmpty() && !s2.isEmpty())
        result.append(' ');
    result.append(s2);

    return result;
}

#define CHECK_STATE(_clean, _count, _index, _canUndo, _undoText, _canRedo, _redoText, \
                    _cleanChanged, _indexChanged, _undoChanged, _redoChanged) \
    QCOMPARE(stack.count(), _count); \
    QCOMPARE(stack.isClean(), _clean); \
    QCOMPARE(stack.index(), _index); \
    QCOMPARE(stack.canUndo(), _canUndo); \
    QCOMPARE(stack.undoText(), QString(_undoText)); \
    QCOMPARE(stack.canRedo(), _canRedo); \
    QCOMPARE(stack.redoText(), QString(_redoText)); \
    if (_indexChanged) { \
        QCOMPARE(indexChangedSpy.count(), 1); \
        QCOMPARE(indexChangedSpy.at(0).at(0).toInt(), _index); \
        indexChangedSpy.clear(); \
    } else { \
        QCOMPARE(indexChangedSpy.count(), 0); \
    } \
    if (_cleanChanged) { \
        QCOMPARE(cleanChangedSpy.count(), 1); \
        QCOMPARE(cleanChangedSpy.at(0).at(0).toBool(), _clean); \
        cleanChangedSpy.clear(); \
    } else { \
        QCOMPARE(cleanChangedSpy.count(), 0); \
    } \
    if (_undoChanged) { \
        QCOMPARE(canUndoChangedSpy.count(), 1); \
        QCOMPARE(canUndoChangedSpy.at(0).at(0).toBool(), _canUndo); \
        QCOMPARE(undo_action->isEnabled(), _canUndo); \
        QCOMPARE(undoTextChangedSpy.count(), 1); \
        QCOMPARE(undoTextChangedSpy.at(0).at(0).toString(), QString(_undoText)); \
        QCOMPARE(undo_action->text(), glue("foo", _undoText)); \
        canUndoChangedSpy.clear(); \
        undoTextChangedSpy.clear(); \
    } else { \
        QCOMPARE(canUndoChangedSpy.count(), 0); \
        QCOMPARE(undoTextChangedSpy.count(), 0); \
    } \
    if (_redoChanged) { \
        QCOMPARE(canRedoChangedSpy.count(), 1); \
        QCOMPARE(canRedoChangedSpy.at(0).at(0).toBool(), _canRedo); \
        QCOMPARE(redo_action->isEnabled(), _canRedo); \
        QCOMPARE(redoTextChangedSpy.count(), 1); \
        QCOMPARE(redoTextChangedSpy.at(0).at(0).toString(), QString(_redoText)); \
        QCOMPARE(redo_action->text(), glue("bar", _redoText)); \
        canRedoChangedSpy.clear(); \
        redoTextChangedSpy.clear(); \
    } else { \
        QCOMPARE(canRedoChangedSpy.count(), 0); \
        QCOMPARE(redoTextChangedSpy.count(), 0); \
    }

void tst_QUndoStack::undoRedo()
{
    QUndoStack stack;
    QAction *undo_action = stack.createUndoAction(0, QString("foo"));
    QAction *redo_action = stack.createRedoAction(0, QString("bar"));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    // push, undo, redo

    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.undo(); // nothing to undo
    QCOMPARE(str, QString());
    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new InsertCommand(&str, 2, "123"));
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged


    stack.undo();
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.redo();
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.redo(); // nothing to redo
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.undo();
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString());
    CHECK_STATE(true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo(); // nothing to undo
    QCOMPARE(str, QString());
    CHECK_STATE(true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    // push after undo - check that undone commands get deleted

    stack.redo();
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new RemoveCommand(&str, 2, 2));
    QCOMPARE(str, QString("heo"));
    CHECK_STATE(false,      // clean
                2,          // count - still 2, last command got deleted
                2,          // index
                true,       // canUndo
                "remove",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "remove",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString());
    CHECK_STATE(true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new InsertCommand(&str, 0, "goodbye"));
    QCOMPARE(str, QString("goodbye"));
    CHECK_STATE(false,      // clean
                1,          // count - two commands got deleted
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
}

void tst_QUndoStack::setIndex()
{
    QUndoStack stack;
    QAction *undo_action = stack.createUndoAction(0, QString("foo"));
    QAction *redo_action = stack.createRedoAction(0, QString("bar"));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    stack.setIndex(10); // should do nothing
    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.setIndex(0); // should do nothing
    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.setIndex(-10); // should do nothing
    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new InsertCommand(&str, 2, "123"));
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(2);
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.setIndex(0);
    QCOMPARE(str, QString());
    CHECK_STATE(true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(10); // should set index to 2
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(-10); // should set index to 0
    QCOMPARE(str, QString());
    CHECK_STATE(true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(1);
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(2);
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
}

void tst_QUndoStack::setClean()
{
    QUndoStack stack;
    QAction *undo_action = stack.createUndoAction(0, QString("foo"));
    QAction *redo_action = stack.createRedoAction(0, QString("bar"));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    QCOMPARE(stack.cleanIndex(), 0);
    stack.setClean();
    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged
    QCOMPARE(stack.cleanIndex(), 0);

    stack.push(new InsertCommand(&str, 0, "goodbye"));
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
    QCOMPARE(stack.cleanIndex(), 0);

    stack.setClean();
    QCOMPARE(str, QString("goodbye"));
    CHECK_STATE(true,       // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.push(new AppendCommand(&str, " cowboy"));
    QCOMPARE(str, QString("goodbye cowboy"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.undo(); // reaching clean state from above
    QCOMPARE(str, QString("goodbye"));
    CHECK_STATE(true,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.undo();
    QCOMPARE(str, QString());
    CHECK_STATE(false,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.redo(); // reaching clean state from below
    QCOMPARE(str, QString("goodbye"));
    CHECK_STATE(true,       // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.undo();
    QCOMPARE(str, QString());
    CHECK_STATE(false,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
    QCOMPARE(stack.cleanIndex(), 1);

    stack.push(new InsertCommand(&str, 0, "foo")); // the clean state gets deleted!
    QCOMPARE(str, QString("foo"));
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
    QCOMPARE(stack.cleanIndex(), -1);

    stack.undo();
    QCOMPARE(str, QString());
    CHECK_STATE(false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
    QCOMPARE(stack.cleanIndex(), -1);
}

void tst_QUndoStack::clear()
{
    QUndoStack stack;
    QAction *undo_action = stack.createUndoAction(this, QString("foo"));
    QAction *redo_action = stack.createRedoAction(this, QString("bar"));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    stack.clear();
    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new InsertCommand(&str, 2, "123"));
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.clear();
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    str.clear();
    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new InsertCommand(&str, 2, "123"));
    QCOMPARE(str, QString("he123llo"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(0);
    QCOMPARE(str, QString());
    CHECK_STATE(true,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "insert",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.clear();
    QCOMPARE(str, QString());
    CHECK_STATE(true,       // clean
                0,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
}

void tst_QUndoStack::childCommand()
{
    QUndoStack stack;
    QAction *undo_action = stack.createUndoAction(0, QString("foo"));
    QAction *redo_action = stack.createRedoAction(0, QString("bar"));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    stack.push(new InsertCommand(&str, 0, "hello"));
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    QUndoCommand *cmd = new QUndoCommand();
    cmd->setText("ding");
    new InsertCommand(&str, 5, "world", cmd);
    new RemoveCommand(&str, 4, 1, cmd);
    stack.push(cmd);
    QCOMPARE(str, QString("hellworld"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "ding",     // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.redo();
    QCOMPARE(str, QString("hellworld"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    delete undo_action;
    delete redo_action;
}

void tst_QUndoStack::macroBeginEnd()
{
    QUndoStack stack;
    QAction *undo_action = stack.createUndoAction(0, QString("foo"));
    QAction *redo_action = stack.createRedoAction(0, QString("bar"));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    stack.beginMacro("ding");
    CHECK_STATE(false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setClean(); // should do nothing
    CHECK_STATE(false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.undo(); // should do nothing
    CHECK_STATE(false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.redo(); // should do nothing
    CHECK_STATE(false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.setIndex(0); // should do nothing
    CHECK_STATE(false,      // clean
                1,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.endMacro();
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index - endMacro() increments index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new InsertCommand(&str, 0, "h"));
    QCOMPARE(str, QString("h"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new InsertCommand(&str, 1, "owdy"));
    QCOMPARE(str, QString("howdy"));
    CHECK_STATE(false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(2);
    QCOMPARE(str, QString("h"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.beginMacro("dong"); // the "owdy" command gets deleted
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new InsertCommand(&str, 1, "ello"));
    QCOMPARE(str, QString("hello"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new RemoveCommand(&str, 1, 2));
    QCOMPARE(str, QString("hlo"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.beginMacro("dong2");
    QCOMPARE(str, QString("hlo"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new RemoveCommand(&str, 1, 1));
    QCOMPARE(str, QString("ho"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.endMacro();
    QCOMPARE(str, QString("ho"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.endMacro();
    QCOMPARE(str, QString("ho"));
    CHECK_STATE(false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "dong",     // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("h"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "insert",     // undoText
                true,       // canRedo
                "dong",     // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString(""));
    CHECK_STATE(false,      // clean
                3,          // count
                1,          // index
                true,       // canUndo
                "ding",     // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(3);
    QCOMPARE(str, QString("ho"));
    CHECK_STATE(false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "dong",     // undoText
                false,      // canRedo
                "",         // redoText
                false,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setIndex(1);
    QCOMPARE(str, QString());
    CHECK_STATE(false,      // clean
                3,          // count
                1,          // index
                true,       // canUndo
                "ding",     // undoText
                true,       // canRedo
                "insert",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    delete undo_action;
    delete redo_action;
}

void tst_QUndoStack::compression()
{
    QUndoStack stack;
    QAction *undo_action = stack.createUndoAction(0, QString("foo"));
    QAction *redo_action = stack.createRedoAction(0, QString("bar"));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    QString str;

    AppendCommand::delete_cnt = 0;

    stack.push(new InsertCommand(&str, 0, "ene"));
    QCOMPARE(str, QString("ene"));
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, " due")); // #1
    QCOMPARE(str, QString("ene due"));
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, " rike")); // #2 should merge
    QCOMPARE(str, QString("ene due rike"));
    QCOMPARE(AppendCommand::delete_cnt, 1); // #2 should be deleted
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setClean();
    CHECK_STATE(true,       // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new AppendCommand(&str, " fake")); // #3 should NOT merge, since the stack was clean
    QCOMPARE(str, QString("ene due rike fake"));  // and we want to be able to return to this state
    QCOMPARE(AppendCommand::delete_cnt, 1); // #3 should not be deleted
    CHECK_STATE(false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("ene due rike"));
    CHECK_STATE(true,       // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("ene"));
    CHECK_STATE(false,      // clean
                3,          // count
                1,          // index
                true,       // canUndo
                "insert",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "ma", true)); // #4 clean state gets deleted!
    QCOMPARE(str, QString("enema"));
    QCOMPARE(AppendCommand::delete_cnt, 3); // #1 got deleted
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "trix")); // #5 should NOT merge
    QCOMPARE(str, QString("enematrix"));
    QCOMPARE(AppendCommand::delete_cnt, 3);
    CHECK_STATE(false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("enema"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    // and now for command compression inside macros

    stack.setClean();
    QCOMPARE(str, QString("enema"));
    CHECK_STATE(true,       // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.beginMacro("ding");
    QCOMPARE(str, QString("enema"));
    QCOMPARE(AppendCommand::delete_cnt, 4); // #5 gets deleted
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    AppendCommand *merge_cmd = new AppendCommand(&str, "top");
    stack.push(merge_cmd); // #6
    QCOMPARE(merge_cmd->merged, false);
    QCOMPARE(str, QString("enematop"));
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new AppendCommand(&str, "eja")); // #7 should merge
    QCOMPARE(str, QString("enematopeja"));
    QCOMPARE(merge_cmd->merged, true);
    QCOMPARE(AppendCommand::delete_cnt, 5); // #7 gets deleted
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged
    merge_cmd->merged = false;

    stack.push(new InsertCommand(&str, 2, "123")); // should not merge
    QCOMPARE(str, QString("en123ematopeja"));
    QCOMPARE(merge_cmd->merged, false);
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.endMacro();
    QCOMPARE(str, QString("en123ematopeja"));
    CHECK_STATE(false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("enema"));
    CHECK_STATE(true,      // clean
                3,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "ding",     // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.redo();
    QCOMPARE(str, QString("en123ematopeja"));
    CHECK_STATE(false,      // clean
                3,          // count
                3,          // index
                true,       // canUndo
                "ding",     // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    delete undo_action;
    delete redo_action;
}

void tst_QUndoStack::undoLimit()
{
    QUndoStack stack;
    QAction *undo_action = stack.createUndoAction(0, QString("foo"));
    QAction *redo_action = stack.createRedoAction(0, QString("bar"));
    QSignalSpy indexChangedSpy(&stack, SIGNAL(indexChanged(int)));
    QSignalSpy cleanChangedSpy(&stack, SIGNAL(cleanChanged(bool)));
    QSignalSpy canUndoChangedSpy(&stack, SIGNAL(canUndoChanged(bool)));
    QSignalSpy undoTextChangedSpy(&stack, SIGNAL(undoTextChanged(QString)));
    QSignalSpy canRedoChangedSpy(&stack, SIGNAL(canRedoChanged(bool)));
    QSignalSpy redoTextChangedSpy(&stack, SIGNAL(redoTextChanged(QString)));
    AppendCommand::delete_cnt = 0;
    QString str;

    QCOMPARE(stack.undoLimit(), 0);
    stack.setUndoLimit(2);
    QCOMPARE(stack.undoLimit(), 2);

    stack.push(new AppendCommand(&str, "1", true));
    QCOMPARE(str, QString("1"));
    QCOMPARE(AppendCommand::delete_cnt, 0);
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "2", true));
    QCOMPARE(str, QString("12"));
    QCOMPARE(AppendCommand::delete_cnt, 0);
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.setClean();
    QCOMPARE(str, QString("12"));
    QCOMPARE(AppendCommand::delete_cnt, 0);
    CHECK_STATE(true,       // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new AppendCommand(&str, "3", true));
    QCOMPARE(str, QString("123"));
    QCOMPARE(AppendCommand::delete_cnt, 1);
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "4", true));
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 2);
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("123"));
    QCOMPARE(AppendCommand::delete_cnt, 2);
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("12"));
    QCOMPARE(AppendCommand::delete_cnt, 2);
    CHECK_STATE(true,       // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "append",   // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "3", true));
    QCOMPARE(str, QString("123"));
    QCOMPARE(AppendCommand::delete_cnt, 4);
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                true,       // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "4", true));
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 4);
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "5", true));
    QCOMPARE(str, QString("12345"));
    QCOMPARE(AppendCommand::delete_cnt, 5);
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 5);
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("123"));
    QCOMPARE(AppendCommand::delete_cnt, 5);
    CHECK_STATE(false,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "4", true));
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 7);
    CHECK_STATE(false,      // clean
                1,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "5"));
    QCOMPARE(str, QString("12345"));
    QCOMPARE(AppendCommand::delete_cnt, 7);
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "6", true)); // should be merged
    QCOMPARE(str, QString("123456"));
    QCOMPARE(AppendCommand::delete_cnt, 8);
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "append",   // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.beginMacro("foo");
    QCOMPARE(str, QString("123456"));
    QCOMPARE(AppendCommand::delete_cnt, 8);
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.push(new AppendCommand(&str, "7", true));
    QCOMPARE(str, QString("1234567"));
    QCOMPARE(AppendCommand::delete_cnt, 8);
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.push(new AppendCommand(&str, "8"));
    QCOMPARE(str, QString("12345678"));
    QCOMPARE(AppendCommand::delete_cnt, 8);
    CHECK_STATE(false,      // clean
                3,          // count
                2,          // index
                false,      // canUndo
                "",         // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                false,      // indexChanged
                false,      // undoChanged
                false)      // redoChanged

    stack.endMacro();
    QCOMPARE(str, QString("12345678"));
    QCOMPARE(AppendCommand::delete_cnt, 9);
    CHECK_STATE(false,      // clean
                2,          // count
                2,          // index
                true,       // canUndo
                "foo",      // undoText
                false,      // canRedo
                "",         // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("123456"));
    QCOMPARE(AppendCommand::delete_cnt, 9);
    CHECK_STATE(false,      // clean
                2,          // count
                1,          // index
                true,       // canUndo
                "append",   // undoText
                true,       // canRedo
                "foo",      // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged

    stack.undo();
    QCOMPARE(str, QString("1234"));
    QCOMPARE(AppendCommand::delete_cnt, 9);
    CHECK_STATE(false,      // clean
                2,          // count
                0,          // index
                false,      // canUndo
                "",         // undoText
                true,       // canRedo
                "append",   // redoText
                false,      // cleanChanged
                true,       // indexChanged
                true,       // undoChanged
                true)       // redoChanged
}

QTEST_MAIN(tst_QUndoStack)

#include "tst_qundostack.moc"
