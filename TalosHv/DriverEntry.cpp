/**********************************************************************
 *  Talos Tiny Hypervisor 
 *	Filename: DriverEntry.cpp
 *	Implement Driver Entry point and startup functions
 *	Last revision: 01/30/2016
 *
 *  Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
 *	All right reserved
 **********************************************************************/

#include "stdafx.h"
#include "DriverEntry.h"
#include "VmxInit.h"
#include "Debug.h"
#include <intrin.h>

// Allocate the DPC routines in the right code section:
#pragma alloc_text(".nonpaged", VmxThread)
#pragma alloc_text(".nonpaged", VmxUnloadThread)

DWORD g_dwNumOfProcs = 0;							// Number of current active processors
DWORD g_dwNumOfVmxCpus = 0;							// Number of Virtualized CPU


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath) {
	KAFFINITY activeProcessorsMask = 0;				// The affinity mask of the current active processors
	DWORD dwCounter = 0;							// Generic counter
	NTSTATUS ntStatus = STATUS_SUCCESS;

	// Driver entry point!
	pDriverObject->DriverUnload = DriverUnload;

#ifdef _DEBUG
	EnableDebugOutput();
	DbgBreak();
#endif

	// Put the needed routines in the NonPaged pool
	MmLockPagableCodeSection(VmxThread);

	// Enter the VMX for each active processor:
	g_dwNumOfProcs = KeQueryActiveProcessorCount(&activeProcessorsMask);

	for (DWORD i = 0; i < 64; i++) {
		OBJECT_ATTRIBUTES oa = { 0 };
		CLIENT_ID cid = { 0 };
		HANDLE thrHandle = NULL;
		if ((activeProcessorsMask & (1i64 << i)) == 0) continue;

		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		ntStatus = PsCreateSystemThread(&thrHandle, GENERIC_ALL, &oa, NULL, &cid, VmxThread, (PVOID)i);		
		if (NT_SUCCESS(ntStatus)) {
			dwCounter++;
			ZwClose(thrHandle);
		}

		if (dwCounter >= g_dwNumOfProcs) break;
		/* CODE that make use of the DPC:
		BOOL bRetVal = FALSE;							// Returned value
		PKDPC pkDpc = NULL;
		pkDpc = (PKDPC)ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC), MEMTAG);
		RtlZeroMemory(pkDpc, sizeof(KDPC));
		KeInitializeDpc(pkDpc, VmxDpc, (PVOID)i);
		KeSetTargetProcessorDpc(pkDpc, (CHAR)i);
		bRetVal = KeInsertQueueDpc(pkDpc, NULL, NULL);		*/
	}


	// Wait all the processor
	while (InterlockedCompareExchange((LONG*)&g_dwNumOfVmxCpus, g_dwNumOfProcs, g_dwNumOfProcs) != (LONG)g_dwNumOfProcs) {
		// We are at PASSIVE_LEVEL IRQL
		ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
		LARGE_INTEGER timeout = { 0 };
		timeout.QuadPart = -(100 * 1000 * 10);			// 100 milliseconds
		KeDelayExecutionThread(KernelMode, FALSE, &timeout);
	}

	return STATUS_SUCCESS;
}

// The DPC routine that Virtualize the processor
VOID VmxThread(PVOID pContext) {
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	DWORD dwProcNumber = (DWORD)((ULONG_PTR)pContext);

	KAFFINITY procAffinity = (KAFFINITY)(1 << dwProcNumber);
	KAFFINITY oldAffinity = KeSetSystemAffinityThreadEx(procAffinity);

	DbgPrint("VmxThread - Virtualizing CPU #%i...\r\n", dwProcNumber);
	ntStatus = CheckVmxSupport();
	if (NT_SUCCESS(ntStatus)) {
		ntStatus = InitializeVM();
		// Here it should return after the CPU is in VMX mode
	}
	else
		DbgPrint("VmxThread - Error 0x%08X while trying to virtualize CPU #%i.\r\n", (DWORD)ntStatus, dwProcNumber);

	// Increment the number of Virtualized CPU counter:
	InterlockedIncrement((LONG*)&g_dwNumOfVmxCpus);
	KeRevertToUserAffinityThreadEx(oldAffinity);
	PsTerminateSystemThread(STATUS_SUCCESS);
}

// The Unload DPC routine
VOID VmxUnloadThread(PVOID pContext) {
	DWORD dwProcNumber = (DWORD)((ULONG_PTR)pContext);
	PER_PROCESSOR_VM_DATA * lpProcData = NULL;

	KAFFINITY procAffinity = (KAFFINITY)(1 << dwProcNumber);
	KAFFINITY oldAffinity = KeSetSystemAffinityThreadEx(procAffinity);

	DbgPrint("VmxUnloadThread - Unloading the VM of CPU #%i...\r\n", dwProcNumber);
	AsmVmxCall(VMX_UNLOAD_CALL_CODE, (LPVOID)&lpProcData);

	if (lpProcData) {
		// VMX is dead, free up the memory
		if (lpProcData->MsrBitmap)
			MmFreeContiguousMemory(lpProcData->MsrBitmap);
		if (lpProcData->VmcsRegion)
			MmFreeContiguousMemory(lpProcData->VmcsRegion);
		if (lpProcData->VmmStackTop)
			MmFreeContiguousMemory(lpProcData->VmmStackTop);
		if (lpProcData->VmxonRegion)
			MmFreeContiguousMemory(lpProcData->VmxonRegion);

		// Finally free the PER_PROCESSOR_VM_DATA
		ExFreePool(lpProcData);
	}
	// Decrement the number of Virtualized CPU counter:
	InterlockedDecrement((LONG*)&g_dwNumOfVmxCpus);
	KeRevertToUserAffinityThreadEx(oldAffinity);
	PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID DriverUnload(PDRIVER_OBJECT pDrvObj) {
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	KAFFINITY activeProcessorsMask = 0;				// The affinity mask of the current active processors
	DWORD dwCounter = 0;

	// The driver unload routine
#ifdef _DEBUG
	DbgPrint("Driver Unload was called!\r\n");
	RevertToDefaultDbgSettings();
#endif

	KeQueryActiveProcessorCount(&activeProcessorsMask);

	for (LONG i = 0; i < sizeof(KAFFINITY) * 8; i++) {
		if ((activeProcessorsMask & (1i64 << i)) == 0) continue;
		OBJECT_ATTRIBUTES oa = { 0 };
		CLIENT_ID cid = { 0 };
		HANDLE thrHandle = NULL;
		if ((activeProcessorsMask & (1i64 << i)) == 0) continue;

		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		ntStatus = PsCreateSystemThread(&thrHandle, GENERIC_ALL, &oa, NULL, &cid, VmxUnloadThread, (PVOID)i);
		if (NT_SUCCESS(ntStatus)) {
			dwCounter++;
			ZwClose(thrHandle);
		}

		if (dwCounter >= g_dwNumOfProcs) break;
	}

	// Wait all the processor
	while (InterlockedCompareExchange((LONG*)&g_dwNumOfVmxCpus, 0, 0) > 0) {
		// We are at PASSIVE_LEVEL IRQL
		ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
		LARGE_INTEGER timeout = { 0 };
		timeout.QuadPart = -(100 * 1000 * 10);			// 100 milliseconds
		KeDelayExecutionThread(KernelMode, FALSE, &timeout);
	}
}