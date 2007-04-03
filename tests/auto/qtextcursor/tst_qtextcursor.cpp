/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtTest/QtTest>


#include <qtextdocument.h>
#include <qtexttable.h>
#include <qvariant.h>
#include <qtextdocumentfragment.h>
#include <qabstracttextdocumentlayout.h>
#include <qtextlayout.h>
#include <qtextcursor.h>
#include <qdebug.h>

//TESTED_FILES=gui/text/qtextcursor.cpp gui/text/qtextcursor_p.h

class QTextDocument;

class tst_QTextCursor : public QObject
{
    Q_OBJECT

public:
    tst_QTextCursor();


public slots:
    void init();
    void cleanup();
private slots:
    void navigation1();
    void navigation2_data();
    void navigation2();
    void navigation3();
    void navigation4();
    void navigation5();
    void navigation6();
    void navigation7();
    void insertBlock();
    void insertWithBlockSeparator1();
    void insertWithBlockSeparator2();
    void insertWithBlockSeparator3();
    void insertWithBlockSeparator4();
    void clearObjectType1();
    void clearObjectType2();
    void clearObjectType3();
    void comparisonOperators1();
    void comparisonOperators2();
    void selection1();
    void dontCopyTableAttributes();

    void checkFrame1();
    void checkFrame2();

    void tableMovement();
    void selectionsInTable();

    void insertBlockToUseCharFormat();

    void selectedText();

    void insertBlockShouldRemoveSelection();
    void insertBlockShouldRemoveSelection2();

    void joinPreviousEditBlock();

    void setBlockFormatInTable();

    void blockCharFormat();
    void blockCharFormat2();
    void blockCharFormat3();
    void blockCharFormatOnSelection();

    void anchorInitialized1();
    void anchorInitialized2();
    void anchorInitialized3();

    void selectWord();
    void selectWordWithSeparators_data();
    void selectWordWithSeparators();
    void startOfWord();
#if QT_VERSION >= 0x040100
    void selectBlock();
#endif

    void insertText();

    void insertFragmentShouldUseCurrentCharFormat();

    void endOfLine();

    void editBlocksDuringRemove();

    void update_data();
    void update();

    void disallowSettingObjectIndicesOnCharFormats();

    void blockAndColumnNumber();

private:
    int blockCount();

    QTextDocument *doc;
    QTextCursor cursor;
};

Q_DECLARE_METATYPE(QList<QVariant>)

tst_QTextCursor::tst_QTextCursor()
{}

void tst_QTextCursor::init()
{
    doc = new QTextDocument;
    cursor = QTextCursor(doc);
}

void tst_QTextCursor::cleanup()
{
    cursor = QTextCursor();
    delete doc;
    doc = 0;
}

void tst_QTextCursor::navigation1()
{

    cursor.insertText("Hello World");
    QVERIFY(doc->toPlainText() == "Hello World");

    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.position() == 11);
    cursor.deletePreviousChar();
    QVERIFY(cursor.position() == 10);
    cursor.deletePreviousChar();
    cursor.deletePreviousChar();
    cursor.deletePreviousChar();
    cursor.deletePreviousChar();
    cursor.deletePreviousChar();
    QVERIFY(doc->toPlainText() == "Hello");

    QTextCursor otherCursor(doc);
    otherCursor.movePosition(QTextCursor::Start);
    otherCursor.movePosition(QTextCursor::Right);
    cursor = otherCursor;
    cursor.movePosition(QTextCursor::Right);
    QVERIFY(cursor != otherCursor);
    otherCursor.insertText("Hey");
    QVERIFY(cursor.position() == 5);

    doc->undo();
    QVERIFY(cursor.position() == 2);
    doc->redo();
    QVERIFY(cursor.position() == 5);

    doc->undo();

    doc->undo();
    QVERIFY(doc->toPlainText() == "Hello World");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 6);
    QVERIFY(cursor.position() == 6);
    otherCursor = cursor;
    otherCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 2);
    otherCursor.deletePreviousChar();
    otherCursor.deletePreviousChar();
    otherCursor.deletePreviousChar();
    QVERIFY(cursor.position() == 5);

    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock();
    {
	int oldPos = cursor.position();
	cursor.movePosition(QTextCursor::End);
	QVERIFY(cursor.position() == oldPos);
    }
    QVERIFY(cursor.atBlockStart());
    QVERIFY(cursor.position() == 9);

    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertText("Test", fmt);
    QVERIFY(fmt == cursor.charFormat());
    QVERIFY(cursor.position() == 13);
}

void tst_QTextCursor::navigation2_data()
{
    QTest::addColumn<QStringList>("sl");
    QTest::addColumn<QList<QVariant> >("movement");
    QTest::addColumn<int>("finalPos");

    QTest::newRow("startBlock1") << QStringList("Happy happy happy joy joy joy")
                              << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)) << 0;
    QTest::newRow("endBlock1") << QStringList("Happy happy happy joy joy joy")
                            << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                     << QVariant(QTextCursor::EndOfBlock)) << 29;
    QTest::newRow("startBlock2") << QStringList("Happy happy happy joy joy joy")
                              << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                     << QVariant(QTextCursor::EndOfBlock)
                                     << QVariant(QTextCursor::StartOfBlock)) << 0;
    QTest::newRow("endBlock2") << QStringList("Happy happy happy joy joy joy")
                            << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                     << QVariant(QTextCursor::EndOfBlock)
                                     << QVariant(QTextCursor::StartOfBlock)
                                     << QVariant(QTextCursor::EndOfBlock)
                                     ) << 29;
    QTest::newRow("multiBlock1") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock))
                             << 18;
    QTest::newRow("multiBlock2") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                                   << QVariant(QTextCursor::EndOfBlock))
                             << 29;
    QTest::newRow("multiBlock3") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::StartOfBlock)
                                                   << QVariant(QTextCursor::StartOfBlock))
                             << 18;
    QTest::newRow("multiBlock4") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::Start)
                                                   << QVariant(QTextCursor::EndOfBlock))
                             << 17;
    QTest::newRow("multiBlock5") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::Start)
                                                   << QVariant(QTextCursor::EndOfBlock)
                                                   << QVariant(QTextCursor::EndOfBlock))
                             << 17;
    QTest::newRow("multiBlock6") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::End)
                                                   << QVariant(QTextCursor::StartOfBlock))
                             << 18;
    QTest::newRow("multiBlock7") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock))
                             << 0;
    QTest::newRow("multiBlock8") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock)
                                                   << QVariant(QTextCursor::EndOfBlock))
                             << 17;
    QTest::newRow("multiBlock9") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock)
                                                   << QVariant(QTextCursor::NextBlock))
                             << 18;
    QTest::newRow("multiBlock10") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                               << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock)
                                                     << QVariant(QTextCursor::NextBlock)
                                                     << QVariant(QTextCursor::NextBlock))
                               << 18;
    QTest::newRow("multiBlock11") << (QStringList() << QString("Happy happy happy")
                                                << QString("Joy Joy Joy"))
                               << (QList<QVariant>() << QVariant(QTextCursor::PreviousBlock)
                                                     << QVariant(QTextCursor::NextBlock)
                                                     << QVariant(QTextCursor::EndOfBlock))
                               << 29;
    QTest::newRow("PreviousWord1") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord))
                                << 26;
    QTest::newRow("PreviousWord2") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::PreviousWord))
                                << 22;
    QTest::newRow("EndWord1") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::EndOfWord))
                                << 25;
    QTest::newRow("NextWord1") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::PreviousWord)
                                                      << QVariant(QTextCursor::NextWord))
                                << 26;
    QTest::newRow("NextWord2") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                                << (QList<QVariant>() << QVariant(QTextCursor::Start)
                                                      << QVariant(QTextCursor::NextWord)
                                                      << QVariant(QTextCursor::EndOfWord))
                                << 11;
    QTest::newRow("StartWord1") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::PreviousWord)
                                                   << QVariant(QTextCursor::PreviousWord)
                                                   << QVariant(QTextCursor::StartOfWord))
                             << 22;
    QTest::newRow("StartWord3") << (QStringList() << QString("Happy happy happy Joy Joy Joy"))
                             << (QList<QVariant>() << QVariant(QTextCursor::Start)
                                                   << QVariant(QTextCursor::NextWord)
                                                   << QVariant(QTextCursor::EndOfWord)
                                                   << QVariant(QTextCursor::StartOfWord))
                             << 6;
}

void tst_QTextCursor::navigation2()
{
    QFETCH(QStringList, sl);
    QFETCH(QList<QVariant>, movement);
    int i;
    for (i = 0; i < sl.size(); ++i) {
        cursor.insertText(sl.at(i));
        if (i < sl.size() - 1)
            cursor.insertBlock();
    }

    for (i = 0; i < movement.size(); ++i)
        cursor.movePosition(QTextCursor::MoveOperation(movement.at(i).toInt()));
    QTEST(cursor.position(), "finalPos");
}

void tst_QTextCursor::navigation3()
{
    cursor.insertText("a");
    cursor.deletePreviousChar();
    QCOMPARE(cursor.position(), 0);
    QVERIFY(doc->toPlainText().isEmpty());
}

void tst_QTextCursor::navigation4()
{
    cursor.insertText("  Test  ");

    cursor.setPosition(4);
    cursor.movePosition(QTextCursor::EndOfWord);
    QCOMPARE(cursor.position(), 6);
}

void tst_QTextCursor::navigation5()
{
    cursor.insertText("Test");
    cursor.insertBlock();
    cursor.insertText("Test");

    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::EndOfBlock);
    QCOMPARE(cursor.position(), 4);
}

void tst_QTextCursor::navigation6()
{
    // triger creation of document layout, so that QTextLines are there
    doc->documentLayout();
    doc->setPageSize(QSizeF(INT_MAX, INT_MAX));

    cursor.insertText("Test    ");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::EndOfLine);
    QCOMPARE(cursor.position(), 8);
}

void tst_QTextCursor::navigation7()
{
    QVERIFY(doc->isEmpty());
    for (int i = QTextCursor::Start; i <= QTextCursor::WordRight; ++i)
        QVERIFY(!cursor.movePosition(QTextCursor::MoveOperation(i)));

    doc->setPlainText("Hello World");
    cursor.movePosition(QTextCursor::Start);
    do {
    } while (cursor.movePosition(QTextCursor::NextCharacter));
    QVERIFY(true /*reached*/);
}

void tst_QTextCursor::insertBlock()
{
    QTextBlockFormat fmt;
    fmt.setTopMargin(100);
    cursor.insertBlock(fmt);
    QVERIFY(cursor.position() == 1);
    QVERIFY(cursor.blockFormat() == fmt);
}

void tst_QTextCursor::insertWithBlockSeparator1()
{
    QString text = "Hello" + QString(QChar::ParagraphSeparator) + "World";

    cursor.insertText(text);

    cursor.movePosition(QTextCursor::PreviousBlock);
    QVERIFY(cursor.position() == 0);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.position() == 6);
}

void tst_QTextCursor::insertWithBlockSeparator2()
{
    cursor.insertText(QString(QChar::ParagraphSeparator));
    QVERIFY(cursor.position() == 1);
}

void tst_QTextCursor::insertWithBlockSeparator3()
{
    cursor.insertText(QString(QChar::ParagraphSeparator) + "Hi" + QString(QChar::ParagraphSeparator));
    QVERIFY(cursor.position() == 4);
}

void tst_QTextCursor::insertWithBlockSeparator4()
{
    cursor.insertText(QString(QChar::ParagraphSeparator) + QString(QChar::ParagraphSeparator));
    QVERIFY(cursor.position() == 2);
}

void tst_QTextCursor::clearObjectType1()
{
    cursor.insertImage("test.png");
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(cursor.charFormat().isImageFormat());
    cursor.insertText("Hey");
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(!cursor.charFormat().isImageFormat());
}

void tst_QTextCursor::clearObjectType2()
{
    cursor.insertImage("test.png");
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(cursor.charFormat().isImageFormat());
    cursor.insertBlock();
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(!cursor.charFormat().isImageFormat());
}

void tst_QTextCursor::clearObjectType3()
{
    // like clearObjectType2 but tests different insertBlock overload
    cursor.insertImage("test.png");
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(cursor.charFormat().isImageFormat());
    QTextBlockFormat bfmt;
    bfmt.setAlignment(Qt::AlignRight);
    cursor.insertBlock(bfmt);
    QVERIFY(cursor.charFormat().isValid());
    QVERIFY(!cursor.charFormat().isImageFormat());
}

void tst_QTextCursor::comparisonOperators1()
{
    cursor.insertText("Hello World");

    cursor.movePosition(QTextCursor::PreviousWord);

    QTextCursor startCursor = cursor;
    startCursor.movePosition(QTextCursor::Start);

    QVERIFY(startCursor < cursor);

    QTextCursor midCursor = startCursor;
    midCursor.movePosition(QTextCursor::NextWord);

    QVERIFY(midCursor <= cursor);
    QVERIFY(midCursor == cursor);
    QVERIFY(midCursor >= cursor);

    QVERIFY(midCursor > startCursor);

    QVERIFY(midCursor != startCursor);
    QVERIFY(!(midCursor == startCursor));

    QTextCursor nullCursor;

    QVERIFY(!(startCursor < nullCursor));
    QVERIFY(!(nullCursor < nullCursor));
    QVERIFY(nullCursor < startCursor);

    QVERIFY(nullCursor <= startCursor);
    QVERIFY(!(startCursor <= nullCursor));

    QVERIFY(!(nullCursor >= startCursor));
    QVERIFY(startCursor >= nullCursor);

    QVERIFY(!(nullCursor > startCursor));
    QVERIFY(!(nullCursor > nullCursor));
    QVERIFY(startCursor > nullCursor);
}

void tst_QTextCursor::comparisonOperators2()
{
    QTextDocument doc1;
    QTextDocument doc2;

    QTextCursor cursor1(&doc1);
    QTextCursor cursor2(&doc2);

    QVERIFY(cursor1 != cursor2);
    QVERIFY(cursor1 == QTextCursor(&doc1));
}

void tst_QTextCursor::selection1()
{
    cursor.insertText("Hello World");

    cursor.setPosition(0);
    cursor.clearSelection();
    cursor.setPosition(4, QTextCursor::KeepAnchor);

    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 4);
}

void tst_QTextCursor::dontCopyTableAttributes()
{
    /* when pressing 'enter' inside a cell it shouldn't
     * enlarge the table by adding another cell but just
     * extend the cell */
    QTextTable *table = cursor.insertTable(2, 2);
    QVERIFY(cursor == table->cellAt(0, 0).firstCursorPosition());
    cursor.insertBlock();
    QCOMPARE(table->columns(), 2);
}

void tst_QTextCursor::checkFrame1()
{
    QVERIFY(cursor.position() == 0);
    QPointer<QTextFrame> frame = cursor.insertFrame(QTextFrameFormat());
    QVERIFY(frame != 0);

    QTextFrame *root = frame->parentFrame();
    QVERIFY(root != 0);

    QVERIFY(frame->firstPosition() == 1);
    QVERIFY(frame->lastPosition() == 1);
    QVERIFY(frame->parentFrame() != 0);
    QVERIFY(root->childFrames().size() == 1);

    QVERIFY(cursor.position() == 1);
    QVERIFY(cursor.selectionStart() == 1);
    QVERIFY(cursor.selectionEnd() == 1);

    doc->undo();

    QVERIFY(!frame);
    QVERIFY(root->childFrames().size() == 0);

    QVERIFY(cursor.position() == 0);
    QVERIFY(cursor.selectionStart() == 0);
    QVERIFY(cursor.selectionEnd() == 0);

    doc->redo();

    frame = doc->frameAt(1);

    QVERIFY(frame);
    QVERIFY(frame->firstPosition() == 1);
    QVERIFY(frame->lastPosition() == 1);
    QVERIFY(frame->parentFrame() != 0);
    QVERIFY(root->childFrames().size() == 1);

    QVERIFY(cursor.position() == 1);
    QVERIFY(cursor.selectionStart() == 1);
    QVERIFY(cursor.selectionEnd() == 1);

//     cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
//     QVERIFY(cursor.position() == 2);
//     QVERIFY(cursor.selectionStart() == 0);
//     QVERIFY(cursor.selectionEnd() == 2);
}

void tst_QTextCursor::checkFrame2()
{
    QVERIFY(cursor.position() == 0);
    cursor.insertText("A");
    QVERIFY(cursor.position() == 1);
    cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);

    QPointer<QTextFrame> frame = cursor.insertFrame(QTextFrameFormat());
    QTextFrame *root = frame->parentFrame();

    QVERIFY(frame->firstPosition() == 1);
    QVERIFY(frame->lastPosition() == 2);
    QVERIFY(frame->parentFrame() != 0);
    QVERIFY(root->childFrames().size() == 1);

    QVERIFY(cursor.position() == 1);
    QVERIFY(cursor.selectionStart() == 1);
    QVERIFY(cursor.selectionEnd() == 2);

    doc->undo();

    QVERIFY(!frame);
    QVERIFY(root->childFrames().size() == 0);

    QVERIFY(cursor.position() == 0);
    QVERIFY(cursor.selectionStart() == 0);
    QVERIFY(cursor.selectionEnd() == 1);

    doc->redo();

    frame = doc->frameAt(1);

    QVERIFY(frame);
    QVERIFY(frame->firstPosition() == 1);
    QVERIFY(frame->lastPosition() == 2);
    QVERIFY(frame->parentFrame() != 0);
    QVERIFY(root->childFrames().size() == 1);

    QVERIFY(cursor.position() == 1);
    QVERIFY(cursor.selectionStart() == 1);
    QVERIFY(cursor.selectionEnd() == 2);

    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    QVERIFY(cursor.position() == 0);
    QVERIFY(cursor.selectionStart() == 0);
    QVERIFY(cursor.selectionEnd() == 3);
}

void tst_QTextCursor::insertBlockToUseCharFormat()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertText("Hello", fmt);
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::blue));

    cursor.insertBlock();
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::blue));

    fmt.setForeground(Qt::red);
    cursor.insertText("Hello\nWorld", fmt);
    cursor.insertText("Blah");
    QCOMPARE(cursor.charFormat().foreground().color(), QColor(Qt::red));

    // ### we might want a testcase for createTable, too, as it calls insertBlock, too,
    // and we might want to have the char format copied (the one that gets inserted
    // as table separators, that are undeletable)
}

void tst_QTextCursor::tableMovement()
{
    QVERIFY(cursor.position() == 0);
    cursor.insertText("AA");
    QVERIFY(cursor.position() == 2);
    cursor.movePosition(QTextCursor::Left);

    cursor.insertTable(3, 3);
    QCOMPARE(cursor.position(), 2);

    cursor.movePosition(QTextCursor::Down);
    QCOMPARE(cursor.position(), 5);

    cursor.movePosition(QTextCursor::Right);
    QCOMPARE(cursor.position(), 6);

    cursor.movePosition(QTextCursor::Up);
    QCOMPARE(cursor.position(), 3);

    cursor.movePosition(QTextCursor::Right);
    QCOMPARE(cursor.position(), 4);

    cursor.movePosition(QTextCursor::Right);
    QCOMPARE(cursor.position(), 5);

    cursor.movePosition(QTextCursor::Up);
    QCOMPARE(cursor.position(), 2);

    cursor.movePosition(QTextCursor::Up);
    QCOMPARE(cursor.position(), 0);

}

void tst_QTextCursor::selectionsInTable()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");

    cursor = table->cellAt(0, 0).lastCursorPosition();
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor) == false);

    cursor = table->cellAt(1, 0).lastCursorPosition();
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor) == false);

    cursor = table->cellAt(0, 1).firstCursorPosition();
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor) == false);

    cursor = table->cellAt(1, 1).firstCursorPosition();
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor));
    QVERIFY(cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor) == false);
}

void tst_QTextCursor::selectedText()
{
    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

    QCOMPARE(cursor.selectedText(), QString("Hello World"));
}

void tst_QTextCursor::insertBlockShouldRemoveSelection()
{
    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectedText(), QString("Hello"));

    cursor.insertBlock();

    QVERIFY(!cursor.hasSelection());
    QVERIFY(doc->toPlainText().indexOf("Hello") == -1);
}

void tst_QTextCursor::insertBlockShouldRemoveSelection2()
{
    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectedText(), QString("Hello"));

    QTextBlockFormat fmt = cursor.blockFormat();
    cursor.insertBlock(fmt);

    QVERIFY(!cursor.hasSelection());
    QVERIFY(doc->toPlainText().indexOf("Hello") == -1);
}

void tst_QTextCursor::joinPreviousEditBlock()
{
    cursor.beginEditBlock();
    cursor.insertText("Hello");
    cursor.insertText("World");
    cursor.endEditBlock();
    QVERIFY(doc->toPlainText().startsWith("HelloWorld"));

    cursor.joinPreviousEditBlock();
    cursor.insertText("Hey");
    cursor.endEditBlock();
    QVERIFY(doc->toPlainText().startsWith("HelloWorldHey"));

    doc->undo();
    QVERIFY(!doc->toPlainText().contains("HelloWorldHey"));
}

void tst_QTextCursor::setBlockFormatInTable()
{
    // someone reported this on qt4-preview-feedback
    QTextBlockFormat fmt;
    fmt.setBackground(Qt::blue);
    cursor.setBlockFormat(fmt);

    QTextTable *table = cursor.insertTable(2, 2);
    cursor = table->cellAt(0, 0).firstCursorPosition();
    fmt.setBackground(Qt::red);
    cursor.setBlockFormat(fmt);

    cursor.movePosition(QTextCursor::Start);
    QVERIFY(cursor.blockFormat().background().color() == Qt::blue);
}

void tst_QTextCursor::blockCharFormat2()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::green);
    cursor.mergeBlockCharFormat(fmt);

    fmt.setForeground(Qt::red);

    cursor.insertText("Test", fmt);
    cursor.movePosition(QTextCursor::Start);
    cursor.insertText("Red");
    cursor.movePosition(QTextCursor::PreviousCharacter);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::red);
}

void tst_QTextCursor::blockCharFormat3()
{
    QVERIFY(cursor.atBlockStart());
    QVERIFY(cursor.atBlockEnd());
    QVERIFY(cursor.atStart());

    QTextCharFormat fmt;
    fmt.setForeground(Qt::green);
    cursor.setBlockCharFormat(fmt);
    cursor.insertText("Test");
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::green);

    cursor.movePosition(QTextCursor::Start);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::green);

    fmt.setForeground(Qt::red);
    cursor.setBlockCharFormat(fmt);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::red);

    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::Start);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::green);

    cursor.insertText("Test");
    QVERIFY(cursor.charFormat().foreground().color() == Qt::green);

    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    QVERIFY(cursor.atBlockStart());
    QVERIFY(cursor.atBlockEnd());
    QVERIFY(cursor.atStart());

    cursor.insertText("Test");
    QVERIFY(cursor.charFormat().foreground().color() == Qt::red);
}

void tst_QTextCursor::blockCharFormat()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertBlock(QTextBlockFormat(), fmt);
    cursor.insertText("Hm");

    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::blue);

    fmt.setForeground(Qt::red);

    cursor.setBlockCharFormat(fmt);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::red);
}

void tst_QTextCursor::blockCharFormatOnSelection()
{
    QTextCharFormat fmt;
    fmt.setForeground(Qt::blue);
    cursor.insertBlock(QTextBlockFormat(), fmt);

    fmt.setForeground(Qt::green);
    cursor.insertText("Hm", fmt);

    fmt.setForeground(Qt::red);
    cursor.insertBlock(QTextBlockFormat(), fmt);
    cursor.insertText("Ah");

    fmt.setForeground(Qt::white);
    cursor.insertBlock(QTextBlockFormat(), fmt);
    cursor.insertText("bleh");

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::blue);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::red);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::white);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);

    fmt.setForeground(Qt::cyan);
    cursor.setBlockCharFormat(fmt);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::cyan);

    cursor.movePosition(QTextCursor::Right);
    cursor.movePosition(QTextCursor::Right);
    QVERIFY(cursor.charFormat().foreground().color() == Qt::green);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::cyan);

    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.blockCharFormat().foreground().color() == Qt::white);
}

void tst_QTextCursor::anchorInitialized1()
{
    cursor.insertBlock();
    cursor = QTextCursor(cursor.block());
    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.anchor(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 1);
}

void tst_QTextCursor::anchorInitialized2()
{
    cursor.insertBlock();
    cursor = QTextCursor(cursor.block().docHandle(), 1);
    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.anchor(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 1);
}

void tst_QTextCursor::anchorInitialized3()
{
    QTextFrame *frame = cursor.insertFrame(QTextFrameFormat());
    cursor = QTextCursor(frame);
    QCOMPARE(cursor.position(), 1);
    QCOMPARE(cursor.anchor(), 1);
    QCOMPARE(cursor.selectionStart(), 1);
    QCOMPARE(cursor.selectionEnd(), 1);
}

void tst_QTextCursor::selectWord()
{
    cursor.insertText("first second     third");
    cursor.insertBlock();
    cursor.insertText("words in second paragraph");

    cursor.setPosition(9);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 6);
    QCOMPARE(cursor.selectionEnd(), 12);

    cursor.setPosition(5);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 5);

    cursor.setPosition(6);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 6);
    QCOMPARE(cursor.selectionEnd(), 12);

    cursor.setPosition(14);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 6);
    QCOMPARE(cursor.selectionEnd(), 12);

    cursor.movePosition(QTextCursor::Start);
    cursor.select(QTextCursor::WordUnderCursor);
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), 5);

    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.select(QTextCursor::WordUnderCursor);
#if QT_VERSION < 0x040200
    QVERIFY(!cursor.hasSelection());
#else
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 17);
    QCOMPARE(cursor.selectionEnd(), 22);
#endif
}

void tst_QTextCursor::selectWordWithSeparators_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("initialPosition");
    QTest::addColumn<QString>("expectedSelectedText");

    QTest::newRow("dereference") << QString::fromLatin1("foo->bar()") << 1 << QString::fromLatin1("foo");
    QTest::newRow("funcsignature") << QString::fromLatin1("bar(int x);") << 1 << QString::fromLatin1("bar");
    QTest::newRow("def") << QString::fromLatin1("foo *f;") << 1 << QString::fromLatin1("foo");
}

void tst_QTextCursor::selectWordWithSeparators()
{
    QFETCH(QString, text);
    QFETCH(int, initialPosition);
    QFETCH(QString, expectedSelectedText);

    cursor.insertText(text);
    cursor.setPosition(initialPosition);
    cursor.select(QTextCursor::WordUnderCursor);

    QCOMPARE(cursor.selectedText(), expectedSelectedText);
}

void tst_QTextCursor::startOfWord()
{
    cursor.insertText("first     second");
    cursor.setPosition(7);
    cursor.movePosition(QTextCursor::StartOfWord);
    QCOMPARE(cursor.position(), 0);
}

#if QT_VERSION >= 0x040100
void tst_QTextCursor::selectBlock()
{
    cursor.insertText("foobar");
    QTextBlockFormat blockFmt;
    blockFmt.setAlignment(Qt::AlignHCenter);
    cursor.insertBlock(blockFmt);
    cursor.insertText("blah");
    cursor.insertBlock(QTextBlockFormat());

    cursor.movePosition(QTextCursor::PreviousBlock);
    QCOMPARE(cursor.block().text(), QString("blah"));

    cursor.select(QTextCursor::BlockUnderCursor);
    QVERIFY(cursor.hasSelection());

    QTextDocumentFragment fragment(cursor);
    doc->clear();
    cursor.insertFragment(fragment);
    QCOMPARE(blockCount(), 2);

    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QVERIFY(cursor.blockFormat().alignment() == Qt::AlignHCenter);
    QCOMPARE(cursor.block().text(), QString("blah"));
}
#endif

void tst_QTextCursor::insertText()
{
    QString txt = "Foo\nBar\r\nMeep";
    txt += QChar::LineSeparator;
    txt += "Baz";
    txt += QChar::ParagraphSeparator;
    txt += "yoyodyne";
    cursor.insertText(txt);
    QCOMPARE(blockCount(), 4);
    cursor.movePosition(QTextCursor::Start);
    QCOMPARE(cursor.block().text(), QString("Foo"));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString("Bar"));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString(QString("Meep") + QChar(QChar::LineSeparator) + QString("Baz")));
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.block().text(), QString("yoyodyne"));
}

void tst_QTextCursor::insertFragmentShouldUseCurrentCharFormat()
{
    QTextDocumentFragment fragment = QTextDocumentFragment::fromPlainText("Hello World");
    QTextCharFormat fmt;
    fmt.setFontUnderline(true);

    cursor.clearSelection();
    cursor.setCharFormat(fmt);
    cursor.insertFragment(fragment);
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter);
    QVERIFY(cursor.charFormat() == fmt);
}

int tst_QTextCursor::blockCount()
{
    int cnt = 0;
    for (QTextBlock blk = doc->begin(); blk.isValid(); blk = blk.next())
        ++cnt;
    return cnt;
}

void tst_QTextCursor::endOfLine()
{
    doc->setPageSize(QSizeF(100000, INT_MAX));

    QString text("First Line    \nSecond Line  ");
    text.replace(QLatin1Char('\n'), QChar(QChar::LineSeparator));
    cursor.insertText(text);

    // ensure layouted
    doc->documentLayout()->documentSize();

    cursor.movePosition(QTextCursor::Start);

    QCOMPARE(cursor.block().layout()->lineCount(), 2);

    cursor.movePosition(QTextCursor::EndOfLine);
    QCOMPARE(cursor.position(), 14);
    cursor.movePosition(QTextCursor::NextCharacter);
    QCOMPARE(cursor.position(), 15);
    cursor.movePosition(QTextCursor::EndOfLine);
    QCOMPARE(cursor.position(), 28);
}

class CursorListener : public QObject
{
    Q_OBJECT
public:
    CursorListener(QTextCursor *_cursor) : lastRecordedPosition(-1), lastRecordedAnchor(-1), recordingCount(0), cursor(_cursor) {}

    int lastRecordedPosition;
    int lastRecordedAnchor;
    int recordingCount;

public slots:
    void recordCursorPosition()
    {
        lastRecordedPosition = cursor->position();
        lastRecordedAnchor = cursor->anchor();
        ++recordingCount;
    }

private:
    QTextCursor *cursor;
};

void tst_QTextCursor::editBlocksDuringRemove()
{
    CursorListener listener(&cursor);

    cursor.insertText("Hello World");
    cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
    QCOMPARE(cursor.selectedText(), QString("Hello World"));

    connect(doc, SIGNAL(contentsChanged()), &listener, SLOT(recordCursorPosition()));
    listener.recordingCount = 0;
    cursor.deleteChar();

    QCOMPARE(listener.recordingCount, 1);
    QCOMPARE(listener.lastRecordedPosition, 0);
    QCOMPARE(listener.lastRecordedAnchor, 0);

    QVERIFY(doc->toPlainText().isEmpty());
}

void tst_QTextCursor::update_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("anchor");
    QTest::addColumn<int>("modifyPosition");
    QTest::addColumn<int>("modifyAnchor");
    QTest::addColumn<QString>("insertText");
    QTest::addColumn<int>("expectedPosition");
    QTest::addColumn<int>("expectedAnchor");

    QString text("Hello big world");
    int charsToDelete = 3;
    QTest::newRow("removeInsideSelection")
        << text
        << /*position*/ 0
        << /*anchor*/ text.length()
        // delete 'big'
        << 6
        << 6 + charsToDelete
        << QString() // don't insert anything, just remove
        << /*expectedPosition*/ 0
        << /*expectedAnchor*/ text.length() - charsToDelete
        ;

    text = "Hello big world";
    charsToDelete = 3;
    QTest::newRow("removeInsideSelectionWithSwappedAnchorAndPosition")
        << text
        << /*position*/ text.length()
        << /*anchor*/ 0
        // delete 'big'
        << 6
        << 6 + charsToDelete
        << QString() // don't insert anything, just remove
        << /*expectedPosition*/ text.length() - charsToDelete
        << /*expectedAnchor*/ 0
        ;


    text = "Hello big world";
    charsToDelete = 3;
    QString textToInsert("small");
    QTest::newRow("replaceInsideSelection")
        << text
        << /*position*/ 0
        << /*anchor*/ text.length()
        // delete 'big' ...
        << 6
        << 6 + charsToDelete
        << textToInsert // ... and replace 'big' with 'small'
        << /*expectedPosition*/ 0
        << /*expectedAnchor*/ text.length() - charsToDelete + textToInsert.length()
        ;

    text = "Hello big world";
    charsToDelete = 3;
    textToInsert = "small";
    QTest::newRow("replaceInsideSelectionWithSwappedAnchorAndPosition")
        << text
        << /*position*/ text.length()
        << /*anchor*/ 0
        // delete 'big' ...
        << 6
        << 6 + charsToDelete
        << textToInsert // ... and replace 'big' with 'small'
        << /*expectedPosition*/ text.length() - charsToDelete + textToInsert.length()
        << /*expectedAnchor*/ 0
        ;


    text = "Hello big world";
    charsToDelete = 3;
    QTest::newRow("removeBeforeSelection")
        << text
        << /*position*/ text.length() - 5
        << /*anchor*/ text.length()
        // delete 'big'
        << 6
        << 6 + charsToDelete
        << QString() // don't insert anything, just remove
        << /*expectedPosition*/ text.length() - 5 - charsToDelete
        << /*expectedAnchor*/ text.length() - charsToDelete
        ;

    text = "Hello big world";
    charsToDelete = 3;
    QTest::newRow("removeAfterSelection")
        << text
        << /*position*/ 0
        << /*anchor*/ 4
        // delete 'big'
        << 6
        << 6 + charsToDelete
        << QString() // don't insert anything, just remove
        << /*expectedPosition*/ 0
        << /*expectedAnchor*/ 4
        ;

}

void tst_QTextCursor::update()
{
    QFETCH(QString, text);

    doc->setPlainText(text);

    QFETCH(int, position);
    QFETCH(int, anchor);

    cursor.setPosition(anchor);
    cursor.setPosition(position, QTextCursor::KeepAnchor);

    QCOMPARE(cursor.position(), position);
    QCOMPARE(cursor.anchor(), anchor);

    QFETCH(int, modifyPosition);
    QFETCH(int, modifyAnchor);

    QTextCursor modifyCursor = cursor;
    modifyCursor.setPosition(modifyAnchor);
    modifyCursor.setPosition(modifyPosition, QTextCursor::KeepAnchor);

    QCOMPARE(modifyCursor.position(), modifyPosition);
    QCOMPARE(modifyCursor.anchor(), modifyAnchor);

    QFETCH(QString, insertText);
    modifyCursor.insertText(insertText);

    QFETCH(int, expectedPosition);
    QFETCH(int, expectedAnchor);

    QCOMPARE(cursor.position(), expectedPosition);
    QCOMPARE(cursor.anchor(), expectedAnchor);
}

void tst_QTextCursor::disallowSettingObjectIndicesOnCharFormats()
{
#if QT_VERSION >= 0x040200
    QTextCharFormat fmt;
    fmt.setObjectIndex(42);
    cursor.insertText("Hey", fmt);
    QCOMPARE(cursor.charFormat().objectIndex(), -1);

    cursor.select(QTextCursor::Document);
    cursor.mergeCharFormat(fmt);
    QCOMPARE(doc->begin().begin().fragment().charFormat().objectIndex(), -1);

    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(fmt);
    QCOMPARE(doc->begin().begin().fragment().charFormat().objectIndex(), -1);

    cursor.setBlockCharFormat(fmt);
    QCOMPARE(cursor.blockCharFormat().objectIndex(), -1);

    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock(QTextBlockFormat(), fmt);
    QCOMPARE(cursor.blockCharFormat().objectIndex(), -1);

    doc->clear();

    QTextTable *table = cursor.insertTable(1, 1);
    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(fmt);

    cursor = table->cellAt(0, 0).firstCursorPosition();
    QVERIFY(!cursor.isNull());
    QCOMPARE(cursor.blockCharFormat().objectIndex(), table->objectIndex());
#endif
}

void tst_QTextCursor::blockAndColumnNumber()
{
    QCOMPARE(QTextCursor().columnNumber(), 0);
    QCOMPARE(QTextCursor().blockNumber(), 0);

    QCOMPARE(cursor.columnNumber(), 0);
    QCOMPARE(cursor.blockNumber(), 0);
    cursor.insertText("Hello");
    QCOMPARE(cursor.columnNumber(), 5);
    QCOMPARE(cursor.blockNumber(), 0);

    cursor.insertBlock();
    QCOMPARE(cursor.columnNumber(), 0);
    QCOMPARE(cursor.blockNumber(), 1);
    cursor.insertText("Blah");
    QCOMPARE(cursor.blockNumber(), 1);

    // trigger a layout
    doc->documentLayout();

    cursor.insertBlock();
    QCOMPARE(cursor.columnNumber(), 0);
    QCOMPARE(cursor.blockNumber(), 2);
    cursor.insertText("Test");
    QCOMPARE(cursor.columnNumber(), 4);
    QCOMPARE(cursor.blockNumber(), 2);
    cursor.insertText(QString(QChar(QChar::LineSeparator)));
    QCOMPARE(cursor.columnNumber(), 0);
    QCOMPARE(cursor.blockNumber(), 2);
    cursor.insertText("A");
    QCOMPARE(cursor.columnNumber(), 1);
    QCOMPARE(cursor.blockNumber(), 2);

}

QTEST_MAIN(tst_QTextCursor)
#include "tst_qtextcursor.moc"
