#include "canvasview.h"
#include "chartform.h"

#include <qcursor.h>
#include <qpoint.h>
#include <qpopupmenu.h>
#include <qstatusbar.h>


void CanvasView::contentsContextMenuEvent( QContextMenuEvent * )
{
    ((ChartForm*)parent())->optionsMenu->exec( QCursor::pos() );
}


void CanvasView::viewportResizeEvent( QResizeEvent *e )
{
    canvas()->resize( e->size().width(), e->size().height() );
    ((ChartForm*)parent())->drawElements();
}


void CanvasView::contentsMousePressEvent( QMouseEvent *e )
{
    QCanvasItemList list = canvas()->collisions( e->pos() );
    for ( QCanvasItemList::iterator it = list.begin(); it != list.end(); ++it )
	if ( (*it)->rtti() == CanvasText::CANVAS_TEXT ) {
	    m_movingItem = *it;
	    m_pos = e->pos();
	    return;
	}
    m_movingItem = 0;
}


void CanvasView::contentsMouseMoveEvent( QMouseEvent *e )
{
    if ( m_movingItem ) {
	QPoint offset = e->pos() - m_pos;
	m_movingItem->moveBy( offset.x(), offset.y() );
	m_pos = e->pos();
	ChartForm *form = (ChartForm*)parent();
	form->setChanged( true );
	int chartType = form->chartType();
	CanvasText *item = (CanvasText*)m_movingItem;
	int i = item->index();
	(*m_elements)[i].setX( chartType, item->x() );
	(*m_elements)[i].setY( chartType, item->y() );
	canvas()->update();
    }
}


