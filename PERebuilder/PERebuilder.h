/**********************************************************************
 * 
 *  AaLl86 PE Rebuilder
 *  PE Utility class rev. 3
 *	Last revision: 18/10/2011
 *
 *  Copyright© 2014 by AaLl86 - Andrea Allievi
 *	All right reserved
 **********************************************************************/
#pragma once 
#include "log.h"

typedef enum _PE_TYPE {
	PE_TYPE_UNKNOWN = 0,
	PE_TYPE_X86,
	PE_TYPE_AMD64,
	PE_TYPE_OTHER = 99
} PE_TYPE;

class CPeRebuilder {
public:
	// Default constructor
	CPeRebuilder();					
	// Specialized Constructor
	CPeRebuilder(CLog * log, LPTSTR fileName = NULL);			
	// Class Destructor
	~CPeRebuilder();			
	// Create a new empty PE 
	LPBYTE NewPe(DWORD Size);
	// Open a new file and get a handle of it
	bool Open(LPTSTR fileName);
	bool Close();
	// Add new PE Section in this File
	bool AddNewSection(char* szName);
	// Read PE file in memory
	//bool ReadPE(DWORD size = 0);
	// Map PE file in memory
	LPBYTE MapPE(DWORD flProtect = PAGE_READONLY, DWORD mapSize = 0);
	// Unmap PE File from memory
	bool UnmapPE();
	// Get file size of this PE
	DWORD GetFileSize();
	// Get Pe Size of this file
	static DWORD GetPeSize(LPBYTE pBuff);
	// Save this PE in a file
	bool Save(LPWSTR FileName);
	// Rebuild PE Image and save a new PE image copy
	bool RebuildPe(QWORD peBaseVa, LPWSTR NewFileName, BOOLEAN bForceSectionReloc = FALSE, BOOLEAN bFixAlign = TRUE);
	// Get Entry point RVA of this PE file
	DWORD GetEntryPointRVA();
	static DWORD GetEntryPointRVA(LPBYTE peHdr);
	// Set Entry Point RVA of this PE file
	bool SetEntryPointRVA(DWORD epRva);
	static bool SetEntryPointRVA(DWORD epRva, LPBYTE peHdr);
	// Set Entry Point VA of this PE file
	bool SetEntryPointVA(DWORD epVa);
	static bool SetEntryPointVA(DWORD epVa, LPBYTE peHdr);
	// Get Entry point Offset of this PE file
	DWORD GetEntryPointOffset() { return RVAToOffset(GetEntryPointRVA());}
	static DWORD GetEntryPointOffset(LPBYTE peHdr) { return RVAToOffset(GetEntryPointRVA(peHdr), peHdr);}
	// Get PE Executable Type
	PE_TYPE GetPeType();
	// Get Image Base RVA of this PE File
	ULONGLONG GetImageBaseRVA();
	// Converte un file offset in un indirizzo RVA
	static DWORD OffsetToRVA(DWORD dwOffset, LPBYTE peHdr);
	DWORD OffsetToRVA(DWORD dwOffset);
	// Converte un indirizzo RVA in un file offset
	static DWORD RVAToOffset(DWORD dwRVA, LPBYTE peHdr); 
	DWORD RVAToOffset(DWORD dwRVA);
	// Check PE file Signature
	static bool IsValidPE(LPBYTE buff);
	// Check PE file Signature
	static bool IsValidPE(LPTSTR strFileName);

private:
	// Internal Rebuild PE Image
	CPeRebuilder * InternalRebuildPe(QWORD peBaseVa, BOOLEAN bForceSectionReloc = FALSE, BOOLEAN bFixAlign = TRUE);
	// Calculate Section Alignment
	static DWORD PEAlign(DWORD align, DWORD TrueSize);
	// Move Bound Directory to free PE space
	bool AdjustBoundDir(IMAGE_DOS_HEADER * dosHdr);
	// Initialize PERebuilder Log file
	bool InitializeLog();
	// Align a value
	static DWORD AlignValue(DWORD Value, DWORD Alignment);
	
private:
	// Current opened file
	LPTSTR g_strFileName;
	// Class Log class pointer
	CLog * g_pLog;
	bool g_bIsMyLog;
	// File buffer
	LPBYTE g_pBuff;
	// Buffer actual and maximum size
	DWORD g_dwMapSize, g_dwMapMaxSize;
	// File Handle
	HANDLE g_hFile;
	// File map handle
	HANDLE g_hFileMap;
	// File size
	DWORD g_dwFileSize;
};

// Various Stuff:
bool FileExist(LPTSTR fileName);

enum ConsoleColor {
	DARKBLUE = 1, DARKGREEN, DARKTEAL, DARKRED, DARKPINK, DARKYELLOW,
	GRAY, DARKGRAY, BLUE, GREEN, TEAL, RED, PINK, YELLOW, WHITE,
	DEFAULT_COLOR = -1
};

// Set console text Color
void SetConsoleColor(ConsoleColor c);

// Color write to STDOUT
void cl_wprintf(LPTSTR string, ConsoleColor c);
