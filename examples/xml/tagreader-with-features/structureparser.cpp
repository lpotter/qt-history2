/*
$Id$
*/

#include "structureparser.h"

#include <qstring.h>
#include <qlistview.h>
 
StructureParser::StructureParser( QListView * t )
                : QXmlDefaultHandler() 
{
    setListView( t );
}

void StructureParser::setListView( QListView * t )
{
    table = t;
    table->setSorting( -1 );
    table->addColumn( "Qualified name" );
    table->addColumn( "Namespace" );
}

bool StructureParser::startElement( const QString& namespaceURI, const QString& , 
                                    const QString& qName, 
                                    const QXmlAttributes& attributes)
{
    QListViewItem * element;

    if ( ! stack.isEmpty() ){
	element = new QListViewItem( stack.top(), qName, namespaceURI );
    } else {
	element = new QListViewItem( table, qName, namespaceURI );
    }
    stack.push( element );
    element->setOpen( TRUE );

    if ( attributes.length() > 0 ){
        QListViewItem * attribute;
	for ( int i = 0 ; i < attributes.length(); i++ ){
	    attribute = new QListViewItem( element,
	                                   attributes.qName(i), 
	                                   attributes.uri(i) ); 
	}      
    } 
    return TRUE;
}

bool StructureParser::endElement( const QString&, const QString&, const QString& )
{
    stack.pop();
    return TRUE;
}
