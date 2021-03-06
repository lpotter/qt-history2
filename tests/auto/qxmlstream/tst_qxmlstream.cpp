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

#include <QDirIterator>
#include <QtTest/QtTest>
#include <QUrl>
#include <QXmlDefaultHandler>
#include <QXmlStreamReader>

#include "qc14n.h"

//TESTED_CLASS=
//TESTED_FILES=xml/qxmlutils.cpp xml/qxmlstream.cpp xml/qxmlstream_p.h

static const char *const catalogFile = "XML-Test-Suite/xmlconf/finalCatalog.xml";
static const int expectedRunCount = 1646;
static const int expectedSkipCount = 532;

static inline int best(int a, int b)
{
    if (a < 0)
        return b;
    if (b < 0)
        return a;
    return qMin(a, b);
}

static inline int best(int a, int b, int c)
{
    if (a < 0)
        return best(b, c);
    if (b < 0)
        return best(a, c);
    if (c < 0)
        return best(a, b);
    return qMin(qMin(a, b), c);
}

/**
 *  Opens @p filename and returns content produced as per
 *  xmlconf/xmltest/canonxml.html.
 *
 *  @p docType is the DOCTYPE name that the returned output should
 *  have, if it doesn't already have one.
 */
static QByteArray makeCanonical(const QString &filename,
                                const QString &docType,
                                bool &hasError,
                                bool testIncremental = false)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QXmlStreamReader reader;

    QByteArray buffer;
    int bufferPos = 0;

    if (testIncremental)
        buffer = file.readAll();
    else
        reader.setDevice(&file);

    QByteArray outarray;
    QXmlStreamWriter writer(&outarray);

    forever {
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.isDTD()) {
                if (!reader.notationDeclarations().isEmpty()) {
                    QString dtd;
                    QTextStream writeDtd(&dtd);

                    writeDtd << "<!DOCTYPE ";
                    writeDtd << docType;
                    writeDtd << " [";
                    writeDtd << endl;
                    QMap<QString, QXmlStreamNotationDeclaration> sortedNotationDeclarations;
                    foreach (QXmlStreamNotationDeclaration notation, reader.notationDeclarations())
                        sortedNotationDeclarations.insert(notation.name().toString(), notation);
                    foreach (QXmlStreamNotationDeclaration notation, sortedNotationDeclarations.values()) {
                        writeDtd << "<!NOTATION ";
                        writeDtd << notation.name().toString();
                        if (notation.publicId().isEmpty()) {
                            writeDtd << " SYSTEM \'";
                            writeDtd << notation.systemId().toString();
                            writeDtd << "\'";
                        } else {
                            writeDtd << " PUBLIC \'";
                            writeDtd << notation.publicId().toString();
                            writeDtd << "\'";
                            if (!notation.systemId().isEmpty() ) {
                                writeDtd << " \'";
                                writeDtd << notation.systemId().toString();
                                writeDtd << "\'";
                            }
                        }
                        writeDtd << ">";
                        writeDtd << endl;
                    }

                    writeDtd << "]>";
                    writeDtd << endl;
                    writer.writeDTD(dtd);
                }
            } else if (reader.isStartElement()) {
                writer.writeStartElement(reader.namespaceUri().toString(), reader.name().toString());

                QMap<QString, QXmlStreamAttribute> sortedAttributes;
                foreach(QXmlStreamAttribute attribute, reader.attributes())
                    sortedAttributes.insert(attribute.name().toString(), attribute);
                foreach(QXmlStreamAttribute attribute, sortedAttributes.values())
                    writer.writeAttribute(attribute);
            } else if (reader.isCharacters()) {
                // make canonical

                QString text = reader.text().toString();
                int i = 0;
                int p = 0;
                while ((i = best(text.indexOf(QLatin1Char(10), p),
                                 text.indexOf(QLatin1Char(13), p),
                                 text.indexOf(QLatin1Char(9), p))) >= 0) {
                    writer.writeCharacters(text.mid(p, i - p));
                    writer.writeEntityReference(QString("#%1").arg(text.at(i).unicode()));
                    p = i + 1;
                }
                writer.writeCharacters(text.mid(p));
            } else if (reader.isStartDocument() || reader.isEndDocument() || reader.isComment()){
                // canonical does not want any of those
            } else if (reader.isProcessingInstruction() && reader.processingInstructionData().isEmpty()) {
                // for some reason canonical wants a space
                writer.writeProcessingInstruction(reader.processingInstructionTarget().toString(), QLatin1String(""));
            } else if (!reader.hasError()){
                writer.writeCurrentToken(reader);
            }
        }
        if (testIncremental && bufferPos < buffer.size()) {
            reader.addData(QByteArray(buffer.data() + (bufferPos++), 1));
        } else {
            break;
        }
    }

    if (reader.hasError()) {
        hasError = true;
        outarray += "ERROR:";
        outarray += reader.errorString().toLatin1();
    }
    else
        hasError = false;

    return outarray;
}

/**
 * @short Returns the lexical QName of the document element in
 * @p document.
 *
 * It is assumed that @p document is a well-formed XML document.
 */
static QString documentElement(const QByteArray &document)
{
    QXmlStreamReader reader(document);

    while(!reader.atEnd())
    {
        if(reader.isStartElement())
            return reader.qualifiedName().toString();

        reader.readNext();
    }

    Q_ASSERT_X(false, Q_FUNC_INFO,
               qPrintable(QString::fromLatin1("The input %1 didn't contain an element.").arg(QString::fromUtf8(document.constData()))));
    return QString();
}

/**
 * @short Loads W3C's XML conformance test suite and runs it on QXmlStreamReader.
 *
 * Since this suite is fairly large, it runs the tests sequentially in order to not
 * have them all loaded into memory at once. In this way, the maximum memory usage stays
 * low, which means one can run valgrind on this test. However, the drawback is that
 * QTestLib's usual error reporting and testing mechanisms are slightly bypassed.
 *
 * Part of this code is a manual, ad-hoc implementation of xml:base.
 *
 * @see <a href="http://www.w3.org/XML/Test/">Extensible
 * Markup Language (XML) Conformance Test Suites</a>
 */
class TestSuiteHandler : public QXmlDefaultHandler
{
public:
    /**
     * The first string is the test ID, the second is
     * a description of what went wrong.
     */
    typedef QPair<QString, QString> GeneralFailure;

    /**
     * The string is the test ID.
     */
    QStringList successes;

    /**
     * The first value is the baseline, while the se
     */
    class MissedBaseline
    {
    public:
        MissedBaseline(const QString &aId,
                       const QByteArray &aExpected,
                       const QByteArray &aOutput) : id(aId),
                                                    expected(aExpected),
                                                    output(aOutput)
        {
            Q_ASSERT(!aId.isEmpty());
        }

        QString     id;
        QByteArray  expected;
        QByteArray  output;
    };

    QList<GeneralFailure> failures;
    QList<MissedBaseline> missedBaselines;

    /**
     * The count of how many tests that were run.
     */
    int runCount;

    int skipCount;

    /**
     * @p baseURI is the the URI of where the catalog file resides.
     */
    TestSuiteHandler(const QUrl &baseURI) : runCount(0),
                                            skipCount(0)
    {
        Q_ASSERT(baseURI.isValid());
        m_baseURI.push(baseURI);
    }

    virtual bool characters(const QString &chars)
    {
        m_ch = chars;
        return true;
    }

    virtual bool startElement(const QString &,
                              const QString &,
                              const QString &,
                              const QXmlAttributes &atts)
    {
        m_atts.push(atts);
        const int i = atts.index(QLatin1String("xml:base"));

        if(i != -1)
            m_baseURI.push(m_baseURI.top().resolved(atts.value(i)));

        return true;
    }

    virtual bool endElement(const QString &,
                            const QString &localName,
                            const QString &)
    {
        if(localName == QLatin1String("TEST"))
        {
            /* We don't want tests for XML 1.1.0, in fact). */
            if(m_atts.top().value(QString(), QLatin1String("VERSION")) == QLatin1String("1.1"))
            {
                ++skipCount;
                m_atts.pop();
                return true;
            }

            /* We don't want tests that conflict with the namespaces spec. Our parser is a
             * namespace-aware parser. */
            else if(m_atts.top().value(QString(), QLatin1String("NAMESPACE")) == QLatin1String("no"))
            {
                ++skipCount;
                m_atts.pop();
                return true;
            }

            const QString inputFilePath(m_baseURI.top().resolved(m_atts.top().value(QString(), QLatin1String("URI")))
                                                                .toLocalFile());
            const QString id(m_atts.top().value(QString(), QLatin1String("ID")));
            const QString type(m_atts.top().value(QString(), QLatin1String("TYPE")));

            QString expectedFilePath;
            const int index = m_atts.top().index(QString(), QLatin1String("OUTPUT"));

            //qDebug() << "Running test case:" << id;

            if(index != -1)
            {
                expectedFilePath = m_baseURI.top().resolved(m_atts.top().value(QString(),
                                                            QLatin1String("OUTPUT"))).toLocalFile();
            }

            /* testcases.dtd: 'No parser should accept a "not-wf" testcase
             * unless it's a nonvalidating parser and the test contains
             * external entities that the parser doesn't read.'
             *
             * We also let this apply to "valid", "invalid" and "error" tests, although
             * I'm not fully sure this is correct. */
            const QString ents(m_atts.top().value(QString(), QLatin1String("ENTITIES")));
            m_atts.pop();

            if(ents == QLatin1String("both") ||
               ents == QLatin1String("general") ||
               ents == QLatin1String("parameter"))
            {
                ++skipCount;
                return true;
            }

            ++runCount;

            QFile inputFile(inputFilePath);
            if(!inputFile.open(QIODevice::ReadOnly))
            {
                failures.append(qMakePair(id, QString::fromLatin1("Failed to open input file %1").arg(inputFilePath)));
                return true;
            }

            if(type == QLatin1String("not-wf"))
            {
                if(isWellformed(&inputFile, ParseSinglePass))
                {
                     failures.append(qMakePair(id, QString::fromLatin1("Failed to flag %1 as not well-formed.")
                                                   .arg(inputFilePath)));

                     /* Exit, the incremental test will fail as well, no need to flood the output. */
                     return true;
                }
                else
                    successes.append(id);

                if(isWellformed(&inputFile, ParseIncrementally))
                {
                     failures.append(qMakePair(id, QString::fromLatin1("Failed to flag %1 as not well-formed with incremental parsing.")
                                                   .arg(inputFilePath)));
                }
                else
                    successes.append(id);

                return true;
            }

            QXmlStreamReader reader(&inputFile);

            /* See testcases.dtd which reads: 'Nonvalidating parsers
             * must also accept "invalid" testcases, but validating ones must reject them.' */
            if(type == QLatin1String("invalid") || type == QLatin1String("valid"))
            {
                QByteArray expected;
                QString docType;

                /* We only want to compare against a baseline when we have
                 * one. Some "invalid"-tests, for instance, doesn't have baselines. */
                if(!expectedFilePath.isEmpty())
                {
                    QFile expectedFile(expectedFilePath);

                    if(!expectedFile.open(QIODevice::ReadOnly))
                    {
                        failures.append(qMakePair(id, QString::fromLatin1("Failed to open baseline %1").arg(expectedFilePath)));
                        return true;
                    }

                    expected = expectedFile.readAll();
                    docType = documentElement(expected);
                }
                else
                    docType = QLatin1String("dummy");

                bool hasError = true;
                bool incremental = false;

                QByteArray input(makeCanonical(inputFilePath, docType, hasError, incremental));

                if (!hasError && !expectedFilePath.isEmpty() && input == expected)
                    input = makeCanonical(inputFilePath, docType, hasError, (incremental = true));

                if(hasError)
                    failures.append(qMakePair(id, QString::fromLatin1("Failed to parse %1%2")
                                              .arg(incremental?"(incremental run only) ":"")
                                              .arg(inputFilePath)));

                if(!expectedFilePath.isEmpty() && input != expected)
                {
                    missedBaselines.append(MissedBaseline(id, expected, input));
                    return true;
                }
                else
                {
                    successes.append(id);
                    return true;
                }
            }
            else if(type == QLatin1String("error"))
            {
                /* Not yet sure about this one. */
                // TODO
                return true;
            }
            else
            {
                Q_ASSERT_X(false, Q_FUNC_INFO, "The input catalog is invalid.");
                return false;
            }
        }
        else if(localName == QLatin1String("TESTCASES") && m_atts.top().index(QLatin1String("xml:base")) != -1)
            m_baseURI.pop();

        m_atts.pop();

        return true;
    }

    enum ParseMode
    {
        ParseIncrementally,
        ParseSinglePass
    };

    static bool isWellformed(QIODevice *const inputFile, const ParseMode mode)
    {
        Q_ASSERT(inputFile);
        Q_ASSERT_X(inputFile->isOpen(), Q_FUNC_INFO, "The caller is responsible for opening the device.");
        Q_ASSERT(mode == ParseIncrementally || mode == ParseSinglePass);

        if(mode == ParseIncrementally)
        {
            QXmlStreamReader reader;
            QByteArray buffer;
            int bufferPos = 0;

            buffer = inputFile->readAll();

            while(true)
            {
                while(!reader.atEnd())
                    reader.readNext();

                if(bufferPos < buffer.size())
                {
                    ++bufferPos;
                    reader.addData(QByteArray(buffer.data() + bufferPos, 1));
                }
                else
                    break;
            }

            return !reader.hasError();
        }
        else
        {
            QXmlStreamReader reader;
            reader.setDevice(inputFile);

            while(!reader.atEnd())
                reader.readNext();

            return !reader.hasError();
        }
    }

private:
    QStack<QXmlAttributes>  m_atts;
    QString                 m_ch;
    QStack<QUrl>            m_baseURI;
};

class tst_QXmlStream: public QObject
{
    Q_OBJECT
public:
    tst_QXmlStream() : m_handler(QUrl::fromLocalFile(QDir::currentPath()  + QLatin1Char('/'))
                                .resolved(QUrl(QLatin1String(catalogFile))))
    {
    }

private slots:
    void initTestCase();
    void reportFailures() const;
    void reportFailures_data();
    void checkBaseline() const;
    void checkBaseline_data() const;
    void testReader() const;
    void testReader_data() const;
    void reportSuccess() const;
    void reportSuccess_data() const;
    void parseXSLTTestSuite() const;
    void addExtraNamespaceDeclarations();
    void setEntityResolver();

private:
    static QByteArray readFile(const QString &filename);

    TestSuiteHandler m_handler;
};

void tst_QXmlStream::initTestCase()
{
    QFile file(QString::fromLatin1(catalogFile));
    QVERIFY2(file.open(QIODevice::ReadOnly),
             qPrintable(QString::fromLatin1("Failed to open the test suite catalog; %1").arg(file.fileName())));

    QXmlInputSource source(&file);
    QXmlSimpleReader reader;
    reader.setContentHandler(&m_handler);

    QVERIFY(reader.parse(&source, false));
}

void tst_QXmlStream::reportFailures() const
{
    QFETCH(bool, isError);
    QFETCH(QString, description);

    QVERIFY2(!isError, qPrintable(description));
}

void tst_QXmlStream::reportFailures_data()
{
    const int len = m_handler.failures.count();

    QTest::addColumn<bool>("isError");
    QTest::addColumn<QString>("description");

    /* We loop over all our failures(if any!), and output them such
     * that they appear in the QTestLib log. */
    for(int i = 0; i < len; ++i)
        QTest::newRow(m_handler.failures.at(i).first.toLatin1().constData()) << true << m_handler.failures.at(i).second;

    /* We need to add at least one column of test data, otherwise QTestLib complains. */
    if(len == 0)
        QTest::newRow("Whole test suite passed") << false << QString();

    /* We compare the test case counts to ensure that we've actually run test cases, that
     * the driver hasn't been broken or changed without updating the expected count, and
     * similar reasons. */
    QCOMPARE(m_handler.runCount, expectedRunCount);
    QCOMPARE(m_handler.skipCount, expectedSkipCount);
}

void tst_QXmlStream::checkBaseline() const
{
    QFETCH(bool, isError);
    QFETCH(QString, expected);
    QFETCH(QString, output);

    if(isError)
        QCOMPARE(output, expected);
}

void tst_QXmlStream::checkBaseline_data() const
{
    QTest::addColumn<bool>("isError");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QString>("output");

    const int len = m_handler.missedBaselines.count();

    for(int i = 0; i < len; ++i)
    {
        const TestSuiteHandler::MissedBaseline &b = m_handler.missedBaselines.at(i);

        /* We indeed don't know what encoding the content is in so in some cases fromUtf8
         * is all wrong, but it's an acceptable guess for error reporting. */
        QTest::newRow(b.id.toLatin1().constData())
                << true
                << QString::fromUtf8(b.expected.constData())
                << QString::fromUtf8(b.output.constData());
    }

    if(len == 0)
        QTest::newRow("dummy") << false << QString() << QString();
}

void tst_QXmlStream::reportSuccess() const
{
    QFETCH(bool, isError);

    QVERIFY(!isError);
}

void tst_QXmlStream::reportSuccess_data() const
{
    QTest::addColumn<bool>("isError");

    const int len = m_handler.successes.count();

    for(int i = 0; i < len; ++i)
        QTest::newRow(m_handler.successes.at(i).toLatin1().constData()) << false;

    if(len == 0)
        QTest::newRow("No test cases succeeded.") << true;
}

QByteArray tst_QXmlStream::readFile(const QString &filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QXmlStreamReader reader;

    reader.setDevice(&file);
    QByteArray outarray;
    QTextStream writer(&outarray);
	// We always want UTF-8, and not what the system picks up.
	writer.setCodec("UTF-8");

    while (!reader.atEnd()) {
        reader.readNext();
        writer << reader.tokenString() << "(";
        if (reader.isWhitespace())
            writer << " whitespace";
        if (reader.isCDATA())
            writer << " CDATA";
        if (reader.isStartDocument() && reader.isStandaloneDocument())
            writer << " standalone";
        if (!reader.text().isEmpty())
            writer << " text=\"" << reader.text().toString() << "\"";
        if (!reader.processingInstructionTarget().isEmpty())
            writer << " processingInstructionTarget=\"" << reader.processingInstructionTarget().toString() << "\"";
        if (!reader.processingInstructionData().isEmpty())
            writer << " processingInstructionData=\"" << reader.processingInstructionData().toString() << "\"";
        if (!reader.dtdName().isEmpty())
            writer << " dtdName=\"" << reader.dtdName().toString() << "\"";
        if (!reader.dtdPublicId().isEmpty())
            writer << " dtdPublicId=\"" << reader.dtdPublicId().toString() << "\"";
        if (!reader.dtdSystemId().isEmpty())
            writer << " dtdSystemId=\"" << reader.dtdSystemId().toString() << "\"";
        if (!reader.documentVersion().isEmpty())
            writer << " documentVersion=\"" << reader.documentVersion().toString() << "\"";
        if (!reader.documentEncoding().isEmpty())
            writer << " documentEncoding=\"" << reader.documentEncoding().toString() << "\"";
        if (!reader.name().isEmpty())
            writer << " name=\"" << reader.name().toString() << "\"";
        if (!reader.namespaceUri().isEmpty())
            writer << " namespaceUri=\"" << reader.namespaceUri().toString() << "\"";
        if (!reader.qualifiedName().isEmpty())
            writer << " qualifiedName=\"" << reader.qualifiedName().toString() << "\"";
        if (!reader.prefix().isEmpty())
            writer << " prefix=\"" << reader.prefix().toString() << "\"";
        if (reader.attributes().size()) {
            foreach(QXmlStreamAttribute attribute, reader.attributes()) {
                writer << endl << "    Attribute(";
                if (!attribute.name().isEmpty())
                    writer << " name=\"" << attribute.name().toString() << "\"";
                if (!attribute.namespaceUri().isEmpty())
                    writer << " namespaceUri=\"" << attribute.namespaceUri().toString() << "\"";
                if (!attribute.qualifiedName().isEmpty())
                    writer << " qualifiedName=\"" << attribute.qualifiedName().toString() << "\"";
                if (!attribute.prefix().isEmpty())
                    writer << " prefix=\"" << attribute.prefix().toString() << "\"";
                if (!attribute.value().isEmpty())
                    writer << " value=\"" << attribute.value().toString() << "\"";
                writer << " )" << endl;
            }
        }
        if (reader.namespaceDeclarations().size()) {
            foreach(QXmlStreamNamespaceDeclaration namespaceDeclaration, reader.namespaceDeclarations()) {
                writer << endl << "    NamespaceDeclaration(";
                if (!namespaceDeclaration.prefix().isEmpty())
                    writer << " prefix=\"" << namespaceDeclaration.prefix().toString() << "\"";
                if (!namespaceDeclaration.namespaceUri().isEmpty())
                    writer << " namespaceUri=\"" << namespaceDeclaration.namespaceUri().toString() << "\"";
                writer << " )" << endl;
            }
        }
        if (reader.notationDeclarations().size()) {
            foreach(QXmlStreamNotationDeclaration notationDeclaration, reader.notationDeclarations()) {
                writer << endl << "    NotationDeclaration(";
                if (!notationDeclaration.name().isEmpty())
                    writer << " name=\"" << notationDeclaration.name().toString() << "\"";
                if (!notationDeclaration.systemId().isEmpty())
                    writer << " systemId=\"" << notationDeclaration.systemId().toString() << "\"";
                if (!notationDeclaration.publicId().isEmpty())
                    writer << " publicId=\"" << notationDeclaration.publicId().toString() << "\"";
                writer << " )" << endl;
            }
        }
        if (reader.entityDeclarations().size()) {
            foreach(QXmlStreamEntityDeclaration entityDeclaration, reader.entityDeclarations()) {
                writer << endl << "    EntityDeclaration(";
                if (!entityDeclaration.name().isEmpty())
                    writer << " name=\"" << entityDeclaration.name().toString() << "\"";
                if (!entityDeclaration.notationName().isEmpty())
                    writer << " notationName=\"" << entityDeclaration.notationName().toString() << "\"";
                if (!entityDeclaration.systemId().isEmpty())
                    writer << " systemId=\"" << entityDeclaration.systemId().toString() << "\"";
                if (!entityDeclaration.publicId().isEmpty())
                    writer << " publicId=\"" << entityDeclaration.publicId().toString() << "\"";
                if (!entityDeclaration.value().isEmpty())
                    writer << " value=\"" << entityDeclaration.value().toString() << "\"";
                writer << " )" << endl;
            }
        }
        writer << " )" << endl;
    }
    if (reader.hasError())
        writer << "ERROR: " << reader.errorString() << endl;
    return outarray;
}

void tst_QXmlStream::testReader() const
{
    QFETCH(QString, xml);
    QFETCH(QString, ref);
    QFile file(ref);
    if (!file.exists()) {
        QByteArray reference = readFile(xml);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(reference);
        file.close();
    } else {
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QString reference = QString::fromUtf8(file.readAll());
        QString qxmlstream = QString::fromUtf8(readFile(xml));
        QCOMPARE(qxmlstream, reference);
    }
}

void tst_QXmlStream::testReader_data() const
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<QString>("ref");
    QDir dir;
    dir.cd("data/");
    foreach(QString filename , dir.entryList(QStringList() << "*.xml")) {
        QString reference =  QFileInfo(filename).baseName() + ".ref";
        QTest::newRow(dir.filePath(filename).toLatin1().data()) << dir.filePath(filename) << dir.filePath(reference);
    }
}

void tst_QXmlStream::parseXSLTTestSuite() const
{
    /* We disable this test for now, so it doesn't show up as an XFAIL. */
#if 0
    QEXPECT_FAIL("", "Two problems needs to be solved in order to enable this test: \n"
                     "* The XSLT suite is 69 MB large, which is quite a lot compared to the existing XML suite on 2 mb.\n"
                     "* We need a c14n-like implementation in order to compare the outputs.", Abort);
    QVERIFY(false);

    /* We don't yet know this. TODO */
    int xsltExpectedRunCount = -1;

    QStringList nameFilters;
    nameFilters.append("*.xsl");
    nameFilters.append("*.xml");

    QDirIterator dirIterator("XSLT-Test-Suite/", nameFilters,
                             QDir::AllEntries, QDirIterator::Subdirectories);

    int filesParsed = 0;

    while(dirIterator.hasNext())
    {
        dirIterator.next();

        const QString fp(dirIterator.filePath());
        qDebug() << "Found" << fp;

        QFile inputFile(fp);
        QVERIFY(inputFile.open(QIODevice::ReadOnly));

        /* Read in and write out to the QByteArray. */
        QByteArray outputArray;
        {
            QXmlStreamReader reader(&inputFile);

            QXmlStreamWriter writer(&outputArray);

            while(!reader.atEnd())
            {
                writer.writeCurrentToken(reader);
                reader.readNext();

                QVERIFY2(!reader.hasError(), qPrintable(reader.errorString()));
            }
            /* Might be we got an error here, but we don't care. */
        }

        /* Read in the two files, and compare them. */
        {
            QBuffer outputBuffer(&outputArray);
            outputBuffer.open(QIODevice::ReadOnly);
            inputFile.close();
            inputFile.open(QIODevice::ReadOnly);

            QString message;
            const bool isEqual = QC14N::isEqual(&inputFile, &outputBuffer, &message);

            QVERIFY2(isEqual, message.toLatin1().constData());

            ++filesParsed;
        }
    }

    QCOMPARE(xsltExpectedRunCount, filesParsed);
#endif
}

void tst_QXmlStream::addExtraNamespaceDeclarations()
{
    const char *data = "<bla><undeclared:foo/><undeclared_too:foo/></bla>";
    {
        QXmlStreamReader xml(data);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY2(xml.hasError(), "namespaces undeclared");
    }
    {
        QXmlStreamReader xml(data);
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("undeclared", "blabla"));
        xml.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration("undeclared_too", "foofoo"));
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY2(!xml.hasError(), xml.errorString().toLatin1().constData());
    }
}


class EntityResolver : public QXmlStreamEntityResolver {
public:
    QString resolveUndeclaredEntity(const QString &name) {
        static int count = 0;
        return name.toUpper() + QString::number(++count);
    }
};
void tst_QXmlStream::setEntityResolver()
{
    const char *data = "<bla foo=\"&undeclared;\">&undeclared_too;</bla>";
    {
        QXmlStreamReader xml(data);
        while (!xml.atEnd()) {
            xml.readNext();
        }
        QVERIFY2(xml.hasError(), "undeclared entities");
    }
    {
        QString foo;
        QString bla_text;
        QXmlStreamReader xml(data);
        EntityResolver resolver;
        xml.setEntityResolver(&resolver);
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement())
                foo = xml.attributes().value("foo").toString();
            if (xml.isCharacters())
                bla_text += xml.text().toString();
        }
        QVERIFY2(!xml.hasError(), xml.errorString().toLatin1().constData());
        QCOMPARE(foo, QLatin1String("UNDECLARED1"));
        QCOMPARE(bla_text, QLatin1String("UNDECLARED_TOO2"));
    }
}

/*
  In addition to QTestLib's flags, one can specify "-c <filename>" and have that file output in its canonical form.
*/
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc == 3 && QByteArray(argv[1]).startsWith("-c")) {
        // output canonical only
        bool error = false;
        QByteArray canonical = makeCanonical(argv[2], "doc", error);
        QTextStream myStdOut(stdout);
        myStdOut << canonical << endl;
        exit(0);
    }

    tst_QXmlStream tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "tst_qxmlstream.moc"
// vim: et:ts=4:sw=4:sts=4
