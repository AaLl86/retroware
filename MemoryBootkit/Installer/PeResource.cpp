/**********************************************************************
 * 
 *  AaLl86 x86 Memory Bootkit
 *  PE Resource management class implementation
 *
 *  Copyright© 2011 by AaLl86
 *	All right reserved
 **********************************************************************/

#include "StdAfx.h"
#include "PeResource.h"
#include "log.h"

// Save specified resource in a file
bool CPeResource::Save(LPCTSTR resName, LPCTSTR resType, LPTSTR fileName, bool Overwrite) {
	DWORD resSize = 0;					// Dimensioni della risorsa estratta
	HANDLE hFile = NULL;				// Handle del file da scrivere
	DWORD bytesWritten = 0;				// Numero di bytes scritti nel file
	DWORD lastErr = 0;
	BOOL retVal = FALSE;
	if (!resType || !fileName) return false;
	PBYTE data = Extract(resName, resType, &resSize);

	// Necessario in quanto se il file esiste ed ha attributi di solo lettura la CreateFile fallisce
	retVal = SetFileAttributes(fileName, FILE_ATTRIBUTE_NORMAL);
	hFile = CreateFile(fileName, FILE_ALL_ACCESS, FILE_SHARE_READ, NULL, 
		Overwrite == true ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	_ASSERT(hFile != INVALID_HANDLE_VALUE);
	
	if (hFile == INVALID_HANDLE_VALUE) {
		WriteToLog(L"CPeResource::Save - CreateFile has failed with error code %i.", (LPVOID)lastErr);
		return false;
	}

	retVal = WriteFile(hFile, (LPVOID)data, resSize, &bytesWritten, NULL);
	lastErr = GetLastError();
	CloseHandle(hFile);
	return (retVal != FALSE);
}


// Extract an HGLOBAL handle of resource from this PE and return corresponding pointer
HGLOBAL CPeResource::ExtractGlobal(LPCTSTR resName, LPCTSTR resType, DWORD * sizeOfRes) {
	HRSRC hRes = NULL;
	HGLOBAL hResData = NULL;
	HMODULE thisMod = GetModuleHandle(NULL);

	hRes = FindResource(thisMod, resName, resType);
	_ASSERT(hRes);
	
	if (sizeOfRes) *sizeOfRes = SizeofResource(thisMod, hRes);
	hResData = LoadResource(thisMod, hRes);
	if (!hResData || !hRes) {
		WriteToLog(L"CPeResource::Extract - FindResource or LoadResource has failed with error code: 0x08X.", (LPVOID)GetLastError());
		return NULL;
	}
	return hResData;
}

// Get size of a resource of this PE
DWORD CPeResource::SizeOfRes(LPCTSTR resName, LPCTSTR resType) {
	HMODULE thisMod = GetModuleHandle(NULL);
	HRSRC hRes = FindResource(thisMod, resName, resType);
	return SizeofResource(thisMod, hRes);
}

// Extract a resource from this PE and return corresponding pointer
PBYTE CPeResource::Extract(LPCTSTR resName, LPCTSTR resType, DWORD * sizeOfRes) {
	HGLOBAL resGlobal = ExtractGlobal(resName, resType, sizeOfRes);
	LPBYTE data = NULL;

	if (resGlobal)
		data = (LPBYTE)LockResource(resGlobal);

	return data;
}

