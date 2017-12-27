/**********************************************************************
 * 
 *  AaLl86 Windows x86 Memory Bootkit
 *  Various Stuff functions implementation and data structures
 *	Last revision: 
 *
 *  Copyright© 2012 by AaLl86
 *	All right reserved
 * 
 **********************************************************************/
#include "stdafx.h"
#include "VariousStuff.h"
// For sendmail support
#include <mapi.h>

#pragma region CRC32 Functions implementation
CCrc32::CCrc32(void) {
	this->Initialize();
}

CCrc32::~CCrc32(void) {
	//No destructor code.
}

/*
	This function initializes "CRC Lookup Table". You only need to call it once to
		initalize the table before using any of the other CRC32 calculation functions.
*/
void CCrc32::Initialize(void)
{
	//0x04C11DB7 is the official polynomial used by PKZip, WinZip and Ethernet.
	ULONG ulPolynomial = 0x04C11DB7;

	memset(&this->ulTable, 0, sizeof(this->ulTable));

	// 256 values representing ASCII character codes.
	for(int iCodes = 0; iCodes <= 0xFF; iCodes++)
	{
		this->ulTable[iCodes] = this->Reflect(iCodes, 8) << 24;

		for(int iPos = 0; iPos < 8; iPos++)
		{
			this->ulTable[iCodes] = (this->ulTable[iCodes] << 1)
				^ ((this->ulTable[iCodes] & (1 << 31)) ? ulPolynomial : 0);
		}

		this->ulTable[iCodes] = this->Reflect(this->ulTable[iCodes], 32);
	}
}

/*
	Reflection is a requirement for the official CRC-32 standard.
	You can create CRCs without it, but they won't conform to the standard.
*/
unsigned long CCrc32::Reflect(ULONG ulReflect, const char cChar)
{
	unsigned long ulValue = 0;

	// Swap bit 0 for bit 7, bit 1 For bit 6, etc....
	for(int iPos = 1; iPos < (cChar + 1); iPos++)
	{
		if(ulReflect & 1)
		{
			ulValue |= (1 << (cChar - iPos));
		}
		ulReflect >>= 1;
	}

	return ulValue;
}

//Calculates the CRC32 by looping through each of the bytes in sData (starting with another partial CRC)
void CCrc32::PartialCRC(ULONG *ulCRC, const BYTE *sData, ULONG ulDataLength)
{
	while(ulDataLength--)
	{
		//If your compiler complains about the following line, try changing each
		//	occurrence of *ulCRC with "((unsigned long)*ulCRC)" or "*(unsigned long *)ulCRC".

		 *(unsigned long *)ulCRC =
			((*(unsigned long *)ulCRC) >> 8) ^ this->ulTable[((*(unsigned long *)ulCRC) & 0xFF) ^ *sData++];
	}
}

//	Returns the calculated CRC23 for the given string.
ULONG CCrc32::FullCRC(const BYTE *sData, ULONG ulDataLength)
{
	unsigned long ulCRC = 0xffffffff; //Initilaize the CRC.
	this->PartialCRC(&ulCRC, sData, ulDataLength);
	return(ulCRC ^ 0xffffffff); //Finalize the CRC and return.
}

// Static function to calculate a CRC of a bytes stream
ULONG CCrc32::GetCrc(const BYTE *sData, ULONG sDataStart, ULONG ulDataLength) {
	CCrc32 mainCrc;
	ULONG retVal =
		mainCrc.FullCRC(sData + sDataStart, ulDataLength);
	return retVal;
}
#pragma endregion

#pragma region Graphical Helper Functions
// Make a Windows transparent and support an optional shadow
bool SetTraspWindow(HWND hwnd, double factor, bool shadow, bool onlyAlpha, COLORREF color) {
	BOOL retVal = FALSE;

	// Set WS_EX_LAYERED on this window 
	retVal = SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

	//Piazza l'ombra alla finestra
	if (shadow)
		retVal = SetClassLongPtr(hwnd, GCL_STYLE, GetClassLongPtr(hwnd, GCL_STYLE) | CS_DROPSHADOW);

	//Trasparentizza la finestra
	if (onlyAlpha)
		retVal = SetLayeredWindowAttributes (hwnd, 0, (BYTE)((255.0 / 100.0) * factor), LWA_ALPHA);
	else
		retVal = SetLayeredWindowAttributes (hwnd, color, (BYTE)((255.0 / 100.0) * factor), LWA_ALPHA | LWA_COLORKEY);

	return (retVal != 0);
}
#pragma endregion

#pragma region Send Mail functions
ULONG SendMailTo(HWND hWndParent, LPSTR destMailAddrs, LPSTR ccMailAddrs, LPSTR subject, LPSTR body, LPSTR strAttchedFile)
{
	static bool bMapiInitialized = false;		// "true" if I have initialized MAPI
	MapiRecipDesc * receipts = NULL;			// MAPI Receipts array
	MapiFileDesc fileDesc = {0};				// MAPI File descriptor
	int numReceipts = 0;						// Number of receipts
    HINSTANCE hMAPI = ::LoadLibrary(L"MAPI32.DLL");
    if (!hMAPI) return false;

	// You may want to remove this check, but if a valid
    // HWND is passed in, the mail dialog will be made
    // modal to it's parent.
    if (!hWndParent || !::IsWindow(hWndParent))
        return ERROR_INVALID_PARAMETER;

/*	if (FALSE) {
		// Grab the exported entry point for the MAPISendMail function
		typedef HRESULT (__stdcall *MAPI_INITIALIZE)(LPVOID);
		MAPIINIT_0 mapiInit = {0, MAPI_MULTITHREAD_NOTIFICATIONS};
		MAPI_INITIALIZE MapiInitializeFunc = NULL;
		(FARPROC&)MapiInitializeFunc = GetProcAddress(hMAPI, "MAPIInitialize");
		if (!MapiInitializeFunc((LPVOID)&mapiInit)) {
			DbgBreak();
			return MAPI_E_FAILURE;
		}
		bMapiInitialized = true;
	} */

    // Grab the exported entry point for the MAPISendMail function
    ULONG (PASCAL *SendMail)(ULONG, ULONG_PTR, MapiMessage*, FLAGS, ULONG);
    (FARPROC&)SendMail = GetProcAddress(hMAPI, "MAPISendMail");
    if (!SendMail) return false;


	// Now search for recipients
	LPSTR curPtr = NULL;
	int count = 0;				// Generic counter

	// Count total number of receips
	for (int i = 0; i < 2; i++) {
		LPSTR dest = NULL;
		if (i == 0) dest = destMailAddrs;
		else dest = ccMailAddrs;

		if (dest) { 
			curPtr = strchr(dest, ',');
			numReceipts++;
		}

		while (curPtr) {
			numReceipts++;
			curPtr = strchr(curPtr + 1, ',');
		}
	}
	receipts = new MapiRecipDesc[numReceipts];
	RtlZeroMemory(receipts, sizeof(MapiRecipDesc) * numReceipts);

	// And add them to mail
	for (int i = 0; i < 2; i++) {
		LPSTR dest = NULL;			// Current destination mail address
		if (i == 0) dest = destMailAddrs;
		else dest = ccMailAddrs;

		// Now build up one MapiRecipDesc struct for each receipts
		curPtr = dest;
		while (curPtr) {
			LPSTR comma = NULL;
			DWORD len = 0;
			comma = strchr(curPtr + 1, ',');
			if (comma) len = comma - curPtr;
			else len = strlen(curPtr);

			LPSTR name = NULL;
			name = new CHAR[len+2];
			strncpy_s(name, len+2, curPtr, len);
			
			if (dest == destMailAddrs) receipts[count].ulRecipClass = MAPI_TO;
			else receipts[count].ulRecipClass = MAPI_CC;
			receipts[count].lpszName = Trim(name);
			count++;

			if (comma) curPtr = comma + 1;
			else curPtr = NULL;
		} 
	}	// END FOR

    MapiMessage message;
    RtlZeroMemory(&message, sizeof(message));

	// Compile file descriptor(s)
	if (strAttchedFile) {
		RtlZeroMemory(&fileDesc, sizeof(fileDesc));
		fileDesc.nPosition = (ULONG)-1;
		fileDesc.lpszPathName = strAttchedFile;
		fileDesc.lpszFileName = strAttchedFile;
		fileDesc.lpFileType = NULL;
	    message.lpFiles = &fileDesc;
	    message.nFileCount = 1;
	} else
		message.nFileCount = 0;

	message.lpszSubject = subject;
	message.lpszNoteText = body;
	//message.flFlags = MAPI_SENT;
	message.lpRecips = receipts;
	message.nRecipCount = numReceipts;

    // Ok to send
    ULONG nError = SendMail(0, (ULONG_PTR)hWndParent, &message, MAPI_LOGON_UI | MAPI_DIALOG, 0);

	// Free up resources
	for (int i = 0; i < numReceipts; i++) 
		if (receipts[i].lpszName) delete[] receipts[i].lpszName;
	if (receipts) delete receipts;

	//if (nError != SUCCESS_SUCCESS && nError != MAPI_USER_ABORT && nError != MAPI_E_LOGIN_FAILURE)
    //      return false;
    return nError;
}
#pragma endregion

// Delete white space from start and end of a string
// NOTE: This function doesn't allocate any buffer
LPSTR Trim(LPSTR string, char leftCharToTrim, char rightCharToTrim) {
	if (!string) return NULL;
	int cch = strlen(string);			// Chars counter
	int startSpaces = 0;				// Start spaces counter
	int endSpaces = 0;					// End Spaces counter

	// Count start spaces
	for (int i = 0; i < cch; i++) 
		if (string[i] != leftCharToTrim) break;
		else startSpaces++;
	
	// Cont end spaces
	for (int i = cch-1; i > 0; i--) 
		if (string[i] != rightCharToTrim) break;
		else endSpaces++;

	if (leftCharToTrim != rightCharToTrim) {
		if (startSpaces < endSpaces) endSpaces = startSpaces;
		else startSpaces = endSpaces;
	}

	if (!startSpaces && !endSpaces) return string;
	RtlCopyMemory(string, string + startSpaces, cch - startSpaces - endSpaces);
	string[cch - startSpaces - endSpaces]= 0;
	return string;
}

// Add privilege to a token
BOOLEAN AddPrivilegeToToken(HANDLE hToken, LPTSTR privName) {
	BOOL retVal = FALSE;
	TOKEN_PRIVILEGES tp = {0};
	LUID privLuid = {0};			// Privilege LUID (locally unique identifier)

	retVal = LookupPrivilegeValue(NULL, privName, &privLuid);
	if (!retVal) return FALSE;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = privLuid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	retVal = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
	return (BOOLEAN)retVal;
}

// Align a file pointer
bool AlignFilePointer(HANDLE hFile, DWORD align) {
	BOOL retVal = 0;
	LARGE_INTEGER curOffset = {0};
	LARGE_INTEGER newOffset = {0};

	// Get current file offset
	retVal = SetFilePointerEx(hFile, curOffset, &curOffset, FILE_CURRENT);
	if (!retVal) return false;
	newOffset.QuadPart = curOffset.QuadPart + (align-1); 
	newOffset.QuadPart -= (newOffset.QuadPart % align);
	retVal = SetFilePointerEx(hFile, newOffset, &curOffset, FILE_BEGIN);
	return (retVal != FALSE);
}