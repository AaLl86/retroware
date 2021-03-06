; COMMON Amd64 Macros

; Saves all general purpose registers to the stack
PUSHAQ MACRO
	; Size: 0x90
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    -1      ; dummy for rsp - aligment issues
    push    rbp
    push    rsi
    push    rdi
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
ENDM


; Loads all general purpose registers from the stack
POPAQ MACRO
	; Size: 0x90
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdi
    pop     rsi
    pop     rbp
    add     rsp, 8    ; dummy for rsp
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
ENDM
