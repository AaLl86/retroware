/**********************************************************************
*   Talos Tiny Hypervisor
*	Filename: asm64.h
*	X64 Assembly functions declarations
*	Last revision: 01/30/2016
*
*  Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
*	All right reserved
**********************************************************************/
#pragma once
#include <ntddk.h>

#ifndef EXTERN_C
#define EXTERN_C extern "C"
#endif

EXTERN_C BOOLEAN InvokeCpuid(DWORD Index, CPUID_CONTEXT * lpCpuIdContext);

// VM Initialization function stub
EXTERN_C NTSTATUS InitializeVmStub();

// VM Exit point function
EXTERN_C NTSTATUS VmxExitHandlerAsm();

// VM Entry point function
EXTERN_C NTSTATUS VmxVmEntryPointAsm();

// Assembly Intrinsic functions:
EXTERN_C USHORT AsmReadES();

EXTERN_C void AsmWriteCS(_In_ USHORT SegmentSelector);

EXTERN_C USHORT AsmReadCS();

EXTERN_C void AsmWriteSS(_In_ USHORT SegmentSelector);

EXTERN_C USHORT AsmReadSS();

EXTERN_C void AsmWriteDS(_In_ USHORT SegmentSelector);

EXTERN_C USHORT AsmReadDS();

EXTERN_C void AsmWriteFS(_In_ USHORT SegmentSelector);

EXTERN_C USHORT AsmReadFS();

EXTERN_C void AsmWriteGS(_In_ USHORT SegmentSelector);

EXTERN_C USHORT AsmReadGS();

EXTERN_C void AsmWriteLDTR(_In_ USHORT LocalSegmengSelector);

EXTERN_C USHORT AsmReadLDTR();

EXTERN_C void AsmWriteTR(_In_ USHORT TaskRegister);

EXTERN_C USHORT AsmReadTR();

EXTERN_C void AsmWriteGDT(_In_ const GDTR *Gdtr);

EXTERN_C void AsmReadGDT(_Out_ GDTR *Gdtr);

EXTERN_C void AsmWriteLDTR(_In_ USHORT LocalSegmengSelector);

EXTERN_C USHORT AsmReadLDTR();

EXTERN_C void AsmInvalidateInternalCaches();

// Perform a VMCALL
EXTERN_C VMX_STATUS AsmVmxCall(ULONG_PTR HyperCallNumber, LPVOID lpCtx);

// Use the LAR instruction to load the access right for a segment (fetched from the GDTR)
EXTERN_C DWORD AsmLoadAccessRightsByte(WORD SegmentSelector);

