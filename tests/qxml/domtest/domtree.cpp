#include <qfile.h>
#include <qmessagebox.h>

#include "domtree.h"

//
// DomTree
//

DomTree::DomTree( const QString fileName, QWidget *parent, const char *name )
    : QHBox( parent, name )
{
    // div. configuration of the list view
    tree = new QListView( this );
    tree->addColumn( "Name" );
    tree->addColumn( "Type" );
    tree->setRootIsDecorated( TRUE );
    tree->setSorting( -1 );
    connect( tree, SIGNAL(selectionChanged(QListViewItem*)),
	    this, SLOT(selectionChanged(QListViewItem*)) );

    // read the XML file and create DOM tree
    QFile file( fileName );
    if ( !file.open( IO_ReadOnly ) ) {
	return;
    }
    if ( !domTree.setContent( &file ) ) {
	file.close();
	return;
    }
    file.close();
    buildTree( 0, domTree );

    // div. configuration of the list view
    text = new QTextView( this );
    text->setMinimumSize( 300, 400 );
    text->setTextFormat( RichText );
}

DomTree::~DomTree()
{
}

void DomTree::buildTree( QListViewItem *parentItem, const QDomNode &actNode )
{
    static int depth = -1;
    QListViewItem *thisItem = 0;
    QDomNode node = actNode;
    depth++;
    while ( !node.isNull() ) {
	if ( parentItem == 0 ) {
	    thisItem = new DomTreeItem( node, tree, thisItem );
	} else {
	    thisItem = new DomTreeItem( node, parentItem, thisItem );
	}
	buildTree( thisItem, node.firstChild() );
	if ( depth <= 1 ) {
	    tree->setOpen( thisItem, TRUE );
	}
	node = node.nextSibling();
    }
    depth--;
}

void DomTree::selectionChanged( QListViewItem *it )
{
    text->setText( ((DomTreeItem*)it)->contentString() );
}

//
// DomTreeItem
//

DomTreeItem::DomTreeItem( const QDomNode &node, QListView *parent, QListViewItem *after )
    : QListViewItem( parent, after )
{
    _node = node;
    init();
}

DomTreeItem::DomTreeItem( const QDomNode &node, QListViewItem *parent, QListViewItem *after )
    : QListViewItem( parent, after )
{
    _node = node;
    init();
}

DomTreeItem::~DomTreeItem()
{
}

void DomTreeItem::init()
{
    setText( 0, _node.nodeName() );

    switch ( _node.nodeType() ) {
	case QDomNode::ElementNode:
	    setText( 1, "Element" );
	    break;
	case QDomNode::AttributeNode:
	    setText( 1, "Attribute" );
	    break;
	case QDomNode::CDATASectionNode:
	    setText( 1, "CDATA Section" );
	    break;
	case QDomNode::EntityReferenceNode:
	    setText( 1, "Entity Reference" );
	    break;
	case QDomNode::EntityNode:
	    setText( 1, "Entity" );
	    break;
	case QDomNode::ProcessingInstructionNode:
	    setText( 1, "PI" );
	    break;
	case QDomNode::CommentNode:
	    setText( 1, "Comment" );
	    break;
	case QDomNode::DocumentNode:
	    setText( 1, "Document" );
	    break;
	case QDomNode::DocumentTypeNode:
	    setText( 1, "Document Type" );
	    break;
	case QDomNode::DocumentFragmentNode:
	    setText( 1, "Document Fragment" );
	    break;
	case QDomNode::NotationNode:
	    setText( 1, "Notation" );
	    break;
	case QDomNode::TextNode:
	    setText( 1, "Character Data" );
	    break;
	default:
	    setText( 1, "" );
	    break;
    }
}

QString DomTreeItem::contentString()
{
    QString s;
    switch ( _node.nodeType() ) {
	case QDomNode::ElementNode:
	    {
		s += "<h3>Text</h3>";
		s += _node.toElement().text();
		s += "<hr/>";
		s += "<h3>Attributes</h3>";
		QDomNamedNodeMap attributes = _node.toElement().attributes();
		for ( uint i=0; i< attributes.length(); i++ ) {
		    s += attributes.item(i).toAttr().name();
		    s += " = '";
		    s += attributes.item(i).toAttr().value();
		    s += "'<br/>";
		}
	    }
	    break;
	case QDomNode::CDATASectionNode:
	    s += _node.toCDATASection().data();
	    break;
	case QDomNode::ProcessingInstructionNode:
	    s += "<b>Target: </b>";
	    s += _node.toProcessingInstruction().target();
	    s += "<br/>";
	    s += "<b>Value: </b>";
	    s += _node.toProcessingInstruction().data();
	    break;
	case QDomNode::CommentNode:
	    s += _node.toComment().data();
	    break;
	case QDomNode::DocumentNode:
	    {
		QDomDocumentType doctype = _node.toDocument().doctype();
		s += "<h2>Document Type</h2>";
		s += doctype.name();
		s += "<hr/><h3>Entities</h3>";
		QDomNamedNodeMap entities = doctype.entities();
		if ( entities.length() > 0 ) {
		    s += "<ul>";
		    for ( uint i=0; i< entities.length(); i++ ) {
			QDomEntity entity = entities.item(i).toEntity();
			s += "<li>";
			s += "<b>Name:</b> ";
			s += entity.nodeName();
			if ( !entity.publicId().isNull() ) {
			    s += "<br/>";
			    s += "<b>Public ID:</b> ";
			    s += entity.publicId();
			}
			if ( !entity.systemId().isNull() ) {
			    s += "<br/>";
			    s += "<b>System ID:</b> ";
			    s += entity.systemId();
			}
			if ( !entity.notationName().isNull() ) {
			    s += "<br/>";
			    s += "<b>Notation Name:</b> ";
			    s += entity.notationName();
			}
		    }
		    s += "</ul>";
		}
		s += "<hr/><h3>Notations</h3>";
		QDomNamedNodeMap notations = doctype.notations();
		if ( notations.length() > 0 ) {
		    s += "<ul>";
		    for ( uint i=0; i< notations.length(); i++ ) {
			QDomNotation notation = notations.item(i).toNotation();
			s += "<li>";
			s += "<b>Name:</b> ";
			s += notation.nodeName();
			if ( !notation.publicId().isNull() ) {
			    s += "<br/>";
			    s += "<b>Public:</b> ";
			    s += notation.publicId();
			}
			if ( !notation.systemId().isNull() ) {
			    s += "<br/>";
			    s += "<b>System:</b> ";
			    s += notation.systemId();
			}
		    }
		    s += "</ul>";
		}
	    }
	    break;
	case QDomNode::TextNode:
	    s += _node.toText().data();
	    break;
	default:
	    break;
    }
    return s;
}
