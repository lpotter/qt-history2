#include "resultwindow.h"
#include <qstringlist.h>
#include <qlistbox.h>
#include <qsql.h>
#include <qsqlview.h>
#include <qsqlfield.h>
#include <qsqlindex.h>
#include <qsqltable.h>

// class MyDataGrid : public QSqlTable
// {
// public:
//     MyDataGrid ( QWidget * parent = 0, const char * name = 0 )
// 	: QSqlGrid( parent, name )
//     {
// 	QSqlConnection::addDatabase( "QMYSQL", "troll", "db", "", "lupinella", "mysql" );
// 	QSql r( "select * from Customer order by 2;", "mysql" );
// 	setQuery( r, FALSE );
// 	addColumn( r.fields().field( 2 ) );
// 	addColumn( r.fields().field( 4 ) );
//     }
//     ~MyDataGrid()
//     {
//     }
// };


ResultWindow::ResultWindow ( QWidget * parent, const char * name, WFlags f )
    : SqlBrowseWindowBase(parent, name, f)
{
    db = QSqlConnection::database();
    QStringList fil = db->tables();
    tableList->insertStringList( fil );
    connect( execButton,SIGNAL(clicked()), this, SLOT(slotExec()));
    //    grid->addWidget( new MyDataGrid( this ), 5, 5 );
}

ResultWindow::~ResultWindow()
{

}

void ResultWindow::slotExec()
{
    if ( db->isOpen() )
	dataGrid->setRowset( tableList->currentText() );
}
