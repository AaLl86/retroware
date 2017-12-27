/**********************************************************************
 *  AaLl86 EFI Application Model 2013
 *	Filename: stdafx.h
 *	Implement EFI Application Standard prototypes and definitions
 *	Last revision: dd/mm/yyyy
 *
 *  Copyright© 2013 Andrea Allievi
 *	All right reserved
 **********************************************************************/
#pragma once


#include <stdio.h>
#include <tchar.h>
#include <intrin.h> 

#define EFI_SPECIFICATION_VERSION EFI_SYSTEM_TABLE_REVISION
#define TIANO_RELEASE_VERSION 0x00080005
#pragma warning(disable: 4005)
#include <efispec.h>
#pragma warning(default: 4005)

typedef wchar_t WCHAR, * LPWSTR;
typedef void * LPVOID;
typedef unsigned char BYTE, *LPBYTE;
typedef unsigned int DWORD, *PDWORD;
typedef unsigned __int64 QWORD, *PQWORD;
typedef CHAR16* EFI_STRING;
#define PAGE_SIZE 4096
#define STDCALL __stdcall

#define DbgBreak() __debugbreak();
// EfiPrint handy definition:
#define EfiPrint(x) g_pEfiTable->ConOut->OutputString(g_pEfiTable->ConOut, x)
