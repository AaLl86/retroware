/*
 *  Copyright (C) 2013 Saferbytes Srl - Andrea Allievi
 *	Filename: TestApp.cpp
 *	Implements some Appcontainer test routines
 *	Last revision: 05/30/2013
 *
 */
#include "stdafx.h"
#include "TestApp.h"
#include "AppContainer.h"
#include <Sddl.h>

bool GetTokenAppContainerState(bool * pbIsAppContainer, HANDLE hToken, bool bDisplayData) {
	BOOL retVal = FALSE;					// Returned value
	DWORD lastErr = 0;						// Last Win32 error
	BOOL bIsAppContainer = FALSE;			// TRUE if Token is an app container
	bool bCloseAtExit = false;				// TRUE if I have to close handle at exit
	DWORD retLen = 0;						// Returned bytes
	DWORD dwAppContNum = 0;					// App Container session number
	LPTSTR stringSid = NULL;				// Sid String 
	TCHAR sidName[128] = {0};				// Readable human Sid name

	if (!hToken) {
		retVal = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
		bCloseAtExit = true;
	}
	lastErr = GetLastError();
	if (!hToken) {
		if (bDisplayData) wprintf(L"Error %i! Unable to open current process token!\r\n", lastErr);
		return false;
	}

	// Call ZwQueryInformationToken to obtain actual Token AppContainer state
	retVal = ZwQueryInformationToken(hToken, TokenIsAppContainer, &bIsAppContainer, sizeof(BOOL), &retLen);
	if (retVal != STATUS_SUCCESS) return false;
	if (pbIsAppContainer) (*pbIsAppContainer) = (bIsAppContainer != FALSE);

	// Get App Container Session number
	retVal = ZwQueryInformationToken(hToken, TokenAppContainerNumber, &dwAppContNum, sizeof(DWORD), &retLen);
	if (retVal != STATUS_SUCCESS) return false;

	if (bIsAppContainer) {
		if (bDisplayData) {
			wprintf(L"Token is an AppContainer living on Windows Session #%i.\r\n", dwAppContNum);
			cl_wprintf(YELLOW, L"We are living in a yellow AppContainer...\r\n");
		}
	} else {
		if (bDisplayData) wprintf(L"Token is not an AppContainer!\r\n");
		return true;
	}
	if (!bDisplayData) goto Cleanup;

	// Get AppContainer SID
	PTOKEN_APPCONTAINER pAppContSid = (PTOKEN_APPCONTAINER)new BYTE[SECURITY_MAX_SID_SIZE];
	RtlZeroMemory(pAppContSid, SECURITY_MAX_SID_SIZE);
	retVal = ZwQueryInformationToken(hToken, TokenAppContainerSid, (PVOID)pAppContSid, SECURITY_MAX_SID_SIZE, &retLen);
	
	// Get Sid String and display it if needed
	if (retVal == STATUS_SUCCESS) 
		retVal = ConvertSidToStringSid(pAppContSid->AppContainerSid, &stringSid);
	if (bDisplayData) wprintf(L"\r\nApp Container SID:\r\n%s.\r\n", stringSid);
	if (stringSid) {LocalFree((HLOCAL)stringSid); stringSid = NULL;}

	// Query LowBox token Capabilities
	PTOKEN_CAPABILITIES pTokCap = NULL;
	retVal = ZwQueryInformationToken(hToken, TokenCapabilities, (PVOID)pTokCap, 0, &retLen);
	if (retVal == STATUS_BUFFER_TOO_SMALL) {
		pTokCap = (PTOKEN_CAPABILITIES)new BYTE[retLen];
		RtlZeroMemory(pTokCap, retLen);
		retVal = ZwQueryInformationToken(hToken, TokenCapabilities, (PVOID)pTokCap, retLen, &retLen);
	}
	
	if ((retVal == STATUS_SUCCESS) && bDisplayData) {
		wprintf(L"Capabilities List:\r\n");
		for (unsigned i = 0; i < pTokCap->CapabilityCount; i++) {
			SID_AND_ATTRIBUTES & curElem = pTokCap->Capabilities[i];
			SID_NAME_USE sidNameUse;				// Type of SID Name
			TCHAR domName[0x80] = {0};				// Domain name
			DWORD domNameCch = COUNTOF(domName);	// Domain name number of chars
			LPTSTR sidAttr = NULL;					// Sid attributes string
			retLen = COUNTOF(sidName);				// Size of "sidName" string

			retVal = ConvertSidToStringSid(curElem.Sid, &stringSid);
			retVal = LookupAccountSid(NULL, curElem.Sid, sidName, &retLen, domName, &domNameCch, &sidNameUse);
			switch (curElem.Attributes) {
				case SE_GROUP_ENABLED:
					sidAttr = L"Enabled"; break;
				case SE_GROUP_ENABLED_BY_DEFAULT:
					sidAttr = L"Enabled by Default"; break;
				case SE_GROUP_OWNER:
					sidAttr = L"Owner"; break;
				case SE_GROUP_INTEGRITY:
					sidAttr = L"Mandatory integrity Level"; break;
				case SE_GROUP_INTEGRITY_ENABLED:
					sidAttr = L"Integrity Level Check"; break;
				default:
					sidAttr = L"Unknown"; break;
			}
			wprintf(L"%i. %s (%s) - %s\r\n", i, stringSid, sidName, sidAttr);
			if (stringSid) {LocalFree((HLOCAL)stringSid); stringSid = NULL;}
		}
	}

Cleanup:
	// Cleanup here
	if (pAppContSid) delete pAppContSid;
	if (pTokCap) delete pTokCap;
	if (bCloseAtExit) CloseHandle(hToken);
	return true;
}

// Test Registry
bool TestRegistry(bool bDisplayData) {
	DWORD disposition = 0;
	DWORD lastErr = 0;
	HKEY hKey = 0;
	LRESULT retVal = 0;
	bool bTokenIsAppCont = false;
	if (IsDebuggerPresent()) DebugBreak();

	wprintf(L"Opening HKCU\\Software key...");
	retVal = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software", NULL, NULL, 0, KEY_ALL_ACCESS,
		NULL, &hKey, &disposition);
	lastErr = GetLastError();
	if (retVal == ERROR_SUCCESS) cl_wprintf(GREEN, L"Success!\r\n");
	else cl_wprintf(RED, L"Failed (error %i)\r\n", (LPVOID)lastErr);

	//if (GetTokenAppContainerState(&bTokenIsAppCont, NULL, false) &&
	//	bTokenIsAppCont) {
	LPTSTR stringSid = NULL;
	TCHAR appContRegKey[256] = {0};
	wprintf(L"Opening AppContainer key...");

	// Construct App container Key
	swprintf_s(appContRegKey, COUNTOF(appContRegKey),
		L"S-1-5-21-3552230024-25621784-1762072011-1001\\"
		L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\"
		L"Storage\\microsoft.bingmaps_8wekyb3d8bbwe");
	retVal = RegCreateKeyEx(HKEY_USERS, appContRegKey, NULL, NULL, 0, KEY_ALL_ACCESS,
		NULL, &hKey, &disposition);
	lastErr = GetLastError();
	if (retVal == ERROR_SUCCESS) cl_wprintf(GREEN, L"Success!\r\n");
	else cl_wprintf(RED, L"Failed (error %i)\r\n", (LPVOID)lastErr);
	return true;
}