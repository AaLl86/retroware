/**********************************************************************
 *  AaLl86 EFI Application Model 2013
 *	Filename: EfiMain.h
 *	Implement EFI Application Main entry point and startup functions
 *	Last revision: dd/mm/yyyy
 *
 *  Copyright© 2013 Andrea Allievi
 *	All right reserved
 **********************************************************************/

#include "stdafx.h"
#include "efimain.h"
#include "efiutils.h"

// Defined in Win8x86Patches.asm
#define PATCH_SIZE 0x8
extern "C" LPVOID WinloadPatchPrologue();
// Winload original patched code
BYTE g_winloadOrgCode[PATCH_SIZE];
// Winload target patch address
LPBYTE g_winloadPatchAddr = NULL;

// Search and patch Winload OslArchTransferToKernel signature
EFI_STATUS PatchWinload(LPBYTE startAddr, DWORD size) {
	LPBYTE oslAddr = NULL;								// Winload!OslArchTransferToKernel address
	LPBYTE patchAddr = NULL;							// Winload patch target address

	if (!startAddr || !size) return EFI_INVALID_PARAMETER;

	// Now search winload!OslArchTransferToKernel 
	BYTE winloadSignature[] = { 
		0x0F, 0x01, 0x15, 0x12, 0x34, 0x56, 0x78,		// lgdt    fword ptr [winload!OslKernelGdt (0037cb68)] ds:0030:0037cb68=80b9900003ff
		0x0F, 0x01, 0x1D, 0x9A, 0xBC, 0xDE, 0xFF,		// lidt    fword ptr [winload!OslKernelIdt (0037cb60)]
		0x66, 0xB8, 0x10, 0x00,							// mov     ax,10h      <-- Apply patch here
		0x66, 0x8E, 0xD8,								// mov     ds,ax
		0x66, 0x8E, 0xC0,								// mov     es,ax
		0x66, 0x8E, 0xD0,								// mov     ss,ax
		0x66, 0x8E, 0xE8,								// mov     gs,ax
		0x66, 0xB8, 0x30, 0x00,							// mov     ax,30h
		0x66, 0x8E, 0xE0,								// mov     fs,ax
		0x66, 0xB8, 0x28, 0x00,							// mov     ax,28h
		0x0f, 0x00, 0xD8,								// ltr     ax
		0x8B, 0x4C, 0x24, 0x04,							// mov	   ecx,dword ptr [esp+4]
		0x8B, 0x44, 0x24, 0x08							// mov	   eax,dword ptr [esp+8]
	};
	
	for (DWORD i = 0; i < size; i++) {
		BOOLEAN retVal = FALSE;
		retVal = (MemCompare(&startAddr[i], winloadSignature, 3) == 3);
		if (!retVal) continue;
		retVal = (MemCompare(&startAddr[i+14], &winloadSignature[14], 10) == 10);
		if (retVal) {
			//OslArchTransferToKernel found
			oslAddr = &startAddr[i];
			break;
		}
	}
	
	// winload!OslArchTransferToKernel modified:
	BYTE detour[] = { 
		0xB8, 0x11, 0x22, 0x33, 0x44,		// mov eax, addr_of_patchcode
		0xFF, 0xD0 };						// call eax
//		0xFF, 0xE0 };						// jmp eax

	// Check if I found right addr
	if (!oslAddr) return EFI_UNSUPPORTED;

	// Save original code
	patchAddr = oslAddr + 0xE;
	MemCpy((LPVOID)g_winloadOrgCode, (LPVOID)patchAddr, PATCH_SIZE);
	g_winloadPatchAddr = patchAddr;

	// Now apply patch
	*((DWORD*)(&detour[1])) = (DWORD)WinloadPatchPrologue;
	MemCpy((LPVOID)patchAddr, (LPVOID)detour, sizeof(detour));

	// Return success
	return EFI_SUCCESS;
}

// ItSec new OslArchTransferToKernel detour
EFI_STATUS InstallNtPatch(LPBYTE ntBaseAddr, DWORD size) {
	EFI_STATUS retStatus = EFI_SUCCESS;
	LPBYTE startAddr = NULL;					// Current search start 
	DWORD curMaxLen = 0;						// Maximum search len
	LPBYTE targetAddr = NULL;					// Patch target address
	DWORD i = 0;								// Counter

	if (!ntBaseAddr || !size) return EFI_INVALID_PARAMETER;

	// First of all, restore overwritten bytes
	if (g_winloadPatchAddr)
		MemCpy(g_winloadPatchAddr, g_winloadOrgCode, PATCH_SIZE);

	/* Windows 8 Nt MxMemoryLicense
	C7 45 F0 70 7B 8E 00  			mov     [ebp+var_10], offset aKernelWindowsm ; "Kernel-WindowsMaxMemAllowedx86"
	C7 45 E8 4A 78 8E 00    		mov     [ebp+var_18], offset aKernelMaxphysi ; "Kernel-MaxPhysicalPage"
	89 5D F4 						mov     [ebp+var_C], ebx
	89 5D FC					    mov     [ebp+dwLicenseValue], ebx
	89 4D F8 						mov     [ebp+var_8], ecx
	E8 xx xx xx xx    				call    _NtQueryLicenseValue@20 ; NtQueryLicenseValue(x,x,x,x,x)
	85 C0						    test    eax, eax
	78 4C 							js      short loc_915467
	8B 45 FC						mov     eax, [ebp+dwLicenseValue] (ebp-4)  
	85 C0						    test    eax, eax				
	74 xx							jz      short loc_915467			<-- Apply patch here
	c1 e0 08 						shl     eax, 8
	A3 xx xx xx xx					mov     ds:_MiTotalPagesAllowed, eax
	6A 04							push 4		*/

	BYTE memFirstSignature[] = {
		0x85, 0xC0,					// test     eax, eax
		0x78, 0x11,					// jx		xx
		0x8B, 0x45, 0xFC,			// mov      eax, [ebp-4]
		0x85, 0xC0,					// test     eax, eax
		0x74, 0x11,					// jz		xx
		0xC1, 0xE0, 0x08,			// shl     eax, 8
		0xA3, 0x11, 0x11, 0x11, 0x11,	// mov     ds:_MiTotalPagesAllowed, eax
	};

	BYTE firstPatch[] = {
		0xB8, 0x00, 0x00, 0xC0, 0x00		// mov eax, 0x0c00000 (48 GB max) 
	};

	// Windows 8 start offset and len (speed up entire search process)
	startAddr = ntBaseAddr + 0x500000;
	curMaxLen = 0xF0000;

	while (startAddr) {
		for (i = 0; i < curMaxLen; i++) {
			// Compare first block
			if (MemCompare(&startAddr[i], memFirstSignature, 2) < 2) continue;
			// Compare second block
			if (MemCompare(&startAddr[i+4], &memFirstSignature[4], 6) < 6) continue;
			// Compare last block
			if (MemCompare(&startAddr[i+11], &memFirstSignature[11], 4) < 4) continue;

			// Target address found
			targetAddr = &startAddr[i+9];
			break;
		}
		if (!targetAddr && ntBaseAddr != startAddr) {
			// Restart search with entire address space
			startAddr = ntBaseAddr; curMaxLen = size;
		}
		else
			break;
	}

	if (!targetAddr) 
		return EFI_LOAD_ERROR;

	// Apply here first patch
	MemCpy(targetAddr, firstPatch, sizeof(firstPatch));

	// Patch second place
	BYTE memSecondSignature[] = {
//		0xE8, 0xD6, 0xE8, 0xDE, 0xFF,	// call    _NtQueryLicenseValue@20 ; NtQueryLicenseValue(x,x,x,x,x)
		0x85, 0xC0,					// test     eax, eax
		0x78, 0x11,					// jx		xx
		0x8B, 0x45, 0xFC,			// mov      eax, [ebp-4]
		0x85, 0xC0,					// test     eax, eax
		0x74, 0x06,					// jz		xx
		0xC1, 0xE0, 0x08,			// shl      eax, 8
		0x48,						// dec		eax
		0x89, 0x07					// mov		[edi], eax
	};

	// Now search second patch place
	curMaxLen = 0xA0;
	startAddr = targetAddr;
	targetAddr = NULL;

	for (i = 0; i < curMaxLen; i++) {
		if (MemCompare(&startAddr[i], &memSecondSignature[4], 5) < 5) continue;
		if (MemCompare(&startAddr[i+(11-4)], &memSecondSignature[11], 4) < 3) continue;

		targetAddr = &startAddr[i-2];
		break;
	}

	// Apply patch and exit
	if (targetAddr) targetAddr[0] = 0xEB;		// JMP rel opcode
	return EFI_SUCCESS;
}