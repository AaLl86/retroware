#pragma once
#include <dialog.h>
#include <stdcontrols.h>
#include "picture.h"
#include "log.h"

class CDlgAbout :
	public CDialog
{
public:
	CDlgAbout(CWnd * parent = NULL);
	~CDlgAbout(void);

protected:
	// Initialize Dialog event
	virtual BOOL OnInitDialog();
	// This instance WindowProc procedure
	virtual BOOL DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	// PAINT Event function
	virtual void OnDraw(CDC* pDC);
	//Ok Routine
	virtual void OnOK() { EndDialog(IDCANCEL); }
	// WM_COMMAND message handler
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	// System info button Click
	void OnSysInfo();
	// Mouse button down event
	void OnLButtonDown(DWORD keys, POINT point);
	// Mouse left button up event
	void OnLButtonUp(DWORD keys, POINT point);
	// Mouse move event
	void OnMouseMove(DWORD keys, POINT point);


private:
	// Saferbytes logo Image
	CPicture m_logoImg;
	// Current executable version info
	CVersionInfo g_verInfo;
	// Current link string rect
	CRect m_rectLink;
	// Default cursor
	HCURSOR m_hdefCur;
};
