;   Talos Hypervisor 0.1
;   Filename: x86Funcs.asm
;	Last revision: 01/30/2016
TITLE Driver Model x86 Assembler file
.386
.MODEL FLAT

;Declare an external function
;EXTERN ExternalCFunc: PROC

.data
_TestAsmFunc@0 PROC
mov eax, 0c098abh
mov edx, eax
ret
_TestAsmFunc@0 ENDP

.code

END