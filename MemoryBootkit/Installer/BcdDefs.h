/**********************************************************************
 * 
 *  AaLl86 Bootkit project
 *	Filename: BcdDefs.h
 *	Description: Windows Boot Configuration Data definitions
 *	Last revision: 14/02/2013
 *
 *  Copyright© 2013 Saferbytes - Andrea Allievi
 *	All right reserved
 **********************************************************************/
#pragma once

#define GUID_WINDOWS_BOOTMGR_STRING L"{9DEA862C-5CDD-4E70-ACC1-F32B344D4795}" 
#define BCDAPPDEVICE_ELEMENT_STRING L"11000001"
const GUID GUID_WINDOWS_BOOTMGR = { 0x9DEA862C, 0x5CDD, 0x4E70, 0xAC, 0xC1, 0xF3, 0x2B, 0x34, 0x4D, 0x47, 0x95 };


typedef enum BcdLibraryElementTypes { 
  BcdLibraryDevice_ApplicationDevice                  = 0x11000001,
  BcdLibraryString_ApplicationPath                    = 0x12000002,
  BcdLibraryString_Description                        = 0x12000004,
  BcdLibraryString_PreferredLocale                    = 0x12000005,
  BcdLibraryObjectList_InheritedObjects               = 0x14000006,
  BcdLibraryInteger_TruncatePhysicalMemory            = 0x15000007,
  BcdLibraryObjectList_RecoverySequence               = 0x14000008,
  BcdLibraryBoolean_AutoRecoveryEnabled               = 0x16000009,
  BcdLibraryIntegerList_BadMemoryList                 = 0x1700000a,
  BcdLibraryBoolean_AllowBadMemoryAccess              = 0x1600000b,
  BcdLibraryInteger_FirstMegabytePolicy               = 0x1500000c,
  BcdLibraryInteger_RelocatePhysicalMemory            = 0x1500000D,
  BcdLibraryInteger_AvoidLowPhysicalMemory            = 0x1500000E,
  BcdLibraryBoolean_DebuggerEnabled                   = 0x16000010,
  BcdLibraryInteger_DebuggerType                      = 0x15000011,
  BcdLibraryInteger_SerialDebuggerPortAddress         = 0x15000012,
  BcdLibraryInteger_SerialDebuggerPort                = 0x15000013,
  BcdLibraryInteger_SerialDebuggerBaudRate            = 0x15000014,
  BcdLibraryInteger_1394DebuggerChannel               = 0x15000015,
  BcdLibraryString_UsbDebuggerTargetName              = 0x12000016,
  BcdLibraryBoolean_DebuggerIgnoreUsermodeExceptions  = 0x16000017,
  BcdLibraryInteger_DebuggerStartPolicy               = 0x15000018,
  BcdLibraryString_DebuggerBusParameters              = 0x12000019,
  BcdLibraryInteger_DebuggerNetHostIP                 = 0x1500001A,
  BcdLibraryInteger_DebuggerNetPort                   = 0x1500001B,
  BcdLibraryBoolean_DebuggerNetDhcp                   = 0x1600001C,
  BcdLibraryString_DebuggerNetKey                     = 0x1200001D,
  BcdLibraryBoolean_EmsEnabled                        = 0x16000020,
  BcdLibraryInteger_EmsPort                           = 0x15000022,
  BcdLibraryInteger_EmsBaudRate                       = 0x15000023,
  BcdLibraryString_LoadOptionsString                  = 0x12000030,
  BcdLibraryBoolean_DisplayAdvancedOptions            = 0x16000040,
  BcdLibraryBoolean_DisplayOptionsEdit                = 0x16000041,
  BcdLibraryDevice_BsdLogDevice                       = 0x11000043,
  BcdLibraryString_BsdLogPath                         = 0x12000044,
  BcdLibraryBoolean_GraphicsModeDisabled              = 0x16000046,
  BcdLibraryInteger_ConfigAccessPolicy                = 0x15000047,
  BcdLibraryBoolean_DisableIntegrityChecks            = 0x16000048,
  BcdLibraryBoolean_AllowPrereleaseSignatures         = 0x16000049,
  BcdLibraryString_FontPath                           = 0x1200004A,
  BcdLibraryInteger_SiPolicy                          = 0x1500004B,
  BcdLibraryInteger_FveBandId                         = 0x1500004C,
  BcdLibraryBoolean_ConsoleExtendedInput              = 0x16000050,
  BcdLibraryInteger_GraphicsResolution                = 0x15000052,
  BcdLibraryBoolean_RestartOnFailure                  = 0x16000053,
  BcdLibraryBoolean_GraphicsForceHighestMode          = 0x16000054,
  BcdLibraryBoolean_IsolatedExecutionContext          = 0x16000060,
  BcdLibraryBoolean_BootUxDisable                     = 0x1600006C,
  BcdLibraryBoolean_BootShutdownDisabled              = 0x16000074,
  BcdLibraryIntegerList_AllowedInMemorySettings       = 0x17000077,
  BcdLibraryBoolean_ForceFipsCrypto                   = 0x16000079
} BcdLibraryElementTypes;

enum BCD_DEVICE_TYPE {
	BcdBootDevice = 1,					// Device that initiated the boot.
	BcdPartitionDevice = 2,				// Disk partition.
 	BcdFileDevice = 3,					// File that contains file system metadata and is treated as a device.
	BcdRamdiskDevice = 4,				// Ramdisk.
	BcdUnknownDevice = 5,				// Unknown.
	BcdQualifiedPartition = 6,			// Qualified disk partition.
	BcdLocateDevice  = 7,				// This value is not used.
	BcdLocateExDevice = 8				// Locate device.
};

typedef struct _BCD_DEVICE_DATA
{
	BYTE zeroed[0x10];				// + 0x00 - Zeroed or unknown data
	BCD_DEVICE_TYPE DeviceType;		// + 0x10 - Device type 
	DWORD reserved;					// + 0x14 - Reserved
	struct {
		DWORD buffSize;				// + 0x18 - Partition data size in bytes
		DWORD reserved;				// + 0x1C - Reserved
		union {
			struct {
				QWORD startOffset;		// + 0x20 - Partition start offset in bytes
				QWORD unknown[2];		// + 0x28 - Unknown values
				DWORD diskSignature;	// + 0x38 - Disk MBR Signature
				DWORD reserved;
			} MbrPartition;
			struct {
				GUID uniqueId;			// + 0x20 - Unique partition GUID
				QWORD unknown;
				GUID unknownGuid;
			}GptPartition;
		} u;
	}QualifiedPartitionType ;
} BCD_DEVICE_DATA, *PBCD_DEVICE_DATA;

