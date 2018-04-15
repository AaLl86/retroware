/**********************************************************************
*   Talos Tiny Hypervisor
*	Filename: Debug.cpp
*	Implement the hypervisor Debug Functions
*	Last revision: 01/30/2016
*
*   Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
*	All right reserved
**********************************************************************/

#include "stdafx.h"
#include "Debug.h"

#pragma region Debug Functions
BOOLEAN g_bOldDbgState = FALSE;

//DPFLTR_SYSTEM_ID = 0x0 - DPFLTR_DEFAULT_ID = 0x65h	
NTSTATUS EnableDebugOutput() {
	NTSTATUS ntStatus = STATUS_SUCCESS;

	ntStatus = DbgQueryDebugFilterState(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL);
	if (ntStatus == FACILITY_DEBUGGER) g_bOldDbgState = TRUE;
	else g_bOldDbgState = FALSE;
	// Now set new Mask
	ntStatus = DbgSetDebugFilterState(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, TRUE);
	return ntStatus;
}

VOID RevertToDefaultDbgSettings() {
	DbgSetDebugFilterState(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, g_bOldDbgState);
}

PVOID DbgAllocateMemory(IN POOL_TYPE  PoolType, IN SIZE_T  NumberOfBytes, IN ULONG  Tag) {
	PVOID retBuff = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
	DbgPrint("ItsecMem - Allocated 0x%08X bytes at base address 0x%08X, Tag '%.04s' from ItSec driver, %s.\r\n", 
		NumberOfBytes, (LPBYTE)retBuff, (LPSTR)&Tag, (PoolType == NonPagedPool ? "NonPaged Pool" : "Paged Pool"));
	return retBuff;
}

VOID DbgFreeMemory(PVOID pMem) {
	ExFreePool(pMem);
	DbgPrint("ItsecMem - DeAllocated memory at base address 0x%08X from ItSec driver.\r\n", (LPBYTE)pMem);
}
#pragma endregion