#pragma once
#include <dialog.h>
#include <stdcontrols.h>
#include "BootkitMbrSetup.h"

class CDlgOptions :
	public CDialog
{
public:
	CDlgOptions(CWnd * parent = NULL);
	~CDlgOptions(void);
	
	void SetBtkSetupInstance(CBootkitMbrSetup * setupInstance) 
	{ g_pCurSetup = setupInstance; }

protected:
	// Initialize Dialog event
	virtual BOOL OnInitDialog();
	// This instance WindowProc procedure
	virtual BOOL DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	// PAINT Event function
	virtual void OnDraw(CDC* pDC);
	// Cancel routine
	virtual void OnCancel() { EndDialog(IDCANCEL); }
	//Ok Routine
	virtual void OnOK();

private:
	// Update user controls based on "setupInstance" setup data
	bool UpdateControls();

private:
	// Current associated Bootkit setup instance
	CBootkitMbrSetup * g_pCurSetup;

	#pragma region This window Controls
	// "Auto Detect" bootkit type radio button
	CButton g_mRdBtkTypeAuto;
	// "Only standard" bootkit type radio button
	CButton g_mRdBtkTypeStandard;
	// "Only compatible" bootkit type radio button
	CButton g_mRdBtkTypeCompatible;
	// "Don't use MBR type Identification" checkbox
	CButton g_mDontUseMbrType;
	#pragma endregion

};
