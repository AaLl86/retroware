/*
 *  Copyright (C) 2013 Saferbytes Srl - Andrea Allievi
 *	Filename: Appcontainer.cpp
 *	Implements a basic Windows 8 Appcontainer Launcher
 *	Last revision: 05/30/2013
 *
 */

#include "stdafx.h"
#include "Win8Launcher.h"
#include <Sddl.h>
#include <AccCtrl.h>
#include <Aclapi.h>
#pragma comment(lib, "ntdll.lib")

// Test Windows 8 Appcontainer Tokens
bool TestAppContainer() {
	STARTUPINFOEX stInfoEx = {0};
	PROC_THREAD_ATTRIBUTE_LIST * pProcList = NULL;
	ULONG_PTR procListLen = 0;
	DWORD dwAttributeCount = 1;			// Only one attribute 
	LPTSTR envBlock = NULL;
	DWORD dwRetSize = 0;				// Returned size
	BOOL retVal = FALSE;				// Win32 Returned value
	
	//SECURITY_CAPABILITIES PROC_THREAD_ATTRIBUTE_INPUT
	DWORD tmpVal = TOKEN_ALL_ACCESS;
	tmpVal = sizeof(STARTUPINFOEX);
			
	system("cls");
	wprintf(L"|-------------------------------------------------------------|\r\n");
	wprintf(L"|               AaLl86 Windows 8 AppContainer TEST            |\r\n");
	wprintf(L"|-------------------------------------------------------------|\r\n");
	retVal = EnablePrivilege(NULL, SE_TCB_NAME);
	if (!retVal) wprintf(L"Warning! Unable to set TCB privilege for current process!\r\n");
	wprintf(L"\r\n");

	// Allocate attribute list 
	procListLen = sizeof(PROC_THREAD_ATTRIBUTE_LIST) - sizeof(PROC_THREAD_ATTRIBUTE_ENTRY) + 
		(dwAttributeCount * sizeof(PROC_THREAD_ATTRIBUTE_ENTRY));
	pProcList = (PROC_THREAD_ATTRIBUTE_LIST*)new BYTE[procListLen];
	RtlZeroMemory(pProcList, procListLen);

	stInfoEx.StartupInfo.cb = sizeof(STARTUPINFOEX);
	stInfoEx.StartupInfo.lpTitle = L"C:\\Windows\\system32\\wwahost.exe";
	stInfoEx.StartupInfo.dwFlags = 0x80;
	stInfoEx.lpAttributeList = (_PROC_THREAD_ATTRIBUTE_LIST*)pProcList;

	// Initialize pProcList
	/* InitializeProcThreadAttributeList initializes the attribute list as follows:
		dwFlags = 0
		Size = Value of dwAttributeCount
		Count = 0
		Reserved = Not modified
		Unknown = NULL
	*/
	pProcList->Count = 0;
	pProcList->dwFlags = 0;
	pProcList->Size = dwAttributeCount;
	
	// Add first attribute
	LPTSTR packageName = L"Microsoft.BingWeather_1.2.0.135_x64__8wekyb3d8bbwe";
	pProcList->Entries[0].cbSize = 0x64;
	pProcList->Entries[0].lpValue = (LPVOID)packageName;
	pProcList->Entries[0].cbSize = wcslen(packageName) * sizeof(TCHAR);			// DON'T include NULL char
	pProcList->Entries[0].Attribute = PROC_THREAD_ATTRIBUTE_PACKAGE_FULL_NAME; 
	pProcList->dwFlags = PACKAGE_FULL_NAME;
	pProcList->Count++;
	
	// Now duplicate my token
	HANDLE hNewToken = NULL,
		hCurToken = NULL;

	retVal = LogonUser(L"Andrea", L".", L"aaaaa", LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hNewToken);
	if (!retVal) {
		retVal = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hCurToken);
		retVal = DuplicateTokenEx(hCurToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &hNewToken);
		CloseHandle(hCurToken);
	}

	// Now create process
	SECURITY_ATTRIBUTES secAttr = {0};		// No handle inheriting
	secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	PROCESS_INFORMATION procInfo = {0};
	DWORD procFlags = CREATE_UNICODE_ENVIRONMENT | EXTENDED_STARTUPINFO_PRESENT;
	envBlock = GetEnvironmentStrings();

	wprintf(L"Launching News App with LowBoxToken...\r\n");
	wprintf(L"Calling Process must be in Session 0 and Token must have 2 Security Attributes:\r\n"
		L"\tWIN://SYSAPPID - Store System App Package Id\r\n"
		L"\tWIN://PKG - Store Package name\r\n");
	wprintf(L"Press any key to start!\r\n\r\n");
	rewind(stdin);
	_getwch(); 
	BYTE nuff[512] = {0};
	retVal = ZwSetInformationToken(hNewToken, TokenSecurityAttributes, nuff, sizeof(nuff));
	if (((INT)retVal) < 0)
		wprintf(L"Warning! ZwSetInformationToken has failed with status 0x%08X.\r\n", retVal);

	//if (IsDebuggerPresent()) DbgBreak();
	retVal = CreateProcessAsUser(hNewToken, L"C:\\Windows\\system32\\wwahost.exe", 
		L"C:\\Windows\\system32\\wwahost.exe -ServerName:AppexNews.wwa", &secAttr, 
		NULL, FALSE, procFlags, (LPVOID)envBlock, NULL, (LPSTARTUPINFO)&stInfoEx, &procInfo);
	DWORD lastErr = GetLastError();
	secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	wprintf(L"%s - Last error: %i\r\n", retVal ? L"Success!" : L"Error!", lastErr);


	wprintf(L"Press any key to continue...\r\n");
	rewind(stdin);
	_getwch();

	wprintf(L"\r\nBuilding AaLl86 App Container...\r\n");
	// Create APP Container SID
	LPTSTR appContainerSidStr = //L"S-1-15-2-1220793744-3666789380-189579892-1973497788-2854962754-2836109804-3864561331";
		L"S-1-15-2-1520854211-3666789380-189579892-1973497788-2854962754-2836109804-386456133";
	PSID appContainerSid = NULL;
	retVal = ConvertStringSidToSid(appContainerSidStr, &appContainerSid);

	// NB. Before doing that you MUST build the following registry key with a proper ACL:
	// HKCU\Software\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppContainer\Mappings\S-1-15-2-1520854211-3666789380-189579892-1973497788-2854962754-2836109804-386456133
	// AND create 2 string values: "Moniker" and "DisplayName"

	// Create Capabilities SIDs
	SID_IDENTIFIER_AUTHORITY packageAuthIA = SECURITY_APP_PACKAGE_AUTHORITY;
	SID_AND_ATTRIBUTES capList[3] = {0};		// Capabilities List
	PSID intCapSid = NULL,			// SECURITY_CAPABILITY_INTERNET_CLIENT Sid
		intClSrvSid = NULL,			// SECURITY_CAPABILITY_INTERNET_CLIENT_SERVER Sid
		privNetCapSid = NULL;		// SECURITY_CAPABILITY_PRIVATE_NETWORK_CLIENT_SERVER Sid

	// Generate capabilities Sids
	AllocateAndInitializeSid(&packageAuthIA, SECURITY_BUILTIN_CAPABILITY_RID_COUNT, SECURITY_CAPABILITY_BASE_RID, 
		SECURITY_CAPABILITY_INTERNET_CLIENT,
		NULL, NULL, NULL, NULL, NULL, NULL, &intCapSid);
	capList[0].Attributes = SE_GROUP_ENABLED;
	capList[0].Sid = intCapSid;

	//AllocateAndInitializeSid(&packageAuthIA, SECURITY_BUILTIN_CAPABILITY_RID_COUNT, SECURITY_CAPABILITY_BASE_RID, 
	//	SECURITY_CAPABILITY_INTERNET_CLIENT_SERVER,
	//	NULL, NULL, NULL, NULL, NULL, NULL, &intClSrvSid);
	//capList[1].Attributes = SE_GROUP_ENABLED;
	//capList[1].Sid = intClSrvSid;
	AllocateAndInitializeSid(&packageAuthIA, SECURITY_BUILTIN_CAPABILITY_RID_COUNT, SECURITY_CAPABILITY_BASE_RID, 
		SECURITY_CAPABILITY_DOCUMENTS_LIBRARY,
		NULL, NULL, NULL, NULL, NULL, NULL, &intClSrvSid);
	capList[1].Attributes = SE_GROUP_ENABLED;
	capList[1].Sid = intClSrvSid;

	AllocateAndInitializeSid(&packageAuthIA, SECURITY_BUILTIN_CAPABILITY_RID_COUNT, SECURITY_CAPABILITY_BASE_RID, 
		SECURITY_CAPABILITY_PRIVATE_NETWORK_CLIENT_SERVER,
		NULL, NULL, NULL, NULL, NULL, NULL, &privNetCapSid);
	capList[2].Attributes = SE_GROUP_ENABLED;
	capList[2].Sid = privNetCapSid;


	// Create security attribute list
	SECURITY_CAPABILITIES secCap = {0};
	secCap.AppContainerSid = appContainerSid;
	secCap.CapabilityCount = 3;
	secCap.Capabilities = capList;
	RtlZeroMemory(pProcList, procListLen);
	retVal = InitializeProcThreadAttributeList((_PROC_THREAD_ATTRIBUTE_LIST*)pProcList, 1, 0, (PSIZE_T)&procListLen);
	retVal = UpdateProcThreadAttribute((_PROC_THREAD_ATTRIBUTE_LIST*)pProcList, 0, PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES, (LPVOID)&secCap, sizeof(secCap), 
		NULL, NULL);

	//if (IsDebuggerPresent()) DbgBreak();
	LPTSTR cmdLine = L"";
	LPTSTR appName = L"d:\\test\\cmd.exe";;
	//retVal = AddAppContainerAclToFile(appContainerSid, L"C:\\Users\\Andrea\\AppData\\Local\\Packages\\Ciao");

	retVal = CreateProcessAsUser(hNewToken, appName, cmdLine,
		&secAttr, NULL, FALSE,				// L"C:\\Windows\\system32\\wwahost.exe -ServerName:App.wwa",
		procFlags, NULL, NULL, (LPSTARTUPINFO)&stInfoEx, &procInfo);
	lastErr = GetLastError();
	
	if (retVal)
		wprintf(L"\tSuccess!\r\n");
	else
		wprintf(L"\tFailed with error %i.\r\n", lastErr);
	wprintf(L"Press any key to continue...\r\n");
	rewind(stdin);
	_getwch();

	return (retVal == TRUE);

}

// Enable a privilege in a token
bool EnablePrivilege(HANDLE hToken, LPTSTR privName) {
	BOOL retVal = FALSE;
	LUID privLuid = {0};
	TOKEN_PRIVILEGES tokPriv = {0};
	bool bTokOpenedByMe = false;
	DWORD lastErr = 0;

	if (!privName) return false;

	if (!hToken) {
		retVal = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
		bTokOpenedByMe = true;
	}
	if (!hToken) return false;

	retVal = LookupPrivilegeValue(NULL, privName, &privLuid);
	if (!retVal) return false;

	tokPriv.PrivilegeCount = 1;
	tokPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tokPriv.Privileges[0].Luid = privLuid;
	retVal = AdjustTokenPrivileges(hToken, FALSE, &tokPriv, NULL, 0, NULL);
	lastErr = GetLastError();

	if (bTokOpenedByMe) CloseHandle(hToken);
	return (retVal != FALSE && lastErr != ERROR_NOT_ALL_ASSIGNED);
}

// Add AppContainer ACL to a file or directory
bool AddAppContainerAclToFile(PSID appContSid, LPTSTR fileName) {
	DWORD dwRes = 0;
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
	EXPLICIT_ACCESS ea = {0};

    if (fileName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}

    // Get a pointer to the existing DACL.
    dwRes = GetNamedSecurityInfo(fileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
          NULL, NULL, &pOldDACL, NULL, &pSD);
    if (ERROR_SUCCESS != dwRes) {
        SetLastError(dwRes);
        goto Cleanup; 
    }  

    // Initialize an EXPLICIT_ACCESS structure for the new ACE. 
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = FILE_ALL_ACCESS;
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.ptstrName = (LPTSTR)appContSid;

    // Create a new ACL that merges the new ACE
    // into the existing DACL.
    dwRes = SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);
    if (ERROR_SUCCESS != dwRes)  {
        SetLastError(dwRes);
        goto Cleanup; 
    }  

    // Attach the new ACL as the object's DACL.
    dwRes = SetNamedSecurityInfo(fileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
          NULL, NULL, pNewDACL, NULL);
    
	if (ERROR_SUCCESS != dwRes)  {
        SetLastError(dwRes);
        goto Cleanup; 
    }  

   
Cleanup:
    if(pSD != NULL) 
        LocalFree((HLOCAL) pSD); 
	if(pNewDACL != NULL) 
        LocalFree((HLOCAL) pNewDACL); 

    return (dwRes == 0);
}

// General purposes Tests
bool DoSecurityTests() {
	HANDLE hObj = NULL;
	DWORD retLength = 0;
	BYTE * buff = NULL;
	NTSTATUS retVal = 0;
	DWORD lastErr = 0;

	// Open target object
	hObj = CreateFile(L"\\\\.\\COM1", FILE_GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();

	// Get memory needed for DACL
	retVal = ZwQuerySecurityObject(hObj, DACL_SECURITY_INFORMATION, NULL, 0, &retLength);
	if (retVal != STATUS_BUFFER_TOO_SMALL) {
		CloseHandle(hObj);
		return false;
	}

	buff = new BYTE[retLength+2];
	RtlZeroMemory(buff, retLength);

	// Get Object DACL
	retVal = ZwQuerySecurityObject(hObj, DACL_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR)buff, retLength, &retLength);
	
	delete buff;
	CloseHandle(hObj);
	return (retVal == STATUS_SUCCESS);
}
