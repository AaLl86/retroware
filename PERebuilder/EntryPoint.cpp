/*		AaLl86 PeRebuilder
 *	Andrea Allievi, Private project
 *	Entrypoint.cpp - Implement PeRebuilder entry point and argument parsing
 *	Last revision: 17/11/2017
 *	Copyright© 2014 - Andrea Allievi (AaLl86)
 */
#include "stdafx.h"
#include "perebuilder.h"
#include <Winternl.h>

LONG64 GetValueFromString(LPWSTR lpStr) {
	LARGE_INTEGER value = {0};
	LPWSTR stopStr = NULL;

	if (lpStr[0] == L'0' && (lpStr[1] == L'x' || lpStr[1] == L'X')) {
		// Hex digits
		if (wcslen(lpStr) > 10) {
			LARGE_INTEGER value = {0};
			WCHAR strChar = lpStr[10];
			lpStr[10] = 0;
			value.HighPart = wcstoul(lpStr, &stopStr, 0x10);
			lpStr[10] = strChar;
			value.LowPart = wcstoul(&lpStr[10], &stopStr, 0x10);
		} else 
			value.LowPart = wcstoul(lpStr, &stopStr, 0x10);
	}
	else {
		// Normal integer
		value.LowPart = wcstoul(lpStr, &stopStr, 10);
	}
	return value.QuadPart;
}

int _tmain(int argc, _TCHAR* argv[])
{
	LPTSTR orgFile = NULL;						// Original file name
	LPTSTR destFile = NULL;						// Possible destination file
	LPTSTR baseAddrStr = NULL;					// Base PE RVA Address
	LPTSTR stopStr = NULL;						// Stop String pointer
	QWORD baseAddr = 0;							// PE File base address
	LPTSTR fileName = NULL;						// File name without path
	BOOLEAN forceSectionReloc = FALSE,			// TRUE if I need to FORCE the section relocation
		fixFileAlign = FALSE,					// TRUE if I need to check the file section alignment (useful for IDA 7)
		bArgParseError = FALSE;
	TCHAR answer = 0;							
	BOOL retVal = FALSE;

	SetConsoleColor(DARKGREEN);
	wprintf(L"AaLl86 PE Rebuilder v0.2\r\n\r\n");
	SetConsoleColor(DEFAULT_COLOR);

	for (int i = 1; i < argc; i++) {
		LPTSTR param = argv[i];
		if (param[0] == L'/' || param[0] == L'-') {
			// This is a command line switch
			if (_wcsicmp(param + 1, L"forceSectReloc") == 0) 
				forceSectionReloc = TRUE;
			// 13 November 2017 - Added support for section file alignment check
			else if (_wcsnicmp(param + 1, L"base", 4) == 0 &&
				param[5] == L':') {
				baseAddrStr =  &param[6];
				baseAddr = GetValueFromString(baseAddrStr);
			}
			else if (_wcsicmp(param + 1, L"fixFileAlign") == 0) 
				fixFileAlign = TRUE;
			else
				bArgParseError = TRUE;
		}
		else 
		{
			// this could be source or destination file
			if (orgFile)
				destFile = argv[i];
			else if (!destFile)
				orgFile = argv[i];
			else
				bArgParseError = TRUE;
		}
	}

	if (argc < 2 || bArgParseError) {
		cl_wprintf(L"Error! ", RED);
		wprintf(L"You have specified wrong parameters.\r\n"
			L"Usage: PERebuilder <PE File> [destfile] [/base:<PE_Base_Addr>] [/forceSectReloc] [/fixFileAlign]\r\n");
		wprintf(L"\r\nWhere:\r\n");
		wprintf(L"/base:<PE_Base_Addr> - Specify the new PE base address.\r\n");
		wprintf(L"/forcesectreloc - Force the moving of all PE sections from their VA to File offset.\r\n");
		wprintf(L"/fixfilealign - Check the minimum size of the section file alignment (useful for IDA 7).\r\n");
		wprintf(L"Press any key to exit...\r\n");
		_getwch();
		return -1;
	}

	if (!FileExist(orgFile) || !CPeRebuilder::IsValidPE(orgFile)) {
		wprintf(L"Error! Specified file doesn't exist or is not a valid PE. \r\nPress any key to exit.");
		_getwch();
		return -1;
	}

	if (!destFile) 
		destFile = orgFile;

	// Get File Name without path
	fileName = wcsrchr(destFile, L'\\');
	if (fileName) fileName++;
	else fileName = destFile;

	CPeRebuilder rebuild;
	rebuild.Open(orgFile);
	wprintf(L"This program checks PE file integrity, rebuilds PE Sections and other stuff...\r\n");
	wprintf(L"Specified PE File: ");
	cl_wprintf(fileName, YELLOW);
	wprintf(L"\r\n");

	if (!baseAddr) {
		wprintf(L"Do you wish to continue? [Y/N] ");
		rewind(stdin);
		wscanf_s(L"%c", &answer);

		if (answer != L'y' && answer != L'Y') {
			rebuild.Close();
			wprintf(L"Execution ended!\r\nPress any key to exit...\r\n");
			_getwch();
			return 0;
		}

		baseAddrStr = new TCHAR[0x20];
		//baseAddrStr[0] = L'0';
		//baseAddrStr[1] = L'x';
		baseAddr = (DWORD)rebuild.GetImageBaseRVA();
		if (rebuild.GetPeType() == PE_TYPE_AMD64)
			wprintf(L"Memory Mapped PE base address [0x%08X'%08X]: ", (DWORD)(baseAddr >> 32), (DWORD)baseAddr);
		else
			wprintf(L"Memory Mapped PE base address [0x%08X]: ", baseAddr);
		rewind(stdin);
		baseAddrStr[0] = getwchar();
		if (baseAddrStr[0] != L'\r' && baseAddrStr[0] != L'\n') {
			wscanf_s(L"%s", &baseAddrStr[1], 0x20);
			baseAddr = GetValueFromString(baseAddrStr);
			if (!baseAddr) baseAddr = rebuild.GetImageBaseRVA();
		}
	}

	retVal = rebuild.RebuildPe(baseAddr, destFile, forceSectionReloc, fixFileAlign);
	if (retVal) 
		wprintf(L"\r\nRebuilding of PE File \"%s\" success! See log file for details...\r\n\r\n", fileName);
	else
		wprintf(L"\r\nThere were errors while rebuilding PE File \"%s\"!\r\nSee application log file for details...\r\n\r\n", fileName);
	
	rebuild.Close();
	wprintf(L"Press any key to exit...\r\n");
	rewind(stdin);
	_getwch();

	return 0;
}

void cl_wprintf(LPTSTR string, ConsoleColor c) {
	SetConsoleColor(c);
	wprintf(string);
	SetConsoleColor(DEFAULT_COLOR);
}

void SetConsoleColor(ConsoleColor c) {
	CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
	static WORD ConsoleDefAttr = 0;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	if (ConsoleDefAttr == 0) {
		GetConsoleScreenBufferInfo(hCon, &csbi);
		ConsoleDefAttr = csbi.wAttributes;
	}
	if (c == DEFAULT_COLOR) 
		c = (ConsoleColor)(ConsoleDefAttr & 0xF);
	SetConsoleTextAttribute(hCon, (WORD)c | (ConsoleDefAttr & 0xF0));
}

// Get if a file Exist
bool FileExist(LPTSTR fileName) {
	WIN32_FIND_DATA wfData = {0};
	HANDLE h = NULL;

	// Get if file is a ADS
	LPTSTR slash = wcsrchr(fileName, L'\\');
	if (slash) slash++;
	else slash = fileName;
	LPTSTR colon = wcsrchr(slash, L':');
	if (colon) colon[0] = 0;

	h = FindFirstFile(fileName, &wfData);
	if (colon) colon[0] = L':';

	if (h == INVALID_HANDLE_VALUE) 
		return false;
	else {
		FindClose(h);
		return true;
	}
}
