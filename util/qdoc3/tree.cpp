/*
  tree.cpp
*/

#include <QtCore>
#include <QDomDocument>

#include "atom.h"
#include "htmlgenerator.h"
#include "location.h"
#include "node.h"
#include "text.h"
#include "tree.h"

struct InheritanceBound
{
    Node::Access access;
    QStringList basePath;
    QString dataTypeWithTemplateArgs;

    InheritanceBound()
	: access(Node::Public) { }
    InheritanceBound( Node::Access access0, const QStringList& basePath0,
		      const QString &dataTypeWithTemplateArgs0)
	: access(access0), basePath(basePath0),
	  dataTypeWithTemplateArgs(dataTypeWithTemplateArgs0) { }
};

struct Target
{
    Node *node;
    Atom *atom;
    int priority;
};

typedef QMap<PropertyNode::FunctionRole, QString> RoleMap;
typedef QMap<PropertyNode *, RoleMap> PropertyMap;
typedef QMultiMap<QString, Node *> GroupMap;
typedef QMultiHash<QString, FakeNode *> FakeNodeHash;
typedef QMultiHash<QString, Target> TargetHash;

class TreePrivate
{
public:
    QMap<ClassNode *, QList<InheritanceBound> > unresolvedInheritanceMap;
    PropertyMap unresolvedPropertyMap;
    GroupMap groupMap;
    FakeNodeHash fakeNodesByTitle;
    TargetHash targetHash;
    QList<QPair<ClassNode*,QString> > basesList;
    QList<QPair<FunctionNode*,QString> > relatedList;
};

Tree::Tree()
    : roo( 0, "" )
{
    priv = new TreePrivate;
}

Tree::~Tree()
{
    delete priv;
}

Node *Tree::findNode(const QStringList &path, Node *relative, int findFlags)
{
    return const_cast<Node *>(const_cast<const Tree *>(this)->findNode(path, relative, findFlags));
}

const Node *Tree::findNode(const QStringList &path, const Node *relative, int findFlags) const
{
    if (!relative)
        relative = root();

    do {
        const Node *node = relative;
        int i;

        for (i = 0; i < path.size(); ++i) {
	    if (node == 0 || !node->isInnerNode())
	        break;

            const Node *next = static_cast<const InnerNode *>(node)->findNode(path.at(i));
            if (!next && (findFlags & SearchEnumValues) && i == path.size() - 1)
                next = static_cast<const InnerNode *>(node)->findEnumNodeForValue(path.at(i));

            if (!next && node->type() == Node::Class && (findFlags & SearchBaseClasses)) {
                NodeList baseClasses = allBaseClasses(static_cast<const ClassNode *>(node));
                foreach (const Node *baseClass, baseClasses) {
                    next = static_cast<const InnerNode *>(baseClass)->findNode(path.at(i));
                    if (!next && (findFlags & SearchEnumValues) && i == path.size() - 1)
                        next = static_cast<const InnerNode *>(baseClass)
                                        ->findEnumNodeForValue(path.at(i));
                    if (next)
                        break;
                }
            }
            node = next;
        }
        if (node && i == path.size()
                && (!(findFlags & NonFunction) || node->type() != Node::Function
                    || ((FunctionNode *)node)->metaness() == FunctionNode::MacroWithoutParams))
            return node;
        relative = relative->parent();
    } while (relative);

    return 0;
}

Node *Tree::findNode(const QStringList &path, Node::Type type, Node *relative, int findFlags)
{
    return const_cast<Node *>(const_cast<const Tree *>(this)->findNode(path, type, relative,
                                                                       findFlags));
}

const Node *Tree::findNode(const QStringList &path, Node::Type type, const Node *relative,
                           int findFlags) const
{
    const Node *node = findNode(path, relative, findFlags);
    if (node != 0 && node->type() == type)
	return node;
    return 0;
}

FunctionNode *Tree::findFunctionNode(const QStringList& path, Node *relative, int findFlags)
{
    return const_cast<FunctionNode *>(
                const_cast<const Tree *>(this)->findFunctionNode(path, relative, findFlags));
}

const FunctionNode *Tree::findFunctionNode(const QStringList &path, const Node *relative,
                                           int findFlags) const
{
    if (!relative)
        relative = root();
    do {
        const Node *node = relative;
        int i;

        for (i = 0; i < path.size(); ++i) {
	    if (node == 0 || !node->isInnerNode())
	        break;

            const Node *next;
            if (i == path.size() - 1)
                next = ((InnerNode *) node)->findFunctionNode(path.at(i));
            else
                next = ((InnerNode *) node)->findNode(path.at(i));

            if (!next && node->type() == Node::Class && (findFlags & SearchBaseClasses)) {
                NodeList baseClasses = allBaseClasses(static_cast<const ClassNode *>(node));
                foreach (const Node *baseClass, baseClasses) {
                    if (i == path.size() - 1)
                        next = static_cast<const InnerNode *>(baseClass)->
                                findFunctionNode(path.at(i));
                    else
                        next = static_cast<const InnerNode *>(baseClass)->findNode(path.at(i));

                    if (next)
                        break;
                }
            }

            node = next;
        }
        if (node && i == path.size() && node->type() == Node::Function) {
            // CppCodeParser::processOtherMetaCommand ensures that reimplemented
            // functions are private.
            const FunctionNode *func = static_cast<const FunctionNode*>(node);

            while (func->access() == Node::Private) {
                const FunctionNode *from = func->reimplementedFrom();
                if (from != 0) {
                    if (from->access() != Node::Private)
                        return from;
                    else
                        func = from;
                } else
                    break;
            }
            return func;
        }
        relative = relative->parent();
    } while (relative);

    return 0;
}

FunctionNode *Tree::findFunctionNode(const QStringList &parentPath, const FunctionNode *clone,
                                     Node *relative, int findFlags)
{
    return const_cast<FunctionNode *>(
		const_cast<const Tree *>(this)->findFunctionNode(parentPath, clone,
                                      				 relative, findFlags));
}

const FunctionNode *Tree::findFunctionNode(const QStringList &parentPath, const FunctionNode *clone,
                                           const Node *relative, int findFlags) const
{
    const Node *parent = findNode(parentPath, relative, findFlags);
    if ( parent == 0 || !parent->isInnerNode() ) {
	return 0;
    } else {
	return ((InnerNode *)parent)->findFunctionNode(clone);
    }
}

static const int NumSuffixes = 3;
static const char * const suffixes[NumSuffixes] = { "", "s", "es" };

const FakeNode *Tree::findFakeNodeByTitle(const QString &title) const
{
    for (int pass = 0; pass < NumSuffixes; ++pass) {
        FakeNodeHash::const_iterator i =
                priv->fakeNodesByTitle.find(Doc::canonicalTitle(title + suffixes[pass]));
        if (i != priv->fakeNodesByTitle.constEnd()) {
            FakeNodeHash::const_iterator j = i;
            ++j;
            if (j == priv->fakeNodesByTitle.constEnd() || j.key() != i.key())
                return i.value();
        }
    }
    return 0;
}

const Node *Tree::findUnambiguousTarget(const QString &target, Atom *&atom) const
{
    Target bestTarget = {0, 0, INT_MAX};
    int numBestTargets = 0;

    for (int pass = 0; pass < NumSuffixes; ++pass) {
        TargetHash::const_iterator i =
                priv->targetHash.find(Doc::canonicalTitle(target + suffixes[pass]));
        if (i != priv->targetHash.constEnd()) {
            TargetHash::const_iterator j = i;
            do {
                const Target &candidate = j.value();
                if (candidate.priority < bestTarget.priority) {
                    bestTarget = candidate;
                    numBestTargets = 1;
                } else if (candidate.priority == bestTarget.priority) {
                    ++numBestTargets;
                }
                ++j;
            } while (j != priv->targetHash.constEnd() && j.key() == i.key());

            if (numBestTargets == 1) {
                atom = bestTarget.atom;
                return bestTarget.node;
            }
        }
    }
    return 0;
}

Atom *Tree::findTarget(const QString &target, const Node *node) const
{
    for (int pass = 0; pass < NumSuffixes; ++pass) {
        QString key = Doc::canonicalTitle(target + suffixes[pass]);
        TargetHash::const_iterator i = priv->targetHash.find(key);

        if (i != priv->targetHash.constEnd()) {
            do {
                if (i.value().node == node)
                    return i.value().atom;
                ++i;
            } while (i != priv->targetHash.constEnd() && i.key() == key);
        }
    }
    return 0;
}

void Tree::addBaseClass( ClassNode *subclass, Node::Access access,
			 const QStringList &basePath,
			 const QString &dataTypeWithTemplateArgs )
{
    priv->unresolvedInheritanceMap[subclass].append(
	    InheritanceBound(access, basePath, dataTypeWithTemplateArgs));
}


void Tree::addPropertyFunction(PropertyNode *property, const QString &funcName,
			       PropertyNode::FunctionRole funcRole)
{
    priv->unresolvedPropertyMap[property].insert(funcRole, funcName);
}

void Tree::addToGroup(Node *node, const QString &group)
{
    priv->groupMap.insert(group, node);
}

void Tree::resolveInheritance()
{
    for ( int pass = 0; pass < 2; pass++ ) {
	NodeList::ConstIterator c = root()->childNodes().begin();
	while ( c != root()->childNodes().end() ) {
	    if ( (*c)->type() == Node::Class )
		resolveInheritance( pass, (ClassNode *) *c );
	    ++c;
	}
	priv->unresolvedInheritanceMap.clear();
    }
}

void Tree::resolveProperties()
{
    PropertyMap::ConstIterator propEntry;
    
    propEntry = priv->unresolvedPropertyMap.begin();
    while (propEntry != priv->unresolvedPropertyMap.end()) {
	PropertyNode *property = propEntry.key();
        InnerNode *parent = property->parent();
	QString getterName = (*propEntry)[PropertyNode::Getter];
	QString setterName = (*propEntry)[PropertyNode::Setter];
	QString resetterName = (*propEntry)[PropertyNode::Resetter];

	NodeList::ConstIterator c = parent->childNodes().begin();
        while (c != parent->childNodes().end()) {
	    if ((*c)->type() == Node::Function) {
		FunctionNode *function = static_cast<FunctionNode *>(*c);
                if (function->status() == property->status()
                        && function->access() == property->access()) {
		    if (function->name() == getterName) {
	                property->addFunction(function, PropertyNode::Getter);
	            } else if (function->name() == setterName) {
	                property->addFunction(function, PropertyNode::Setter);
	            } else if (function->name() == resetterName) {
	                property->addFunction(function, PropertyNode::Resetter);
                    }
                }
	    }
	    ++c;
        }
	++propEntry;
    }

    propEntry = priv->unresolvedPropertyMap.begin();
    while (propEntry != priv->unresolvedPropertyMap.end()) {
	PropertyNode *property = propEntry.key();
        // redo it to set the property functions
        if (property->overriddenFrom())
            property->setOverriddenFrom(property->overriddenFrom());
	++propEntry;
    }

    priv->unresolvedPropertyMap.clear();
}

void Tree::resolveInheritance(int pass, ClassNode *classe)
{
    if ( pass == 0 ) {
	QList<InheritanceBound> bounds = priv->unresolvedInheritanceMap[classe];
	QList<InheritanceBound>::ConstIterator b = bounds.begin();
	while ( b != bounds.end() ) {
	    ClassNode *baseClass = (ClassNode *)findNode((*b).basePath, Node::Class);
	    if (baseClass)
		classe->addBaseClass((*b).access, baseClass, (*b).dataTypeWithTemplateArgs);
	    ++b;
	}
    } else {
	NodeList::ConstIterator c = classe->childNodes().begin();
	while ( c != classe->childNodes().end() ) {
	    if ( (*c)->type() == Node::Function ) {
		FunctionNode *func = (FunctionNode *) *c;
		FunctionNode *from = findVirtualFunctionInBaseClasses( classe, func );
		if ( from != 0 ) {
		    if ( func->virtualness() == FunctionNode::NonVirtual )
			func->setVirtualness( FunctionNode::ImpureVirtual );
		    func->setReimplementedFrom( from );
		}
	    } else if ((*c)->type() == Node::Property) {
                fixPropertyUsingBaseClasses(classe, static_cast<PropertyNode *>(*c));
            }
	    ++c;
	}
    }
}

void Tree::resolveGroups()
{
    GroupMap::const_iterator i;
    QString prevGroup;
    for (i = priv->groupMap.constBegin(); i != priv->groupMap.constEnd(); ++i) {
        if (i.value()->access() == Node::Private)
            continue;

        FakeNode *fake = static_cast<FakeNode *>(findNode(QStringList(i.key()), Node::Fake));
        if (fake && fake->subType() == FakeNode::Group) {
            fake->addGroupMember(i.value());
        } else {
            if (prevGroup != i.key())
                i.value()->doc().location().warning(tr("No such group '%1'").arg(i.key()));
        }

        prevGroup = i.key();
    }

    priv->groupMap.clear();
}

void Tree::resolveTargets()
{
    // need recursion

    foreach (Node *child, roo.childNodes()) {
        if (child->type() == Node::Fake) {
            FakeNode *node = static_cast<FakeNode *>(child);
            priv->fakeNodesByTitle.insert(Doc::canonicalTitle(node->title()), node);
        }

        if (child->doc().hasTableOfContents()) {
            const QList<Atom *> &toc = child->doc().tableOfContents();
            Target target;
            target.node = child;
            target.priority = 3;

            for (int i = 0; i < toc.size(); ++i) {
                target.atom = toc.at(i);
                QString title = Text::sectionHeading(target.atom).toString();
                if (!title.isEmpty())
                    priv->targetHash.insert(Doc::canonicalTitle(title), target);
            }
        }
        if (child->doc().hasKeywords()) {
            const QList<Atom *> &keywords = child->doc().keywords();
            Target target;
            target.node = child;
            target.priority = 1;

            for (int i = 0; i < keywords.size(); ++i) {
                target.atom = keywords.at(i);
                priv->targetHash.insert(Doc::canonicalTitle(target.atom->string()), target);
            }
        }
        if (child->doc().hasTargets()) {
            const QList<Atom *> &toc = child->doc().targets();
            Target target;
            target.node = child;
            target.priority = 2;

            for (int i = 0; i < toc.size(); ++i) {
                target.atom = toc.at(i);
                priv->targetHash.insert(Doc::canonicalTitle(target.atom->string()), target);
            }
        }
    }
}

void Tree::fixInheritance()
{
    NodeList::ConstIterator c = root()->childNodes().begin();
    while ( c != root()->childNodes().end() ) {
	if ( (*c)->type() == Node::Class )
	    static_cast<ClassNode *>(*c)->fixBaseClasses();
	++c;
    }
}

FunctionNode *Tree::findVirtualFunctionInBaseClasses(ClassNode *classe, FunctionNode *clone)
{
    QList<RelatedClass>::ConstIterator r = classe->baseClasses().begin();
    while ( r != classe->baseClasses().end() ) {
	FunctionNode *func;
        if ( ((func = findVirtualFunctionInBaseClasses((*r).node, clone)) != 0 ||
	      (func = (*r).node->findFunctionNode(clone)) != 0) ) {
	    if (func->virtualness() != FunctionNode::NonVirtual)
	        return func;
        }
 	++r;
    }
    return 0;
}

void Tree::fixPropertyUsingBaseClasses(ClassNode *classe, PropertyNode *property)
{
    QList<RelatedClass>::const_iterator r = classe->baseClasses().begin();
    while (r != classe->baseClasses().end()) {
	PropertyNode *baseProperty = static_cast<PropertyNode *>(r->node->findNode(property->name(), Node::Property));
        if (baseProperty) {
            fixPropertyUsingBaseClasses(r->node, baseProperty);
            property->setOverriddenFrom(baseProperty);
        } else {
            fixPropertyUsingBaseClasses(r->node, property);
        }
 	++r;
    }
}

NodeList Tree::allBaseClasses(const ClassNode *classe) const
{
    NodeList result;
    foreach (RelatedClass r, classe->baseClasses()) {
        result += r.node;
        result += allBaseClasses(r.node);
    }
    return result;
}

void Tree::readIndexes(const QStringList &indexFiles)
{
    foreach (QString indexFile, indexFiles)
        readIndexFile(indexFile);
}

void Tree::readIndexFile(const QString &path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        QDomDocument document;
        document.setContent(&file);
        file.close();

        QDomElement indexElement = document.documentElement();
        QString indexUrl = indexElement.attribute("url");
        priv->basesList.clear();
        priv->relatedList.clear();

        // Scan all elements in the XML file, constructing a map that contains
        // base classes for each class found.

        QDomElement child = indexElement.firstChildElement();
        while (!child.isNull()) {
            readIndexSection(child, root(), indexUrl);
            child = child.nextSiblingElement();
        }

        // Now that all the base classes have been found for this index,
        // arrange them into an inheritance hierarchy.

        resolveIndex();
    }
}

void Tree::readIndexSection(const QDomElement &element,
    InnerNode *parent, const QString &indexUrl)
{
    QString name = element.attribute("name");

    Node *section;
    if (element.nodeName() == "namespace") {
        section = new NamespaceNode(parent, name);
    } else if (element.nodeName() == "class") {
        section = new ClassNode(parent, name);
        priv->basesList.append(QPair<ClassNode*,QString>(
            static_cast<ClassNode*>(section), element.attribute("bases")));

    } else if (element.nodeName() == "page") {
        FakeNode::SubType subtype;
        if (element.attribute("subtype") == "example")
            subtype = FakeNode::Example;
        else if (element.attribute("subtype") == "header")
            subtype = FakeNode::HeaderFile;
        else if (element.attribute("subtype") == "file")
            subtype = FakeNode::File;
        else if (element.attribute("subtype") == "group")
            subtype = FakeNode::Group;
        else if (element.attribute("subtype") == "module")
            subtype = FakeNode::Module;
        else if (element.attribute("subtype") == "page")
            subtype = FakeNode::Page;
        else if (element.attribute("subtype") == "externalpage")
            subtype = FakeNode::ExternalPage;
        else
            return;

        FakeNode *fakeNode = new FakeNode(parent, name, subtype);
        fakeNode->setTitle(element.attribute("title"));
        if (element.hasAttribute("location"))
            fakeNode->setLocation(Location(element.attribute("location")));

        section = fakeNode;

    } else if (element.nodeName() == "enum") {
        section = new EnumNode(parent, name);

    } else if (element.nodeName() == "typedef") {
        section = new TypedefNode(parent, name);

    } else if (element.nodeName() == "property") {
        section = new PropertyNode(parent, name);

    } else if (element.nodeName() == "function") {
        FunctionNode::Virtualness virt;
        if (element.attribute("virtual") == "non")
            virt = FunctionNode::NonVirtual;
        else if (element.attribute("virtual") == "impure")
            virt = FunctionNode::ImpureVirtual;
        else if (element.attribute("virtual") == "pure")
            virt = FunctionNode::PureVirtual;
        else
            return;

        FunctionNode::Metaness meta;
        if (element.attribute("meta") == "plain")
            meta = FunctionNode::Plain;
        else if (element.attribute("meta") == "signal")
            meta = FunctionNode::Signal;
        else if (element.attribute("meta") == "slot")
            meta = FunctionNode::Slot;
        else if (element.attribute("meta") == "constructor")
            meta = FunctionNode::Ctor;
        else if (element.attribute("meta") == "destructor")
            meta = FunctionNode::Dtor;
        else if (element.attribute("meta") == "macro")
            meta = FunctionNode::MacroWithParams;
        else if (element.attribute("meta") == "macrowithparams")
            meta = FunctionNode::MacroWithParams;
        else if (element.attribute("meta") == "macrowithoutparams")
            meta = FunctionNode::MacroWithoutParams;
        else
            return;

        FunctionNode *functionNode = new FunctionNode(parent, name);
        functionNode->setReturnType(element.attribute("return"));
        functionNode->setVirtualness(virt);
        functionNode->setMetaness(meta);
        functionNode->setConst(element.attribute("const") == "true");
        functionNode->setStatic(element.attribute("static") == "true");
        functionNode->setOverload(element.attribute("overload") == "true");

        if (element.hasAttribute("relates")
            && element.attribute("relates") != parent->name()) {
            priv->relatedList.append(
                QPair<FunctionNode*,QString>(functionNode,
                                             element.attribute("relates")));
        }

        QDomElement child = element.firstChildElement("parameter");
        while (!child.isNull()) {
            Parameter parameter(child.attribute("left"),
                                child.attribute("right"),
                                child.attribute("name"),
                                child.attribute("default"));
            functionNode->addParameter(parameter);
            child = child.nextSiblingElement("parameter");
        }

        section = functionNode;

    } else if (element.nodeName() == "variable") {
        section = new VariableNode(parent, name);

    } else if (element.nodeName() == "keyword") {
        Target target;
        target.node = parent;
        target.priority = 1;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    } else if (element.nodeName() == "target") {
        Target target;
        target.node = parent;
        target.priority = 2;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    } else if (element.nodeName() == "contents") {
        Target target;
        target.node = parent;
        target.priority = 3;
        target.atom = new Atom(Atom::Target, name);
        priv->targetHash.insert(name, target);
        return;

    } else
        return;

    QString access = element.attribute("access");
    if (access == "public")
        section->setAccess(Node::Public);
    else if (access == "protected")
        section->setAccess(Node::Protected);
    else if (access == "private")
        section->setAccess(Node::Private);
    else
        section->setAccess(Node::Public);

    section->setUrl(indexUrl);

    // Create some content for the node.
    QSet<QString> emptySet;
    Location location(indexUrl + "/" + name.toLower() + ".html");
    Doc doc(location, " ", emptySet); // placeholder
    section->setDoc(doc);

    InnerNode *inner = dynamic_cast<InnerNode*>(section);
    if (inner) {
        QDomElement child = element.firstChildElement();

        while (!child.isNull()) {
            if (element.nodeName() == "class")
                readIndexSection(child, inner, indexUrl);
            else if (element.nodeName() == "page")
                readIndexSection(child, inner, indexUrl);
            else if (element.nodeName() == "namespace" && !name.isEmpty())
                // The root node in the index is a namespace with an empty name.
                readIndexSection(child, inner, indexUrl);
            else
                readIndexSection(child, parent, indexUrl);

            child = child.nextSiblingElement();
        }
    }
}

QString Tree::readIndexText(const QDomElement &element)
{
    QString text;
    QDomNode child = element.firstChild();
    while (!child.isNull()) {
        if (child.isText())
            text += child.toText().nodeValue();
        child = child.nextSibling();
    }
    return text;
}

void Tree::resolveIndex()
{
    QPair<ClassNode*,QString> pair;

    foreach (pair, priv->basesList) {
        foreach (QString base, pair.second.split(",")) {
            Node *baseClass = root()->findNode(base, Node::Class);
            if (baseClass) {
                pair.first->addBaseClass(Node::Public,
                                         static_cast<ClassNode*>(baseClass));
            }
        }
    }

    QPair<FunctionNode*,QString> relatedPair;

    foreach (relatedPair, priv->relatedList) {
        Node *classNode = root()->findNode(relatedPair.second, Node::Class);
        if (classNode)
            relatedPair.first->setRelates(static_cast<ClassNode*>(classNode));
    }
}

void Tree::generateIndexSubSections(QString indent, QTextStream& out,
                                    const Node *node) const
{
    if (!node->url().isEmpty())
        return;

    QString nodeName;
    switch (node->type()) {
        case Node::Namespace:
            nodeName = "namespace";
            break;
        case Node::Class:
            nodeName = "class";
            break;
        case Node::Fake:
            nodeName = "page";
            break;
        case Node::Enum:
            nodeName = "enum";
            break;
        case Node::Typedef:
            nodeName = "typedef";
            break;
        case Node::Property:
            nodeName = "property";
            break;
        case Node::Function:
            nodeName = "function";
            break;
        case Node::Variable:
            nodeName = "variable";
            break;
        case Node::Target:
            nodeName = "target";
            break;
        default:
            return;
    }

    QString access;
    switch (node->access()) {
        case Node::Public:
            access = "public";
            break;
        case Node::Protected:
            access = "protected";
            break;
        case Node::Private:     // Do not include private nodes in the index.
        default:
            return;
    }

    QString objName = node->name();
    QString childNodes;
    QString thisNode;
    QTextStream thisStream(&thisNode);

    thisStream << indent << "<" << nodeName
        << " access=\"" << access << "\""
        << " name=\"" << HtmlGenerator::protect(objName) << "\"";
        
    // Class contain information about their base classes.

    if (node->type() == Node::Class) {

        if (objName.isEmpty())
            return;

        const ClassNode *classNode = static_cast<const ClassNode*>(node);
        QList<RelatedClass> bases = classNode->baseClasses();
        QStringList baseStrings;
        foreach (RelatedClass related, bases) {
            ClassNode *baseClassNode = related.node;
            baseStrings.append(baseClassNode->name());
        }
        thisStream << " bases=\"" + HtmlGenerator::protect(baseStrings.join(","))
                   << "\"";
    }

    // Fake nodes (such as manual pages) contain subtypes, titles and other
    // attributes.

    if (node->type() == Node::Fake) {

        if (objName.isEmpty())
            return;

        const FakeNode *fakeNode = static_cast<const FakeNode*>(node);
        switch (fakeNode->subType()) {
            case FakeNode::Example:
                thisStream << " subtype=\"example\"";
                break;
            case FakeNode::HeaderFile:
                thisStream << " subtype=\"header\"";
                break;
            case FakeNode::File:
                thisStream << " subtype=\"file\"";
                break;
            case FakeNode::Group:
                thisStream << " subtype=\"group\"";
                break;
            case FakeNode::Module:
                thisStream << " subtype=\"module\"";
                break;
            case FakeNode::Page:
                thisStream << " subtype=\"page\"";
                break;
            case FakeNode::ExternalPage:
                thisStream << " subtype=\"externalpage\"";
                break;
            default:
                break;
        }
        thisStream << " title=\"" << HtmlGenerator::protect(fakeNode->title()) << "\"";
        thisStream << " fulltitle=\"" << HtmlGenerator::protect(fakeNode->fullTitle()) << "\"";
        thisStream << " subtitle=\"" << HtmlGenerator::protect(fakeNode->subTitle()) << "\"";
        thisStream << " location=\"" << HtmlGenerator::protect(fakeNode->doc().location().fileName()) << "\"";
    }

    // Function nodes contain information about the type of function being
    // described.

    if (node->type() == Node::Function) {

        if (objName.isEmpty())
            return;

        const FunctionNode *functionNode = static_cast<const FunctionNode*>(node);
        switch (functionNode->virtualness()) {
            case FunctionNode::NonVirtual:
                thisStream << " virtual=\"non\"";
                break;
            case FunctionNode::ImpureVirtual:
                thisStream << " virtual=\"impure\"";
                break;
            case FunctionNode::PureVirtual:
                thisStream << " virtual=\"pure\"";
                break;
            default:
                break;
        }
        switch (functionNode->metaness()) {
            case FunctionNode::Plain:
                thisStream << " meta=\"plain\"";
                break;
            case FunctionNode::Signal:
                thisStream << " meta=\"signal\"";
                break;
            case FunctionNode::Slot:
                thisStream << " meta=\"slot\"";
                break;
            case FunctionNode::Ctor:
                thisStream << " meta=\"constructor\"";
                break;
            case FunctionNode::Dtor:
                thisStream << " meta=\"destructor\"";
                break;
            case FunctionNode::MacroWithParams:
                thisStream << " meta=\"macrowithparams\"";
                break;
            case FunctionNode::MacroWithoutParams:
                thisStream << " meta=\"macrowithoutparams\"";
                break;
            default:
                break;
        }
        thisStream << " const=\"" << (functionNode->isConst()?"true":"false") << "\"";
        thisStream << " static=\"" << (functionNode->isStatic()?"true":"false") << "\"";
        thisStream << " overload=\"" << (functionNode->isOverload()?"true":"false") << "\"";
        if (functionNode->relates())
            thisStream << " relates=\"" << HtmlGenerator::protect(
                functionNode->relates()->name()) << "\"";
    }

    // Inner nodes and function nodes contain child nodes of some sort, either
    // actual child nodes or function parameters. For these, we close the
    // opening tag, create child elements, then add a closing tag for the
    // element. Elements for all other nodes are closed in the opening tag.

    const InnerNode *inner = dynamic_cast<const InnerNode*>(node);
    if (inner) {

        // For internal pages, we canonicalize the target, keyword and content
        // item names so that they can be used by qdoc for other sets of
        // documentation.
        // The reason we do this here is that we don't want to ruin
        // externally composed indexes, containing non-qdoc-style target names
        // when reading in indexes.

        if (inner->doc().hasTargets()) {
            bool external = false;
            if (inner->type() == Node::Fake) {
                const FakeNode *fakeNode = static_cast<const FakeNode *>(inner);
                if (fakeNode->subType() == FakeNode::ExternalPage)
                    external = true;
            }

            foreach (Atom *target, inner->doc().targets()) {
                QString targetName = target->string();
                if (!external)
                    targetName = Doc::canonicalTitle(targetName);
                childNodes += indent + " <target name=\""
                            + HtmlGenerator::protect(targetName) + "\" />\n";
            }
        }
        if (inner->doc().hasKeywords()) {
            foreach (Atom *keyword, inner->doc().keywords()) {
                childNodes += indent + " <keyword name=\""
                            + HtmlGenerator::protect(
                                Doc::canonicalTitle(keyword->string()))
                            + "\" />\n";
            }
        }
        if (inner->doc().hasTableOfContents()) {
            foreach (Atom *item, inner->doc().tableOfContents()) {
                QString title = Text::sectionHeading(item).toString();
                childNodes += indent + " <contents name=\""
                            + HtmlGenerator::protect(
                                Doc::canonicalTitle(title))
                            + "\" title=\""
                            + HtmlGenerator::protect(title) + "\" />\n";
            }
        }

        QString temp;
        QTextStream childOut(&temp);
        foreach (Node *child, inner->childNodes()) {
            generateIndexSubSections(indent + " ", childOut, child);
        }

        childNodes += temp;
        temp.clear();

        foreach (Node *child, inner->relatedNodes())
            generateIndexSubSections(indent + "  ", childOut, child);


    } else if (node->type() == Node::Function) {

        const FunctionNode *functionNode = static_cast<const FunctionNode*>(node);
        foreach (Parameter parameter, functionNode->parameters()) {
            // Do not supply a default value for the parameter; it will only
            // cause disagreement when it is read from an index file later on.
            childNodes += indent + " <parameter"
                + " left=\"" + HtmlGenerator::protect(parameter.leftType()) + "\""
                + " right=\"" + HtmlGenerator::protect(parameter.rightType()) + "\""
                + " name=\"" + HtmlGenerator::protect(parameter.name()) + "\""
//                + " default=\"" + HtmlGenerator::protect(parameter.defaultValue())
                + " />\n";
        }
    }

    // Construct the opening tag for the node.

    out << thisNode;

    if (!childNodes.isEmpty()) {
        out << ">\n";
        out << childNodes;
        out << indent << "</" << nodeName << ">\n";
    } else
        out << " />\n";

    out.flush();
}

void Tree::generateIndexSections(const QString &fileName, const QString &url,
                                 const QString &title) const
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return ;

    QTextStream out(&file);

    out << "<!DOCTYPE QDOCINDEX>\n";
    out << "<INDEX url=\"" << HtmlGenerator::protect(url)
        << "\" title=\"" << HtmlGenerator::protect(title) + "\">\n";

    generateIndexSubSections("", out, root());

    out << "</INDEX>\n";
    out.flush();
}

void Tree::addExternalLink(const QString &url, const Node *relative)
{
    FakeNode *fakeNode = new FakeNode(root(), url, FakeNode::ExternalPage);
    fakeNode->setAccess(Node::Public);

    // Create some content for the node.
    QSet<QString> emptySet;
    Location location(relative->doc().location());
    Doc doc(location, " ", emptySet); // placeholder
    fakeNode->setDoc(doc);
}
