#pragma once
#include "GPTDefs.h"
#include "SystemAnalyzer.h"
#include "log.h"

class CBootkitGptSetup {
public:
	//Default constructor
	CBootkitGptSetup(void);
	// Constructor with an initialized log
	CBootkitGptSetup(CLog & curLog);
	// Destructor
	~CBootkitGptSetup();

	// Get if bootkit is currently installed in this System on a particular volume
	BOOTKIT_STATE GetBootkitState(LPTSTR volName = NULL, bool bIsRemovable = false);

	// Setup UEFI Bootkit to a particular volume
	bool InstallEfiBootkit(LPTSTR volName = NULL, bool bIsRemovable = false);

	// Remove UEFI Bootkit from a particular volume
	bool RemoveEfiBootkit(LPTSTR volName = NULL, bool bIsRemovable = false);

	// Get UEFI System Volume name
	LPTSTR GetEfiSystemBootVolumeName();

	// Get EFI System boot partition 
	bool GetEfiSystemPartition(LPTSTR devName, GUID_PARTITION * pSysPart, bool * pbIsGptPartition, bool bUseOnlyFirmware = false);
	
private:
	CLog * g_pLog;
	bool g_bIsMyOwnLog;

};