#pragma once
#include "log.h"

class CBootkitGptSetup;

enum BIOS_SYSTEM_TYPE {
	BIOS_TYPE_UNKNOWN = 0,
	BIOS_TYPE_MBR,
	BIOS_TYPE_GPT
};

enum BOOTKIT_STATE {
	BOOTKIT_NOT_PRESENT = 0,
	BOOTKIT_INSTALLED = 1,
	BOOTKIT_DAMAGED = 2,
	BOOTKIT_STATE_UNKNOWN = 99
};


class CSystemAnalyzer {
public:
	// Default constructor
	CSystemAnalyzer(void);
	// Log constructor
	CSystemAnalyzer(CLog & log);
	// Default destructor
	~CSystemAnalyzer(void);

public:
	// Obtain information of Current Os Version
	static bool GetOsVersionInfo(LPTSTR * outVerStr, bool * isAmd64 = NULL);

	// Get if this system is a GPT or MBR one
	BIOS_SYSTEM_TYPE GetCurrentBiosType(bool bFastDetect = false);
	
	// Deep analyze system disk and get if it is really a GPT one
	BIOS_SYSTEM_TYPE EnsureSystemBiosType();

	// Get Windows system startup physical disk
	bool GetStartupPhysDisk(DWORD * pDskNum);

	// Set new log class instance
	bool SetLog(CLog & log);

	// Generate System report and save it in a file
	bool GenerateSystemReport(LPTSTR outFile, DWORD dskIdx = (DWORD)-1);

	// Get if Secure boot is active on this system
	bool IsSecureBootActive(bool * pIsActive);

private:
	// This class log instance
	CLog * g_pLog;
	bool g_bIsMyLog;
	// Bootkit GPT setup temporary instance
	CBootkitGptSetup * g_pTmpGptSetup;
	// Current system bios type
	BIOS_SYSTEM_TYPE g_sysBiosType;
};

#pragma region Undocumented NTDLL exported APIs and data structures
#define STATUS_SUCCESS 0x0000L
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004
typedef __success(return >= 0) LONG NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,						// 0 Y N
	SystemProcessorInformation,					// 1 Y N
	SystemPerformanceInformation,				// 2 Y N
	SystemTimeOfDayInformation,					// 3 Y N
	SystemNotImplemented1,						// 4 Y N
	SystemProcessesAndThreadsInformation,		// 5 Y N
	SystemCallCounts,							// 6 Y N
	SystemConfigurationInformation,				// 7 Y N
	SystemProcessorTimes,						// 8 Y N
	SystemGlobalFlag,							// 9 Y Y
	SystemNotImplemented2,						// 10 Y N
	SystemModuleInformation,					// 11 Y N
	SystemLockInformation,						// 12 Y N
	SystemNotImplemented3,						// 13 Y N
	SystemNotImplemented4,						// 14 Y N
	SystemNotImplemented5,						// 15 Y N
	SystemHandleInformation,					// 16 Y N
	SystemObjectInformation,					// 17 Y N
	SystemPagefileInformation,					// 18 Y N
	SystemInstructionEmulationCounts,			// 19 Y N
	SystemInvalidInfoClass1						// 20
} SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS;

NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, 
	PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

typedef struct _SYSTEM_MODULE_INFORMATION { // Information Class 11
	ULONG Reserved[2];
	PVOID Base;
	ULONG Size;
	ULONG Flags;
	USHORT Index;
	USHORT Unknown;
	USHORT LoadCount;
	USHORT ModuleNameOffset;
	CHAR ImageName[256];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef struct _SYSTEM_MODULES {
	DWORD numOfModules;
	SYSTEM_MODULE_INFORMATION modules[ANYSIZE_ARRAY];
} SYSTEM_MODULES, * PSYSTEM_MODULES;

#pragma endregion
