#include "stdafx.h"
#include "EfiUtils.h"

#pragma region EFI Needed protocols GUIDs
EFI_GUID efiDevicePathFromTextGuid = EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID;
EFI_GUID efiDevPathUtilitiesGuid = EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID;
EFI_GUID efiFileSystemGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID efiFileInfoId = EFI_FILE_INFO_ID;
//EFI_GUID efiDevicePathGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
extern EFI_GUID efiDevicePathGuid;
#pragma endregion

// Get string length ItSec Implementation
DWORD GetStringLen(EFI_STRING str, DWORD maxNumElem) {
	DWORD curLen = 0;
	if (!str) return 0;
	if (!maxNumElem) maxNumElem = 0xFFFFFFFF;

	for (curLen; curLen < maxNumElem; curLen++) 
		if (str[curLen] == 0) break;

	return curLen;
}

// Compare some memory
DWORD MemCompare(LPVOID source, LPVOID dest, DWORD numBytes) {
	DWORD count = 0;			// Bytes counter
	for (count = 0; count < numBytes; count++) {
		if (((LPBYTE)source)[count] != ((LPBYTE)dest)[count]) 
			break;
	}
	return count;
}

//Copy some memory
DWORD MemCpy(LPVOID dest, LPVOID source, DWORD numBytes) {
	DWORD i = 0;
	for (i = 0; i <numBytes; i++)
		((LPBYTE)dest)[i] = ((LPBYTE)source)[i];
	return i;
}


// Wait and read a key from console
CHAR16 EfiReadAndWaitKey(BOOLEAN bReset, EFI_SYSTEM_TABLE * SystemTable) {
	EFI_STATUS efiStatus = EFI_SUCCESS;					
	EFI_SIMPLE_TEXT_IN_PROTOCOL * pTxtIn = NULL;		// EFI Text input protocol
	UINTN evtIdx = 0;									// Key event index
	EFI_INPUT_KEY efiKey = {0};

	if (!SystemTable) SystemTable = g_pEfiTable;
	pTxtIn = SystemTable->ConIn;

	if (bReset) efiStatus = pTxtIn->Reset(pTxtIn, FALSE);

	do {
		#ifndef _DEBUG
		// Wait for keyboard input buffer
		efiStatus = SystemTable->BootServices->WaitForEvent(1, &pTxtIn->WaitForKey, &evtIdx);
		#else
		SystemTable->BootServices->Stall(1000 * 200);			// 200 milliseconds
		#endif

		// Now retreive key
		efiStatus = pTxtIn->ReadKeyStroke(pTxtIn, &efiKey);
		if (efiStatus == EFI_DEVICE_ERROR) break;

	} while (efiStatus == EFI_NOT_READY || efiKey.UnicodeChar == 0);
	return (efiKey.UnicodeChar);
}

// Load an external EFI Application
EFI_STATUS EfiLoadImage(EFI_HANDLE hRootVol, CHAR16 * relPath, EFI_HANDLE * phLoadedImg, EFI_HANDLE pParentImage) {
	EFI_HANDLE allHandles[10] = {0};
	EFI_STATUS efiStatus = EFI_SUCCESS;
	EFI_BOOT_SERVICES * eBs = NULL;								// Boot Services quick link
	EFI_DEVICE_PATH_PROTOCOL * pDevPath = NULL;					// Root device EFI path
	EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL * pDevPathConvert = NULL;
	EFI_DEVICE_PATH_UTILITIES_PROTOCOL * pDevPathUtilities = NULL;
	DWORD buffSize = sizeof(allHandles);
	
	if (!g_pEfiTable) return NULL;
	eBs = g_pEfiTable->BootServices;

	// Get Device Path protocol to obtain this startup path
	efiStatus = eBs->HandleProtocol(hRootVol, &efiDevicePathGuid, (LPVOID*)&pDevPath);
	if (EFI_ERROR(efiStatus)) {
		EfiPrint(L"DevicePath protocol is not supported for specified root Image handle.\r\n");
		return efiStatus;
	}
	// Get Utilities protocols that I neeed
	efiStatus = eBs->LocateHandle(ByProtocol, &efiDevicePathFromTextGuid, NULL, &buffSize, allHandles);
	if (efiStatus == EFI_SUCCESS) {
		eBs->HandleProtocol(allHandles[0], &efiDevicePathFromTextGuid, (LPVOID*)&pDevPathConvert);
		buffSize = sizeof(allHandles) - sizeof(EFI_HANDLE);
		efiStatus = eBs->LocateHandle(ByProtocol, &efiDevPathUtilitiesGuid, NULL, &buffSize, &allHandles[1]);
	}

	if (efiStatus == EFI_SUCCESS) 
		efiStatus = eBs->HandleProtocol(allHandles[1], &efiDevPathUtilitiesGuid, (LPVOID*)&pDevPathUtilities);

	// Create EFI path
	EFI_DEVICE_PATH_PROTOCOL * relEfiPath = NULL,
		* pFullDevPath = NULL;
	if (pDevPathUtilities && pDevPathConvert) {
		relEfiPath = pDevPathConvert->ConvertTextToDevicePath(relPath);
	} else {
		// NO Device Path Utilities Protocols found, manually convert path
		DbgBreak();			// TODO: Fix the following routine
		DWORD relPathNameLen = GetStringLen(relPath, 0x200);
		buffSize = (relPathNameLen * sizeof(CHAR16)) + (sizeof(EFI_DEVICE_PATH_PROTOCOL) * 2);
		eBs->AllocatePool(EfiLoaderData, buffSize, (LPVOID*)&relEfiPath);
		relEfiPath->Type = 0x04;				// Media Device Path 
		relEfiPath->SubType = 0x04;			// File device subtype
		relEfiPath->Length[0] = relPathNameLen * sizeof(CHAR16) + sizeof(EFI_DEVICE_PATH_PROTOCOL);
		eBs->CopyMem((LPBYTE)relEfiPath + sizeof(EFI_DEVICE_PATH_PROTOCOL), relPath, relPathNameLen * sizeof(CHAR16));
		EFI_DEVICE_PATH_PROTOCOL endPath = {0x7F, 0xFF, 0x0004};
		eBs->CopyMem((LPBYTE)relEfiPath + sizeof(EFI_DEVICE_PATH_PROTOCOL) + relPathNameLen * sizeof(CHAR16), &endPath, 4);
	}
	
	pFullDevPath = pDevPathUtilities->AppendDevicePath(pDevPath, relEfiPath);
	eBs->FreePool(relEfiPath);
	
	// Now actually load EFI image
	eBs->SetMem(allHandles, sizeof(allHandles), 0);
	efiStatus = eBs->LoadImage(FALSE, pParentImage, pFullDevPath, NULL, 0, &allHandles[0]);
	if (efiStatus != EFI_SUCCESS) {
		// Try to manually load image
		PEFI_FILE pFile = NULL;
		PEFI_FILE_INFO pFileInfo = NULL;
		LPVOID buff = NULL;						// Target EFI filename content buffer
		DWORD buffSize = 0;						// Buffer size
		efiStatus = EfiOpenFile(hRootVol, relPath, pFile);			//pDevHandle
		if (!EFI_ERROR(efiStatus))
			efiStatus = EfiGetFileInfo(pFile, &pFileInfo);

		// If all went fine now read entire file in memory
		if (!pFile || !EFI_ERROR(efiStatus)) {
			EfiPrint(L"EfiLoadImage has found an error while opening target file!\r\n");
			return efiStatus;
		}
		
		// Allocate needed pages
		EFI_PHYSICAL_ADDRESS retAddr = 0;
		efiStatus = eBs->AllocatePages(AllocateAnyPages, EfiLoaderCode, 
			(UINTN)(pFileInfo->FileSize / PAGE_SIZE) + 1, &retAddr);
		buff = (LPVOID)retAddr;
		pFile->Read(pFile, &buffSize, buff);
		
		// and close underlying file
		pFile->Close(pFile);
		efiStatus = eBs->LoadImage(FALSE, pParentImage, NULL, buff, buffSize, &allHandles[0]);
		
		if (EFI_ERROR(efiStatus)) 
			eBs->FreePages(retAddr, (UINTN)(pFileInfo->FileSize / PAGE_SIZE) + 1);
	}

	if (phLoadedImg) *phLoadedImg = allHandles[0];
	return efiStatus;
}

// Open a particular file and return its handle
EFI_STATUS EfiOpenFile(EFI_HANDLE volHandle, EFI_STRING fileRelPath, PEFI_FILE & pNewFile) {
	EFI_STATUS efiStatus = EFI_SUCCESS;
	EFI_BOOT_SERVICES * eBs = NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL * efiFileSystem = NULL;				// EFI Simple File System
	EFI_FILE * efiVolFs = NULL;											// EFI file representing current volume
	//EFI_FILE_INFO * pFileInfo = NULL;
	//EFI_HANDLE hNewFile = NULL;

	if (!g_pEfiTable) return NULL;
	eBs = g_pEfiTable->BootServices;

	// Get File System Protocol
	efiStatus = eBs->HandleProtocol(volHandle, &efiFileSystemGuid, (LPVOID*)&efiFileSystem);
	if (EFI_ERROR(efiStatus)) {
		// Error, devHandle doesn't support SIMPLE_FILE_SYSTEM protocol
		EfiPrint(L"OpenFile error: device handle doesn't support SIMPLE_FILE_SYSTEM protocol!\r\n");
		return EFI_PROTOCOL_ERROR;
	}

	// Open current volume
	efiStatus = efiFileSystem->OpenVolume(efiFileSystem, &efiVolFs);
	if (EFI_ERROR(efiStatus)) 
		// OpenVolume error
		return efiStatus;

	// Open a file
	efiStatus = efiVolFs->Open(efiVolFs, &pNewFile, fileRelPath, EFI_FILE_MODE_READ, EFI_FILE_ARCHIVE);
	efiVolFs->Close(efiVolFs);				// Close my volume
	return efiStatus;
}


// Get EFI file information
EFI_STATUS EfiGetFileInfo(PEFI_FILE pFile, PEFI_FILE_INFO * ppFileInfo) {
	EFI_BOOT_SERVICES * eBs = NULL;
	EFI_STATUS efiStatus = EFI_SUCCESS;
	DWORD buffSize = 0;
	LPBYTE buff = NULL;
	
	if (!pFile || !g_pEfiTable) return NULL;
	eBs = g_pEfiTable->BootServices;

	// Get file info
	buffSize = sizeof(EFI_FILE_INFO);
	efiStatus = pFile->GetInfo(pFile, &efiFileInfoId, &buffSize, buff);
	if (efiStatus == EFI_BUFFER_TOO_SMALL) {
		efiStatus = eBs->AllocatePool(EfiLoaderData, buffSize, (LPVOID*)&buff);
		efiStatus = pFile->GetInfo(pFile, &efiFileInfoId, &buffSize, buff);
		if (!EFI_ERROR(efiStatus) && ppFileInfo)
			(*ppFileInfo) = (PEFI_FILE_INFO)buff;
		else
			eBs->FreePool(buff);
		buff = NULL;
	}
	return efiStatus;
}

