/*		AaLl86 PeRebuilder
 *	Andrea Allievi, Private project
 *	PeRebuilder.cpp - Implement main PeRebuilder logic
 *	Last revision: 09/08/2014
 *	Copyright© 2014 - Andrea Allievi (AaLl86)
 *
*/
#include "StdAfx.h"
#include "PERebuilder.h"
#include "Imagehlp.h"
#pragma comment  (lib, "Imagehlp.lib")

// Analyze export table of a module
__declspec(noinline) DWORD __stdcall GetExportFuncRva(IMAGE_DOS_HEADER * dosHdr, LPSTR funcName) {
	DWORD cont = 0;
	PIMAGE_NT_HEADERS ntHdr = NULL;
	IMAGE_EXPORT_DIRECTORY * exportDir = NULL;
	LPSTR fName = NULL;

	ntHdr = (PIMAGE_NT_HEADERS)(dosHdr->e_lfanew + (LPBYTE)dosHdr);
	exportDir = (PIMAGE_EXPORT_DIRECTORY) (ntHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress	+ (LPBYTE)dosHdr);

	DWORD * Functions = (DWORD *) (exportDir->AddressOfFunctions + (DWORD) dosHdr);
	DWORD * Names = (DWORD *) (exportDir->AddressOfNames + (DWORD) dosHdr);
	WORD * NameOrds = (WORD*)(exportDir->AddressOfNameOrdinals + (DWORD) dosHdr);

	for (cont = 0; cont < exportDir->NumberOfNames; cont++) {
		fName = (LPSTR) (Names[cont] + (DWORD) dosHdr);
		if (strcmp(funcName, fName) == 0) {
			int funcOrd = NameOrds[cont];
			return Functions[funcOrd];
		}	
	}
	return 0;
}

CPeRebuilder::CPeRebuilder() :
	g_strFileName(NULL),
	g_pLog(NULL),
	g_pBuff(NULL),
	g_hFile(NULL),
	g_hFileMap(NULL),
	g_dwMapSize(0),
	g_dwMapMaxSize(0),
	g_dwFileSize(0)
{	
	g_pLog = new CLog();
	g_bIsMyLog = true;
	InitializeLog();
}

CPeRebuilder::CPeRebuilder(CLog * log, LPTSTR fileName) {
	g_pBuff = NULL;
	g_dwMapSize = 0;
	g_bIsMyLog = false;
	g_hFile = NULL;
	g_hFileMap = NULL;
	g_dwFileSize = 0;
	g_strFileName = NULL;

	if (!log) {
		g_pLog = new CLog();
		g_bIsMyLog = true;
	} else {
		g_pLog = log;
		g_bIsMyLog = false;
	}

	if (g_bIsMyLog)
		InitializeLog();

	if (fileName != NULL) 
		Open(fileName);
}

CPeRebuilder::~CPeRebuilder() {
	Close();
	if (g_bIsMyLog) delete g_pLog;
	if (g_strFileName) delete g_strFileName;
}

// RVAToOffset: Convert value from RVA to file offset.
DWORD CPeRebuilder::RVAToOffset(DWORD dwRVA) {
	if (!g_pBuff) MapPE();
	return RVAToOffset(dwRVA, this->g_pBuff);
}

DWORD CPeRebuilder::RVAToOffset(DWORD dwRVA, LPBYTE peHdr)
 {
	if (!peHdr) return (-1);
 	IMAGE_DOS_HEADER * dosHdr = (PIMAGE_DOS_HEADER)peHdr;
	IMAGE_NT_HEADERS * pNtHdr = (PIMAGE_NT_HEADERS)((LONG)dosHdr + dosHdr->e_lfanew);

	WORD wSections = 0;
 	PIMAGE_SECTION_HEADER pSectionHdr = NULL;
 
	/* Map first section */
	pSectionHdr = IMAGE_FIRST_SECTION(pNtHdr);
	wSections = pNtHdr->FileHeader.NumberOfSections;
 
	for (int i = 0; i < wSections; i++)
	{
		if (pSectionHdr->VirtualAddress <= dwRVA)
			if ((pSectionHdr->VirtualAddress + pSectionHdr->SizeOfRawData) > dwRVA)
			{
				dwRVA -= pSectionHdr->VirtualAddress;
				dwRVA += pSectionHdr->PointerToRawData;
 
				return (dwRVA);
			}
 
		pSectionHdr++;
	}
 
	return (-1);
 }
 
// Initialize PERebuilder Log file
bool CPeRebuilder::InitializeLog() {
	// I try to Write to a default log file if I can, otherwise I write to a temp file
	LPTSTR logFile = new TCHAR[MAX_PATH];
	LPTSTR slashStr = NULL;
	DWORD retVal = 0;
	RtlZeroMemory(logFile, MAX_PATH * sizeof(TCHAR));
	retVal = GetModuleFileName(GetModuleHandle(NULL), logFile, MAX_PATH);
	if (!retVal) return false;
	slashStr = wcsrchr(logFile, L'\\');
	if (slashStr) slashStr[1] = 0;

	wcscat_s(logFile, MAX_PATH, L"PERebuilder.log");
	retVal = g_pLog->Open(logFile, false);
	
	if (!retVal) {
		retVal = GetTempPath(MAX_PATH, logFile);
		wcscat_s(logFile, MAX_PATH, L"PERebuilder.log");
		retVal = g_pLog->Open(logFile, false);
	}
	return (retVal > 0);
}

// OffsetToRVA: Convert value from file offset to RVA.
DWORD CPeRebuilder::OffsetToRVA(DWORD dwOffset) {
	if (!this->g_pBuff) MapPE();
	return OffsetToRVA(dwOffset, this->g_pBuff);
}

DWORD CPeRebuilder::OffsetToRVA(DWORD dwOffset,LPBYTE peHdr)
{
	if (!peHdr) return (-1);
 	IMAGE_DOS_HEADER * dosHdr = (PIMAGE_DOS_HEADER)peHdr;
	IMAGE_NT_HEADERS * pNtHdr = (PIMAGE_NT_HEADERS)((LONG)dosHdr + dosHdr->e_lfanew);

	WORD wSections = 0;
 	PIMAGE_SECTION_HEADER pSectionHdr = NULL;
 
 	/* Map first section */
 	pSectionHdr = IMAGE_FIRST_SECTION(pNtHdr);
	wSections = pNtHdr->FileHeader.NumberOfSections;
 
 	for (int i = 0; i < wSections; i++)
 	{
 		if (pSectionHdr->PointerToRawData <= dwOffset)
 			if ((pSectionHdr->PointerToRawData + pSectionHdr->SizeOfRawData) > dwOffset)
 			{
 				dwOffset -= pSectionHdr->PointerToRawData;
 				dwOffset += pSectionHdr->VirtualAddress;
 
 				return (dwOffset);
 			}
 
 		pSectionHdr++;
 	}
 
 	return (-1);
 }

// Get file size of this PE
DWORD CPeRebuilder::GetFileSize() { 
	if (!g_dwFileSize) g_dwFileSize = ::GetFileSize(g_hFile, NULL);
	return g_dwFileSize; 
}

// Get Pe Size of this file
DWORD CPeRebuilder::GetPeSize(LPBYTE pBuff) {
	if (!pBuff) return 0;
	DWORD totalSize = 0;
	PIMAGE_DOS_HEADER pDosHdr = NULL;									// DOS Header
	PIMAGE_NT_HEADERS pNtHdr = NULL;									// PE NT Header
	PIMAGE_SECTION_HEADER pFirstSect = NULL, pCurSect = NULL;			// PE Image Section
	if (!IsValidPE(pBuff)) return 0;

	pDosHdr = (PIMAGE_DOS_HEADER)pBuff;
	pNtHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)pDosHdr->e_lfanew + (ULONG_PTR)pDosHdr);
	pFirstSect = IMAGE_FIRST_SECTION(pNtHdr);

	for (int i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++) {
		pCurSect = (PIMAGE_SECTION_HEADER)((LPBYTE)pFirstSect + sizeof(IMAGE_SECTION_HEADER) * i);
		totalSize = pCurSect->SizeOfRawData + pCurSect->PointerToRawData;
	}
	return totalSize;
}

// Align a value
DWORD CPeRebuilder::AlignValue(DWORD Value, DWORD Alignment)
{
	DWORD Aligned = 0;
	if ((Value % Alignment) == 0) return Value;
	Aligned = (Value + Alignment) & (DWORD)(-(int)Alignment);
	return Aligned;
}

bool CPeRebuilder::Save(LPWSTR FileName) {
	DWORD peSize = 0;
	HANDLE hFile = NULL;
	BOOL bRetVal = FALSE;
	DWORD dwBytesIo = 0;

	if (!g_pBuff) MapPE();
	if (!g_pBuff || !IsValidPE(g_pBuff)) return false;

	peSize = this->GetPeSize(g_pBuff);
	if (peSize > g_dwMapSize) return false;
	
	hFile = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
		return false;
	
	bRetVal = WriteFile(hFile, g_pBuff, peSize, &dwBytesIo, NULL);

	if (bRetVal) {
		size_t len = wcslen(FileName);
		// copy the file name (if needed)
		if (g_strFileName) delete[] g_strFileName;
		g_strFileName = new TCHAR[len+1];
		wcscpy_s(g_strFileName, len+1, FileName);
	}

	// Do not store this handle in g_hFile
	// The memory section is not file based indeed
	CloseHandle(hFile);
	return (bRetVal != 0);
}

// Internal Rebuild PE Image
bool CPeRebuilder::RebuildPe(QWORD peBaseVa, LPWSTR NewFileName, BOOLEAN bForceSectionReloc, BOOLEAN bFixAlign)  {
	DWORD excCode = 0x0;
	CPeRebuilder * pNewObj = NULL;
	BOOL bRetVal = FALSE;
	DWORD lastErr = 0;
	if (!NewFileName) return false;

	__try {
		pNewObj = InternalRebuildPe(peBaseVa, bForceSectionReloc, bFixAlign);
	} __except (excCode = GetExceptionCode(),
		EXCEPTION_EXECUTE_HANDLER) {
		if (g_pLog)
			g_pLog->WriteLine(L"RebuilPe - InternalRebuildPe has raised exception 0x%08X while processing %s file.", (LPVOID)excCode, g_strFileName);
	}
	if (!pNewObj)
		// Something has gone wrong
		return false;

	// Now save the file content in a file
	bRetVal = (pNewObj->Save(NewFileName) == true);
	lastErr = GetLastError();
	if (!bRetVal && lastErr == ERROR_SHARING_VIOLATION) 
	{
		// Close this PE and retry again
		this->Close();			// If needed it will be re-mapped again later
		bRetVal = (pNewObj->Save(NewFileName) == true);
	}

	if (bRetVal)
		g_pLog->WriteLine(L"RebuildPe - Successfully saved the new PE in \"%s\" file.",
			NewFileName);
	else
		g_pLog->WriteLine(L"RebuildPe - The PE image could not be saved in \"%s\" file. Last error: %i",
			NewFileName, (LPVOID)lastErr);
	
	pNewObj->Close();
	delete pNewObj;

	return true;
}



// Internal Rebuild PE Image
CPeRebuilder * CPeRebuilder::InternalRebuildPe(QWORD peBaseVa, BOOLEAN bForceSectionReloc, BOOLEAN bFixAlign) {
	PIMAGE_DOS_HEADER dosHdr = NULL;					// DOS Header
	PIMAGE_NT_HEADERS ntHdr = NULL;						// PE NT Header
	PIMAGE_SECTION_HEADER pOrgImgSect = NULL;			// PE Image Section
	PIMAGE_SECTION_HEADER pDestImgSect = NULL;			// New PE Image Section
	DWORD rva = 0, fOffset = 0;							// Current RVA and File Offset
	DWORD FileAlign = 0, SectAlign = 0;					// File and Section Alignment pointers
	DWORD NewRawAlign = 0;								// The calculated new file section RAW alignment
	DWORD SizeOfHeaders = 0;							// Size of the PE headers
	BYTE zeroMem[0x100] = {0};							// Zeroed out memory
	LPTSTR fileOnlyName = NULL;							// Filename without extension
	QWORD peImageBase = NULL;							// PE File Preferred Image Base
	bool bAmd64 = FALSE;								// TRUE if PE is 64 bit one
	IMAGE_DATA_DIRECTORY importDir = {0};				// PE Import Data Directory
	IMAGE_DATA_DIRECTORY relocDir = {0};				// PE Relocation directory
	IMAGE_OPTIONAL_HEADER32 * pOptHdr32 = NULL;			// 32 bit optional Header
	IMAGE_OPTIONAL_HEADER64 * pOptHdr64 = NULL;			// 64 bit optional Header
	DWORD fileSize = 0, peSize = 0;						// File size in bytes and PE size
	BOOLEAN bNeedReloc = FALSE;							// TRUE if I need to relocate the section
	CPeRebuilder * pNewPe = NULL;
	DWORD lastOffset = 0;
	BOOL retVal = FALSE;

	if (!this->g_strFileName)
		return NULL;
	RtlZeroMemory(zeroMem, sizeof(zeroMem));

	// Map PE in memory if necessary
	if (!this->g_hFileMap) {
		if (MapPE(PAGE_READONLY) == NULL) return NULL;
	}

	// 0. Check PE File 
	retVal = this->IsValidPE(g_pBuff);
	if (!retVal) {
		g_pLog->WriteLine(L"RebuildPe - Error, \"%s\" is not a valid PE File.", this->g_strFileName);
		return NULL;
	}

	// 1. Check Pe size and File Size
	fileSize = this->GetFileSize();
	peSize = GetPeSize(g_pBuff);
	if (peSize > fileSize) {
		g_pLog->WriteLine(L"RebuildPe - Warning! Pe size is bigger then file Size (PE Size: %i - File Size: %i). "
			L"Results can be strange and application could crash.", (LPVOID)peSize, (LPVOID)fileSize);
	} else
		g_pLog->WriteLine(L"RebuildPe - Found PE Size: %i bytes - File Size: %i bytes. ", (LPVOID)peSize, (LPVOID)fileSize);
	
	fileOnlyName = wcsrchr(g_strFileName, L'\\');
	if (fileOnlyName) fileOnlyName++;
	else fileOnlyName = g_strFileName;

	dosHdr = (PIMAGE_DOS_HEADER)this->g_pBuff;
	ntHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)dosHdr->e_lfanew + (ULONG_PTR)dosHdr);

	// 1. Get original File Alignment and Memory Alignment
	bAmd64 = (ntHdr->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
	if (bAmd64) {
		g_pLog->WriteLine(L"Begin relocation of \"%s\" 64 bit PE file from memory address 0x%08x'%08x...", (LPVOID)fileOnlyName, (LPVOID) (peBaseVa >> 32), (LPVOID)peBaseVa);
		pOptHdr64 = (IMAGE_OPTIONAL_HEADER64*)&ntHdr->OptionalHeader;
		FileAlign = pOptHdr64->FileAlignment;
		SectAlign = pOptHdr64->SectionAlignment;
		peImageBase = pOptHdr64->ImageBase;
		importDir = pOptHdr64->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		relocDir = pOptHdr64->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		SizeOfHeaders = pOptHdr64->SizeOfHeaders;
	} else {
		g_pLog->WriteLine(L"Begin relocation of \"%s\" 32 bit PE file from memory address 0x%08x...", (LPVOID)fileOnlyName, (LPVOID) peBaseVa);
		pOptHdr32 = (IMAGE_OPTIONAL_HEADER32*)&ntHdr->OptionalHeader;
		FileAlign = pOptHdr32->FileAlignment;
		SectAlign = pOptHdr32->SectionAlignment;
		peImageBase = pOptHdr32->ImageBase;
		importDir = pOptHdr32->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		relocDir = pOptHdr32->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		SizeOfHeaders = pOptHdr32->SizeOfHeaders;
	}
	pOrgImgSect = IMAGE_FIRST_SECTION(ntHdr);

	// Phase 1 - Fix the file alignment (if needed)
	if (bFixAlign) 
	{
		// The file mapping should be already opened with a tail
		if (FileAlign < 0x200) 
			NewRawAlign = 0x200;
		else if (FileAlign % 0x200) 
			NewRawAlign = AlignValue(FileAlign, 0x200);
		
		// TODO: Insert even Section Virtual size alignment check
		//if ((*pSectAlign) % (*pFileAlign))
		//	(*pSectAlign) += (*pFileAlign) - ((*pSectAlign) % (*pFileAlign));
	} else
		NewRawAlign = FileAlign;

	// Check the new Base Address:
	if (AlignValue((DWORD)peBaseVa, NewRawAlign) != (DWORD)(peBaseVa & 0xFFFFFFFF))
	{
		g_pLog->WriteLine(L"InternalRebuildPe - Error! The specified base address (0x%08X'%08X)"
			L" is not properly aligned (0x%08X).", (LPVOID)(peBaseVa >> 32), (LPVOID)peBaseVa,
			(LPVOID)NewRawAlign);
		return NULL;
	}

	// Check the security directory 
	PIMAGE_DATA_DIRECTORY pSecDir = NULL;
	if (bAmd64) 
		pSecDir = &pOptHdr64->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
	else
		pSecDir = &pOptHdr32->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
	if (pSecDir->VirtualAddress > 0 && (NewRawAlign != FileAlign))
		g_pLog->WriteLine(L"InternalRebuildPe - Warning! The target file appears to be digitally signed. However the signature will be trimmed down due to sections relocation.");


	// Phase 2 - Calculate the new PE size
	DWORD SectOffset = 0;
	DWORD SectSize = 0;

	SectOffset = AlignValue(pOrgImgSect[0].PointerToRawData, NewRawAlign);
	for (int i = 0; i < ntHdr->FileHeader.NumberOfSections; i++) {
		// I need an external buffer here, because section re-alignment could cause overlapping
		SectSize = AlignValue(pOrgImgSect[i].SizeOfRawData, NewRawAlign);
		SectOffset += SectSize;
	}

	// Create the new CPeRebuilder object
	pNewPe = new CPeRebuilder(g_pLog);
	LPBYTE lpNewBuff = pNewPe->NewPe(SectOffset);

	// Start by copying the headers
	RtlCopyMemory(lpNewBuff, dosHdr, SizeOfHeaders);
	pDestImgSect = IMAGE_FIRST_SECTION(((PIMAGE_DOS_HEADER)lpNewBuff)->e_lfanew + lpNewBuff);
	

	// Phase 3 - Now copy and realign each section
	SectOffset = AlignValue(pOrgImgSect[0].PointerToRawData, NewRawAlign);
	for (int i = 0; i < ntHdr->FileHeader.NumberOfSections; i++) {
		LPBYTE address = NULL;
		LPBYTE alignAddress = NULL; 
		BOOLEAN bNeedReloc = FALSE;

		// Get if this section has to be relocated
		rva = pOrgImgSect[i].VirtualAddress;
		fOffset = pOrgImgSect[i].PointerToRawData;
		SectSize = pOrgImgSect[i].SizeOfRawData;
		address = (LPBYTE)dosHdr + fOffset;

		if (!fOffset || !SectSize) 
			continue;
		
		if (fOffset > fileSize) 
			// Break here, otherwise an exception will be raised
			break;

		// Check the calculated Raw offset
		if (AlignValue(pOrgImgSect[i].PointerToRawData, NewRawAlign) > SectOffset)
			SectOffset = AlignValue(pOrgImgSect[i].PointerToRawData, NewRawAlign);

		// Determine if I need to relocate this section
		bNeedReloc = bForceSectionReloc;
		if (!bNeedReloc) bNeedReloc = memcmp(address, zeroMem, sizeof(zeroMem)) == 0;

		if (bNeedReloc) {
			// Move section into the right place
			RtlCopyMemory(lpNewBuff + SectOffset, (LPBYTE)dosHdr + rva, SectSize);
			g_pLog->WriteLine(L"InternalRebuildPe - Section named \"%S\" has been relocated.", (LPVOID)pOrgImgSect[i].Name);	
		} else {
			RtlCopyMemory(lpNewBuff + SectOffset, (LPBYTE)dosHdr + fOffset, SectSize);
			if (SectOffset != fOffset) {
				g_pLog->WriteLine(L"InternalRebuildPe - Section named \"%S\" has been moved due to file misalignment!", (LPVOID)pOrgImgSect[i].Name);	
				pDestImgSect[i].PointerToRawData = SectOffset;
			} else 
				g_pLog->WriteLine(L"InternalRebuildPe - Section named \"%S\" didn't need to be relocated!", (LPVOID)pOrgImgSect[i].Name);	
		}

		// Now calculate the new offset
		SectOffset += AlignValue(SectSize, NewRawAlign);
	}
	
	// Phase 4 - Update underlying variables (from now on they will point to the new buffer)
	dosHdr = (PIMAGE_DOS_HEADER)lpNewBuff;
	ntHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)dosHdr->e_lfanew + (ULONG_PTR)dosHdr);
	// Update the new PE sizes
	DWORD GenericSize = 0;
	GenericSize = pDestImgSect[ntHdr->FileHeader.NumberOfSections-1].Misc.VirtualSize +
		pDestImgSect[ntHdr->FileHeader.NumberOfSections-1].VirtualAddress;
	if (bAmd64) {
		pOptHdr64 = (PIMAGE_OPTIONAL_HEADER64)&ntHdr->OptionalHeader;
		pOptHdr64->FileAlignment = NewRawAlign;
		pOptHdr64->SizeOfImage = AlignValue(GenericSize, SectAlign);
		pOptHdr64->SizeOfHeaders = AlignValue(SizeOfHeaders, NewRawAlign);
	} else  {
		pOptHdr32 = (PIMAGE_OPTIONAL_HEADER32)&ntHdr->OptionalHeader;
		pOptHdr32->FileAlignment = NewRawAlign;
		pOptHdr32->SizeOfImage = AlignValue(GenericSize, SectAlign);
		pOptHdr32->SizeOfHeaders = AlignValue(SizeOfHeaders, NewRawAlign);
	}


	// Phase 5 - Relocate all VAs
	QWORD diff = peBaseVa - peImageBase;
	PIMAGE_BASE_RELOCATION peBaseReloc = NULL;
	
	if (relocDir.VirtualAddress)
		// Get the base relocation directory of the new buffer
		peBaseReloc = (PIMAGE_BASE_RELOCATION)(
			(DWORD)pNewPe->RVAToOffset(relocDir.VirtualAddress) + (LPBYTE)dosHdr);
	
	if (diff != 0 && peBaseReloc) {
		DWORD curSize = 0;
		while (curSize < relocDir.Size && peBaseReloc->VirtualAddress) {
			DWORD baseRva = peBaseReloc->VirtualAddress;
			for (DWORD j = 0; j < (peBaseReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD); j++) {
				WORD item = *((WORD*)((ULONG_PTR)peBaseReloc + sizeof(IMAGE_BASE_RELOCATION)) + j);
				WORD relocType = (item & 0xf000) >> 12;
				if (item == 0) continue;
				
				item &= 0x0fff;
				rva = baseRva + item;
				fOffset = pNewPe->RVAToOffset(rva);
				if (relocType == IMAGE_REL_BASED_DIR64) {
					// AMD64 Relocation Type
					QWORD * destVa =(QWORD*)(fOffset + (LPBYTE)dosHdr);
					*destVa = (*destVa) - diff;
				} else if (relocType == IMAGE_REL_BASED_HIGHLOW) {
					// X86 Relocation type
					DWORD * destVa =(DWORD*)(fOffset + (LPBYTE)dosHdr);
					*destVa = (*destVa) - (DWORD)diff;
				}
			}
			curSize += peBaseReloc->SizeOfBlock;
			peBaseReloc = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)peBaseReloc + peBaseReloc->SizeOfBlock);
		}
		g_pLog->WriteLine(L"InternalRebuildPe - All Virtual Addresses inside \"%s\" file have been relocated!", (LPVOID)fileOnlyName);
	}
	else if (diff && !peBaseReloc) 
		g_pLog->WriteLine(L"InternalRebuildPe - Warning! Code of \"%s\" file needs to be relocated, but the relocation directory is empty. The new PE could not work.", (LPVOID)fileOnlyName);
	else
		g_pLog->WriteLine(L"InternalRebuildPe - Virtual Addresses inside \"%s\" didn't need to be relocated!", (LPVOID)fileOnlyName);

	
	// Phase 6 - Unbind IAT
	IMAGE_IMPORT_DESCRIPTOR * iDirDesc = 
		(PIMAGE_IMPORT_DESCRIPTOR)
		(pNewPe->RVAToOffset(importDir.VirtualAddress) + (LPBYTE)dosHdr);

	for (DWORD k = 0; k < (importDir.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR)); k++) {
		int iat_idx = 0;
		if (bAmd64) {
			// AMD64 Import Address Table
			QWORD * iat = NULL, * oft = NULL;
			if (iDirDesc->Characteristics == 0) continue;
			if (iDirDesc->FirstThunk)
				iat = (QWORD*)((ULONG_PTR)pNewPe->RVAToOffset(iDirDesc->FirstThunk) + 
					(LPBYTE)dosHdr);
			if (iDirDesc->OriginalFirstThunk)
				oft = (QWORD*)((ULONG_PTR)pNewPe->RVAToOffset(iDirDesc->OriginalFirstThunk) + 
					(LPBYTE)dosHdr);
			if (!iat || !oft) 
				break;
			
			while (oft[iat_idx] != 0) {
				iat[iat_idx] = oft[iat_idx];
				iat_idx++;
			}
		} else {
			// X86 Import Address Table
			DWORD * iat = NULL, * oft = NULL;
			//if (iDirDesc->Characteristics == 0) continue;
			if (iDirDesc->FirstThunk)
				iat = (DWORD*)((ULONG_PTR)pNewPe->RVAToOffset(iDirDesc->FirstThunk) + 
					(LPBYTE)lpNewBuff);
			if (iDirDesc->OriginalFirstThunk)
				oft = (DWORD*)((ULONG_PTR)pNewPe->RVAToOffset(iDirDesc->OriginalFirstThunk) + 
					(LPBYTE)dosHdr);
			if (!iat || !oft) 
				break;
			
			while (oft[iat_idx] != 0) {
				iat[iat_idx] = oft[iat_idx];
				iat_idx++;
			}
		}
		iDirDesc++;
	}
	g_pLog->WriteLine(L"InternalRebuildPe - Import Address Table has been checked and fixed!");

	// Phase 7 - Recalcolate and update the PE Checksum
	DWORD oldCheckSum = 0, newCheckSum = 0;
	PIMAGE_NT_HEADERS pNewNtHdr = NULL;
	pNewNtHdr = CheckSumMappedFile(lpNewBuff, SectOffset, &oldCheckSum, &newCheckSum);

	if (pNewNtHdr) {
		if (oldCheckSum != newCheckSum)
			g_pLog->WriteLine(L"PERebuilder - New checksum of file \"%s\": 0x%08x, old checksum: 0x%08x.", (LPVOID)fileOnlyName, (LPVOID)newCheckSum, (LPVOID)oldCheckSum);
		if (!bAmd64) 
			pNewNtHdr->OptionalHeader.CheckSum = newCheckSum;
		else 
			((PIMAGE_OPTIONAL_HEADER64)&pNewNtHdr->OptionalHeader)->CheckSum = newCheckSum;
	} else
		g_pLog->WriteLine(L"PERebuilder - Unable to calcolate new Checksum of file.");
	g_pLog->WriteLine(L"PERebuilder - File \"%s\" new Size: 0x%08x bytes.", (LPVOID)fileOnlyName, (LPVOID)SectOffset);

	return pNewPe;
}

// Read PE file in memory
//bool CPeRebuilder::ReadPE(DWORD size) {
//	DbgBreak();
//	return false;
//}

// Map PE file in memory
LPBYTE CPeRebuilder::MapPE(DWORD flProtect, DWORD dwMapSize) {
	if (!g_hFile)
		Open(this->g_strFileName);
	if (!g_hFile) return NULL;

	// Clenup used resorce
	if (g_pBuff && !g_hFileMap) {
		// File read in memory with VirtualAlloc
		VirtualFree(g_pBuff, 0 , MEM_RELEASE | MEM_COMMIT);
		g_pBuff = NULL;
		g_dwMapSize = 0;
		g_dwMapMaxSize = 0;
	}

	// Get file Size
	DWORD sizeHigh = 0;
	DWORD desiredAccess = 0;
	g_dwFileSize = ::GetFileSize(g_hFile, &sizeHigh);
	g_dwMapSize = 0;
	if (sizeHigh > 0 || g_dwFileSize > 1024 * 1024 * 1024) {
		// Sanity check
		g_pLog->WriteLine(L"MapPe - File \"%s\" is to big to map in memory!", (LPVOID)this->g_strFileName);
		return NULL;
	}

	if (dwMapSize < 1) dwMapSize = g_dwFileSize;

	if (!g_hFileMap) {
		g_hFileMap = CreateFileMapping(g_hFile, NULL, flProtect, NULL, dwMapSize, NULL);
		if (!g_hFileMap) return NULL;
	}
	
	desiredAccess = FILE_MAP_ALL_ACCESS;
	if (flProtect == PAGE_READONLY) desiredAccess = FILE_MAP_READ;

	if (g_pBuff) {UnmapViewOfFile(g_pBuff); g_pBuff = NULL; }
	g_pBuff = (LPBYTE)MapViewOfFile(g_hFileMap, desiredAccess, 0, 0, dwMapSize);
	if (!g_pBuff) return NULL;
	g_dwMapSize = dwMapSize;
	g_dwMapMaxSize = dwMapSize;
	g_pLog->WriteLine(L"MapPe - Succesfully mapped %i Kb of \"%s\" file.", (LPVOID)(dwMapSize / 1024), (LPVOID)this->g_strFileName);
	return g_pBuff;
}


// Calculate Section Alignment
DWORD CPeRebuilder::PEAlign(DWORD trueSize, DWORD align) {
	DWORD outAlign = 0;
	if (!trueSize) return 0;
	if (align == 0) trueSize = align;
	//outAlign = ((trueSize + align - 1) / align) * align;
	//return outAlign;
	outAlign = align;
	while (trueSize > outAlign)
		outAlign += align;
	return outAlign;
}

bool CPeRebuilder::AddNewSection(LPSTR szName) { 
	// STATUS_NOT_IMPLEMENTED
	DbgBreak();
	return false;
}

// Get Image Base RVA of this PE File
ULONGLONG CPeRebuilder::GetImageBaseRVA() {
	if (!g_pBuff) this->MapPE();
	PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)g_pBuff;
	PIMAGE_NT_HEADERS ntHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)dosHdr->e_lfanew + (ULONG_PTR)dosHdr);
	ULONGLONG imageBase = NULL;
	if (ntHdr->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
		IMAGE_OPTIONAL_HEADER64 * pOptHdr = (IMAGE_OPTIONAL_HEADER64*)&ntHdr->OptionalHeader;
		imageBase = pOptHdr->ImageBase;
	} else  {
		IMAGE_OPTIONAL_HEADER32 optHdr = ntHdr->OptionalHeader;
		imageBase = (ULONGLONG)optHdr.ImageBase;
	}
	return imageBase;
}


// Get Entry point RVA of this PE file
DWORD CPeRebuilder::GetEntryPointRVA(LPBYTE peHdr) {
	if (!peHdr) return NULL;
	PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)peHdr;
	PIMAGE_NT_HEADERS ntHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)dosHdr->e_lfanew + (ULONG_PTR)dosHdr);
	DWORD epRva = NULL;
	if (ntHdr->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
		IMAGE_OPTIONAL_HEADER64 * pOptHdr = (IMAGE_OPTIONAL_HEADER64*)&ntHdr->OptionalHeader;
		epRva = pOptHdr->AddressOfEntryPoint;
	} else  {
		IMAGE_OPTIONAL_HEADER32 optHdr = ntHdr->OptionalHeader;
		epRva = optHdr.AddressOfEntryPoint;
	}
	return epRva;
}

DWORD CPeRebuilder::GetEntryPointRVA() {
	if (!g_pBuff) this->MapPE();
	return (GetEntryPointRVA(g_pBuff));
}

// Set Entry Point VA of this PE file
bool CPeRebuilder::SetEntryPointVA(DWORD epVa) {
	if (!g_pBuff) this->MapPE();
	return SetEntryPointVA(epVa, this->g_pBuff);
}

bool CPeRebuilder::SetEntryPointVA(DWORD epVa, LPBYTE peHdr) {
	if (!peHdr) return false;
	PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)peHdr;
	PIMAGE_NT_HEADERS ntHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)dosHdr->e_lfanew + (ULONG_PTR)dosHdr);
	ntHdr->OptionalHeader.AddressOfEntryPoint = epVa - ntHdr->OptionalHeader.ImageBase;
	return true;
}

// Set Entry Point RVA of this PE file
bool CPeRebuilder::SetEntryPointRVA(DWORD epRva) {
	if (!g_pBuff) this->MapPE();
	return SetEntryPointVA(epRva, this->g_pBuff);
}

bool CPeRebuilder::SetEntryPointRVA(DWORD epRva, LPBYTE peHdr) {
	if (!peHdr) return false;
	PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)peHdr;
	PIMAGE_NT_HEADERS ntHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)dosHdr->e_lfanew + (ULONG_PTR)dosHdr);
	ntHdr->OptionalHeader.AddressOfEntryPoint = epRva;
	return true;
}

// Move Bound Directory to free PE space
bool CPeRebuilder::AdjustBoundDir(IMAGE_DOS_HEADER * dosHdr) {
	LPBYTE buff = (LPBYTE)dosHdr;
	PIMAGE_NT_HEADERS ntHdr = (PIMAGE_NT_HEADERS)((ULONG_PTR)dosHdr->e_lfanew + (ULONG_PTR)dosHdr);
	int numSect = ntHdr->FileHeader.NumberOfSections;
	PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(ntHdr);

	DWORD * pBoundOffset = &ntHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;
	DWORD pBoundSize = ntHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size;
	if ((*pBoundOffset) == 0 || !pBoundSize)
		return false;
	int freeSpace = (int)(sections[0].PointerToRawData - (*pBoundOffset + pBoundSize)); 
	// Check if there is sufficent space
	if (freeSpace < sizeof(IMAGE_SECTION_HEADER)) 
		return false;

	RtlCopyMemory(buff + (*pBoundOffset) + freeSpace, buff + (*pBoundOffset), pBoundSize);
	RtlZeroMemory(buff + (*pBoundOffset), freeSpace);

	// Update offset
	(*pBoundOffset) += freeSpace;
	return true;
}

// Create a new empty PE 
LPBYTE CPeRebuilder::NewPe(DWORD Size) {
	if (this->g_hFileMap) UnmapPE();
	if (this->g_hFile) Close();

	if (Size > 1024 * 1024 * 1024)
		// Too big
		return NULL;

	g_dwMapMaxSize = Size + 0x8000;
	g_hFileMap = CreateFileMapping(NULL, NULL, PAGE_READWRITE, NULL, g_dwMapMaxSize, NULL);
	if (!g_hFileMap) return false;

	if (g_pBuff) {UnmapViewOfFile(g_pBuff); g_pBuff = NULL; }
	g_pBuff = (LPBYTE)MapViewOfFile(g_hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, Size);
	if (!g_pBuff) return NULL;

	g_dwMapSize = Size;
	g_pLog->WriteLine(L"NewPe - Successfully created an empty PE of %i Kb.", (LPVOID)(Size / 1024));
	if (g_strFileName) {delete[] g_strFileName; g_strFileName = NULL; }
	return g_pBuff;
}


bool CPeRebuilder::Open(LPTSTR fileName) {
	if (!fileName) return false;
	HANDLE hFile = NULL;
	DWORD lastErr = 0;

	if (this->g_hFile) Close();

	hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	lastErr = GetLastError();
	if (lastErr == ERROR_SHARING_VIOLATION || lastErr == ERROR_ACCESS_DENIED) 
		hFile = CreateFile(fileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		g_pLog->WriteLine(L"CPeRebuilder::Open - Unable to open \"%s\" file. CreatFile last error: %i", (LPVOID)fileName, (LPVOID)lastErr);
		return false;
	}

	// Get file Size
	g_dwFileSize = ::GetFileSize(hFile, NULL);

	// Now copy file name
	int len = wcslen(fileName);
	g_strFileName = new TCHAR[len + 2];
	wcscpy_s(g_strFileName, len+2, fileName);

	this->g_hFile = hFile;
	return true;
}

// Unmap PE File from memory
bool CPeRebuilder::UnmapPE() {
	if (!this->g_hFileMap) 
		return false;
	if (g_pBuff) UnmapViewOfFile(g_pBuff);
	g_pBuff = NULL;
	CloseHandle(g_hFileMap);
	g_hFileMap = NULL;
	g_dwMapSize = 0;
	g_dwMapMaxSize = 0;
	return true;
}

bool CPeRebuilder::Close() {
	UnmapPE();

	// Release user buffer
	if (g_pBuff) {
		VirtualFree(g_pBuff, 0, MEM_RELEASE | MEM_COMMIT);
		g_pBuff = NULL;
		g_dwMapSize = 0;
	}

	// Close file handle
	if (this->g_hFile) {
		CloseHandle(g_hFile);
		g_hFile = NULL;
		g_dwFileSize = 0;
	}

	// Cleanup file name string
	if (g_strFileName) {
		delete g_strFileName;
		g_strFileName = NULL;
	}

	return true;
}


// Check PE file Signature
bool CPeRebuilder::IsValidPE(LPBYTE buff) {
	if (!buff) return false;
	__try {
		IMAGE_DOS_HEADER * dosHdr = (IMAGE_DOS_HEADER *)buff;
		IMAGE_NT_HEADERS * ntHdr = (PIMAGE_NT_HEADERS)(dosHdr->e_lfanew + buff);
		if (dosHdr->e_magic == 0x5A4D && ntHdr->Signature == 0x4550) {
			return true;
		}

	} __except (EXCEPTION_EXECUTE_HANDLER) {
	}
	return false;
}

// Check PE file Signature
bool CPeRebuilder::IsValidPE(LPTSTR strFileName) {
	HANDLE hFile = NULL;					// File Handle
	LPBYTE buff = new BYTE[4096];			// Read buffer (small as enough)
	BOOL retVal = FALSE;					// Win32 API returned value
	DWORD numBytesRead = 0;					// Number of bytes read
	bool isValid = false;					// Is fileName a valid executable File

	hFile = CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	retVal = ReadFile(hFile, (LPVOID)buff, 4096, &numBytesRead, NULL);
	DWORD lastErr = GetLastError();
	if (hFile == INVALID_HANDLE_VALUE || !retVal) {
		delete[] buff;
		return false; 
	}
	CloseHandle(hFile);
	isValid = IsValidPE(buff);
	delete[] buff;
	return isValid;
}

// Get PE Executable Type
PE_TYPE CPeRebuilder::GetPeType() {
	bool retVal = false;
	bool bForcedMap = false;
	LPBYTE buff = NULL;
	PE_TYPE retType = PE_TYPE_UNKNOWN;

	if (!g_hFile) return PE_TYPE_UNKNOWN;
	if (!g_pBuff) {
		buff = MapPE(4, 4096);
		bForcedMap = true; 
	} else
		buff = g_pBuff;

	retVal = IsValidPE(g_pBuff);
	IMAGE_DOS_HEADER * dosHdr = (IMAGE_DOS_HEADER *)g_pBuff;
	IMAGE_NT_HEADERS * ntHdr = (PIMAGE_NT_HEADERS)(dosHdr->e_lfanew + g_pBuff);

	if (ntHdr->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
		retType = PE_TYPE_X86;
	else if (ntHdr->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
		retType = PE_TYPE_AMD64;
	else
		retType = PE_TYPE_UNKNOWN;

	if (bForcedMap) UnmapPE();
	return retType;
}



