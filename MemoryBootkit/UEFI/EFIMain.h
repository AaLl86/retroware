/**********************************************************************
 *  AaLl86 EFI Application Model 2013
 *	Filename: EfiMain.h
 *	Implement EFI Application Main entry point and startup functions prototypes
 *	Last revision: dd/mm/yyyy
 *
 *  Copyright© 2013 Andrea Allievi
 *	All right reserved
 **********************************************************************/

#pragma once
#include "stdafx.h"

// EFI Main entry point
EFI_STATUS EfiMain (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE * SystemTable);

// New ItSec exit boot services API
EFI_STATUS ItSecExitBootServices(EFI_HANDLE ImageHandle, UINTN MapKey);

// Print debug title information and wait for keystroke
CHAR16 PrintDebugData(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE * SystemTable = NULL);

// ItSec new OslArchTransferToKernel detour
extern "C" EFI_STATUS InstallNtPatch(LPBYTE ntStartAddr, DWORD size);

// Search and patch Winload OslArchTransferToKernel signature
EFI_STATUS PatchWinload(LPBYTE startAddr, DWORD size);


#ifdef _AMD64_
	// ASM64 Functions prototypes:
	extern "C" EFI_STATUS TestFunc1();
#endif

// Global EFI System table
extern EFI_SYSTEM_TABLE * g_pEfiTable;
