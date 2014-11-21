// ColorListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "ColorListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorListCtrl

IMPLEMENT_DYNCREATE(CColorListCtrl, CListCtrl)

	CColorListCtrl::CColorListCtrl()
{
	GridMode = false;
	AltRowMode = false;
	MaskMode = false;
}

CColorListCtrl::~CColorListCtrl()
{
}


BEGIN_MESSAGE_MAP(CColorListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CColorListCtrl)
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteitem)
	ON_NOTIFY_REFLECT(LVN_DELETEALLITEMS, OnDeleteallitems)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, CustDraw)
	ON_NOTIFY_REFLECT(LVN_INSERTITEM, OnInsertitem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorListCtrl drawing


/////////////////////////////////////////////////////////////////////////////
// CColorListCtrl diagnostics

#ifdef _DEBUG
void CColorListCtrl::AssertValid() const
{
	CListCtrl::AssertValid();
}

void CColorListCtrl::Dump(CDumpContext& dc) const
{
	CListCtrl::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CColorListCtrl message handlers
void CColorListCtrl::CustDraw(NMHDR *pNotifyStruct, LRESULT *result)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW) pNotifyStruct; // cast our generic structure to bigger/specialized strc
	int i = 0;
	long iRow = 0;
	long iCol = 0;
	long iCIndex = 0;
	long iCelIndex = 0;
	long iRIndex = 0;
	switch(lplvcd->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*result  = CDRF_NOTIFYITEMDRAW|CDRF_NOTIFYSUBITEMDRAW;	// request notifications for individual items
		break;

	case CDDS_ITEMPREPAINT:
		*result = CDRF_NOTIFYSUBITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT|CDDS_SUBITEM:	// drawing subitem
		iRow = lplvcd->nmcd.dwItemSpec;	
		iCol = lplvcd->iSubItem;

		if(GridMode)		// grid mode
		{
			if( iRow %2 == 1)
			{
				if( iCol %2 == 0)
					lplvcd->clrTextBk = Gx;
				else
					lplvcd->clrTextBk = Gy;
			}
			else
			{
				if( iCol %2 == 0)
					lplvcd->clrTextBk = Gy;
				else
					lplvcd->clrTextBk = Gx;
			}
		}

		if(AltRowMode)	//alternate row mode
		{
			if( iRow %2 == 1)
				lplvcd->clrTextBk = Rx;
			else
				lplvcd->clrTextBk = Ry;
		}

		iRIndex = findDetails(iRow,ROW);	// individual cell,coloumn,row color setting
		iCIndex = findDetails(iCol,COL);
		iCelIndex = findDetails(iRow,CELL,iCol);

		if(iRIndex == -1 && iCIndex == -1 && iCelIndex == -1 )
		{
			if(!GridMode && !AltRowMode)
				lplvcd->clrTextBk = GetBkColor();
		}
		else
		{
			if( iCelIndex != -1)
				lplvcd->clrTextBk = CellColors[iCelIndex].cf;
			else
			{
				if(iRIndex != -1)
					lplvcd->clrTextBk = RowColors[iRIndex].cf;
				else
					lplvcd->clrTextBk = ColColors[iCIndex].cf;
			}

		}

		if(MaskMode)		// text mask mode
			lplvcd->clrText = InvertColor(lplvcd->clrTextBk);
		*result = CDRF_NEWFONT;

		break;
	default:
		*result = CDRF_DODEFAULT;
	}

}

void CColorListCtrl::SetRowColor(COLORREF cf,long row)
{
	ColorStruct cs;
	cs.cf = cf;
	cs.id = row;
	RowColors.push_back(cs);
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void CColorListCtrl::SetColColor(COLORREF cf,long col)
{
	ColorStruct cs;
	cs.cf = cf;
	cs.id = col;
	ColColors.push_back(cs);
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void CColorListCtrl::SetCellColor(COLORREF cf,long row,long col)
{
	ColorStruct cs;
	cs.cf = cf;
	cs.id = row;
	cs.subId = col;
	CellColors.push_back(cs);
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

long CColorListCtrl::findDetails(long id,RecType rt,long subId)
{
	vector <int>::size_type i;
	if( rt == ROW)
	{
		for( i=0;i< RowColors.size(); i++)
		{
			if(RowColors[i].id == id)
				return i;
		}
	}

	if( rt == COL)
	{
		for( i=0;i< ColColors.size(); i++)
		{
			if(ColColors[i].id == id)
				return i;
		}
	}

	if( rt == CELL)
	{
		for(i=0;i< CellColors.size(); i++)
		{
			if( subId >=0){
				if(CellColors[i].id == id && CellColors[i].subId == subId)
					return i;
			}
		}
	}
	return -1;
}

void CColorListCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// TODO: Add your control notification handler code here
	long row  = pNMListView->iItem;
	long index = findDetails(row);

	for(BYTE i=0;i< RowColors.size();i++)
	{
		if(RowColors[i].id == row)
		{
			RowColors.erase(RowColors.begin()+i);
			i--;
			continue;
		}
		if(RowColors[i].id > row)
		{
			RowColors[i].id--;
		}
	}
	vector <int>::size_type i;
	for(i=0;i< CellColors.size();i++)
	{
		if(CellColors[i].id == row)
		{
			CellColors.erase(CellColors.begin()+i);
			i--;
			continue;
		}
		if(CellColors[i].id > row)
		{
			CellColors[i].id--;
		}
	}
	*pResult = 0;
}

void CColorListCtrl::OnDeleteallitems(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	RowColors.clear();
	ColColors.clear();
	CellColors.clear();
	*pResult = 0;
}

void CColorListCtrl::OnInsertitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	long row  = pNMListView->iItem;
	long col  = pNMListView->iSubItem;

	vector <int>::size_type i;
	for(i=0;i< RowColors.size();i++)
	{
		if(RowColors[i].id > row)
		{
			RowColors[i].id++;
		}
	}

	for(i=0;i< CellColors.size();i++)
	{
		if(CellColors[i].id > row)
		{
			CellColors[i].id++;
		}
	}

	*pResult = 0;
}

void CColorListCtrl::SetGridMode(bool TurnOn,COLORREF x,COLORREF y)
{
	Gx = y;
	Gy = x;
	GridMode = TurnOn;
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void CColorListCtrl::SetAltRowColors(bool TurnOn,COLORREF x,COLORREF y)
{
	Rx = x;
	Ry = y;
	AltRowMode = TurnOn;
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void CColorListCtrl::SetTextMask(bool TurnOn)
{
	MaskMode = TurnOn;
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void CColorListCtrl::ClrRowColor(long row)
{
	long index = findDetails(row);
	if(index != -1)
		RowColors.erase(RowColors.begin()+index);
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void CColorListCtrl::ClrColColor(long col)
{
	long index = findDetails(col,COL);
	if(index != -1)
		ColColors.erase(ColColors.begin()+index);
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void CColorListCtrl::ClrCellColor(long row,long col)
{
	long index = findDetails(row,CELL,col);
	if(index != -1)
		CellColors.erase(CellColors.begin()+index);
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

COLORREF CColorListCtrl::InvertColor(COLORREF cf)
{
	return RGB(abs(220-GetRValue(cf)),abs(220-GetGValue(cf)),abs(220-GetBValue(cf)));
}

void CColorListCtrl::ResetColors(void)
{
	RowColors.clear();
	ColColors.clear();
	CellColors.clear();
	GridMode = false;
	AltRowMode = false;
	MaskMode = false;
	RedrawWindow(NULL,NULL,RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}