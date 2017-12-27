/**********************************************************************
 * 
 *  AaLl86 Windows x86 Memory Bootkit
 *  Installer application startup module
 *	Last revision: 
 *
 *  Copyright© 2012 by AaLl86
 *	All right reserved
 * 
 **********************************************************************/
#include "stdafx.h"
#include "DialogMain.h"
#include "BootkitGptSetup.h"
#include "SystemAnalyzer.h"

#pragma region Bootkit test functions
#ifdef _DEBUG
#include "VariousStuff.h"
ULONG GetCrcOfFile(LPTSTR fileName, ULONG start, ULONG len) {
	const ULONG crcStartOffset = start;
	const ULONG crcEndOffset = start + len;
	DWORD bytesRead = 0;
	BOOL retVal = 0;
	HANDLE hFile = NULL;
	BYTE buff[0x400] = {0};

	hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return 0;
	retVal = ReadFile(hFile, buff, 0x200, &bytesRead, NULL);
	CloseHandle(hFile);
	if (!retVal) return 0;
	ULONG crc = CCrc32::GetCrc(buff, crcStartOffset, crcEndOffset - crcStartOffset);
	return crc;
}

void DoTests() {
	// MBR CRC32 Tests
	TCHAR dbgStr[0x100] = {0};
	ULONG hpMbr = GetCrcOfFile(L"G:\\AaLl86 Projects\\Win7x86Bootkit\\Other MBRs\\HP MBR.bin", 0x2B, 0x120);
	ULONG hp2Mbr = GetCrcOfFile(L"G:\\AaLl86 Projects\\Win7x86Bootkit\\Other MBRs\\HP New MBR Pack.bin", 0x2B, 0x120);
	ULONG sevenMbr = GetCrcOfFile(L"G:\\Disassembly\\Windows 7\\Seven_MBR.bin", 0x0, 0x148);
	ULONG vistaMbr = GetCrcOfFile(L"G:\\Disassembly\\Vista SP2\\Vista MBR.bin", 0x0, 0x148);
	swprintf_s(dbgStr, COUNTOF(dbgStr), L"Hp Mbr CRC: 0x%08X, HP New Mbr CRC: 0x%08X, Seven Mbr CRC: 0x%08X, Vista Mbr CRC: 0x%08X\r\n",
		hpMbr, hp2Mbr, sevenMbr, vistaMbr);
	OutputDebugString(dbgStr);
}

#endif
#pragma endregion

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	// Initialize GDIPlus
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	INT_PTR res = 0;
#ifdef _DEBUG
	DoTests();
#endif 

	// Start Win32++
    CWinApp MyApp;

    // Create our dialog box window
	CBtkDialog mainDlg(IDD_DLGMAIN, NULL);
	res = mainDlg.DoModal();

    // Run the application's message loop
	return res;
}


