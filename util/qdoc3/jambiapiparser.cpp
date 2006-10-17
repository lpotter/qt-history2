/*
    jambiapiparser.cpp

    TODO:
        * deal with enum_1
*/

#include <QtXml>

#include "cppcodeparser.h"
#include "jambiapiparser.h"
#include "node.h"
#include "tree.h"

static void setPass1JambifiedDoc(Node *javaNode, const Node *cppNode)
{
    Doc newDoc(cppNode->doc());

    if (javaNode->type() == Node::Function) {
        if (cppNode->type() == Node::Function) {
            const FunctionNode *cppFunc = static_cast<const FunctionNode *>(cppNode);
            QStringList javaParams = static_cast<const FunctionNode *>(javaNode)->parameterNames();
            QStringList cppParams = cppFunc->parameterNames();
            newDoc.renameParameters(cppParams, javaParams);

            if (cppNode->access() == Node::Private && cppFunc->reimplementedFrom()) {
                Text text;
                text << Atom::ParaLeft << "This function is reimplemented for internal reasons."
                     << Atom::ParaRight;
                newDoc.setBody(text);
            }
        }
    } else {    // ### enum value names?
        
    }

    javaNode->setDoc(newDoc);
}

static void setStatus(Node *javaNode, const Node *cppNode)
{
    if (cppNode->status() == Node::Compat) {
        javaNode->setStatus(Node::Obsolete);
    } else {
        javaNode->setStatus(cppNode->status());
    }
}

static Text findEnumText(Node *javaEnum, const QString &enumItemName)
{
    const Text &body = javaEnum->doc().body();
    const Atom *atom = body.firstAtom();
    while (atom) {
        if (atom->type() == Atom::ListTagLeft && atom->string() == ATOM_LIST_VALUE) {
            atom = atom->next();
            if (atom) {
                // ### paras?
                if (atom->string() == enumItemName)
                    return body.subText(Atom::ListItemLeft, Atom::ListItemRight, atom);
            }
        } else {
            atom = atom->next();
        }
    }
    return Text();
}

JambiApiParser::JambiApiParser(Tree *cppTree)
    : cppTre(cppTree), javaTre(0), metJapiTag(false)
{
}

JambiApiParser::~JambiApiParser()
{
}

void JambiApiParser::initializeParser(const Config &config)
{
    CodeParser::initializeParser(config);
}

void JambiApiParser::terminateParser()
{
    CodeParser::terminateParser();
}

QString JambiApiParser::language()
{
    return "Java";
}

QString JambiApiParser::sourceFileNameFilter()
{
    return "*.japi";
}

void JambiApiParser::parseSourceFile(const Location &location, const QString &filePath, Tree *tree)
{
    javaTre = tree;
    metJapiTag = false;

    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        location.warning(tr("Cannot open JAPI file '%1'").arg(filePath));
        return;
    }

    japiLocation = Location(filePath);
    QXmlInputSource xmlSource(&file);
    reader.parse(xmlSource);
}

void JambiApiParser::doneParsingSourceFiles(Tree * /* tree */)
{
    /*
        Also import the overview documents.
    */
    foreach (Node *cppNode, cppTre->root()->childNodes()) {
        if (cppNode->type() == Node::Fake) {
            FakeNode *cppFake = static_cast<FakeNode *>(cppNode);
            if (cppFake->subType() == FakeNode::Page) {
                FakeNode *javaFake = new FakeNode(javaTre->root(), cppFake->name(),
                                                  cppFake->subType());
                javaFake->setTitle(cppFake->title());
                javaFake->setSubTitle(cppFake->subTitle());
                setStatus(javaFake, cppFake);
                setPass1JambifiedDoc(javaFake, cppFake);
            }
        }
    }

    /*
        Fix the docs.
    */
    if (javaTre) {
        jambifyDocsPass2(javaTre->root());
        javaTre = 0;
    }
}

bool JambiApiParser::startElement(const QString & /* namespaceURI */,
                                  const QString & /* localName */,
                                  const QString &qName,
                                  const QXmlAttributes &attributes)
{
    if (!metJapiTag && qName != "japi") {
        // ### The file is not a JAPI file.
        return true;
    }
    metJapiTag = true;

    EnumNode *javaEnum = 0;
    EnumNode *cppEnum = 0;
    InnerNode *javaParent = javaTre->root();
    InnerNode *cppParent = cppTre->root();

    for (int i = 0; i < classAndEnumStack.count(); ++i) {
        const ClassOrEnumInfo &info = classAndEnumStack.at(i);
        if (info.cppNode) {
            if (info.cppNode->type() == Node::Enum) {
                Q_ASSERT(info.javaNode->type() == Node::Enum);
                javaEnum = static_cast<EnumNode *>(info.javaNode);
                cppEnum = static_cast<EnumNode *>(info.cppNode);
            } else {
                Q_ASSERT(info.javaNode->type() == Node::Class
                         || info.javaNode->type() == Node::Namespace);
                javaParent = static_cast<InnerNode *>(info.javaNode);
                cppParent = static_cast<InnerNode *>(info.cppNode);
            }
        }
    }

    if (qName == "class" || qName == "enum") {
        Node::Type type = (qName == "class") ? Node::Class : Node::Enum;

        ClassOrEnumInfo info;
        info.tag = qName;
        info.javaName = attributes.value("java");
        info.javaImplements = attributes.value("javaimplements");
        info.cppName = attributes.value("cpp");
        info.cppNode = cppTre->findNode(info.cppName.split("::"), type, cppParent);
        if (!info.cppNode && type == Node::Class) {
            type = Node::Namespace;
            info.cppNode = cppTre->findNode(info.cppName.split("::"), type, cppParent);
        }

        if (!info.cppNode) {
            japiLocation.warning(tr("Cannot find C++ class or enum '%1'").arg(info.cppName));
        } else {
            if (qName == "class") {
                info.javaNode = new ClassNode(javaParent, info.javaName); // ###
            } else {
                info.javaNode = new EnumNode(javaParent, info.javaName);
            }
            info.javaNode->setLocation(japiLocation);
            setStatus(info.javaNode, info.cppNode);

            setPass1JambifiedDoc(info.javaNode, info.cppNode);
        }
        classAndEnumStack.push(info);
    } else if (qName == "method" || qName == "signal") {
        QString javaSignature = attributes.value("java");
        if (javaSignature.startsWith("private"))
            return true;

        QString cppSignature = attributes.value("cpp");

        CppCodeParser cppParser;
        const FunctionNode *cppNode = cppParser.findFunctionNode(cppSignature, cppTre,
                                                                 cppParent,
                                                                 true /* fuzzy */);
        if (!cppNode) {
            bool quiet = false;

            /*
                Default constructors sometimes don't exist in C++.
            */
            if (!quiet && javaSignature == "public " + javaParent->name() + "()")
                quiet = true;

            if (!quiet)
                japiLocation.warning(tr("Cannot find C++ function '%1' ('%2')")
                                     .arg(cppSignature).arg(cppParent->name()));
        }

        FunctionNode *javaNode;
        if (makeFunctionNode(javaParent, javaSignature, &javaNode)) {
            javaNode->setLocation(japiLocation);
            if (qName == "signal")
                javaNode->setMetaness(FunctionNode::Signal);

            if (cppNode) {
                setStatus(javaNode, cppNode);

                int overloadNo = cppNode->parameters().count() - javaNode->parameters().count() + 1;
                if (overloadNo == 1) {
                    setPass1JambifiedDoc(javaNode, cppNode);
                } else {
                    Text text;

                    text << Atom::ParaLeft << "Equivalent to "
                         << Atom(Atom::Link, javaNode->name() + "()")
                         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                         << javaNode->name()
                         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                         << "(";

                    for (int i = 0; i < cppNode->parameters().count(); ++i) {
                        if (i > 0)
                            text << ", ";
                        if (i < javaNode->parameters().count()) {
                            text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_PARAMETER)
                                 << javaNode->parameters().at(i).name()
                                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_PARAMETER);
                        } else {
                            // ### convert to Java
                            text << cppNode->parameters().at(i).defaultValue();
                        }
                    }

                    text << ").";

                    Doc doc;
                    doc.setBody(text);
                    javaNode->setDoc(doc);
                }
                javaNode->setOverload(overloadNo > 1);
            }
        }
    } else if (qName == "variablesetter" || qName == "variablegetter") {
        QString javaSignature = attributes.value("java");
        if (javaSignature.startsWith("private"))
            return true;

        QString cppVariable = attributes.value("cpp");

        VariableNode *cppNode = static_cast<VariableNode *>(cppParent->findNode(cppVariable,
                                                                                Node::Variable));
        FunctionNode *javaNode;
        if (makeFunctionNode(javaParent, javaSignature, &javaNode)) {
            javaNode->setLocation(japiLocation);

            if (!cppNode) {
#if 0
                japiLocation.warning(tr("Cannot find C++ variable '%1' ('%2')")
                                     .arg(cppVariable).arg(cppParent->name()));
#endif
                javaNode->setDoc(Doc(japiLocation,
                                     "This method is used internally by Qt Jambi.\n"
                                     "Do not use it in your applications.",
                                     QSet<QString>()));
            } else {
                setPass1JambifiedDoc(javaNode, cppNode);
                setStatus(javaNode, cppNode);
            }
        }
    } else if (qName == "enum-value") {
        QString javaName = attributes.value("java");
        QString cppName = attributes.value("cpp");
        QString value = attributes.value("value");

        if (javaEnum) {
            EnumItem item(javaName, value, findEnumText(javaEnum, javaName));
            javaEnum->addItem(item);
        }
    }

    return true;
}

bool JambiApiParser::endElement(const QString & /* namespaceURI */,
                                const QString & /* localName */,
                                const QString &qName)
{
    if (qName == "class" || qName == "enum")
        classAndEnumStack.pop();
    return true;
}

bool JambiApiParser::fatalError(const QXmlParseException &exception)
{
    japiLocation.setLineNo(exception.lineNumber());
    japiLocation.setColumnNo(exception.columnNumber());
    japiLocation.warning(tr("Syntax error in JAPI file (%1)").arg(exception.message()));
    return true;
}

void JambiApiParser::jambifyDocsPass2(Node *node)
{
    const Doc &doc = node->doc();
    if (!doc.isEmpty()) {
        if (node->type() == Node::Enum) {
            Doc newDoc(doc);
            newDoc.simplifyEnumDoc();
            node->setDoc(newDoc);
        }
    }

    if (node->isInnerNode()) {
        InnerNode *innerNode = static_cast<InnerNode *>(node);
        foreach (Node *child, innerNode->childNodes())
            jambifyDocsPass2(child);
    }
}

bool JambiApiParser::makeFunctionNode(InnerNode *parent, const QString &synopsis,
				      FunctionNode **funcPtr)
{
    Node::Access access = Node::Public;
    FunctionNode::Metaness metaness = FunctionNode::Plain;
    bool final = false;
    bool statique = false;

    QString mySynopsis = synopsis.simplified();
    int oldLen;
    do {
        oldLen = mySynopsis.length();

        if (mySynopsis.startsWith("public ")) {
            mySynopsis.remove(0, 7);
            access = Node::Public;
        }
        if (mySynopsis.startsWith("protected ")) {
            mySynopsis.remove(0, 10);
            access = Node::Protected;
        }
        if (mySynopsis.startsWith("private ")) {
            mySynopsis.remove(0, 8);
            access = Node::Private;
        }
        if (mySynopsis.startsWith("native ")) {
            mySynopsis.remove(0, 7);
            metaness = FunctionNode::Native;
        }
        if (mySynopsis.startsWith("final ")) {
            mySynopsis.remove(0, 6);
            final = true;
        }
        if (mySynopsis.startsWith("static ")) {
            mySynopsis.remove(0, 7);
            statique = true;
        }
    } while (oldLen != mySynopsis.length());

    // method or constructor
    QRegExp funcRegExp("(?:(.*) )?([A-Za-z_0-9]+)\\((.*)\\)");
    if (!funcRegExp.exactMatch(mySynopsis))
        return false;

    QString retType = funcRegExp.cap(1);
    QString funcName = funcRegExp.cap(2);
    QStringList params = funcRegExp.cap(3).split(",");

    FunctionNode *func = new FunctionNode(parent, funcName);
    func->setReturnType(retType);
    func->setAccess(access);
    func->setStatic(statique);
    func->setConst(final);
    func->setMetaness(metaness);

    QRegExp paramRegExp(" ?([^ ].*) ([A-Za-z_0-9]+) ?");

    foreach (QString param, params) {
        if (paramRegExp.exactMatch(param)) {
            func->addParameter(Parameter(paramRegExp.cap(1), "", paramRegExp.cap(2)));
        } else {
            // problem
        }
    }

    if (funcPtr) {
        *funcPtr = func;
    } else if (!parent) {
        delete func;
    }
    return true;
}
