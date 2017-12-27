/**********************************************************************
 * 
 *  AaLl86 Bootkit project
 *	Filename: MbrDefs.h
 *	Description: Master Boot Record data structures 
 *	Last revision: 05/09/2011
 *
 *  Copyright© 2012 Andrea Allievi
 *	All right reserved
 **********************************************************************/
#pragma once

// Strutture dati che descrivono il MBR (vedi http://en.wikipedia.org/wiki/Master_boot_record)
// Elemento della Partition Table
#pragma pack(1) 

// Struttura per un indirizzo CHS
typedef struct CHSValue{
	BYTE Head;
	BYTE Sector: 6;
	BYTE HiCyl: 2;
	BYTE LowCyl;
} *PCHSValue;

typedef struct PARTITION_ELEMENT{
	BYTE Bootable;						// + 0x00 - 0x80 if partition is bootable, 0 otherwise
	CHSValue StartCHS;					// + 0x01 - Starting Chyl, Head, Sector
	BYTE FileSystemType;				// + 0x04 - File System Type 
	CHSValue LastCHS;					// + 0x05 - Ending Chyl, Head, Sector
	DWORD FreeSectorsBefore;			// + 0x08 - Free Sector before (LBA format)
	DWORD TotalSectors;					// + 0x0C - Partition total number of sectors 
} *PPARTITION_ELEMENT;

#define MBPSIZE 440						// Dimensione del Master Boot Program (escluso i 4 bytes necessari allo startup di Windows)
typedef struct _MASTER_BOOT_RECORD{
	BYTE MBP[MBPSIZE];						// Master Boot Program (+ 0x00)
	DWORD dwDiskId;							// Windows DiskID (+ 0x1B8)
	BYTE pad[2];							// + 0x1BC
	struct {								// Partition Table composed of 4 partitions (+ 0x1BE)
		PARTITION_ELEMENT Partition[4];		
	} PTable;
	WORD signature;							// Firma dell'MBR
}MASTER_BOOT_RECORD, * PMASTER_BOOT_RECORD;

// NTFS Boot Sector 
typedef struct _NTFS_BOOT_SECTOR {
	BYTE chJumpInstruction[3];			//  +0x00
	char chOemID[8];					//  +0x03
	struct NTFS_BPB {				
		WORD wBytesPerSec;				//  +0x0B
		BYTE uchSecPerClust;			//  +0x0D
		WORD wReservedSec;				//  +0x0E
		BYTE uchReserved[3];			//  +0x10
		WORD wUnused1;					//  +0x13
		BYTE uchMediaDescriptor;		//  +0x15
		WORD wUnused2;					//  +0x16
		WORD wSecPerTrack;				//  +0x18
		WORD wNumberOfHeads;			//  +0x1A
		QWORD dqHiddenSec;				//  +0x1C
		DWORD dwUnused4;				//  +0x24
		LONGLONG dwTotalSec;			//  +0x28
		LONGLONG dqMFTLCN;				//  +0x30
		LONGLONG dqMFTMirrorLCN;		//  +0x38
		int nClustPerMFTRecord;			//  +0x40
		int nClustPerIndexRecord;		//  +0x44
		LONGLONG dqVolumeSerialNum;		//  +0x48
		DWORD dwChecksum;				//  +0x50
	} BPB;
	BYTE chBootstrapCode[426];			//  +0x54
	WORD wSecMark;						//  +0x1FE
} NTFS_BOOT_SECTOR, * PNTFS_BOOT_SECTOR;

typedef struct _NTFS_BOOT_PROGRAM {
	WORD wBtmgrNameLen;				// + 0x00 - Boot program string length (in WCHAR), usually is 0x07
	TCHAR wsBootmgr[7];				// + 0x02 - "BOOTMGR" Unicode string 
	WORD wAttrNameLen;				// + 0x10 - Boot program file attribute type string length (in WCHAR), usually is 0x04
	TCHAR wsAttrName[4];			// + 0x12 - Boot program attribute type name ($I30 equals to $FILE_NAME)
	BYTE pad[0x40];					// + 0x16 - Padding
	DWORD dwJmpOpcode;				// + 0x56 - Starting JMP opcode trampoline
	WORD wOldBootNameLen;			// + 0x5A - Compatible boot program name string length (in WCHAR)
	TCHAR wsLoader[0x10];			// + 0x5C - Real Mode Boot startup file
} NTFS_BOOT_PROGRAM, *PNTFS_BOOT_PROGRAM; 
#pragma pack()

#define MBR_SIGNATURE 0xAA55
#define NTFS_SIGNATURE "NTFS    "
#define FAT32_MBR_FSTYPE 0x0B
#define FAT32LBA_MBR_FSTYPE 0x0C
#define NTFS_MBR_FSTYPE 0x07

