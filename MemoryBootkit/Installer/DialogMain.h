/**********************************************************************
 * 
 *  AaLl86 Windows x86 Memory Bootkit
 *  Installer main dialog box data structures and function prototypes
 *	Last revision: 
 *
 *  Copyright© 2012 by AaLl86
 *	All right reserved
 * 
 **********************************************************************/
#pragma once
#include <Dialog.h>
#include <stdcontrols.h>
#include <controls.h>
#include "log.h"
#include "BootkitMbrSetup.h"
#include "SystemAnalyzer.h"
#include "BootkitGptSetup.h"

#include "Picture.h"


class CBtkDialog: public CDialog {
public:
	CBtkDialog(UINT nResID, CWnd* pParent = NULL);
	CBtkDialog(LPCTSTR lpszResName, CWnd* pParent = NULL);
	virtual ~CBtkDialog();

	// Initialize ME
	void Initialize();

protected:
	virtual BOOL OnInitDialog();
	virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void OnOK();
	virtual void OnDraw(CDC* pDc);
	virtual void OnCancel();
	//virtual void OnPaint();

	// Move a GDI object in this window
	bool MoveObject(int nResId, int x, int y);

private:
	// Initialize Removable drives list in Removable Combo Box
	bool UpdateRemDriveList(void);
	// Start analysis and then install Bootkit on a removable volume
	bool LaunchMbrRemovableSetup(LPTSTR volName);
	// Start analysis and then install Bootkit on main hard disk
	bool LaunchMbrFixedSetup(void);
	// Start analysis and then install Bootkit on system boot partition VBR
	bool LaunchVbrFixedSetup(void);
	// Start analysis and then install UEFI bootkit on a fixed drive 
	bool LaunchGptFixedSetup(void);
	// Modify menù and button data according to bootkit State
	void SetBtkStateData();
	// Set a menu bitmap
	bool SetMenuItemBitmap(UINT mnuId, UINT bmpId);
	// Initialize and start log
	bool InitializeLog(LPTSTR logFileName = L"x86_Bootkit_Log.txt");
	// Open Memory bootkit help file and show it via Web browser
	void ShowHelp(void);
	// Launch system report creation and send
	void CreateAndSendSystemReport(void);
	// TaskdialogIndirect callback
	static HRESULT CALLBACK TDCallback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData);

private:
	// OS Version string (that will be printed on Main window) 
	LPTSTR m_strOsVersion;
	// Current log class instance
	CLog g_Log;
	// Current log filename
	LPTSTR g_strLogFile;
	// true if this is an unsupported OS
	bool g_bRightOs;

	// Make Mbr Installation Radio button
	CButton m_MbrRadioBtn;
	// Make Vbr installation Radio Button
	CButton m_VbrRadioBtn;
	// Make Floppy disk installation Radio Button
	CButton m_FloppyRadioBtn;
	// Combo box for removable drives
	CComboBox m_RemDrvList;

	// Bootkit MBR setup class instance
	CBootkitMbrSetup * g_pBtkMbrSetup;
	// Disk system boot device found index
	int g_iSysBootDiskIdx;

	// Bootkit GPT setup class instance
	CBootkitGptSetup * g_pBtkGptSetup;
	// Is secure boot enabled
	bool g_bSecureBootEnabled;

	// Current Bootkit State
	BOOTKIT_STATE g_btkState;
	// Current installed MBR Bootkit type (if any)
	BYTE g_uchMbrActiveBtkType;				// CBootkitMbrSetup::BOOTKIT_SETUP_TYPE

	// Logo picture
	CPicture * m_gpLogoPct;
	// Have I found an unknown MBR?
	bool g_bUnkMbr;
	// current System Bios Startup type
	BIOS_SYSTEM_TYPE g_BiosType;
	// Class instance system analyzer
	CSystemAnalyzer g_sysAnalyzer;
};