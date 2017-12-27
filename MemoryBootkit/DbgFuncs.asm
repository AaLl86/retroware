; AaLl86 Debug Function File
TITLE AaLl86 Debug Functions

.MODEL tiny, stdcall               ; Prima direttiva .model poi direttiva .386 altrimenti
.386                               ; codice pieno di 0x66h in giro per l'allinemaneto a 32 bit

DBGLOADLOC equ 7e00h            ; L'indirizzo in cui caricare le funzioni Dbg

.code
org                 DBGLOADLOC
; Tutto questo codice va in esecuzione dall'indirizzo 9f00:1000h

main:

; Scrive una stringa sullo schermo:
;        Input parametri: DS:SI puntatore alla stringa
; VIDEO - TELETYPE OUTPUT
;    AH = 0Eh
;    AL = character to write
;    BH = page number
;    BL = foreground color (graphics modes only)

WriteString PROC
     ; Premessa: la stringa è presente in DS:SI, la carico pezzo per pezzo in AL
     push ax
     push bx

WRITE:
     LODSB                            ; Carica un byte in AL da DS:SI
     OR	AL, AL			      ; last char ='\0'?
     JZ	NEXT_CHAR_RET                 ; Se l'ultimo carattere è 0 termina la procedura
     ; Utilizzo l'INT 10 con funzione TELETYPE OUTPUT, che mi scorre automaticamente lo schermo
     MOV	AH, 0Eh
     MOV	BX, 0
     PUSH	SI
     INT	10h
     POP	SI
     JMP short	WRITE
NEXT_CHAR_RET:
     pop bx
     pop ax
     RETF
WriteString ENDP

;
; Write a character to the screen.
; INPUT al = Carattere da scrivere
WriteChr PROC
     pushfd
     push bx
     mov ah,0Eh
     xor bx,bx
     int 10h
     pop bx
     popfd
     retf
WriteChr ENDP

; Ottiene un carattere da tastiera (risultato in AL)
GetChar PROC
      XOR AH, AH
      INT 16h
      RETF
GetChar ENDP

; Output di un Dump della memori all'indirizzo DS:SI
; In input DS:SI - puntatore alla memoria
;          CX - Lunghezza del dump
DumpMemory PROC                          ; USES AX BX DI   Sta direttiva non funziona con la retf
.data
    hexmap BYTE '0123456789ABCDEF',0
    dmpstr BYTE 'Mem: ', 123 DUP(0) 			; 128 bytes totali

.code
     ; Salvo i registri
     push ax
     push bx
     push di

     ;Sistema i segmenti
     ;mov ax, 0
     ;mov es, ax

     ; Controlla che CX non sia maggiore di 20h
     CMP CX, 20h
     JBE dm_go
     MOV CX, 20h

dm_go:
     CLD			; Clear Directional Flag (per STOSB LODSB)

comment @          ; riduciamo la grandezza del codice

     ; Inizializzo la memoria e i segmenti
     mov ax, 18h
     mov es, ax
     mov di, HEXMAP_OFFSET

     ; Scrivo la hexmap (dimensione 16 bytes)
     mov dword ptr es:[di], "3210"
     mov dword ptr es:[di+4h], "7654"
     mov dword ptr es:[di+8h], "BA98"
     mov dword ptr es:[di+0Ch], "FEDC"

     ; Scrivo la memoria su cui mettere il dump
     mov di, DMPSTR_OFFSET
     mov dword ptr es:[di], ":meM"
     mov byte ptr es:[di+4], 0

     ; Ora scrivo in memoria di destinazione
     push si
     mov si, offset dmpstr
     call WriteString
     pop si

     ; Pulisce la stringa puntata da si
     push cx
     push ax
     mov cx, 32
     mov al, 0
     mov di, offset dmpstr
     rep stosb
     pop ax
     pop cx
 @
     mov di, offset dmpstr        ; 20h = offset dmpstr
     push di                      ; DI tiene l'offset della stringa su cui scrivere
     add di, 4

     ; Carico un byte per volta
L1:
     ; High order
     XOR AX, AX
     mov al, byte ptr ds:[SI]      ; = LODSB, dec si
     shr al, 4			   ; Converto l'high order in byte hex
     mov bx, offset hexmap
     add bx, ax
     mov al, byte ptr cs:[bx]

     ; Scrive il byte nella stringa di destinazione (nel CS corrente)
     mov byte ptr cs:[DI], al      ; = STOSB
     inc di

     ; Low order
     XOR AX, AX
     mov al, byte ptr ds:[SI]      ; = LODSB
     inc si
     and al, 0Fh        ; Converto il low order in byte hex
     mov bx, offset hexmap
     add bx, ax
     mov al, byte ptr cs:[bx]

     ; Scrive il byte nella stringa di destinazione (nel CS corrente)
     mov byte ptr cs:[DI], al      ; = STOSB
     inc di

     ; Stampo lo spazio
     mov al, ' '
     mov byte ptr cs:[DI], al      ; = STOSB
     inc di

LOOP L1

     mov byte ptr cs:[di], 0				; Terminatore di controllo
     pop si
     push ds
     mov ax, cs
     mov ds, ax

     ; Emulo una far call (notazione little endian), in quanto nel codice della WriteString c'è una retf
     push cs
     call WriteString

     pop ds

     ; Lo sapevate che con la retf la direttiva USES non funziona??? Maledetto MASM
     pop di
     pop bx
     pop ax

     retf

DumpMemory ENDP
; TEST 04/04/2010 OOOKKK!!

END main

