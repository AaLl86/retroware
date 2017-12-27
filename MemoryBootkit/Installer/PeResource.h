/**********************************************************************
 * 
 *  AaLl86 x86 Memory Bootkit
 *  PE Resource management class
 *
 *  Copyright© 2011 by AaLl86 
 *	All right reserved
 **********************************************************************/
#pragma once

class CPeResource
{
public:
	// Save specified resource in a file
	static bool Save(LPCTSTR resName, LPCTSTR resType, LPTSTR fileName, bool Overwrite);

	// Extract a resource from this PE and return corresponding pointer
	static PBYTE Extract(LPCTSTR resName, LPCTSTR resType, DWORD * sizeOfRes);

	// Extract an HGLOBAL handle of resource from this PE and return corresponding pointer
	static HGLOBAL ExtractGlobal(LPCTSTR resName, LPCTSTR resType, DWORD * sizeOfRes);

	// Get size of a resource of this PE
	static DWORD SizeOfRes(LPCTSTR resName, LPCTSTR resType);
};
