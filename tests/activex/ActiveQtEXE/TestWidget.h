// TestWidget.h: interface for the CTestWidget class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TESTWIDGET_H__DA4969C1_5578_42FE_AD7E_8E6924794EBB__INCLUDED_)
#define AFX_TESTWIDGET_H__DA4969C1_5578_42FE_AD7E_8E6924794EBB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "QActiveXBase.h"
#include <QSlider.h>
#include <QLCDNumber.h>
#include <QMultiLineEdit.h>

class QActiveX;

class CTestWidget : public QActiveXBase 
{
public:
	CTestWidget( QActiveX* pControl);
	~CTestWidget();

   void updateControl();
private:
	QSlider* m_pSlider;
	QLCDNumber* m_pLCD;
	QMultiLineEdit* m_pEdit;
	QMultiLineEdit* m_pEdit2;

    QActiveX* m_pControl;
protected:
    void	drawControl( QPainter* pPainter, const QRect& rc );
    void	resizeEvent( QResizeEvent* pEvent );
        

public:
/*
** Hide until we get the other stuff to work
// These are pure virtual functions in the baseclass
	virtual void InitWidget();
	virtual void UnInitWidget();

protected:
// Event handlers
	virtual void resizeEvent( QResizeEvent* pEvent );
*/
};

#endif // !defined(AFX_TESTWIDGET_H__DA4969C1_5578_42FE_AD7E_8E6924794EBB__INCLUDED_)
