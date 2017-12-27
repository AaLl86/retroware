/**********************************************************************
 * 
 *  AaLl86 Bootkit project
 *	Filename: GptDefs.h
 *	Description: UEFI and GPT partitions data structures
 *	Last revision: 05/02/2013
 *
 *  Copyright© 2013 Andrea Allievi
 *	All right reserved
 **********************************************************************/
#pragma once

// EFI Device path protocol
typedef struct _EFI_DEVICE_PATH_PROTOCOL {
	UINT8 Type;
	UINT8 SubType;
	UINT16 Length;
} EFI_DEVICE_PATH_PROTOCOL;

typedef struct _EFI_LOAD_OPTION {
	UINT32 Attributes;
	UINT16 FilePathListLength;
	WCHAR Description[ANYSIZE_ARRAY];
//	EFI_DEVICE_PATH_PROTOCOL FilePathList[1];
//	UINT8 OptionalData[1];
} EFI_LOAD_OPTION, *PEFI_LOAD_OPTION;

#define EFI_GLOBAL_VARIABLE_GUID {0x8BE4DF61,0x93CA,0x11d2,0xAA,0x0D,0x00,0xE0,0x98,0x03,0x2B,0x8C}
#define EFI_GLOBAL_VARIABLE_GUID_STRING L"{8BE4DF61-93CA-11D2-AA0D-00E098032B8C}"

typedef struct _GPT_PARTITION_HEADER {
	CHAR signature[8];					// + 0x00 - "EFI PART" Signature 
	DWORD revision;						// + 0x08 - Header revision
	DWORD hdrSize;						// + 0x0C - Header size
	DWORD hdrCrc;						// + 0x10 - CRC32 of header (offset +0 up to header size), with this field zeroed during calculation
	DWORD reserverd;					// + 0x14 - Must be zero
	QWORD currentLba;					// + 0x18 - Current header LBA
	QWORD backupLba;					// + 0x20 - Backup header LBA
	QWORD firstUsableLba;				// + 0x28 - First usable LBA for partitions (primary partition table last LBA + 1)
	QWORD lastUsableLba;				// + 0x30 - Last usable LBA (secondary partition table first LBA - 1)
	GUID diskGuid;						// + 0x38 - Disk GUID
	QWORD partEntriesStartLba;			// + 0x48 - Starting LBA of array of partition entries (always 2 in primary copy)
	DWORD numOfPartitions;				// + 0x50 - Number of partition entries in array
	DWORD sizeOfPartEntry;				// + 0x54 - Size of a single partition entry (usually 128)
	DWORD partArrayCrc;					// + 0x58 - CRC32 of partition array
	DWORD endOfStruct[ANYSIZE_ARRAY];	// + 0x5C - Reserved; must be zeroes for the rest of the block
} GPT_PARTITION_HEADER, * PGPT_PARTITION_HEADER;

// GUID Mbr partition file system index
#define EFI_GUID_PARTITION_FS_INDEX 0xEE
// GUID Partition entry
typedef struct _GUID_PARTITION {
	GUID partitionType;
	GUID uniqueId;
	QWORD startLba;
	QWORD endLba;
	QWORD flags;
	union {
		struct {
			TCHAR mustBeZero;			// If zero this structure is valid
			WORD partNum;				// Partition number
			BYTE partFormat;			// Partition format: 0x01 – PC-AT compatible legacy MBR. 0x02 – GUID Partition Table. Others are reserved values
			BYTE signatureType;			// Signature type: 0x00 – No Disk Signature. 
										// 0x01 - MBR. 0x02 – GUID signature.
		} partInfo;
		TCHAR partName[36];
	} u;
} GUID_PARTITION, *PGUID_PARTITION;

// UEFI Partition types GUIDs
const GUID EFI_SYSTEM_PARTITION_GUID = {0xC12A7328, 0xF81F, 0x11D2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B};
const GUID EFI_BASIC_DATA_PARTITION_GUID = {0xEBD0A0A2, 0xB9E5, 0x4433, 0xC0, 0x87, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7};
