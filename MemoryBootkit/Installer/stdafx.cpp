#include "stdafx.h"

// Get if a file Exist
bool FileExist(LPTSTR fileName) {
	WIN32_FIND_DATA wfData = {0};
	HANDLE h = NULL;

	// Get if file is a
	LPTSTR slash = wcsrchr(fileName, L'\\');
	if (slash) slash++;
	else slash = fileName;
	LPTSTR colon = wcsrchr(slash, L':');
	if (colon) colon[0] = 0;

	h = FindFirstFile(fileName, &wfData);
	if (colon) colon[0] = L':';

	if (h == INVALID_HANDLE_VALUE) 
		return false;
	else {
		FindClose(h);
		return true;
	}
}
