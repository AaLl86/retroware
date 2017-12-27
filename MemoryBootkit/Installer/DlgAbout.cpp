#include "stdafx.h"
#include "DlgAbout.h"
#include "variousStuff.h"
#include "Shellapi.h"

CDlgAbout::CDlgAbout(CWnd * parent):
	CDialog(IDD_DLGABOUT, parent),
	g_verInfo(GetModuleHandle(NULL))
{
	m_hdefCur = NULL;
}

CDlgAbout::~CDlgAbout(void)
{
}

	// Initialize Dialog event
BOOL CDlgAbout::OnInitDialog() {
	SetWindowText(L"About Memory Bootkit...");
	SetTraspWindow(this->m_hWnd, 90.80);
	HICON titleIco = LoadIcon(GetApp()->GetInstanceHandle(), MAKEINTRESOURCE(IDI_INFOICO));
	SetIcon(titleIco, TRUE);

	// Load SaferBytes Logo
	m_logoImg.CreatePicture(IDB_SBLOGOPNG);
	m_hdefCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));

	return TRUE;
}

// This instance WindowProc procedure
BOOL CDlgAbout::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Background color message from Controls and DialogBox
	if (uMsg == WM_CTLCOLORSTATIC || uMsg == WM_CTLCOLORBTN ||
		uMsg == WM_CTLCOLORDLG || uMsg == WM_CTLCOLORBTN) {
		//uMsg == WM_CTLCOLOREDIT
		//HDC hDc = (HDC)wParam;
		//HWND hWnd = (HWND)lParam;
		HBRUSH bckBrush = CreateSolidBrush(GLOBAL_DLGS_BKCOLOR);
		return (INT_PTR)bckBrush;
	}
	else if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_MOUSEMOVE ) {
		POINT point;
		point.x = LOWORD(lParam);
		point.y = HIWORD(lParam);
		if (uMsg == WM_LBUTTONDOWN) OnLButtonDown(wParam, point);
		else if (uMsg == WM_LBUTTONUP) OnLButtonUp(wParam, point);
		else if (uMsg == WM_MOUSEMOVE) OnMouseMove(wParam, point);
	}
	// Pass unhandled messages on to parent DialogProc
	return DialogProcDefault(uMsg, wParam, lParam); 
}

// WM_COMMAND message handler
BOOL CDlgAbout::OnCommand(WPARAM wParam, LPARAM lParam) {
	if (HIWORD(wParam) == BN_CLICKED) {
		if (LOWORD(wParam) == IDC_BTNSYSINFO){
			OnSysInfo();
			return TRUE;
		}
	}
	return CDialog::OnCommand(wParam, lParam);
}

// System info button Click
void CDlgAbout::OnSysInfo() {
	long retVal = NULL;
	HKEY mskey = NULL;
	DWORD valueType = NULL;
	TCHAR * value = new TCHAR[MAX_PATH]; 
	DWORD lbuffSize = MAX_PATH;
	TCHAR * defDirectory = new TCHAR[MAX_PATH];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // Tenta di recuperare dal registro di configurazione il percorso e il nome
    // del programma che consente di visualizzare le informazioni sul sistema
	retVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools\\MSINFO", 0, KEY_ALL_ACCESS, &mskey);
	retVal = RegQueryValueEx(mskey, L"PATH", 0, &valueType, (LPBYTE)value, &lbuffSize);

	RegCloseKey(mskey);

	if (value[0] == NULL) {
		// Tenta di recuperare dal registro di configurazione solo il percorso
		// del programma che consente di visualizzare le informazioni sul sistema
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Shared Tools Location", 0, KEY_ALL_ACCESS, &mskey);
		RegQueryValueEx(mskey, L"MSINFO", 0, &valueType, (LPBYTE)value, &lbuffSize);
		
		RegCloseKey(mskey);
	}
	if (value) {
		::GetCurrentDirectory(MAX_PATH, defDirectory);
		si.cb = sizeof(si);
		SecureZeroMemory(&si, sizeof(si));
		SecureZeroMemory(&pi, sizeof(pi));
		::CreateProcess(value, NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, defDirectory, &si, &pi);
	}
	delete[] value;
	delete[] defDirectory;
}

// Mouse move event
void CDlgAbout::OnMouseMove(DWORD keys, POINT point) {
	UNREFERENCED_PARAMETER(keys);
	
	if (point.x > m_rectLink.left && point.x < m_rectLink.right && 
		point.y > m_rectLink.top && point.y < m_rectLink.bottom) {
		//Il cursore è sul link
		SetCursor(LoadCursor(NULL, IDC_HAND));
	} else {
		if (GetCursor() != m_hdefCur)
			SetCursor(m_hdefCur);
	}
}

// Mouse button down event
void CDlgAbout::OnLButtonDown(DWORD keys, POINT point) { 
	UNREFERENCED_PARAMETER(keys);
	int retVal = 0;
	if (point.x > m_rectLink.left && point.x < m_rectLink.right && 
		point.y > m_rectLink.top && point.y < m_rectLink.bottom) 
		// Click on link text
		retVal = (int)ShellExecute(this->m_hWnd, L"open", L"http://www.saferbytes.it", NULL, NULL, SW_SHOW);
}

// Mouse left button up event
void CDlgAbout::OnLButtonUp(DWORD keys, POINT point) {
	UNREFERENCED_PARAMETER(keys);
	if (point.x > m_rectLink.left && point.x < m_rectLink.right && 
		point.y > m_rectLink.top && point.y < m_rectLink.bottom) 
		// Mouse is located on link
		SetCursor(LoadCursor(NULL, IDC_HAND));
}

#pragma region Dialog painting functions
// PAINT Event function
void CDlgAbout::OnDraw(CDC* pDc) {
	CRect cRect = GetClientRect();
	CFont txtFont, verFont, titleFont,
		linkFont, warningFont;
	CSize textSize;
	LOGFONT txtLogFont =						// LOGFONT structure
		{GetHeightFromPt(11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			_T("Calibri")};

	// Build needed fonts and data structures
	txtFont.CreateFontIndirect(&txtLogFont);
	// Title font
	txtLogFont.lfHeight = GetHeightFromPt(20);
	txtLogFont.lfWeight = FW_BOLD;
	txtLogFont.lfUnderline = FALSE;
	titleFont.CreateFontIndirect(&txtLogFont);
	// Version information font
	txtLogFont.lfHeight = GetHeightFromPt(13);
	txtLogFont.lfWeight = FW_BOLD;
	txtLogFont.lfItalic = TRUE;
	txtLogFont.lfUnderline = FALSE;
	verFont.CreateFontIndirect(&txtLogFont);
	// Warning font
	txtLogFont.lfHeight = GetHeightFromPt(12);
	txtLogFont.lfItalic = FALSE;
	warningFont.CreateFontIndirect(&txtLogFont);
	// Link font
	txtLogFont.lfHeight = GetHeightFromPt(17);
	txtLogFont.lfWeight = FW_SEMIBOLD;
	txtLogFont.lfItalic = FALSE;
	txtLogFont.lfUnderline = TRUE;
	linkFont.CreateFontIndirect(&txtLogFont);
	txtLogFont.lfUnderline = FALSE;
	cRect.left += 140 + 20;
	cRect.right -= 10;

	// Draw logo
	m_logoImg.Draw(pDc, CPoint(10,40), CSize(138,110));

	// Draw title
	pDc->SetTextColor(RGB(161,31,15));
	pDc->SetBkMode(TRANSPARENT);
	pDc->SelectObject(&titleFont);
	cRect.top += 16;
	pDc->DrawText(L"X86 Memory Bootkit", -1, cRect, DT_INTERNAL | DT_CENTER);
	textSize = pDc->GetTextExtentPoint32(L"X86", 3);
	cRect.top += textSize.cy + 12;

	// Draw Version information
	pDc->SelectObject(&verFont);
	pDc->SetTextColor(0);
	TCHAR verString[0x80] = {0};
	swprintf_s(verString, COUNTOF(verString), L"Version: %s", g_verInfo.GetFileVersionString(false));
	pDc->DrawText(verString, -1, cRect, DT_INTERNAL | DT_LEFT);
	textSize = pDc->GetTextExtentPoint32(L"X86", 3);
	cRect.top += textSize.cy + 10;

	// Draw description string
	LPTSTR descString = 
		L"This application is designed to remove physical memory limits for every Microsoft 32 bit "
		L"operation systems (starting from Windows Vista)...\r\n"
		L"In this way you can exploit up to 64 GB of physical memory, even on 32 bit Windows.\r\n";
	pDc->SelectObject(&txtFont);
	pDc->DrawText(descString, -1, cRect, DT_LEFT | DT_WORDBREAK);
	textSize = pDc->GetTextExtentPoint32(descString, wcslen(descString));

	// Draw copyright string
	txtLogFont.lfHeight = GetHeightFromPt(11);
	txtLogFont.lfWeight = FW_NORMAL; txtLogFont.lfItalic = TRUE;
	txtFont.Detach();
	txtFont.CreateFontIndirect(&txtLogFont);
	pDc->SelectObject(&txtFont);
	cRect.left -= 140;
	cRect.top += (textSize.cy * 5) + 15;
	descString = L"Copyright© 2013 Saferbytes S.r.l.s.\r\n"
		L"This application is developed and based on an idea of Andrea Allievi";
	pDc->DrawText(descString, -1, cRect, DT_LEFT | DT_WORDBREAK);
	textSize = pDc->GetTextExtentPoint32(descString, wcslen(descString));

	// Draw Warning string
	cRect.top += (textSize.cy * 2) + 10;
	descString = L"Warning: This software is just for research purposes and it must "
		L"be used only on testing environment and at your own risk.";
	pDc->SelectObject(&warningFont);
	pDc->SetTextColor(RGB(9,10,130));
	pDc->DrawText(descString, -1, cRect, DT_LEFT | DT_WORDBREAK);
	textSize = pDc->GetTextExtentPoint32(descString, wcslen(descString));

	// Draw link
	LPTSTR linkStr = L"www.saferbytes.it";
	cRect.top += (textSize.cy * 2) + 10;
	pDc->SelectObject(&linkFont);
	pDc->SetTextColor(RGB(161,31,15));
	pDc->DrawText(linkStr, -1, cRect, DT_CENTER | DT_INTERNAL);
	textSize = pDc->GetTextExtentPoint32(linkStr, wcslen(linkStr));
	if (m_rectLink.IsRectEmpty()) {
		m_rectLink.top = cRect.top;
		m_rectLink.bottom = cRect.top + textSize.cy;
		m_rectLink.left = cRect.left + (cRect.Width() - textSize.cx) / 2; 
		m_rectLink.right = m_rectLink.left + textSize.cx; 
	}
}
#pragma endregion
