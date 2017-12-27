#pragma once
#include "Dialog.h"
#include <stdcontrols.h>

class CMainDlg :
	public CDialog
{
public:
	CMainDlg(UINT nResID, CWnd* pParent = NULL);
	CMainDlg();
	~CMainDlg(void);

protected:
	// This instance WindowProc procedure
	virtual BOOL DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnDraw(CDC* pDC);
	virtual BOOL OnInitDialog();

	#pragma region Dialog controls
private:
	CEdit m_appPathEdt;					// Application full path edit control
	CEdit m_appContSidEdt;				// App container package Sid edit control
	#pragma endregion
};

// Ottiene il valore nHeight per l'API CreateFont
int GetHeightFromPt(int PointSize);
