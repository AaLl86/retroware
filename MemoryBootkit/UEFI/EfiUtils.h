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

#pragma region EFI Needed protocols definitions
#include EFI_PROTOCOL_DEFINITION (DevicePathFromText)
#include EFI_PROTOCOL_DEFINITION (DevicePathUtilities)
#include EFI_PROTOCOL_DEFINITION (SimpleFileSystem)
#include EFI_PROTOCOL_DEFINITION (FileInfo)
typedef EFI_FILE_INFO* PEFI_FILE_INFO;
typedef EFI_FILE * PEFI_FILE;
#pragma endregion

// Get string length ItSec Implementation
DWORD GetStringLen(EFI_STRING str, DWORD maxNumElem = 0);

// Compare some memory
// extern "C" __inline
DWORD MemCompare(LPVOID source, LPVOID dest, DWORD numBytes);

//Copy some memory
DWORD MemCpy(LPVOID dest, LPVOID source, DWORD numBytes);

// Load an external EFI Application
EFI_STATUS EfiLoadImage(EFI_HANDLE hRootVol, CHAR16 * relPath, EFI_HANDLE * phLoadedImg, 
						EFI_HANDLE pParentImage = NULL);

// Get EFI file information
EFI_STATUS EfiGetFileInfo(PEFI_FILE pFile, PEFI_FILE_INFO * ppFileInfo);

// Open a particular file and return its handle
EFI_STATUS EfiOpenFile(EFI_HANDLE volHandle, EFI_STRING fileRelPath, PEFI_FILE & pNewFile);

// Wait and read a key from console
CHAR16 EfiReadAndWaitKey(BOOLEAN bReset = FALSE, EFI_SYSTEM_TABLE * SystemTable = NULL);

// Global EFI System table
extern EFI_SYSTEM_TABLE * g_pEfiTable;
