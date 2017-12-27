/*		AaLl86 PeRebuilder
 *	Andrea Allievi, Private project
 *	Log engine header file 
 *	Last revision: 01/07/2011
 *	Copyright© 2011 Andrea Allievi (AaLl86)
 */

#pragma once
#include <windows.h>

#ifndef LOGTITLE
	// Log file title
	#define LOGTITLE L"AaLl86 PERebuilder 0.2 Log File"
#endif

class __declspec(dllexport) CLog {
private:
	LPTSTR g_strLogFile;			// This instance log file string
	HANDLE g_hLogFile;				// Log file handle of this CLog
	LPTSTR g_strLogTitle;			// Log title (see LOGTITLE definition)
	bool g_bIsAutoDeleteLog;		// Set if this log has to delete itself if there are no writing
	bool g_bAtLeastOneWrite;		// Set if user called at least one Write function
	bool g_bImCopy;					// True if this instance is a copy of another ones

public:
	// Default Constructor
	CLog();

	// Specialized Constructor
	CLog(LPTSTR logFile, LPTSTR logTitle = NULL);

	// Destructor
	~CLog();

	// Copy constructor
	CLog(CLog &log);

	// Set log Title
	void SetLogTitle(LPTSTR strTitle);

	// Create or open a log file
	bool Open(LPTSTR logFile, bool overwrite = false);

	// Get log file name
	const LPTSTR GetLogFileName() { 
		return this->g_strLogFile; }

	// Close this log
	void Close(bool WriteEnd = true);

	// Write a log string (4 parameters max)
	void Write(LPTSTR dbgStr, LPVOID arg1, LPVOID arg2 = NULL, LPVOID arg3 = NULL, LPVOID arg4 = NULL);

	// Write an ANSI log string (4 parameters max)
	void Write(LPSTR dbgStr, LPVOID arg1, LPVOID arg2 = NULL, LPVOID arg3 = NULL, LPVOID arg4 = NULL);

	// Write an ANSI log string (NO parameters)
	void Write(LPSTR dbgStr);

	// Write a Unicode log string (NO parameters)
	void Write(LPWSTR dbgStr);

	// Write a log line (4 parameters max)
	void WriteLine(LPTSTR dbgStr, LPVOID arg1, LPVOID arg2 = NULL, LPVOID arg3 = NULL, LPVOID arg4 = NULL);

	// Write a log line (NO parameters)
	void WriteLine(LPTSTR dbgStr);

	// Flush file to perform actual disk write
	void Flush();

private:
	// Write current time to log
	void WriteCurTime();
};

// Static function to write data to an uninitialized log
void WriteToLog(LPTSTR dbgStr, LPVOID arg1 = NULL, LPVOID arg2 = NULL, LPVOID arg3 = NULL, LPVOID arg4 = NULL);