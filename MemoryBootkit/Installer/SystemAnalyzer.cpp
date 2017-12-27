/**********************************************************************
 * 
 *  AaLl86 Windows x86 Memory Bootkit
 *  Bootkit setup routines module
 *	Last revision: 
 *
 *  Copyright© 2012 by AaLl86
 *	All right reserved
 * 
 **********************************************************************/

#include "stdafx.h"
#include "SystemAnalyzer.h"
#include "GPTDefs.h"
#include "MbrDefs.h"
#include "BcdDefs.h"
#include "VariousStuff.h"
#include "BootkitGPTSetup.h"
#pragma warning (disable: 4995)
#include <Strsafe.h>
#include <WinIoCtl.h>
#define PAGE_SIZE 4096
#pragma comment (lib, "ntdll.lib")

// Default constructor
CSystemAnalyzer::CSystemAnalyzer():
	g_pTmpGptSetup(NULL),						// Set Gpt Setup instance to NULL
	g_sysBiosType(BIOS_TYPE_UNKNOWN)			
{
	// Build a default empty log
	g_pLog = new CLog();
	g_bIsMyLog = true;
}

// Log constructor
CSystemAnalyzer::CSystemAnalyzer(CLog & log):
	g_pTmpGptSetup(NULL),						// Set Gpt Setup instance to NULL
	g_sysBiosType(BIOS_TYPE_UNKNOWN)			
{
	g_pLog = &log;
	g_bIsMyLog = false;
}

// Class instance destructor
CSystemAnalyzer::~CSystemAnalyzer() {
	if (g_bIsMyLog && g_pLog) delete g_pLog;
	if (g_pTmpGptSetup) delete g_pTmpGptSetup;
}

// Set new log class instance
bool CSystemAnalyzer::SetLog(CLog & log) {
	if (g_bIsMyLog && g_pLog) delete g_pLog;
	g_bIsMyLog = FALSE;
	g_pLog = &log;
	return true;
}

BIOS_SYSTEM_TYPE CSystemAnalyzer::GetCurrentBiosType(bool bFastDetect) {
	BOOL retVal = FALSE;					// Win32 API returned value
	BYTE buff[0x80] = {0};					// Generic buffer
	HANDLE hToken = NULL;					// Current process token handle
	DWORD buffLen = sizeof(buff);
	DWORD lastErr = 0;						// Last Win32 error
	BIOS_SYSTEM_TYPE outType;

	retVal = GetFirmwareEnvironmentVariable(L"",  L"{00000000-0000-0000-0000-000000000000}", buff, buffLen);
	lastErr = GetLastError();
	if (lastErr == ERROR_INVALID_FUNCTION) return BIOS_TYPE_MBR;
	else if (lastErr != ERROR_NOACCESS) {
		// Unknown system type, let's do some other tests...
		//g_sysBiosType = BIOS_TYPE_MBR; 
		return BIOS_TYPE_UNKNOWN;
	}

	// UEFI Bios case
	if (!bFastDetect) {
		retVal = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
		retVal = AddPrivilegeToToken(hToken, L"SeSystemEnvironmentPrivilege");		// Adjust my privileges
		lastErr = GetLastError();
		if (retVal) 
			::WriteToLog(L"AaLl86 Memory Bootkit - Successfully added System Environment Privilege in application token!");
		else
			::WriteToLog(L"AaLl86 Memory Bootkit - Warning! Unable to enable \"System Environment Privilege\" in application token. "
			L"Last error: %i.", (LPVOID)lastErr);
		CloseHandle(hToken);
		outType = EnsureSystemBiosType();
		g_sysBiosType = outType;
	} else
		outType = BIOS_TYPE_GPT;

	return outType;
}

// Get if Secure boot is active on this system
bool CSystemAnalyzer::IsSecureBootActive(bool * pIsActive) {
	BOOL retVal = FALSE;					// Win32 API returned value
	BYTE buff[0x10] = {0};					// Generic buffer
	DWORD buffLen = sizeof(buff);
	DWORD value = 0;
	DWORD lastErr = 0;						// Last Win32 error

	retVal = GetFirmwareEnvironmentVariable(L"SecureBoot", EFI_GLOBAL_VARIABLE_GUID_STRING, buff, buffLen);
	lastErr = GetLastError();
	if (!retVal) return false;

	value = *((DWORD*)buff);
	if (pIsActive) *pIsActive = (value != 0);
	return true;
}

// Deep analyze system disk and get if it is really a GPT one
BIOS_SYSTEM_TYPE CSystemAnalyzer::EnsureSystemBiosType() {
	bool bIsGpt = false, retVal = false;

	if (!g_pTmpGptSetup) g_pTmpGptSetup = new CBootkitGptSetup(*g_pLog);

	// Get Efi System partition to absolutely be secure that we are in a UEFI environment
	retVal = g_pTmpGptSetup->GetEfiSystemPartition(NULL, NULL, &bIsGpt);

	if (retVal) {
		if (bIsGpt) return BIOS_TYPE_GPT;
		else return BIOS_TYPE_MBR;
	} else
		return BIOS_TYPE_UNKNOWN;
}

// Get Windows system startup physical disk
bool CSystemAnalyzer::GetStartupPhysDisk(DWORD * pDskNum) {
	BOOL retVal = FALSE;						// API returned value
	DWORD lastErr = 0;							// Last win32 error
	HKEY bcdKey = NULL;							// Handle to BCD opened key
	LPTSTR subKeyStr = NULL;					// BCD Subkey string
	DWORD valueType = REG_BINARY;				// Registry data type
	BYTE dataBuff[0x100] = {0};					// Generic data buffer
	DWORD dataSize = sizeof(dataBuff);			// Data buffer size
	bool bIsGpt = false;
	PBCD_DEVICE_DATA pBcdDevData = NULL;

	subKeyStr = L"BCD00000000\\Objects\\" GUID_WINDOWS_BOOTMGR_STRING L"\\Elements\\" BCDAPPDEVICE_ELEMENT_STRING;
	lastErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyStr, NULL, KEY_READ | KEY_WOW64_64KEY, &bcdKey);
	if (lastErr) return false;

	lastErr = RegQueryValueEx(bcdKey, L"Element", NULL, &valueType, dataBuff, &dataSize);
	if (lastErr || dataSize < sizeof(BCD_DEVICE_DATA)) return false;

	RegCloseKey(bcdKey);
	pBcdDevData = (PBCD_DEVICE_DATA)dataBuff;
	dataSize = pBcdDevData->QualifiedPartitionType.buffSize;
	
	bIsGpt = (GetCurrentBiosType(true) == BIOS_TYPE_GPT);
	if (bIsGpt) {
		// TODO: Implement UEFI case
		DbgBreak();
		RaiseException(STATUS_NOT_IMPLEMENTED, 0, 0, 0);
	}

	// MBR Case
	HANDLE hDev = NULL;					// Target device handle
	DWORD dwBytesRead = 0;				// Total number of bytes read
	MASTER_BOOT_RECORD mbr = {0};		// Read disk master boot record
	TCHAR devName[0x20] = {0};			// Disk device name string
	LPTSTR devNumPtr = NULL;			// Disk device number pointer
	int devIdx = 0;						// Current disk device number
	TCHAR devIdxStr[4];					// Current disk device number string
	bool bFound = false;				// TRUE if I found correct disk
	DISK_GEOMETRY diskGeom = {0};
	
	wcscpy_s(devName, COUNTOF(devName), L"\\\\.\\PhysicalDrive");
	devNumPtr = devName + wcslen(devName);
	retVal = (devNumPtr != NULL);

	while (retVal) {
		// Get current index string
		devNumPtr[0] = 0; retVal = FALSE;
		_itow(devIdx, devIdxStr, 10);
		wcscat_s(devName, COUNTOF(devName), devIdxStr);

		hDev = 	CreateFile(devName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		lastErr = GetLastError();
		if (hDev == INVALID_HANDLE_VALUE) {
			if (lastErr == ERROR_FILE_NOT_FOUND)		// STATUS_NO_SUCH_DEVICE
				break;									// Last physical disk found!

			g_pLog->WriteLine(L"CSystemAnalyzer::GetStartupPhysDisk - Error, unable to open physical disk drive #%i. Last error: %i",
				(LPVOID)devIdx, (LPVOID)lastErr);
			devIdx++;
			continue;
		}

		// Get disk Geometry to obtain Total Disk Sector
		retVal = DeviceIoControl(hDev, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, 
			(LPVOID)&diskGeom, sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
		dwBytesRead = 0;
		if (!retVal) {
			diskGeom.BytesPerSector = 0x200;
			diskGeom.MediaType = FixedMedia;
		}

		if (diskGeom.MediaType == FixedMedia) 
			retVal = ReadFile(hDev, (LPBYTE)&mbr, sizeof(mbr), &dwBytesRead, NULL);
		lastErr = GetLastError();
		CloseHandle(hDev);				// We no longer need it
		
		if (retVal && dwBytesRead) {
			if (mbr.dwDiskId == pBcdDevData->QualifiedPartitionType.u.MbrPartition.diskSignature) {
				// Debug code:
				for (int i = 0; i < COUNTOF(mbr.PTable.Partition); i++) {
					DWORD startSector = (DWORD)(pBcdDevData->QualifiedPartitionType.u.MbrPartition.startOffset / 
						(QWORD)diskGeom.BytesPerSector);
					if (mbr.PTable.Partition[i].FreeSectorsBefore == startSector) {
						bFound = true;
						break;
					}
				}
				// Found it
				_ASSERT(bFound);
				bFound = true;
				break;
			}
		} 
		else if (dwBytesRead) {
			// We are not interested on removable disk devices
			if (lastErr != ERROR_NOT_READY || diskGeom.MediaType != FixedMedia)			
				g_pLog->WriteLine(L"CSystemAnalyzer::GetStartupPhysDisk - Error, unable to read from physical disk drive #%i. "
					L"Last error: %i", (LPVOID)devIdx, (LPVOID)lastErr);
		}
		// Go on in searching....
		devIdx++;
		retVal = TRUE;
	}

	if (pDskNum && bFound) *pDskNum = devIdx;
	if (bFound)
		g_pLog->WriteLine(L"CSystemAnalyzer::GetStartupPhysDisk - Found system boot disk device number #%i. ", (LPVOID)devIdx);
	else
		g_pLog->WriteLine(L"CSystemAnalyzer::GetStartupPhysDisk - Unable to find system boot disk device! (signature mismatch)");
	return bFound;
}

// Obtain information of Current Os Version
bool CSystemAnalyzer::GetOsVersionInfo(LPTSTR * pOutVerStr, bool * isAmd64)
{
	OSVERSIONINFOEX osVer = {0};
	SYSTEM_INFO si = {0};
	LPTSTR verStr = NULL;
	bool rightOs = false;
	osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((LPOSVERSIONINFO)&osVer);

	// Look if there is GetNatvieSystemInfo API
	typedef void (WINAPI *NATIVESYSINFOFUNC)(LPSYSTEM_INFO);
	HMODULE hMod = GetModuleHandle(L"kernel32");
	NATIVESYSINFOFUNC GetNativeSysInfo = (NATIVESYSINFOFUNC)GetProcAddress(hMod, "GetNativeSystemInfo");
	if (GetNativeSysInfo != NULL)
		GetNativeSysInfo(&si);
	else
		GetSystemInfo(&si);
	
	// Allocate version string
	verStr = new TCHAR[0x80];

	// Minimum system version: Windows Vista x86 
	// Maximum system Version: Windows Eight x86

	if (osVer.dwMajorVersion == 6 && osVer.dwMinorVersion == 0) {
		// Windows Vista
		if (osVer.wProductType == VER_NT_WORKSTATION) 
			wcscpy_s(verStr, 0x80, L"Windows Vista ");
		else 
			wcscpy_s(verStr, 0x80, L"Windows Server 2008 ");
		rightOs = true;
	} 
	else if (osVer.dwMajorVersion == 6 && osVer.dwMinorVersion == 1) {
		// Windows Seven
		if (osVer.wProductType == VER_NT_WORKSTATION) 
			wcscpy_s(verStr, 0x80, L"Windows Seven ");
		else 
			wcscpy_s(verStr, 0x80, L"Windows Server 2008 R2 ");
		rightOs = true;
	} 
	else if (osVer.dwMajorVersion == 6 && osVer.dwMinorVersion == 2) {
		// Windows Eight And Windows Server 2012
		if (osVer.wProductType == VER_NT_WORKSTATION) 
			wcscpy_s(verStr, 0x80, L"Windows Eight ");
		else 
			wcscpy_s(verStr, 0x80, L"Windows Server 2012 ");
		rightOs = true;
	} 
	else if (osVer.dwMajorVersion == 5 && osVer.dwMinorVersion == 1) {
		// Windows Xp
		wcscpy_s(verStr, 0x80, L"Windows Xp ");
	} 
	else if (osVer.dwMajorVersion == 5 && osVer.dwMinorVersion == 2) {
		// Windows Xp64 or Server 2003
		if (osVer.wProductType == VER_NT_WORKSTATION) 
			wcscpy_s(verStr, 0x80, L"Windows Xp ");
		else 
			wcscpy_s(verStr, 0x80, L"Windows Server 2003 ");
	} 
	else if (osVer.dwMajorVersion == 5 && osVer.dwMinorVersion == 0) {
		// Windows 2000
		wcscpy_s(verStr, 0x80, L"Windows 2000 ");
	} 
	else {
		// Unknown Operation System
		wcscpy_s(verStr, 0x80, L"Unknown ");
	}

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
		wcscat_s(verStr, 0x80, L"64 bit ");	
		if (isAmd64) *isAmd64 = true;
		rightOs = false;
	} else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
		wcscat_s(verStr, 0x80, L"32 bit ");	
		if (isAmd64) *isAmd64 = false;
	}
	wsprintf(verStr, L"%sVersion %i.%i.%i ", verStr, osVer.dwMajorVersion, osVer.dwMinorVersion, osVer.dwBuildNumber);
	wcscat_s(verStr, 0x80, osVer.szCSDVersion);	

	if (pOutVerStr) *pOutVerStr = verStr;
	else delete[] verStr;
	return rightOs;
}

// Generate System report and save it in a file
bool CSystemAnalyzer::GenerateSystemReport(LPTSTR outFile, DWORD dskIdx) {
	LPBYTE buff = NULL;							// Generic buffer
	DWORD buffSize = 0;							// Generic buffer size
	DWORD retSize = 0;							// Returned size
	NTSTATUS ntStatus = STATUS_SUCCESS;			// Nt returned value
	BOOL retVal = 0;							// Win32 Returned value
	CHAR sysRootDir[0x30] = {0};				// System root directory
	HANDLE hFile = NULL;						// Target log file handle
	DWORD lastErr = 0;							// Last win32 error
	DWORD dwBytesIo = 0;						// Number of I/O bytes 
	PSYSTEM_MODULES pSysMod = NULL;					

	// Get system root directory
	retVal = GetWindowsDirectoryA(sysRootDir, COUNTOF(sysRootDir));

	// Open and create log file
	hFile = CreateFile(outFile, FILE_ALL_ACCESS, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	lastErr = GetLastError();
	if (hFile == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CSystemAnalyzer::GenerateSystemReport - Unable to create \"%s\" file. Last error: %i.", 
			(LPVOID)outFile, (LPVOID)lastErr);
		return false;
	}

	// |-------------------------------------------------------------------------------------------------------------------------|
	// | Get loaded module list
	ntStatus = ZwQuerySystemInformation(SystemModuleInformation, buff, buffSize, &retSize);
	if (ntStatus == STATUS_INFO_LENGTH_MISMATCH) {
		// Allocate enough buffer
		buffSize = (retSize + PAGE_SIZE - 1) & 0xFFFFF000;
		buff = (LPBYTE)VirtualAlloc(NULL, retSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		ntStatus = ZwQuerySystemInformation(SystemModuleInformation, buff, buffSize, &retSize);
	}
	if (NT_ERROR(ntStatus)) {
		if (buff) VirtualFree(buff, 0, MEM_RELEASE);
		g_pLog->WriteLine(L"CSystemAnalyzer::GenerateSystemReport - ZwQuerySystemInformation has failed with error 0x%08X.", (LPVOID)ntStatus);
		CloseHandle(hFile);
		return false;
	}
	pSysMod = (PSYSTEM_MODULES)buff;
	buff = NULL;

	// Get Windows system version
	LPTSTR osVerStr = NULL;
	GetOsVersionInfo(&osVerStr);
	// |-------------------------------------------------------------------------------------------------------------------------|

	// Get my version information
	TCHAR myVerStr[0x80] = {0};
	CVersionInfo myVer(GetModuleHandle(NULL));
	StringCchPrintf(myVerStr, COUNTOF(myVerStr), L"Application version: %s", myVer.GetFileVersionString(false));

	// Start to write log file
	DWORD curLineSize = 0x200;
	LPSTR curLine = new CHAR[curLineSize];
	RtlZeroMemory(curLine, curLineSize);
	strcpy_s(curLine, curLineSize, 
		"|----------------------------------------------------------------------------------------------------|\r\n");
	StringCchPrintfA(curLine, curLineSize, "%s%s%S %s", curLine, "                          ", 
		COMPANYNAME L" " APPTITLE, "Report File\r\n\r\n");
	StringCchPrintfA(curLine, curLineSize, "%s%S\r\n", curLine, myVerStr);
	retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);

	// Write Current OS Version
	StringCchPrintfA(curLine, curLineSize, "Current Operation System: %S\r\n", osVerStr);
	delete[] osVerStr; osVerStr = NULL;
	retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);

	// Write current disk partition type:
	strcpy_s(curLine, curLineSize, "Current system disk partitioning type: ");
	BIOS_SYSTEM_TYPE biosType = g_sysBiosType;
	if (biosType == BIOS_TYPE_UNKNOWN) biosType = GetCurrentBiosType();
	if (biosType == BIOS_TYPE_GPT) strcat_s(curLine, curLineSize, "GPT - GUID Partition scheme\r\n");
	else strcat_s(curLine, curLineSize, "MBR - Master boot record\r\n");
	retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);

	// Get current local time
	SYSTEMTIME localTime = {0};
	TIME_ZONE_INFORMATION tzInfo = {0};

	GetLocalTime(&localTime);
	retVal = GetTimeZoneInformation(&tzInfo);
	StringCchPrintfA(curLine, curLineSize, "Current report time: %.2i/%.2i/%.4i - %.2i:%.2i     UTC %i %S\r\n",
		localTime.wDay, localTime.wMonth, localTime.wYear, localTime.wHour, localTime.wMinute,
		tzInfo.Bias / 60, tzInfo.StandardName);
	retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);

	strcpy_s(curLine, curLineSize, "\r\nLoaded and active kernel modules list:\r\n");
	StringCchPrintfA(curLine, curLineSize, "%sName\t\t\t Description\t\t\t\t\t Company Name\t\t\t File Full Path\r\n", curLine);
	retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);

	for (DWORD i = 0; i < pSysMod->numOfModules; i++) {
		TCHAR fullName[MAX_PATH] = {0};			// Current analysis module full path
		CHAR onlyName[0x40] = {0};					// Module only name
		LPTSTR slashPtr = NULL, dotPtr = NULL;		// Slash and dot pointer
		SYSTEM_MODULE_INFORMATION & curMod = pSysMod->modules[i];
		
		// Get full module name path
		if (_strnicmp(curMod.ImageName, "\\systemroot", 11) == 0) 
			swprintf_s(fullName, COUNTOF(fullName), L"%S%S", sysRootDir, curMod.ImageName + 11);
		else if (_strnicmp(curMod.ImageName, "\\\\.\\", 4) == 0 ||
			_strnicmp(curMod.ImageName, "\\??\\", 4) == 0) 
			swprintf_s(fullName, COUNTOF(fullName), L"%S", curMod.ImageName + 4);
		else
			swprintf_s(fullName, COUNTOF(fullName), L"%S", curMod.ImageName);
		
		// Get only name
		slashPtr = wcsrchr(fullName, L'\\');
		dotPtr = wcsrchr(fullName, L'.');
		if (dotPtr) dotPtr[0] = 0;
		sprintf_s(onlyName, COUNTOF(onlyName), "%S", slashPtr+1);
		if (dotPtr) dotPtr[0] = L'.';

		// Get Module version information
		CVersionInfo curVer(fullName);
		lastErr = GetLastError();
		if (!curVer.IsCreated()) { 
			g_pLog->WriteLine(L"CSystemAnalyzer::GenerateSystemReport - Warning! Unable to open \"%s\" driver. "
				L"Last error: %i.", (LPVOID)fullName, (LPVOID)lastErr);
			//DbgBreak()
		}

		// Output all 
		LPTSTR desc = curVer.QueryVersionValue(L"FileDescription");
		LPTSTR company = curVer.GetCompanyName();
		CHAR formatStr[0x20] = {0};
		DWORD len = strlen(onlyName);
		
		// Analyze file name
		if (len < 8) strcpy_s(formatStr, COUNTOF(formatStr), "%s\t\t\t ");
		else if (len < 15) strcpy_s(formatStr, COUNTOF(formatStr), "%s\t\t ");
		else if (len < 23) strcpy_s(formatStr, COUNTOF(formatStr), "%s\t ");
		else strcpy_s(formatStr, COUNTOF(formatStr), "%s ");

		// Analyze file description
		if (!desc) desc = L"File access error" ;
		len = wcslen(desc);
		if (len < 8) strcat_s(formatStr, COUNTOF(formatStr), "%S\t\t\t\t\t\t ");
		else if (len < 15) strcat_s(formatStr, COUNTOF(formatStr), "%S\t\t\t\t\t ");
		else if (len < 23) strcat_s(formatStr, COUNTOF(formatStr), "%S\t\t\t\t ");
		else if (len < 31) strcat_s(formatStr, COUNTOF(formatStr), "%S\t\t\t ");
		else if (len < 39) strcat_s(formatStr, COUNTOF(formatStr), "%S\t\t ");
		else if (len < 46) strcat_s(formatStr, COUNTOF(formatStr), "%S\t ");
		else strcat_s(formatStr, COUNTOF(formatStr), "%S ");

		// Analyze company name
		if (!company) company = L"File access error" ;
		len = wcslen(company);
		if (len < 15) strcat_s(formatStr, COUNTOF(formatStr), "%S\t\t\t ");
		else if (len < 23) strcat_s(formatStr, COUNTOF(formatStr), "%S\t\t ");
		else strcat_s(formatStr, COUNTOF(formatStr), "%S\t ");
		strcat_s(formatStr, COUNTOF(formatStr), "%S\r\n");

		StringCchPrintfA(curLine, curLineSize, formatStr, onlyName, desc, company, fullName);
		retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);
	}
	// Free up resources used by SYSTEM_MODULES
	VirtualFree(pSysMod, 0, MEM_RELEASE); pSysMod = NULL;

	// Open physical disk and read its content
	HANDLE hDisk = NULL;						// Disk device handle
	TCHAR devName[0x20] = {0};					// Disk device name
	DISK_GEOMETRY diskGeom = {0};				// Disk geometry
	PMASTER_BOOT_RECORD pMbr = NULL;			// Master boot record pointer
	PARTITION_ELEMENT * pBootPart = NULL;		// System Boot partition pointer
	LARGE_INTEGER diskOffset = {0};				// Disk offset
	buffSize = 16 * 1024;
	diskGeom.BytesPerSector = SECTOR_SIZE;

	swprintf_s(devName, COUNTOF(devName), L"\\\\.\\PhysicalDrive%i", dskIdx);
	hDisk = CreateFile(devName, FILE_READ_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	retVal = (hDisk != INVALID_HANDLE_VALUE);
	lastErr = GetLastError();
	if (retVal) {
		// Get Disk drive geometry
		retVal = DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, (LPVOID)&diskGeom, sizeof(DISK_GEOMETRY), &dwBytesIo, NULL);
		if (!retVal) diskGeom.BytesPerSector = SECTOR_SIZE;			// Default sector size
	
		// allocate enough buffer and read MBR
		buff = (LPBYTE)VirtualAlloc(NULL, buffSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		SetFilePointerEx(hDisk, diskOffset, &diskOffset, FILE_BEGIN);
		retVal = ReadFile(hDisk, buff, buffSize, &dwBytesIo, NULL);
		lastErr = GetLastError();
		if (!retVal) { VirtualFree(buff, 0, MEM_RELEASE); buff = NULL; }
		else pMbr = (PMASTER_BOOT_RECORD)buff;
	}

	// Write out MBR sectors 
	StringCchPrintfA(curLine, curLineSize, "\r\nSystem disk drive #%i Master boot sector dump (%i sectors):\r\n",
		dskIdx, buffSize / diskGeom.BytesPerSector);
	retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);

	if (pMbr) {
		//Align output file
		AlignFilePointer(hFile, 0x10);

		// Write MBR sectors to log
		retVal = WriteFile(hFile, buff, buffSize, &dwBytesIo, NULL);
		
		// Search System VBR 
		for (int i = 0; i < 4; i++) {
			PARTITION_ELEMENT & curElem = pMbr->PTable.Partition[i];
			if (!curElem.FileSystemType) continue;

			if (curElem.Bootable == 0x80) {
				// Bootable partition found
				pBootPart = &curElem;
				break;
			}
		}
	} else {
		StringCchPrintfA(curLine, curLineSize, "Error %i while reading from drive!", lastErr);
		retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);
	}

	if (pBootPart && biosType == BIOS_TYPE_MBR) {
		buffSize = 8 * 1024;
		StringCchPrintfA(curLine, curLineSize, "\r\nDisk drive #%i System boot partition VBR dump (%i sectors, "
			"starts at sector 0x%08X, size 0x%08X):\r\n", dskIdx, buffSize / diskGeom.BytesPerSector,
			pBootPart->FreeSectorsBefore, pBootPart->TotalSectors);
		WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);

		diskOffset.QuadPart = (QWORD)pBootPart->FreeSectorsBefore * diskGeom.BytesPerSector;
		retVal = SetFilePointerEx(hDisk, diskOffset, &diskOffset, FILE_BEGIN);
		if (retVal) retVal = ReadFile(hDisk, buff, buffSize, &dwBytesIo, NULL);
		lastErr = GetLastError();

		if (retVal) {
			//Align output file
			AlignFilePointer(hFile, 0x10);

			// Write VBR sectors to drive
			retVal = WriteFile(hFile, buff, buffSize, &dwBytesIo, NULL);
		} else {
			StringCchPrintfA(curLine, curLineSize, "Error %i while reading from drive!", lastErr);
			retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);
		}
	}
	
	if (biosType == BIOS_TYPE_GPT) {
		// Open EFI boot manager and get information
		const LPTSTR msBootFile = L"EFI\\Microsoft\\Boot\\bootmgfw.efi";
		const LPTSTR aallBootFile = L"EFI\\Microsoft\\Boot\\bootmgfw_org.efi";
		LPTSTR volName = NULL;
		LPTSTR bootFullPath = new TCHAR[MAX_PATH];
		CVersionInfo * pEfiBootVerInfo= NULL;
		DWORD strLen = 0;
		
		strcpy_s(curLine, curLineSize, "\r\nUEFI System Boot manager Information\r\n");
		if (!g_pTmpGptSetup) g_pTmpGptSetup = new CBootkitGptSetup(*g_pLog);
		volName = g_pTmpGptSetup->GetEfiSystemBootVolumeName();
		if (volName) {
			strLen = wcslen(volName);
			if (volName[strLen-1] != L'\\') volName[strLen++] = L'\\';
			volName[strLen] = 0;

			wcscpy_s(bootFullPath, MAX_PATH, volName);
			wcscat_s(bootFullPath, MAX_PATH, aallBootFile);
			if (!FileExist(bootFullPath)) {
				bootFullPath[strLen] = 0;
				wcscat_s(bootFullPath, MAX_PATH, msBootFile);
			}
			pEfiBootVerInfo = new CVersionInfo(bootFullPath);
		}
		if (pEfiBootVerInfo) retVal = pEfiBootVerInfo->IsCreated();
		else retVal = FALSE;

		if (retVal) {
			volName[--strLen] = 0;
			StringCchPrintfA(curLine, curLineSize, "%sVolume name: %S\r\nFile name: %S\r\n",
				curLine, volName, bootFullPath + wcslen(volName));
			StringCchPrintfA(curLine, curLineSize, "%sCompany name: %S\r\nFile description: %S\r\n",
				curLine, pEfiBootVerInfo->GetCompanyName(), pEfiBootVerInfo->QueryVersionValue(L"FileDescription"));
			StringCchPrintfA(curLine, curLineSize, "%sFile version: %S\r\nInternal name: %S\r\n",
				curLine, pEfiBootVerInfo->QueryVersionValue(L"FileVersion"), pEfiBootVerInfo->QueryVersionValue(L"InternalName"));
			StringCchPrintfA(curLine, curLineSize, "%sLegal copyright: %S\r\nOriginal filename: %S\r\n",
				curLine, pEfiBootVerInfo->QueryVersionValue(L"LegalCopyright"), pEfiBootVerInfo->QueryVersionValue(L"OriginalFilename"));
			StringCchPrintfA(curLine, curLineSize, "%sProduct name: %S\r\nProduct Version: %S",
				curLine, pEfiBootVerInfo->QueryVersionValue(L"ProductName"), pEfiBootVerInfo->QueryVersionValue(L"ProductVersion"));
			retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);
			delete pEfiBootVerInfo;
		} else {
			strcat_s(curLine, curLineSize, "Unable to get UEFI system Boot manager information. GetEfiSystemBootVolumeName has failed.\r\n");
			retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);
		}

		if (volName) {delete[] volName; volName = NULL;}
		if (bootFullPath) {delete[] bootFullPath; bootFullPath = NULL;}
	}
	// Write end of file signature
	StringCchPrintfA(curLine, curLineSize, "\r\n\r\nEND OF FILE! %c%c", 0xFF, 0xFF);
	retVal = WriteFile(hFile, curLine, strlen(curLine), &dwBytesIo, NULL);

	if (hDisk != INVALID_HANDLE_VALUE)
		CloseHandle(hDisk);
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (curLine)
		delete[] curLine;

	VirtualFree(buff, 0, MEM_RELEASE); buff = NULL;
	return true;
}
