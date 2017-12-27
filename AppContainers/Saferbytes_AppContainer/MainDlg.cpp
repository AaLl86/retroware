#include "stdafx.h"
#include "MainDlg.h"
#include "picture.h"
#include "log.h"

// Default constructor
CMainDlg::CMainDlg():
	CDialog(IDD_DLGMAIN, NULL)
{ }

CMainDlg::CMainDlg(UINT nResID, CWnd* pParent):
	CDialog(nResID, pParent)
{
}

CMainDlg::~CMainDlg(void)
{
}

// Init function
BOOL CMainDlg::OnInitDialog() {
	// Set main icon
	SetIconLarge(IDI_MAIN);
	SetIconSmall(IDI_MAIN);

	// Set main menù
	CMenu mainMenu;
	mainMenu.LoadMenu(IDR_MAINMENU);
	this->SetMenu(&mainMenu);

	// Attach each controls to its variable
	AttachItem(IDC_TXTPATH, m_appPathEdt);
	AttachItem(IDC_TXTAPPSID, m_appContSidEdt);

	// Put a proper SID in edit control
	m_appContSidEdt.SetWindowText(L"S-1-15-2-1520854211-3666789380-189579892-1973497788-2854962754-2836109804-3864561331");
	return TRUE;
}
// This instance WindowProc procedure
BOOL CMainDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

// Paint function
void CMainDlg::OnDraw(CDC* pDc) {
	CFont txtFont;								// Main text font
	CFont titleFont;							// Title font
	CFont verFont;								// Version string font
	CRect titleRect(290, 30, 620, 70);			// Title Rectangle
	CRect cRect;								// Client rect
	LOGFONT txtLogFont =						// LOGFONT structure
	{GetHeightFromPt(10), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		_T("Calibri")};

	// Draw Saferbytes Logo
	CPicture logoPct(IDB_SAFERBYTES_LOGO);
	logoPct.Draw(pDc, CPoint(10, 8), CSize(270,72));

	// Create text, title font ...
	LOGFONT titleLogFont = txtLogFont;
	txtFont.CreateFontIndirect(&txtLogFont);
	titleLogFont.lfHeight = GetHeightFromPt(19);
	titleLogFont.lfWeight = FW_BOLD;
	wcscpy_s(titleLogFont.lfFaceName, 32, L"Arial");
	titleFont.CreateFontIndirect(&titleLogFont);

	// ... and Version font
	txtLogFont.lfWeight = FW_NORMAL; 
	txtLogFont.lfHeight = GetHeightFromPt(10);
	txtLogFont.lfItalic = TRUE;
	verFont.CreateFontIndirect(&txtLogFont);

	// Draw Title
	pDc->SelectObject(&titleFont);
	pDc->SetBkMode(TRANSPARENT);
	pDc->SetTextColor(RGB(74,114,177));
	pDc->DrawText(L"AppContainer Launcher", -1, titleRect, DT_LEFT | DT_INTERNAL); 

	// Draw text
	pDc->SelectObject(&txtFont);
	pDc->SetTextColor(0);
	pDc->TextOut(13, 104, L"Target application full path:");
	pDc->TextOut(13, 190, L"App Container package SID:");
	pDc->TextOut(16, 316, L"App Container Capabilities list:");

	// Draw version information
	CVersionInfo verInfo(GetModuleHandle(NULL));
	TCHAR verStr[0x80] = {0};
	wsprintf(verStr, L"Version %s", verInfo.GetFileVersionString(false));
	cRect = GetClientRect();
	cRect.right -= 6; cRect.top = (cRect.bottom-=2) - 20;
	pDc->SelectObject(&verFont);
	pDc->SetTextColor(RGB(48, 48, 48));			// Dark gray color
	pDc->DrawText(verStr, -1, cRect, DT_RIGHT | DT_INTERNAL);
}



// Ottiene il valore nHeight per l'API CreateFont
__inline int GetHeightFromPt(int PointSize) { 
	return -MulDiv(PointSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72); }
