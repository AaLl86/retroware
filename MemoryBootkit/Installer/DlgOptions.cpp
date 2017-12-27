#include "stdafx.h"
#include "DlgOptions.h"
#include "VariousStuff.h"

//CDlgOptions::CDlgOptions(UINT nResID, CWnd* pParent)  {
//}

CDlgOptions::CDlgOptions(CWnd * parent):
	CDialog(IDD_DLGOPTIONS, parent)
{
	g_pCurSetup = NULL;
}

CDlgOptions::~CDlgOptions(void)
{
}

// Initialize Dialog event
BOOL CDlgOptions::OnInitDialog()
{
	SetTraspWindow(this->m_hWnd, 90.80);
	AttachItem(IDC_RBTKTYPEAUTO, g_mRdBtkTypeAuto);
	AttachItem(IDC_RBTKTYPESTD, g_mRdBtkTypeStandard);
	AttachItem(IDC_RBTKTYPECOMPATIBLE, g_mRdBtkTypeCompatible);
	AttachItem(IDC_CHKIGNOREMBRTYPE, g_mDontUseMbrType);
	this->SetWindowText(L"Bootkit Setup Options");

	UpdateControls();

	return TRUE;
}

// Update user controls based on "setupInstance" setup data
bool CDlgOptions::UpdateControls() {
	if (!g_pCurSetup) return false;
	BYTE btkType = 0; BOOLEAN bDontAnalyzeMbr = FALSE;
	g_pCurSetup->GetOptions(btkType, bDontAnalyzeMbr);
	
	// Reset Radio buttons
	g_mRdBtkTypeAuto.SetCheck(BST_UNCHECKED);
	g_mRdBtkTypeStandard.SetCheck(BST_UNCHECKED);
	g_mRdBtkTypeCompatible.SetCheck(BST_UNCHECKED);

	if (btkType == (BYTE)CBootkitMbrSetup::BOOTKIT_TYPE_UNKNOWN_OR_AUTO)
		g_mRdBtkTypeAuto.SetCheck(BST_CHECKED);
	else if (btkType == (BYTE)CBootkitMbrSetup::BOOTKIT_TYPE_STANDARD)
		g_mRdBtkTypeStandard.SetCheck(BST_CHECKED);
	else if (btkType == (BYTE)CBootkitMbrSetup::BOOTKIT_TYPE_COMPATIBLE)
		g_mRdBtkTypeCompatible.SetCheck(BST_CHECKED);

	if (bDontAnalyzeMbr)
		g_mDontUseMbrType.SetCheck(BST_CHECKED);
	else
		g_mDontUseMbrType.SetCheck(BST_UNCHECKED);
	return true;
}

//Ok Routine
void CDlgOptions::OnOK() {
	BYTE btkType = 0;
	if (g_mRdBtkTypeAuto.GetCheck() == BST_CHECKED)
		btkType = (BYTE)CBootkitMbrSetup::BOOTKIT_TYPE_UNKNOWN_OR_AUTO;
	else if (g_mRdBtkTypeStandard.GetCheck() == BST_CHECKED)
		btkType = (BYTE)CBootkitMbrSetup::BOOTKIT_TYPE_STANDARD;
	if (g_mRdBtkTypeCompatible.GetCheck() == BST_CHECKED)
		btkType = (BYTE)CBootkitMbrSetup::BOOTKIT_TYPE_COMPATIBLE;
		
	if (g_pCurSetup) 
		g_pCurSetup->SetOptions(btkType, (BOOLEAN)g_mDontUseMbrType.GetCheck());
	EndDialog(IDOK);
}


// This instance WindowProc procedure
BOOL CDlgOptions::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Background color message from Controls and DialogBox
	if (uMsg == WM_CTLCOLORSTATIC || uMsg == WM_CTLCOLORBTN ||
		uMsg == WM_CTLCOLORDLG || uMsg == WM_CTLCOLORBTN) {
		//uMsg == WM_CTLCOLOREDIT
		//HDC hDc = (HDC)wParam;
		//HWND hWnd = (HWND)lParam;
		HBRUSH bckBrush = CreateSolidBrush(GLOBAL_DLGS_BKCOLOR);
		return (INT_PTR)bckBrush;
	}
	// Pass unhandled messages on to parent DialogProc
	return DialogProcDefault(uMsg, wParam, lParam); 
}

// PAINT Event function
void CDlgOptions::OnDraw(CDC* pDc) {
	CRect cRect =
		GetClientRect();
	CFont titleFont;				// Title Font
	CFont subtitleFont;				// Subtitle Font
	CFont textFont;					// Text font

	// Create Text and title font
	LOGFONT txtLogFont = 
	{GetHeightFromPt(10), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		_T("Calibri")};
	LOGFONT titleLogFont = txtLogFont;
	titleLogFont.lfWeight = FW_SEMIBOLD;
	titleLogFont.lfHeight = GetHeightFromPt(18);

	textFont.CreateFontIndirect(&txtLogFont);
	titleFont.CreateFontIndirect(&titleLogFont);

	// subtitle font
	titleLogFont.lfHeight = GetHeightFromPt(12);
	titleLogFont.lfWeight = FW_SEMIBOLD;
	subtitleFont.CreateFontIndirect(&titleLogFont);

	pDc->SetTextColor(RGB(185,5,7));
	pDc->SetBkMode(TRANSPARENT);
	pDc->SelectObject(&titleFont);
	cRect.top += 12;
	pDc->DrawText(L"Memory bootkit Options", -1, cRect, DT_TOP | DT_SINGLELINE | DT_CENTER);
	
	// Draw text
	cRect.top += 38;
	cRect.left += 20;
	pDc->SelectObject(&textFont);
	pDc->SetTextColor(0);
	pDc->DrawText(L"Bootkit program type: ", -1, cRect, DT_TOP | DT_SINGLELINE | DT_LEFT);

	// Advanced options
	//cRect.top += 100;
	//pDc->SelectObject(&subtitleFont);
	//pDc->DrawText(L"Advanced optimizations:", -1, cRect, DT_LEFT | DT_INTERNAL);

}
