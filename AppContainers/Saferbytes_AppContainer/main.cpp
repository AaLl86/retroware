///////////////////////////////////
// main.cpp

#include "stdafx.h"
#include "wincore.h"
#include "MainDlg.h"

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	// Initialize GDIPlus
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	INT_PTR res = 0;

	CWinApp theApp;

	CMainDlg dlg;
	dlg.DoModal();

}
