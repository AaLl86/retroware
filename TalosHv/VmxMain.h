/**********************************************************************
*   Talos Tiny Hypervisor
*	Filename: VmxMain.h
*	Virtual Machine Entry and Exit point definitions
*	Last revision: 01/30/2016
*
*  Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
*	All right reserved
**********************************************************************/
#pragma once
#include "VmxStructs.h"

// Context for the Exit handler routines
struct VMX_HANDLER_CONTEXT {
	union {
		VMX_HANDLER_STACK * GuestStack;
		GP_REGISTERS * gpRegs;
	};
	ULONG_PTR GuestRip;
	EFLAGS GuestRflags;
	ULONG_PTR GuestCr8;
	KIRQL kIrql;
};

// Vm Exit Handler ep
EXTERN_C BOOL VmxExitHandler(VMX_HANDLER_STACK * GuestStack);

// VMX instructions except for VMCALL
BOOL VmxHandleVmx(VMX_HANDLER_CONTEXT *ctx);

// CPUID instruction handler
BOOL VmxHandleCpuid(VMX_HANDLER_CONTEXT *ctx);

// VMX instructions except for access to CR registers
BOOL VmxHandleCrAccess(VMX_HANDLER_CONTEXT *ctx);

// VMX VmCall Handler
BOOL VmxHandleVmCall(VMX_HANDLER_CONTEXT *ctx, BOOL * pbContinue);

// VMX ReadMSR Handler
BOOL VmxHandleRdmsr(VMX_HANDLER_CONTEXT *ctx);



#pragma region Wrapper Functions
// A wrapper for vmx_vmread
SIZE_T VmxVmRead(SIZE_T Field);
// A wrapper for vmx_vmwrite
VMX_STATUS VmxVmWrite(SIZE_T Field, SIZE_T FieldValue);
// Adjust the Guest Instruction pointer after processing an exit opcode
VMX_STATUS VmxAdjustGuestIP(ULONG_PTR baseRip);
#pragma endregion

#pragma region Utility Functions
// Fix the VM Control register with the allowed and disallowed values (see Appendix A.5 of Intel System Programming Guide volume3)
DWORD FixVmControlValue(DWORD MsrNumber, ULONG orgValue);

// Get the segment descripto Access right and convert it to a VMM format (section 24.4.1)
DWORD GetSegmentARForVmm(WORD dwSegSelector);

// Get a segment base address from its selector
ULONG_PTR GetSegmentBaseAddr(WORD dwSegSelector);

// Selects a register to be used based on the Index (see Table 27-4. Exit Qualification for MOV DR)
ULONG_PTR *VmxSelectRegister(ULONG Index, VMX_HANDLER_CONTEXT *GuestContext);
#pragma endregion