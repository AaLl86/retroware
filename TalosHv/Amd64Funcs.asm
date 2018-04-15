;   Talos Hypervisor 0.1
;   Filename: Amd64Funcs.asm
;	Last revision: 01/30/2016
;   Author: Andrea Allievi - Cisco Talos
TITLE Talos Hypervisor Assembler file
include asm64.inc

;Declare an external function
EXTERN InitializeVM_internal: PROC
EXTERN VmxExitHandler: PROC

.CONST
VM_INSTRUCTION_ERROR        EQU     00004400h
GUEST_RSP					EQU		0000681ch
GUEST_RIP					EQU		0000681eh
GUEST_RFLAGS				EQU		00006820h
VMX_OK                      EQU     0
VMX_ERROR_WITH_STATUS       EQU     1
VMX_ERROR_WITHOUT_STATUS    EQU     2

.data
	ciao DWORD 4

.code
; BOOLEAN InvokeCpuid(DWORD Index, LPVOID * lpCpuIdContext)
InvokeCpuid PROC
	MOV R8, RDX
	MOV RAX, RCX
	CPUID

	; Save the context
	TEST R8, R8
	JZ NoCtx
	MOV DWORD PTR [R8], EAX
	MOV DWORD PTR [R8+04], EAX
	MOV DWORD PTR [R8+08], EAX
	MOV DWORD PTR [R8+0Ch], EAX
	
	XOR EAX, EAX
	INC EAX

NoCtx:
	RET
InvokeCpuid ENDP

; VMX management routines
;VM Initialization function stub
;NTSTATUS InitializeVmStub();
InitializeVmStub PROC
	; Stub routine used to save the GP registers and grab the real RSP
	PUSHFQ
	PUSHAQ

	MOV RCX, RSP
	SUB RSP, 28H
	CALL InitializeVM_internal			; NTSTATUS InitializeVM_internal(lpGuestRsp)
	ADD RSP, 28H

	POPAQ
	POPFQ
	RET				; Return the status from InitializeVM_internal
InitializeVmStub ENDP

; The Guest Vm Entry point routine
VmxVmEntryPointAsm PROC
	popaq
	popfq

	xor rax, rax		; STATUS_SUCCESS
	ret				; Return to DriverEntry
	int 3
	int 3
VmxVmEntryPointAsm ENDP


; The VMX Exit handler routine
VmxExitHandlerAsm PROC
	; Function Duties: dump the working state of the GP registers and call the C++ Exit handler
    ; No need to save the flag registers since it is restored from the VMCS at the time of vmresume.
	SUB RSP, 10h			; Move the stack ptr to include even the PerProcessorVmData and the magic value
	PUSHAQ

	mov rcx, rsp			; Context argument
	sub rsp, 28h			; Allocate space for the arguments
	call VmxExitHandler		; BOOL vmContinue = VmxExitHandler(GuestContext);
	add rsp, 28h

	test eax, eax
	jz VmExit

	POPAQ
	vmresume

	; One or more errors occured
	int 3
	; Section 30.2 of System Programming Guide Volume:
	; VMFailInvalid: CF = 1; VmFailValid: ZF = 1
	jz VmErrorWithCode
	jc VmErrorNoCode
	
	VmExit:
	POPAQ
	
	; At this point my RSP register points to 
	; f0f0f0f0f0f0f0f0 <perProcessorVmData>
	; We need a way to recover the original Guest state
	mov rcx, GUEST_RSP			
	vmread rdx, rcx				; <-- RDX = Guest RSP

	mov rcx, GUEST_RFLAGS
	vmread r10, rcx

	mov rcx, GUEST_RIP
	vmread rcx, rcx				; <-- RCX = Guest RIP
	; We don't need to save the original content of the register because .... see VmxVmEntryPointAsm

	; Exit from the VM
	vmxoff

	jz VmErrorWithCode
	jc VmErrorNoCode
	
	push r10
	popfq				; RFLAGS <- [R10]
	mov rsp, rdx		; Restore the old Stack pointer
	push rcx
	ret					; Go to VmxVmEntryPointAsm - the return address of InitializeVM_internal

	VmErrorWithCode:
	; Grab the error code
	mov rcx, VM_INSTRUCTION_ERROR
	vmread rax, rcx				; <-- Put the error code in RAX

	VmErrorNoCode:
	int 3
	jmp VmErrorNoCode			; NO IDEA on how to exit from here
VmxExitHandlerAsm ENDP

; EXTERN_C VMX_STATUS AsmVmxCall(ULONG_PTR HyperCallNumber, void *Context);
; Executes vmcall with the given hypercall number and a context parameter.
AsmVmxCall PROC
    vmcall                  
    jz errorWithCode        ; if (ZF) jmp
    jc errorWithoutCode     ; if (CF) jmp
    xor rax, rax            ; return VMX_OK
    ret

errorWithoutCode:
    mov rax, VMX_ERROR_WITHOUT_STATUS
    ret

errorWithCode:
    mov rax, VMX_ERROR_WITH_STATUS
    ret
AsmVmxCall ENDP

; GDT
AsmWriteGDT PROC
    lgdt fword ptr [rcx]
    ret
AsmWriteGDT ENDP

AsmReadGDT PROC
    sgdt [rcx]
    ret
AsmReadGDT ENDP


; LDTR
AsmWriteLDTR PROC
    lldt cx
    ret
AsmWriteLDTR ENDP

AsmReadLDTR PROC
    sldt ax
    ret
AsmReadLDTR ENDP


; TR
AsmWriteTR PROC
    ltr cx
    ret
AsmWriteTR ENDP

AsmReadTR PROC
    str ax
    ret
AsmReadTR ENDP


; ES
AsmWriteES PROC
    mov es, cx
    ret
AsmWriteES ENDP

AsmReadES PROC
    mov ax, es
    ret
AsmReadES ENDP


; CS
AsmWriteCS PROC
    mov cs, cx
    ret
AsmWriteCS ENDP

AsmReadCS PROC
    mov ax, cs
    ret
AsmReadCS ENDP


; SS
AsmWriteSS PROC
    mov ss, cx
    ret
AsmWriteSS ENDP

AsmReadSS PROC
    mov ax, ss
    ret
AsmReadSS ENDP


; DS
AsmWriteDS PROC
    mov ds, cx
    ret
AsmWriteDS ENDP

AsmReadDS PROC
    mov ax, ds
    ret
AsmReadDS ENDP


; FS
AsmWriteFS PROC
    mov fs, cx
    ret
AsmWriteFS ENDP

AsmReadFS PROC
    mov ax, fs
    ret
AsmReadFS ENDP


; GS
AsmWriteGS PROC
    mov gs, cx
    ret
AsmWriteGS ENDP

AsmReadGS PROC
    mov ax, gs
    ret
AsmReadGS ENDP

; MISC

AsmLoadAccessRightsByte PROC
	; Loads the access rights from the segment descriptor specified by the second operand (source operand) into the
	; first operand (destination operand) and sets the ZF flag in the flag register.
    lar rax, rcx
    ret
AsmLoadAccessRightsByte ENDP

AsmInvalidateInternalCaches PROC
    invd
    ret
AsmInvalidateInternalCaches ENDP


END


