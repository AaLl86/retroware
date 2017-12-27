/**********************************************************************
 * 
 *  AaLl86 Windows x86 Memory Bootkit
 *  Various Stuff Classes and functions prototypes
 *	All logic is implemented in VariousStuff.cpp
 *	Last revision: 
 *
 *  Copyright© 2012 by AaLl86
 *	All right reserved
 * 
 **********************************************************************/
#pragma once

#pragma region MAPI Definitions
#define 	MAPI_MULTITHREAD_NOTIFICATIONS   0x00000001
typedef struct
{
	ULONG ulVersion;
	ULONG ulFlags;
} MAPIINIT_0, FAR *LPMAPIINIT_0;
#pragma endregion

#pragma region CRC32 Class Definition
class CCrc32 {
public:
	CCrc32();
	~CCrc32();

	// Initializes "CRC Lookup Table". You only need to call it once 
	void Initialize(void);

	// Returns the calculated CRC23 for the given byte stream
	ULONG FullCRC(const BYTE *sData, ULONG ulDataLength);

	//Calculates the CRC32 by looping through each of the bytes in sData (starting with another partial CRC)
	void PartialCRC(ULONG *ulCRC, const BYTE *sData, ULONG ulDataLength);

	// Static function to calculate a CRC of a bytes stream
	static ULONG GetCrc(const BYTE *sData, ULONG sDataStart, ULONG ulDataLength);

private:
	ULONG Reflect(unsigned long ulReflect, const char cChar);
	ULONG ulTable[0x100]; // CRC lookup table array.
};

#pragma endregion

#pragma region Graphical Helper Functions
// Ottiene il valore nHeight per l'API CreateFont
__inline int GetHeightFromPt(int PointSize) { 
	return -MulDiv(PointSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72); }

// Make a Windows transparent and support an optional shadow
bool SetTraspWindow(HWND hwnd, double factor, bool shadow = true, bool onlyAlpha = true, COLORREF color = 0);
#pragma endregion

// Delete white space from start and end of a string
LPSTR Trim(LPSTR string, char leftCharToTrim = ' ', char rightCharToTrim = ' ');

// Add privilege to a token
BOOLEAN AddPrivilegeToToken(HANDLE hToken, LPTSTR privName);

// Align a file pointer
bool AlignFilePointer(HANDLE hFile, DWORD align);

// Send a mail using MAPI 
ULONG SendMailTo(HWND hWndParent, LPSTR destMailAddrs, LPSTR ccMailAddrs, LPSTR subject, LPSTR body, LPSTR strAttchedFile);
