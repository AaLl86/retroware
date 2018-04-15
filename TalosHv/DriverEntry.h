/**********************************************************************
 *  Driver Model 2012
 *	Filename: DriverEntry.h
 *	Implement Driver Entry point and startup functions prototypes
 *	Last revision: 05/07/2013
 *
 *  Copyright© 2012 Andrea Allievi
 *	All right reserved
 **********************************************************************/
#pragma once

// Driver entry point
DDKBUILD NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath);

// Driver unload routine
VOID DriverUnload(PDRIVER_OBJECT pDrvObj);

// The DPC routine that Virtualize the processor
EXTERN_C VOID VmxThread(PVOID pContext);

// The Unload DPC routine
EXTERN_C VOID VmxUnloadThread(PVOID pContext);
