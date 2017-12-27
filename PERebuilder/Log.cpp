/*		AaLl86 PeRebuilder
 *	Andrea Allievi, Private project
 *	Log engine module
 *	Represent a reusable log engine class
 *	Last revision: 01/07/2011
 *	Copyright© 2011 - Andrea Allievi
 */
#include "stdafx.h"
#include "log.h"
//#include <crtdbg.h>
CLog staticLog;

// Static function to write data to an uninitialized log
void WriteToLog(LPTSTR dbgStr, LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4) {
	//CLog log;
	staticLog.WriteLine(dbgStr, arg1, arg2, arg3, arg4);
}

// Default Class Constructor
CLog::CLog():
	g_hLogFile(NULL),
	g_strLogTitle(NULL),
	g_bIsAutoDeleteLog(true),
	g_bAtLeastOneWrite(false),
	g_bImCopy(false),
	g_strLogFile(NULL)
{
	g_strLogFile = new TCHAR[MAX_PATH];
	RtlZeroMemory(g_strLogFile, MAX_PATH * sizeof(TCHAR));
}

// Specialized Class Constructor
CLog::CLog(LPTSTR logFile, LPTSTR logTitle) {
	g_bIsAutoDeleteLog = true;
	g_bAtLeastOneWrite = false;
	g_bImCopy = false;
	g_hLogFile = NULL;
	g_strLogFile = new TCHAR[MAX_PATH];
	g_strLogTitle = NULL;
	RtlZeroMemory(g_strLogFile, MAX_PATH * sizeof(TCHAR));
	if (logTitle) this->SetLogTitle(logTitle);
	if (!this->Open(logFile))
		DbgBreak();
}

// Copy constructor
CLog::CLog(CLog &log) {
	g_bAtLeastOneWrite = log.g_bAtLeastOneWrite;
	g_bIsAutoDeleteLog = log.g_bIsAutoDeleteLog;
	this->g_hLogFile = log.g_hLogFile;
	if (log.g_hLogFile) {
		HANDLE hProc = GetCurrentProcess();
		DuplicateHandle(hProc, log.g_hLogFile, hProc, &g_hLogFile, 0, FALSE, DUPLICATE_SAME_ACCESS);
	}
	if (log.g_strLogFile) {
		g_strLogFile = new TCHAR[MAX_PATH];
		RtlCopyMemory(g_strLogFile, log.g_strLogFile, MAX_PATH * sizeof(TCHAR));
	}
	if (log.g_strLogTitle) {
		int titleLen = wcslen(log.g_strLogTitle);
		g_strLogTitle = new TCHAR[titleLen+1];
		RtlZeroMemory(g_strLogTitle, (titleLen+1) * sizeof(TCHAR));
		RtlCopyMemory(g_strLogTitle, log.g_strLogTitle, (titleLen+1) * sizeof(TCHAR));
	}
	g_bImCopy = true;
	log.g_bAtLeastOneWrite = true;
}

// Destructor
CLog::~CLog() {
	this->Close();
	if (g_strLogTitle) delete[] g_strLogTitle;
	if (g_strLogFile) delete[] g_strLogFile;
}

// Set log Title
void CLog::SetLogTitle(LPTSTR strTitle) {
	if (!strTitle) return;
	int titleLen = wcslen(strTitle);

	if (this->g_strLogTitle) {
		delete g_strLogTitle;
		g_strLogTitle = NULL;
	}
	g_strLogTitle = new TCHAR[titleLen+1];
	wcscpy_s(g_strLogTitle, titleLen+1, strTitle);
}



// Create or open a log file
bool CLog::Open(LPTSTR fileName, bool overwrite) {
	BOOL retVal = FALSE;
	HANDLE hFile = NULL;
	SYSTEMTIME time = {0};
	CHAR logStr[255] = {0};
	DWORD bytesToWrite = 0;
	DWORD bytesWritten = 0;
	DWORD lastErr = 0;

	if (this->g_hLogFile != NULL) {
		// Log File already opened, close it
		if (wcscmp(fileName, this->g_strLogFile) != 0) this->Close();
		else
			// File already opened
			return true;
	}

	hFile = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, (overwrite ? CREATE_ALWAYS : OPEN_ALWAYS),
		FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	if (lastErr == ERROR_ALREADY_EXISTS && overwrite == false)
		retVal = SetFilePointer(hFile, 0, 0, FILE_END);
	

	// Mi memorizzo il nome del file
	if (!g_strLogFile)
		g_strLogFile = new TCHAR[MAX_PATH];
	wcscpy_s(g_strLogFile, MAX_PATH, fileName);

	GetLocalTime(&time);
	LPTSTR logTitle = this->g_strLogTitle;
	if (!logTitle) logTitle = LOGTITLE;
	bytesToWrite = sprintf_s(logStr, 255, "%S\r\nExecution time: %02i/%02i/%02i - %02i:%02i\r\n", logTitle, 
		time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute) + 1;
	retVal = WriteFile(hFile, logStr, bytesToWrite - 1, &bytesWritten, NULL);
	this->g_bAtLeastOneWrite = false;

	if (retVal) {
		this->g_hLogFile = hFile;
		return true;
	} else {
		RtlZeroMemory(g_strLogFile, MAX_PATH * sizeof(TCHAR));
		return false;
	}
}

// Close this log file
void CLog::Close(bool WriteEnd) {
	if (!this->g_hLogFile || g_hLogFile == INVALID_HANDLE_VALUE) 
		return;

	bool deleteMe = false;
	if (!g_bAtLeastOneWrite && g_bIsAutoDeleteLog && !g_bImCopy)
		// Auto delete this log if there aren't any write
		deleteMe = true;

	if (WriteEnd && !g_bImCopy) WriteLine(L"Execution Ended!\r\n\r\n");
	CloseHandle(g_hLogFile);
	if (deleteMe) DeleteFile(g_strLogFile);

	if (g_strLogFile) 
		RtlZeroMemory(g_strLogFile, MAX_PATH * sizeof(TCHAR));
	
}


// Write a log line (NO parameters)
void CLog::WriteLine(LPTSTR dbgStr) {
	WriteCurTime();
	Write(dbgStr);
	Write(L"\r\n");
}

// Write a log line (4 parameters max)
void CLog::WriteLine(LPTSTR dbgStr, LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4) {
	WriteCurTime();
	Write(dbgStr, arg1, arg2, arg3, arg4);
	Write(L"\r\n");
}

// Write current time to log
void CLog::WriteCurTime() {
	TCHAR timeStr[20] = {0};
	SYSTEMTIME curTime = {0};
	GetLocalTime(&curTime);
	swprintf_s(timeStr, 20, L"%02i:%02i:%02i - ", curTime.wHour, curTime.wMinute, curTime.wSecond);
	Write(timeStr);
}


// Write a Unicode log string (NO parameters)
void CLog::Write(LPWSTR dbgStr) {
	DWORD bytesToWrite = 0;
	DWORD bytesWritten = 0;
	CHAR * logStr = NULL;			// String to write in file

	if (!this->g_hLogFile || g_hLogFile == INVALID_HANDLE_VALUE) {		// If I don't have opened a log file
		OutputDebugString(dbgStr);										// write to debug output
	} else {
		bytesToWrite = wcslen(dbgStr) + 1;
		logStr = new CHAR[bytesToWrite];
		sprintf_s(logStr, bytesToWrite, "%S", dbgStr);
		BOOL retVal = WriteFile(g_hLogFile, logStr, bytesToWrite - 1, &bytesWritten, NULL);
		delete[] logStr;
	}
	g_bAtLeastOneWrite = true;
}

// Write a log string (4 parameters max)
void CLog::Write(LPTSTR dbgStr, LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4) {
	LPTSTR resStr = NULL;			// Formatted string

	// Format unicode original string
	resStr = new TCHAR[2048];
	swprintf_s(resStr, 2048, dbgStr, arg1, arg2, arg3, arg4);
	Write(resStr);
	if (resStr != dbgStr) delete[] resStr;
}

// Write an ANSI log string (NO parameters)
void CLog::Write(LPSTR dbgStr) {
	DWORD bytesToWrite = 0;
	DWORD bytesWritten = 0;

	if (!this->g_hLogFile || g_hLogFile == INVALID_HANDLE_VALUE) {		// If I don't have opened a log file
		OutputDebugStringA(dbgStr);										// write to debug output
	} else {
		bytesToWrite = strlen(dbgStr);
		BOOL retVal = WriteFile(g_hLogFile, dbgStr, bytesToWrite, &bytesWritten, NULL);
	}
	g_bAtLeastOneWrite = true;
}

// Write an ANSI log string (4 parameters max)
void CLog::Write(LPSTR dbgStr, LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4) {
	LPSTR resStr = NULL;			// Formatted string

	// Format ansi original string
	resStr = new CHAR[2048];
	sprintf_s(resStr, 2048, dbgStr, arg1, arg2, arg3, arg4);
	Write(resStr);
	delete[] resStr;
}


// Flush file to perform actual disk write
void CLog::Flush() {
	if (!this->g_hLogFile) return;
	FlushFileBuffers(g_hLogFile);
}
