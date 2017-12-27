/**********************************************************************
 * 
 *  AaLl86 Windows x86 Memory Bootkit
 *  Installer main dialog box code
 *	Last revision: 
 *
 *  Copyright© 2012 by AaLl86
 *	All right reserved
 * 
 **********************************************************************/
#include "stdafx.h"
#include "VariousStuff.h"
#include "DialogMain.h"
#include "DlgOptions.h"
#include "DlgAbout.h"
#include "MbrDefs.h"
#include "SystemAnalyzer.h"
#include <Commdlg.h>
// For device notifications
#include <dbt.h>
#include <shellapi.h>

// Dafault class Constructor, accept Dialog box resource id
CBtkDialog::CBtkDialog(UINT nResID, CWnd* pParent): 
	CDialog(nResID, pParent),
	g_sysAnalyzer(g_Log)
{
	Initialize();
}

// Initialize ME
void CBtkDialog::Initialize() {
	m_strOsVersion = NULL;
	g_bRightOs = FALSE;
	g_strLogFile = NULL;
	g_btkState = BOOTKIT_STATE_UNKNOWN;
	m_gpLogoPct = NULL;
	g_bUnkMbr = false;
	g_uchMbrActiveBtkType = 0;
	g_BiosType = g_sysAnalyzer.GetCurrentBiosType();
	g_iSysBootDiskIdx = 0;
	g_bSecureBootEnabled = false;
	if (g_BiosType == BIOS_TYPE_GPT) {
		g_pBtkGptSetup = new CBootkitGptSetup(g_Log);
		g_pBtkMbrSetup = NULL;
	} else {
		g_BiosType = BIOS_TYPE_MBR;		 // Default to MBR bios type
		g_pBtkMbrSetup = new CBootkitMbrSetup(g_Log);
		g_pBtkGptSetup = NULL;
	}
}

// Initialize and start log
bool CBtkDialog::InitializeLog(LPTSTR logFileName) {
	bool retVal = false;			// Returned value
	LPTSTR slash = NULL;			// Slash string pointer
	DWORD lastErr = 0;				// Last win32 error
	DWORD len = 0;					// string length

	if (g_strLogFile) { delete[] g_strLogFile; g_strLogFile = NULL; }
	// Get current log filename
	g_strLogFile = new TCHAR[MAX_PATH];
	GetModuleFileName(NULL, g_strLogFile, MAX_PATH);
	slash = wcsrchr(g_strLogFile, L'\\');
	if (slash) slash[1] = 0;
	wcscat_s(g_strLogFile, MAX_PATH, logFileName);

	// Create log 
	retVal = g_Log.Open(g_strLogFile);
	lastErr = GetLastError();

	if (!retVal) 
		::WriteToLog(L"CBtkDialog::InitializeLog - Unable to create log file in current directory (last error %i), "
		L"using \"Temp\" environment variable as current directory...", (LPVOID)lastErr);
	else 
		return true;

	len = GetTempPath(MAX_PATH, g_strLogFile);
	if (g_strLogFile[len-1] != L'\\') {slash[len++] = L'\\'; slash[len] = 0; }
	wcscat_s(g_strLogFile, MAX_PATH, logFileName);

	// Recreate log 
	retVal = g_Log.Open(g_strLogFile);
	lastErr = GetLastError();

	if (!retVal) {
		::WriteToLog(L"CBtkDialog::InitializeLog - Unable to create log file in temp path (last error %i).", (LPVOID)lastErr);
		delete[] g_strLogFile; g_strLogFile = NULL;
	} else
		::WriteToLog(L"CBtkDialog::InitializeLog - Successfully created log file in \"Temp\" directory...");
	return retVal;
}


CBtkDialog::CBtkDialog(LPCTSTR lpszResName, CWnd* pParent):
	CDialog(lpszResName, pParent),
	g_sysAnalyzer(g_Log)
{
	Initialize();
}

// Class destructor
CBtkDialog::~CBtkDialog() {
	if (m_gpLogoPct) delete m_gpLogoPct;
	if (g_strLogFile) { delete[] g_strLogFile; g_strLogFile = NULL; }
	if (m_strOsVersion) { delete[] m_strOsVersion; m_strOsVersion = NULL; }
}

BOOL CBtkDialog::OnInitDialog() {
	bool retVal = false;
	SetIconLarge(IDI_MAINICO);
	SetIconSmall(IDI_MAINICO);

	// Check current OS Version
	g_bRightOs = CSystemAnalyzer::GetOsVersionInfo(&m_strOsVersion);

	// Set main title
	this->SetWindowText(APPTITLE L" Setup");

	//g_bRightOs = TRUE;					// Debug only
	// Attach items
	AttachItem(IDC_BTNMBR, m_MbrRadioBtn);
	AttachItem(IDC_BTNVBR, m_VbrRadioBtn);
	AttachItem(IDC_BTNFLOPPY, m_FloppyRadioBtn);
	AttachItem(IDC_REMDRVLIST, m_RemDrvList); 

	// Apply transparents
	SetTraspWindow(this->m_hWnd, 91.80);

	// Load main logo picture
	m_gpLogoPct = new CPicture(IDB_MAINPNG);

	// Initialize and start log
	InitializeLog();

#ifndef _SDEBUG
	if (!g_bRightOs) {
		m_FloppyRadioBtn.SetCheck(BST_CHECKED);
		m_MbrRadioBtn.EnableWindow(FALSE);
		g_btkState = BOOTKIT_NOT_PRESENT;
	} else {
		// Check secure Boot
		if (g_pBtkGptSetup)
			g_sysAnalyzer.IsSecureBootActive(&g_bSecureBootEnabled);

		if (g_bSecureBootEnabled)  {
			g_Log.WriteLine(L"Warning! Enabled secureboot detected on this system. This application is "
				L"not compatible with Secure Boot.");
			MessageBox(L"Warning!\r\nThis application is not Secure Boot compatible, please disable it!",
				APPTITLE, MB_ICONEXCLAMATION);
			g_bRightOs = false;
		}
#endif
		// ...get bootkit state
		if (g_pBtkMbrSetup) {

			// Startup here, get system boot disk index 
			g_sysAnalyzer.SetLog(g_Log);
			retVal = g_sysAnalyzer.GetStartupPhysDisk((DWORD*)&g_iSysBootDiskIdx); 
			if (retVal) g_pBtkMbrSetup->SetSystemBootDiskIndex((DWORD)g_iSysBootDiskIdx);

			g_btkState = g_pBtkMbrSetup->GetBootkitState(g_iSysBootDiskIdx, 
				(CBootkitMbrSetup::BOOTKIT_SETUP_TYPE*)&g_uchMbrActiveBtkType);
			//g_pBtkMbrSetup->SetOptions(2, FALSE);		// Set default options to: compatible Bootkit and default MBR search
		}
		else 
			g_btkState = g_pBtkGptSetup->GetBootkitState(0);
#ifndef _SDEBUG
	}
#endif

	// Set menu icons
	SetMenuItemBitmap(IDM_FILE_INSTALL, IDB_INSTALLICO);
	SetMenuItemBitmap(IDM_FILE_EXIT, IDB_EXITICO);
	SetMenuItemBitmap(ID_HELP_HELP, IDB_HELPICO);
	SetMenuItemBitmap(IDM_FILE_OPTIONS, IDB_SETTINGSICO);
	SetMenuItemBitmap(ID_HELP_ABOUT, IDB_ABOUTICO);

	// Set bootkit controls and enable right check box
	SetBtkStateData();
	if (m_MbrRadioBtn.IsWindowEnabled()) 
		m_MbrRadioBtn.SetCheck(BST_CHECKED);
	else if (m_VbrRadioBtn.IsWindowEnabled()) 
		m_VbrRadioBtn.SetCheck(BST_CHECKED);
	else if (m_FloppyRadioBtn.IsWindowEnabled()) 
		m_FloppyRadioBtn.SetCheck(BST_CHECKED);

	// Initialize removable drive list
	UpdateRemDriveList();
	m_RemDrvList.SetCurSel(0);

	// Disable Bootkit option menù in case of GPT firmware
	if (g_BiosType == BIOS_TYPE_GPT)
		GetMenu()->EnableMenuItem(IDM_FILE_OPTIONS, MF_GRAYED | MF_BYCOMMAND);
		
	return TRUE;
}

// Set a menu bitmap
bool CBtkDialog::SetMenuItemBitmap(UINT mnuId, UINT bmpId) {
	HBITMAP hBitmap = NULL,
		hTraspBitmap = NULL;
	HMENU targetMenu = GetMenu()->GetHandle();
    hBitmap = (HBITMAP)LoadImage((HMODULE)GetModuleHandle(NULL), MAKEINTRESOURCE(bmpId), 
		IMAGE_BITMAP, 16, 16, 0);
	hTraspBitmap = CPicture::MakeBitMapTransparent(hBitmap, GetSysColor(COLOR_MENU));
	if (hBitmap) DeleteObject(hBitmap);
	BOOL retVal = SetMenuItemBitmaps(targetMenu , mnuId, MF_BYCOMMAND , hTraspBitmap, hTraspBitmap);
	return (retVal == TRUE);
}


// This instance WindowProc procedure
BOOL CBtkDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Background color message from Controls and DialogBox
	if (uMsg == WM_CTLCOLORSTATIC || uMsg == WM_CTLCOLORBTN ||
		uMsg == WM_CTLCOLORDLG || uMsg == WM_CTLCOLORBTN) {
		//uMsg == WM_CTLCOLOREDIT
		//HDC hDc = (HDC)wParam;
		//HWND hWnd = (HWND)lParam;
		HBRUSH bckBrush = CreateSolidBrush(GLOBAL_DLGS_BKCOLOR);
		return (INT_PTR)bckBrush;
	}
	else if (uMsg == WM_DEVICECHANGE) {
		// Device change message
		PDEV_BROADCAST_HDR pDevHdr = (PDEV_BROADCAST_HDR)lParam;
		if ((wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
			&& pDevHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
			Sleep(50);
			// Update controls
			::WriteToLog(L"CBtkDialog::DialogProc - Detected device change!");
			UpdateRemDriveList();
			m_RemDrvList.SetCurSel(0);
		}
	}
	// Pass unhandled messages on to parent DialogProc
	return DialogProcDefault(uMsg, wParam, lParam); 
}

void CBtkDialog::OnDraw(CDC* pDc) {
	CRect tRect, 
		cRect = GetClientRect();				// Client Rect and Text Rect
	CFont titleFont;							// Title font
	CFont txtFont;								// Normal text font
	CFont txtBoldFont;							// Bold text font
	CFont txtVerFont;							// Version information font
	SIZE fontSize = {0};						// Title font dimension
	LOGFONT txtLogFont =						// LOGFONT structure
	
	{GetHeightFromPt(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		_T("Calibri")};
	LOGFONT titleLogFont = txtLogFont;			// LOGFONT Title structure
	titleLogFont.lfHeight = GetHeightFromPt(22);

	// Create fonts:
	titleFont.CreateFontIndirect(&titleLogFont);
	txtFont.CreateFontIndirect(&txtLogFont);
	txtLogFont.lfWeight = FW_BOLD; 
	txtLogFont.lfHeight = GetHeightFromPt(13);
	txtBoldFont.CreateFontIndirect(&txtLogFont);
	txtLogFont.lfWeight = FW_NORMAL; 
	txtLogFont.lfHeight = GetHeightFromPt(11);
	txtLogFont.lfItalic = TRUE;
	txtVerFont.CreateFontIndirect(&txtLogFont);

	pDc->SetBkMode(TRANSPARENT);
	pDc->SetTextColor(RGB(240,164,46));
	pDc->SelectObject(&titleFont);
	
	tRect = cRect;
	tRect.top = 10;
	pDc->DrawTextW(APPTITLE, -1, &tRect, DT_CENTER | DT_INTERNAL);
	pDc->SetTextColor(RGB(225,0,0));
	tRect.top +=2; tRect.left +=3;
	pDc->DrawTextW(APPTITLE, -1, &tRect, DT_CENTER | DT_INTERNAL);

	// Draw main text
	pDc->SetTextColor(RGB(0,0,0));
	pDc->SelectObject(&txtFont);
	GetTextExtentPoint32(pDc->GetHDC(), L"AaLl86", 6, &fontSize);
	tRect.top += fontSize.cy + 20;
	tRect.left += 20;
	tRect.right -= 40;
	LPTSTR mainTxt = L"This program will check and install " COMPANYNAME L" x86 Memory bootkit.\r\n"
		L"It will enable OS ability to use all real system memory (and not limit only to 4 Gb) "
		L"\r\nPlease read \"Readme\" file before proceed, and very important:\r\n";
	GetTextExtentPoint32(pDc->GetHDC(), mainTxt, wcslen(mainTxt), &fontSize);
	pDc->DrawTextW(mainTxt, -1, &tRect, DT_LEFT | DT_WORDBREAK);
	pDc->SelectObject(&txtBoldFont);
	tRect.top += (fontSize.cy * 3) + 6;
	pDc->DrawText(L"USE IT AT YOUR OWN RISK!", -1, &tRect, DT_CENTER | DT_WORDBREAK);
	pDc->SelectObject(&txtFont);

	// Draw OS Version Information
	pDc->SetTextColor(RGB(0,0,164));
	mainTxt = L"Current Windows OS Version: ";
	tRect.top += fontSize.cy + 20;
	pDc->DrawTextW(mainTxt, -1, &tRect, DT_LEFT | DT_INTERNAL);
	GetTextExtentPoint32(pDc->GetHDC(), mainTxt, wcslen(mainTxt), &fontSize);
	if (!g_bRightOs) {
		pDc->SelectObject(&txtBoldFont);
		pDc->SetTextColor(RGB(200,0,0));
		tRect.left += (100 + fontSize.cx);
		if (!g_bSecureBootEnabled) 
			pDc->DrawText(L"UNSUPPORTED OS!", -1, &tRect, DT_LEFT | DT_INTERNAL);
		else {
			tRect.left -= 60;
			pDc->DrawText(L"UNSUPPORTED SECURE BOOT!", -1, &tRect, DT_LEFT | DT_INTERNAL);
			tRect.left += 60;
		}
		tRect.left -= (100 + fontSize.cx);
		pDc->SelectObject(&txtFont);
	}
	tRect.left += 24;
	tRect.top += fontSize.cy + 2;
	pDc->SetTextColor(RGB(0,0,0));
	pDc->DrawTextW(m_strOsVersion, -1, &tRect, DT_LEFT | DT_INTERNAL);

	// Draw current bootkit state
	pDc->SetTextColor(RGB(0,0,164));
	mainTxt = L"Current Bootkit State: ";
	tRect.top += fontSize.cy + 8;
	tRect.left -= 24;
	pDc->DrawTextW(mainTxt, -1, &tRect, DT_LEFT | DT_INTERNAL);
	GetTextExtentPoint32(pDc->GetHDC(), mainTxt, wcslen(mainTxt), &fontSize);
	tRect.left += fontSize.cx + 6;
	pDc->SetTextColor(RGB(0,0,0));
	switch (this->g_btkState) {
		case BOOTKIT_NOT_PRESENT:
			pDc->DrawTextW(L"Not Installed", -1, &tRect, DT_LEFT | DT_INTERNAL);
			break;
		case BOOTKIT_INSTALLED:
			pDc->DrawTextW(L"Installed and Active", -1, &tRect, DT_LEFT | DT_INTERNAL);
			break;
		case BOOTKIT_DAMAGED:
			pDc->DrawTextW(L"Damaged", -1, &tRect, DT_LEFT | DT_INTERNAL);
			break;
	}
	
	// Draw logo picture
	CPoint logoPos(500, 120);
	CSize logoSize(96, 96);
	if (m_gpLogoPct)
		m_gpLogoPct->Draw(pDc, logoPos, logoSize);

	titleLogFont.lfWeight = FW_BOLD; 
	titleLogFont.lfHeight = GetHeightFromPt(18);
	titleLogFont.lfItalic = TRUE;
	titleFont.Detach();
	titleFont.CreateFontIndirect(&titleLogFont);
	cRect = GetClientRect();
	tRect = GetDlgItem(IDOK)->GetWindowRect();
	ScreenToClient(tRect);
	GetTextExtentPoint32(pDc->GetHDC(), L"GPT Mode", 8, &fontSize);

	cRect.top = tRect.top - 6;
	pDc->SelectObject(&titleFont);
	pDc->SetTextColor(0);
	if (g_BiosType == BIOS_TYPE_GPT)
		pDc->DrawText(L"GPT Mode", -1, cRect, DT_INTERNAL | DT_CENTER);
	else 
		pDc->DrawText(L"MBR Mode", -1, cRect, DT_INTERNAL | DT_CENTER);

	// Draw version information
	CVersionInfo verInfo(GetModuleHandle(NULL));
	TCHAR verStr[0x80] = {0};
	wsprintf(verStr, L"Version %s", verInfo.GetFileVersionString(false));
	cRect = GetClientRect();
	cRect.right -= 6; cRect.top = (cRect.bottom-=2) - 20;
	pDc->SelectObject(&txtVerFont);
	pDc->SetTextColor(RGB(48, 48, 48));			// Dark gray color
	pDc->DrawText(verStr, -1, cRect, DT_RIGHT | DT_INTERNAL);
}

BOOL CBtkDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	if (HIWORD(wParam) != BN_CLICKED)
		// Process only menu and BN_CLICKED messages
		return FALSE;

	switch (LOWORD(wParam)) {
		case IDM_FILE_EXIT:
			OnCancel();
			break;
		case IDM_FILE_INSTALL:
			OnOK();
			break;
		case IDM_FILE_OPTIONS: {
			CDlgOptions opts(this);
			opts.SetBtkSetupInstance(g_pBtkMbrSetup);
			opts.DoModal();
			break;
		}
		case ID_HELP_ABOUT: {
			CDlgAbout about(this);
			about.DoModal();
			break;
		}
		case ID_HELP_HELP:
			// Show HTML Help
			ShowHelp();
			break;
		case ID_HELP_GENERATE_REPORT:
			// Create and send a system report
			CreateAndSendSystemReport();
			break;
		case IDC_BTNFLOPPY:
		case IDC_BTNMBR:
		case IDC_BTNVBR:
			SetBtkStateData();
			break;
	}
	return TRUE;
}

// Move a GDI object in this window
bool CBtkDialog::MoveObject(int nResId, int x, int y) {
	CWnd * obj = GetDlgItem(nResId);
	if (!obj) return false;
	CRect objRect = obj->GetWindowRect();
	ScreenToClient(objRect);
	int width = objRect.Width();
	int height = objRect.Height();
	if (x == -1) x = objRect.left;
	if (y == -1) y = objRect.top;
	CRect destRect(x, y, x + width, y + height);
	obj->MoveWindow(destRect, 1);
	return true;
}

// Modify menù and button data according to bootkit State
void CBtkDialog::SetBtkStateData() {
	CMenu * mainMenu = GetMenu();
	_ASSERT(mainMenu);

	if (g_BiosType == BIOS_TYPE_GPT) {
		// UEFI Bios disk type
		m_VbrRadioBtn.ShowWindow(FALSE);
		MoveObject(IDC_BTNFLOPPY, 114, 290);
		MoveObject(IDC_REMDRVLIST, -1, 290); 
		switch (g_btkState) {
			case BOOTKIT_NOT_PRESENT:
			case BOOTKIT_STATE_UNKNOWN:
				// Leave all info as default
				m_MbrRadioBtn.SetWindowText(L"Install as new Windows UEFI Boot Launcher");
				mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Install");
				break;
			case BOOTKIT_INSTALLED:
				mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Remove");
				m_MbrRadioBtn.SetWindowText(L"Remove UEFI Bootkit and restore System defaults");
				if (m_MbrRadioBtn.GetCheck() == BST_CHECKED)
					mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Remove");
				else
					mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Install");
				break;
			case BOOTKIT_DAMAGED:
				m_MbrRadioBtn.SetWindowText(L"Reinstall as Windows UEFI Boot Launcher");
				if (m_MbrRadioBtn.GetCheck() == BST_CHECKED || m_VbrRadioBtn.GetCheck() == BST_CHECKED)
					mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Reinstall");
				else
					mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Install");
				break;
		}
		#ifndef _DEBUG
		if (!g_bRightOs)
			m_MbrRadioBtn.EnableWindow(FALSE);
		#endif
	}

	else {
		// MBR Bios disk type, default one
		if (g_btkState == BOOTKIT_NOT_PRESENT ||  g_btkState == BOOTKIT_STATE_UNKNOWN) {
			// Leave all info as default
			m_MbrRadioBtn.SetWindowText(L"Install on System Boot Disk as MBR Bootkit");
			m_VbrRadioBtn.SetWindowText(L"Install on System Boot Disk as new Boot Sector");
			mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Install");
		}
		else if (g_btkState == BOOTKIT_INSTALLED) {
			mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Remove");
			m_MbrRadioBtn.SetWindowText(L"Remove MBR Bootkit and restore System defaults");
			m_VbrRadioBtn.SetWindowText(L"Remove VBR Bootkit and restore System defaults");
			if (m_MbrRadioBtn.GetCheck() == BST_CHECKED || m_VbrRadioBtn.GetCheck() == BST_CHECKED)
				mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Remove");
		}
		else if (g_btkState == BOOTKIT_DAMAGED) {
			m_MbrRadioBtn.SetWindowText(L"Reinstall on System Boot Disk as MBR Bootkit and Repair");
			m_VbrRadioBtn.SetWindowText(L"Reinstall on System Boot Disk as new Boot Sector and Repair");
			if (m_MbrRadioBtn.GetCheck() == BST_CHECKED || m_VbrRadioBtn.GetCheck() == BST_CHECKED)
				mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Repair");
		}

		if (g_btkState == BOOTKIT_INSTALLED || g_btkState == BOOTKIT_DAMAGED) {
			if (g_uchMbrActiveBtkType == 2) {
				m_VbrRadioBtn.EnableWindow(TRUE);
				if (m_MbrRadioBtn.GetCheck()) m_MbrRadioBtn.SetCheck(FALSE);
				m_MbrRadioBtn.EnableWindow(FALSE);
			} else {
				m_MbrRadioBtn.EnableWindow(TRUE);
				if (m_VbrRadioBtn.GetCheck()) m_VbrRadioBtn.SetCheck(FALSE);
				m_VbrRadioBtn.EnableWindow(FALSE);
			}
		} else {
			#ifndef _DEBUG
			if (g_bRightOs) {
			#endif
				m_MbrRadioBtn.EnableWindow(TRUE);
				m_VbrRadioBtn.EnableWindow(TRUE); 
			#ifndef _DEBUG
			} else {
				m_MbrRadioBtn.EnableWindow(FALSE);
				m_VbrRadioBtn.EnableWindow(FALSE);
			}
			#endif
		}

		if (m_FloppyRadioBtn.GetCheck())
			mainMenu->ModifyMenu(IDM_FILE_INSTALL, MF_BYCOMMAND, IDM_FILE_INSTALL, L"&Make disk");
	}	// END OF Mbr Type code
}


void CBtkDialog::OnOK() {
	bool retVal = false;
	if (!g_Log.IsOpened())
		g_Log.Open(g_strLogFile);

	this->EnableWindow(FALSE);
	Sleep(200);
	if (m_MbrRadioBtn.GetCheck() == BST_CHECKED) {
		if (g_BiosType == BIOS_TYPE_GPT)
			// GPT Fixed drive setup
			retVal = LaunchGptFixedSetup();
		else
			// Classical MBR Fixed drive setup
			retVal = LaunchMbrFixedSetup();
	}
	else if (m_FloppyRadioBtn.GetCheck() == BST_CHECKED) {
		TCHAR volName[48] = {0};
		m_RemDrvList.GetLBText(m_RemDrvList.GetCurSel(), volName);
		if (g_BiosType == BIOS_TYPE_GPT)
			// GPT Fixed drive setup
			MessageBox(L"The setup type you have chosen is not yet supported on this version!", APPTITLE, MB_ICONEXCLAMATION);
		else
			// Classical MBR Removable device setup
			retVal = LaunchMbrRemovableSetup(volName);
	}
	else if (m_VbrRadioBtn.GetCheck() == BST_CHECKED) {
		// Fixed VBR Setup
		if (g_BiosType != BIOS_TYPE_GPT)
			retVal = LaunchVbrFixedSetup();
	}
	else {
		MessageBox(L"Please select an option.", APPTITLE, MB_ICONINFORMATION);
		EnableWindow(TRUE);
		return;
	}

	// Get new state
	if (g_pBtkMbrSetup)
		g_btkState = g_pBtkMbrSetup->GetBootkitState(g_iSysBootDiskIdx, 
		(CBootkitMbrSetup::BOOTKIT_SETUP_TYPE*)&g_uchMbrActiveBtkType);
	else if (g_pBtkGptSetup)
		g_btkState = g_pBtkGptSetup->GetBootkitState(0);
	SetBtkStateData();

	// Invalidate myself
	this->Invalidate();
	Sleep(300);
	this->EnableWindow(TRUE);
}

void CBtkDialog::OnCancel() {
	EndDialog(IDCANCEL);
}


// Initialize/Update Removable drives list in Removable Combo Box
bool CBtkDialog::UpdateRemDriveList(void)
{
	DWORD driveMap = 0;							// Logical drive map
	UINT driveType = 0;							// Current drive type
	BOOL remCtrlsState = FALSE;					// Removable controls state
	WCHAR curDrv[4] = { L'A', L':', L'\\', 0 };
	if (!m_RemDrvList.IsWindow())
		return false;

	driveMap = GetLogicalDrives();
	m_RemDrvList.ResetContent();
	for (int i = 0; i < (sizeof(DWORD) * 8); i++) {
		if ((driveMap & (1 << i)) == 0) 
			// This drive doesn't exists
			continue;
		curDrv[0] = L'A' + (WCHAR)i;
		driveType = GetDriveType(curDrv);
		if (driveType  == DRIVE_REMOVABLE) {
			// Found a removable drive, add it to list
			m_RemDrvList.AddString(curDrv);	
		}
	}

	// Enable / disable removable devices controls
	if (m_RemDrvList.GetCount() < 1) {
		remCtrlsState = FALSE;
		m_RemDrvList.AddString(L"None");
		m_FloppyRadioBtn.SetCheck(FALSE);
	} else
		remCtrlsState = TRUE;

	m_RemDrvList.EnableWindow(remCtrlsState);
	m_FloppyRadioBtn.EnableWindow(remCtrlsState);
	return true;
}

// Open Memory bootkit help file and show it via Web browser
void CBtkDialog::ShowHelp(void)
{
	#define  HELP_FILE_NAME L"x86_Btk_Help.html"
	TCHAR helpFilePath[MAX_PATH] = {0};
	DWORD retVal = FALSE;
	LPTSTR slashPtr = NULL;

	retVal = GetModuleFileName(NULL, helpFilePath, MAX_PATH);
	if (retVal) slashPtr = wcsrchr(helpFilePath, L'\\');
	else {
		MessageBox(L"Internal GetModuleFileName error.", APPTITLE, MB_ICONERROR);
		return;
	}
	if (slashPtr) slashPtr[1] = 0;
	wcscat_s(helpFilePath, MAX_PATH, HELP_FILE_NAME);

	if (!FileExist(helpFilePath)) {
		// TODO: Place here blog post help code
		MessageBox(APPTITLE L" html help file not found! \r\nUnable to show help content!", APPTITLE, MB_ICONEXCLAMATION);
		return;
	}
	retVal = (DWORD)ShellExecute(m_hWnd, L"open", helpFilePath, NULL, NULL, SW_SHOW);
	if (retVal <= 32)
		MessageBox(L"Unable to show help content! Shell API error.", APPTITLE, MB_ICONERROR);
}

// Launch system report creation and send
void CBtkDialog::CreateAndSendSystemReport(void)
{
	OPENFILENAME ofn = {0};
	TCHAR outFile[MAX_PATH] = {0};			// Output file full path
	DWORD lastErr = 0;						// Last win32/Commdlg error
	BOOL retVal = 0;						// Win32 returned value
	HCURSOR curCursor = NULL,				// Current cursor
		waitCursor = NULL;					// Wait cursor
	#define MAPI_USER_ABORT 1

	// Load cursors
	waitCursor = LoadCursor(NULL, IDC_WAIT);
	curCursor = GetCursor();

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = L"Bootkit log files (*.LOG)\0*.log\0All files (*.*)\0*.*\0\0";
	ofn.lpstrFile = outFile;
	ofn.nMaxFile = COUNTOF(outFile);
	ofn.lpstrTitle = L"Please specify a file used to store report...";
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"log";
	
	retVal = GetSaveFileName(&ofn);
	lastErr = CommDlgExtendedError();

	if (!retVal){
		if (lastErr != 0) 
			MessageBox(L"Error while showing Save file dialog box!", APPTITLE, MB_ICONEXCLAMATION);
		return;
	}

	retVal = g_sysAnalyzer.GenerateSystemReport(outFile, g_iSysBootDiskIdx);
	if (!retVal) {
		MessageBox(L"Error while creating system report file! Please read log file for details...", APPTITLE, MB_ICONERROR);
		return;
	}

	// How do I have to continue?
	LPTSTR contentStr = NULL;
	LPTSTR mainStr = L"System report file successfully generated!";
	LPTSTR szFooter = NULL;
	int nClickedBtn = 0;
	HRESULT hr = S_OK;
	
	if (wcscmp(COMPANYNAME, L"Saferbytes") == 0)
		szFooter = L"For any other information visit <a href=\"http://www.saferbytes.it\">www.saferbytes.it</a>";
	else
		szFooter = L"For any other information visit <a href=\"http://www.aall86.altervista.org\">www.aall86.altervista.org</a>";

	TASKDIALOG_BUTTON aCustomButtons[] = {
	  { 1001, L"Send report to Saferbytes\n"
			  L"It will be analyzed by Saferbytes experts..."},
	  { 1002, L"Don't send it" },
	};

	contentStr = new TCHAR[256];
	wsprintf(contentStr, L"You can open and read this report or, if you have any problems with " APPTITLE
		L" you can send it to " COMPANYNAME L". This could aid " COMPANYNAME L" experts to resolve your issue."
		L" By the way please send it ONLY if you really encountered issues.");

	TASKDIALOGCONFIG tdc = {0};
	tdc.cbSize = sizeof(tdc);
	tdc.hwndParent = this->m_hWnd;
	tdc.hInstance = GetModuleHandle(NULL);
	tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS | TDF_ENABLE_HYPERLINKS;
	tdc.cxWidth = 200;
//		tdc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
	tdc.pszWindowTitle = APPTITLE L" System Report";
	tdc.pszMainIcon = TD_INFORMATION_ICON;
	tdc.pszMainInstruction = mainStr;
	tdc.pszContent = contentStr;
	tdc.pButtons = aCustomButtons;
	tdc.cButtons = COUNTOF(aCustomButtons);
	//tdc.pszExpandedInformation = szExtraInfo;
	tdc.pszFooter = szFooter;
	tdc.pszFooterIcon =  TD_INFORMATION_ICON;
	tdc.pfCallback = CBtkDialog::TDCallback;
	tdc.lpCallbackData = (LONG_PTR) this;

	hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
	delete[] contentStr;

	if (!SUCCEEDED(hr) || nClickedBtn != 1001) 
		// Stop procedure and exit
		return;

	// If I am here, send mail.
	LPSTR outFileA = new CHAR[MAX_PATH];
	sprintf_s(outFileA, MAX_PATH, "%S", outFile);
	SetCursor(waitCursor);
	retVal = SendMailTo(m_hWnd, "andrea.allievi@saferbytes.it", "info@saferbytes.it", 
		"x86 Memory Bootkit Report", "Please write here a detailed description of your problem....", outFileA);
	SetCursor(curCursor);

	if (retVal != ERROR_SUCCESS && retVal != MAPI_USER_ABORT) {
		MessageBox(L"Oops, some errors occured in communication with default client Mail application.\r\n"
			L"Please, manually send a mail to \"andrea.allievi@saferbytes.it\" or \"info@saferbytes.it\", "
			L"attaching report file and a brief description of encountered issue.", APPTITLE, MB_ICONEXCLAMATION);
	}
	delete[] outFileA;
}

#pragma region MBR Graphical Setup / Remove functions
// Start analysis and then install/remove Bootkit on main hard disk
bool CBtkDialog::LaunchMbrFixedSetup() {
	bool bRemove = false;				// TRUE if I want to remove Bootkit
	BOOL retVal = FALSE;				// Returned value
	DWORD stdMbrSectNum = 0;			// Standard MBR sector number

	bRemove = (g_btkState == BOOTKIT_INSTALLED);
	if (bRemove) {
		retVal = g_pBtkMbrSetup->MbrRemoveBootkit(g_iSysBootDiskIdx);
		if (retVal) 
			MessageBox(L"X86 Memory bootkit was successfully removed from this system!\r\n"
			L"Please restart your PC to apply new changes...", 
			APPTITLE, MB_ICONINFORMATION);
		else
			MessageBox(L"One or more errors were occured while removing Bootkit from system MBR.\r\n"
			L"Please, see log file for details...", APPTITLE, MB_ICONERROR);
		return (retVal != FALSE);
	}

	// Get current MBR type
	BYTE mbrType = g_pBtkMbrSetup->IsStandardMbr(g_iSysBootDiskIdx, &stdMbrSectNum);

	// If System Mbr is unknown raise global flag
	if (mbrType == 0x02) g_bUnkMbr = true;

	if ((mbrType != 0x01 && stdMbrSectNum == (DWORD)-1) || mbrType == (BYTE)-1) {
		// Unknown System MBR, show a message to the user
		LPTSTR contentStr = NULL;
		LPTSTR mainStr = L"Unknown MBR found on system boot disk!";
		HRESULT hr = 0;
		int nClickedBtn = 0;

		TASKDIALOG_BUTTON aCustomButtons[] = {
		  { 1001, L"Install Bootkit anyway\n" L"I perfectly know all risk!"},
		  { 1002, L"Stop memory bootkit installation" },
		};

		contentStr = new TCHAR[256];
		wsprintf(contentStr, L"This setup application has detected an unknown MBR on system partition. "
			L"Installing Memory bootkit can bring unpredictable results: system could work well BUT it could also not be able to startup.\r\n"
			L"What would you like to do?");

		TASKDIALOGCONFIG tdc = {0};
		tdc.cbSize = sizeof(tdc);
		tdc.hwndParent = this->m_hWnd;
		tdc.hInstance = GetModuleHandle(NULL);
		tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
		tdc.cxWidth = 200;
//		tdc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
		tdc.pszWindowTitle = APPTITLE L"Setup";
		tdc.pszMainIcon = TD_WARNING_ICON;
		tdc.pszMainInstruction = mainStr;
		tdc.pszContent = contentStr;
		tdc.pButtons = aCustomButtons;
		tdc.cButtons = COUNTOF(aCustomButtons);

		hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
		delete[] contentStr;

		if (!SUCCEEDED(hr) || nClickedBtn != 1001) 
			// Stop procedure and exit
			return false;
	}

	// I would like to install bootkit on System Hard disk
	retVal = g_pBtkMbrSetup->MbrInstallBootkit(g_iSysBootDiskIdx);

	if (retVal)
		MessageBox(L"X86 Memory bootkit was successfully installed on this system!\r\n"
		L"Now you have to restart your pc to unlock all memory...", 
		APPTITLE, MB_ICONINFORMATION);
	else
		MessageBox(L"One or more errors were occured while installing Bootkit on system MBR.\r\n"
			L"Please, see log file for details...", APPTITLE, MB_ICONERROR);

	return (retVal != FALSE);
}

// Start analysis and then install Bootkit on a removable volume
bool CBtkDialog::LaunchMbrRemovableSetup(LPTSTR volName)
{
	BOOTKIT_STATE btkState =						// Bootkit state in volume
		BOOTKIT_STATE_UNKNOWN;
	CBootkitMbrSetup::DISK_TYPE dskType =				// Volume physical disk type
		CBootkitMbrSetup::DISK_TYPE_UNKNOWN;
	TCHAR realDosVolName[0x20] = {0};				// Real DOS device name
	BOOLEAN bRemove = FALSE, bInstall = TRUE;
	bool retVal = FALSE;							// Return value

	// Build real DOS Device name to pass to CBootkitMbrSetup
	wcscpy_s(realDosVolName, COUNTOF(realDosVolName), L"\\\\.\\A:");
	if (volName[0] == L'\\') wcsncpy(realDosVolName, volName, COUNTOF(realDosVolName) -1);
	else realDosVolName[4] = volName[0];

	// Get if drive is readable
	retVal = g_pBtkMbrSetup->IsDeviceReadable(realDosVolName);
	if (!retVal) {
		MessageBox(L"Error! Specified removable device is not ready.", APPTITLE, MB_ICONEXCLAMATION);
		return false;
	}

	// Step 0. Get if bootkit is already installed in removable volume
	btkState = g_pBtkMbrSetup->GetVolBootkitState(realDosVolName);
	dskType = g_pBtkMbrSetup->GetVolumeDiskType(realDosVolName);

	if (btkState == BOOTKIT_INSTALLED) {
		LPTSTR contentStr = NULL;
		LPTSTR mainStr = COMPANYNAME L" " APPTITLE L" already installed!";
		HRESULT hr = 0;
		int nClickedBtn = 0;

		TASKDIALOG_BUTTON aCustomButtons[] = {
		  { 1001, L"Re&move bootkit from removable device" },
		  { 1002, L"Re&install bootkit in removable device" },
		};

		contentStr = new TCHAR[256];
		wsprintf(contentStr, APPTITLE L" is already installed on \"%.2s\" drive.\r\n"
			L"What would you like to do?", volName);

		TASKDIALOGCONFIG tdc = {0};
		tdc.cbSize = sizeof(tdc);
		tdc.hwndParent = this->m_hWnd;
		tdc.hInstance = GetModuleHandle(NULL);
		tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
		tdc.cxWidth = 200;
		tdc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
		tdc.pszWindowTitle = APPTITLE;
		tdc.pszMainIcon = MAKEINTRESOURCE(TD_INFORMATION_ICON);
		tdc.pszMainInstruction = mainStr;
		tdc.pszContent = contentStr;
		tdc.pButtons = aCustomButtons;
		tdc.cButtons = COUNTOF(aCustomButtons);

		hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
		delete[] contentStr;

		if (SUCCEEDED(hr) && nClickedBtn == 1001) {
			// Remove bootkit
			bRemove = TRUE; bInstall = FALSE; }
		else if (SUCCEEDED(hr) && nClickedBtn == 1002) 
			// Reinstall bootkit
			bRemove = TRUE;
		else 
			return false;
	}

	if (bRemove) {
		retVal = g_pBtkMbrSetup->VolRemoveBootkit(realDosVolName);
		if (!retVal) {
			MessageBox(L"Error! Removal process has encountered some errors! Unable to continue.\r\n"
				L"See log file for details...", APPTITLE, MB_ICONERROR);
			return false;
		} else {
			TCHAR text[0x100] = {0};
			wsprintf(text, L"X86 Memory bootkit was successfully removed from \"%.2s\" volume!", 
				wcsrchr(realDosVolName, L':') - 1);
			g_Log.WriteLine(text);
			if (!bInstall) {
				MessageBox(text, APPTITLE, MB_ICONINFORMATION);
				return true;
			}
		}
	}

	if (bInstall && 
		(dskType != CBootkitMbrSetup::DISK_TYPE_CLEAN) &&
		!(bRemove && dskType == CBootkitMbrSetup::DISK_TYPE_MBR)) {
		// Now analyze Volume type
		LPTSTR contentStr = NULL;
		LPTSTR mainStr = L"Removable device Warning!";
		HRESULT hr = 0;
		int nClickedBtn = 0;

		TASKDIALOG_BUTTON aCustomButtons[] = {
		  { 1001, L"OK, I know it, proceed anyway!\n"
				  L"All data stored inside can be destroyed..." },
		  { 1002, L"NO, Please stop and abort" },
		};

		contentStr = new TCHAR[256];
		if (dskType == CBootkitMbrSetup::DISK_TYPE_MBR) {
			wsprintf(contentStr, COMPANYNAME L" " APPTITLE L" will be installed on \"%.2s\" drive.\r\n"
			L"Warning! Data on removable disk may be damaged or erased.\r\n"
			L"Are you sure that you made a backup?\r\nHow would you like to continue?", volName);
		} else
			wsprintf(contentStr, COMPANYNAME L" " APPTITLE L" will be installed on \"%.2s\" drive.\r\n"
			L"Warning! All data on removable disk will be ERASED.\r\n"
			L"Are you sure that you made a backup?\r\nHow would you like to continue?", volName);

		TASKDIALOGCONFIG tdc = {0};
		tdc.cbSize = sizeof(tdc);
		tdc.hwndParent = this->m_hWnd;
		tdc.hInstance = GetModuleHandle(NULL);
		tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
		tdc.cxWidth = 200;
		//tdc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
		tdc.pszWindowTitle = APPTITLE;
		tdc.pszMainIcon = MAKEINTRESOURCE(TD_WARNING_ICON);
		tdc.pszMainInstruction = mainStr;
		tdc.pszContent = contentStr;
		tdc.pButtons = aCustomButtons;
		tdc.cButtons = COUNTOF(aCustomButtons);

		hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
		delete[] contentStr;

		if (!(SUCCEEDED(hr) && nClickedBtn == 1001)) 
			// Stop procedure
			return false;
	}

	// Now do actual installation
	if (bInstall) 
		retVal = g_pBtkMbrSetup->VolInstallBootkit(realDosVolName);
	
	if (retVal) {
		TCHAR text[0x100] = {0};
		wsprintf(text, APPTITLE L" was successfully installed on \"%.2s\" removable volume!", 
				wcsrchr(realDosVolName, L':') - 1);
		g_Log.WriteLine(text);
		wcscat_s(text, COUNTOF(text), L" Now you can restart your PC and boot with this disk plugged in to unlock all Memory.");
		MessageBox(text, APPTITLE, MB_ICONINFORMATION);
	}
	else
	{
		MessageBox(L"Warning! There were some errors while installing X86 Memory bootkit...\r\n See Log file for details.",
			APPTITLE, MB_ICONERROR);
	}
	
	return retVal;
}
#pragma endregion

#pragma region UEFI Graphical Setup / Remove functions
// Start analysis and then install UEFI bootkit on a fixed drive 
bool CBtkDialog::LaunchGptFixedSetup(void) {
	bool bRemove = false;				// TRUE if I want to remove Bootkit
	BOOL retVal = FALSE;				// Returned value

	bRemove = (g_btkState == BOOTKIT_INSTALLED);
	if (bRemove) {
		retVal = g_pBtkGptSetup->RemoveEfiBootkit(0);
		if (retVal) 
			MessageBox(L"UEFI X86 Memory bootkit was successfully removed from this system!\r\n"
			L"Please restart your PC to apply new changes...", 
			APPTITLE, MB_ICONINFORMATION);
		else
			MessageBox(L"One or more errors were occured while removing UEFI Bootkit from system.\r\n"
			L"Please, see log file for details...", APPTITLE, MB_ICONERROR);
		return (retVal != FALSE);
	}

	// I would like to install bootkit on System Hard disk
	retVal = g_pBtkGptSetup->InstallEfiBootkit(NULL);

	if (retVal)
		MessageBox(L"UEFI X86 Memory bootkit was successfully installed on this system!\r\n"
		L"Keep in mind however that some Windows Updates can replace Bootkit files... "
		L"If this will happen in future (and your system will not support memory above 4 GB), "
		L"you just only have to reinstall.\r\n\r\n"
		L"Please restart your pc to unlock all memory...", 
		APPTITLE, MB_ICONINFORMATION);
	else
		MessageBox(L"One or more errors were occured while installing UEFI Bootkit on system.\r\n"
			L"Please, see log file for details...", APPTITLE, MB_ICONERROR);

	return (retVal != FALSE);
}

#pragma endregion

#pragma region VBR Graphical Setup / Remove functions
// Start analysis and then install Bootkit on system boot partition VBR
bool CBtkDialog::LaunchVbrFixedSetup(void) {
	bool bRemove = false;				// TRUE if I want to remove Bootkit
	BOOL retVal = FALSE;				// Returned value

	bRemove = (g_btkState == BOOTKIT_INSTALLED);
	if (bRemove) {
		if (g_uchMbrActiveBtkType != 2) {
			MessageBox(L"Internal error. You can't remove VBR Bootkit if is installed as System MBR!",
				APPTITLE, MB_ICONERROR);
			return false;
		}
		retVal = g_pBtkMbrSetup->VbrRemoveBootkit(g_iSysBootDiskIdx);

		if (retVal) 
			MessageBox(APPTITLE L" was successfully removed from this system!\r\n"
			L"Please restart your PC to apply new changes...", 
			APPTITLE, MB_ICONINFORMATION);
		else
			MessageBox(L"One or more errors were occured while removing Bootkit from system MBR.\r\n"
			L"Please, see log file for details...", APPTITLE, MB_ICONERROR);
		return (retVal != FALSE);
	}

	// Do actual installlation
	retVal = g_pBtkMbrSetup->VbrInstallBootkit(g_iSysBootDiskIdx);

	if (retVal)
		MessageBox(APPTITLE L" was successfully installed on system boot partition!\r\n"
		L"Now you have to restart your pc to unlock all memory...", 
		APPTITLE, MB_ICONINFORMATION);
	else
		MessageBox(L"One or more errors were occured while installing Bootkit on system MBR.\r\n"
			L"Please, see log file for details...", APPTITLE, MB_ICONERROR);

	return (retVal != FALSE);
}
#pragma endregion 

// TaskdialogIndirect callback
HRESULT CALLBACK CBtkDialog::TDCallback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData) {
	UNREFERENCED_PARAMETER(wParam);
	CBtkDialog * thisDlg = (CBtkDialog*)dwRefData;
	switch ( uNotification )
	{
	case TDN_HYPERLINK_CLICKED:
		ShellExecute ( hwnd, _T("open"), (LPCWSTR) lParam, NULL, NULL, SW_SHOW );
		break;
	}

	return S_OK;
}
