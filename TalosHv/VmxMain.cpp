/**********************************************************************
*   Talos Tiny Hypervisor
*	Filename: VmxMain.cpp
*	Virtual Machine Entry and Exit point implementation
*	Last revision: 01/30/2016
*
*  Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
*	All right reserved
**********************************************************************/

#include "stdafx.h"
#include "Debug.h"
#include "VmxInit.h"
#include "VmxMain.h"
#include "IA32_Structs.h"
#include <intrin.h>
#include "asm64.h"
#define TALOSHV_CPUID_CODE 0x40000001

#pragma code_seg(".nonpaged")
// Vm Exit Handler ep
BOOL VmxExitHandler(VMX_HANDLER_STACK * GuestStack) {
	VMX_HANDLER_CONTEXT guest_ctx = { 0 };				// The Exit handler context
	VM_EXIT_INFORMATION vmExitInfo = { 0 };				// The exit reason information
	BOOL bSuccess = FALSE;								// TRUE if I have to continue the VM

	// Build Guest Context
	guest_ctx.GuestStack = GuestStack;			// <-- You can modify directly these values (without need to use VM_WRITE)
	guest_ctx.kIrql = KeGetCurrentIrql();		// See the assembly code at VmxExitHandlerAsm: 	PUSHAQ	- mov rcx, rsp
	guest_ctx.GuestRflags.All = VmxVmRead(GUEST_RFLAGS);
	guest_ctx.GuestCr8 = __readcr8();
	guest_ctx.GuestRip = VmxVmRead(GUEST_RIP);
	guest_ctx.gpRegs->rsp = VmxVmRead(GUEST_RSP);

	// Read the Exit reason
	vmExitInfo.All = (ULONG32)VmxVmRead(VM_EXIT_REASON);

	switch (vmExitInfo.Fields.Reason) {
		case EXIT_REASON_VMCALL: {
			BOOL bContinue = FALSE;
			bSuccess = VmxHandleVmCall(&guest_ctx, &bContinue);
			if (bSuccess) 
				return bContinue;			// We Should exit from this VM
		}
		//
		// Instructions That Cause VM Exits Unconditionally (sect 25.1.2)
		case EXIT_REASON_CPUID:
			bSuccess = VmxHandleCpuid(&guest_ctx);
			break;
		//case EXIT_REASON_GETSEC:
		case EXIT_REASON_INVD:
			AsmInvalidateInternalCaches();
			VmxAdjustGuestIP(guest_ctx.GuestRip);
			break;

		case EXIT_REASON_XSETBV: {
			// Writes the contents of registers EDX:EAX into the 64-bit extended control register (XCR) specified in the ECX register.
			DWORD xcrNumber = (DWORD)guest_ctx.gpRegs->rcx;
			LARGE_INTEGER xcr_value = 
				{ (ULONG)guest_ctx.gpRegs->rax, (LONG)guest_ctx.gpRegs->rdx };
			_xsetbv(xcrNumber, xcr_value.QuadPart);
			VmxAdjustGuestIP(guest_ctx.GuestRip);
			break;
		}

		case EXIT_REASON_INVEPT:
			// Invalidates mappings in the translation lookaside buffers (TLBs) and paging-structure caches that were derived from extended page tables(EPT).
		case EXIT_REASON_INVVPID:
			// Invalidates mappings in the translation lookaside buffers (TLBs) and paging-structure caches based on virtualprocessor identifier(VPID)
			DbgBreak();
			break;

		case EXIT_REASON_VMCLEAR:
		case EXIT_REASON_VMLAUNCH:
		case EXIT_REASON_VMPTRLD:
		case EXIT_REASON_VMPTRST:
		case EXIT_REASON_VMREAD:
		case EXIT_REASON_VMRESUME:
		case EXIT_REASON_VMWRITE:
		case EXIT_REASON_VMOFF:
		case EXIT_REASON_VMON:
			bSuccess = VmxHandleVmx(&guest_ctx);
			break;
		// -------------------------------------------------------------------

		//
		// Instructions That Cause VM Exits set from VmSetupVmcs routine:
		case EXIT_REASON_CR_ACCESS:
			// Guest software attempted to access CR0, CR3, CR4, or CR8 using CLTS, LMSW, or MOV CR and the VM - execution control fields indicate that a VM exit should occur
			bSuccess = VmxHandleCrAccess(&guest_ctx);
			break;
		
		case EXIT_REASON_INVALID_GUEST_STATE: {
			// An invalid Guest state has been detected, see section 26.3.1.1 of System Programming Guide
			ULONG_PTR cr0 = VmxVmRead(GUEST_CR0);
			ULONG_PTR cr3 = VmxVmRead(GUEST_CR3);
			ULONG_PTR cr4 = VmxVmRead(GUEST_CR4);
			ULONG_PTR sysEnterEsp = VmxVmRead(GUEST_SYSENTER_ESP);
			ULONG_PTR sysEnterEip = VmxVmRead(GUEST_SYSENTER_EIP);

			DbgPrint("TalosHv: Warning! The VM has exited with an Invalid guest state.\r\n");
			DbgPrint("CR0 value: 0x%08X, CR3 value: 0x%08X, CR4 value: 0x%08X, SYSENTER_ESP value: 0x%08X, SYSENTER_EIP value: 0x%08X.\r\n",
				cr0, cr3, cr4, sysEnterEsp, sysEnterEip);
			DbgBreak();

		}

		case EXIT_REASON_MSR_READ:
			bSuccess = VmxHandleRdmsr(&guest_ctx);
			break;

		case EXIT_REASON_MSR_WRITE:
		default:
			DbgPrint("TalosHv: Error! An unknown exit reason (ID: %i) has been detected. The VM is exiting....\r\n", vmExitInfo.Fields.Reason);
			DbgBreak();
			bSuccess = FALSE;
			break;
	}

	// Write back the Cr8 register (if needed) 
	__writecr8(guest_ctx.GuestCr8);

	return bSuccess;
}

// VMX instructions except for access to CR registers
BOOL VmxHandleCrAccess(VMX_HANDLER_CONTEXT *ctx) {
	MOV_CR_QUALIFICATION crDetails = { 0 };
	ULONG_PTR * pSourceReg = NULL;
	BOOL bSuccess = 0;						// If 0 I have to exit

	//DbgPrint("TalosHv!VmxHandleCrAccess - Detected CRX access. Guest RIP: 0x%08X%08X", (DWORD)(ctx->GuestRip >> 32), (DWORD)ctx->GuestRip);
	//DbgBreak();

	// Grab the Exit Qualification
	crDetails.All = VmxVmRead(EXIT_QUALIFICATION);

	// Grab the source register
	pSourceReg = VmxSelectRegister(crDetails.Fields.SourceRegister, ctx);

	switch (crDetails.Fields.AccessType) {
		case MOVE_TO_CR:
			bSuccess = TRUE;
			switch (crDetails.Fields.ControlRegister) {
				case 0:				// CR0
					VmxVmWrite(GUEST_CR0, *pSourceReg);
					VmxVmWrite(CR0_READ_SHADOW, *pSourceReg);
					break;
				case 3:				// CR3
					VmxVmWrite(GUEST_CR3, *pSourceReg);
					break;
				case 4:				// CR4
					VmxVmWrite(GUEST_CR4, *pSourceReg);
					VmxVmWrite(CR4_READ_SHADOW, *pSourceReg);
					break;
				case 8:
					ctx->GuestCr8 = *pSourceReg;
					break;
				default:
					bSuccess = FALSE;
					DbgBreak();
			}
			break;

		case MOVE_FROM_CR: {
			ULONG_PTR crReg = 0;
			bSuccess = TRUE;
			switch (crDetails.Fields.ControlRegister) {
				case 0:				// CR0
					crReg = VmxVmRead(GUEST_CR0);
					break;
				case 3:				// CR3
					crReg = VmxVmRead(GUEST_CR3);
					break;
				case 4:				// CR4
					crReg = VmxVmRead(GUEST_CR4);
					break;
				case 8:
					crReg = ctx->GuestCr8;
					break;
				default:
					bSuccess = FALSE;
					DbgBreak();
					break;
				}
			if (bSuccess) *pSourceReg = crReg;
			break;
		}

		case CLTS:
		case LMSW:
		default:
			// Unimplemented
			DbgBreak();
			break;
	}

	VmxAdjustGuestIP(ctx->GuestRip);
	return bSuccess;
}

// VMX ReadMSR Handler
BOOL VmxHandleRdmsr(VMX_HANDLER_CONTEXT *ctx) {
	DbgBreak();
	// Unimplemented!!!!
	VmxAdjustGuestIP(ctx->GuestRip);
	return FALSE;
}

// VMX instructions except for VMCALL
BOOL VmxHandleVmx(VMX_HANDLER_CONTEXT *ctx) {
	VMX_STATUS vmxStatus = VMX_STATUS_SUCCESS;
	ctx->GuestRflags.Fields.CF = 1;  // Error without status
	ctx->GuestRflags.Fields.PF = 0;
	ctx->GuestRflags.Fields.AF = 0;
	ctx->GuestRflags.Fields.ZF = 0;  // Error without status
	ctx->GuestRflags.Fields.SF = 0;
	ctx->GuestRflags.Fields.OF = 0;
	vmxStatus = VmxVmWrite(GUEST_RFLAGS, ctx->GuestRflags.All);
	vmxStatus = VmxAdjustGuestIP(ctx->GuestRip);
	return (vmxStatus == VMX_STATUS_SUCCESS);
}

// VMX VmCall Handler
BOOL VmxHandleVmCall(VMX_HANDLER_CONTEXT *ctx, BOOL * pbContinue) {
	DWORD dwVmCallCode = 0;					// Guest VmCall code
	ULONG_PTR * lpGuestCtx = NULL;			// Guest Context parameter

	// Recover here the original Guest parameters:
	dwVmCallCode = (DWORD)ctx->gpRegs->rcx;
	lpGuestCtx = (ULONG_PTR*)ctx->gpRegs->rdx;

	if (dwVmCallCode == VMX_UNLOAD_CALL_CODE) {
		// Unload the Hypervisor on THIS Cpu
		if (lpGuestCtx)
			*lpGuestCtx = (ULONG_PTR)ctx->GuestStack->ProcData;
		VmxAdjustGuestIP(ctx->GuestRip);

		// Adjust RFLAGS
		ctx->GuestRflags.Fields.CF = 0;
		ctx->GuestRflags.Fields.ZF = 0;
		VmxVmWrite(GUEST_RFLAGS, ctx->GuestRflags.All);
		if (pbContinue) *pbContinue = FALSE;
	}
	else {
		// Handle the VmCall like the other VM opcodes
		VmxHandleVmx(ctx);
		if (pbContinue) *pbContinue = TRUE;
	}
	
	return TRUE;
}

// CPUID instruction handler
BOOL VmxHandleCpuid(VMX_HANDLER_CONTEXT *ctx) {
	VMX_STATUS vmxStatus = VMX_STATUS_SUCCESS;
	int cpuInfo[4] = { 0 };					// The information returned from CPUID opcode
	int dwFuncId = (int)ctx->gpRegs->rax;
	int dwSubFuncId = (int)ctx->gpRegs->rcx;

	if (dwFuncId == TALOSHV_CPUID_CODE && dwSubFuncId == 0) {
		//"Talos Hypervisor"
		ctx->gpRegs->rax = (DWORD)'olaT';
		ctx->gpRegs->rbx = (DWORD)'yH s';
		ctx->gpRegs->rcx = (DWORD)'vrep';
		ctx->gpRegs->rdx = (DWORD)'rosi';
	}
	else if (dwFuncId == TALOSHV_CPUID_CODE && dwSubFuncId == 1) {
		// "ver 0.1"
		ctx->gpRegs->rax = (DWORD)' rev';
		ctx->gpRegs->rbx = (DWORD)'1.0 ';
		ctx->gpRegs->rcx = 0;
		ctx->gpRegs->rdx = 0;
	}
	else {
		// Invoke the standard CPUID (https://msdn.microsoft.com/en-us/library/hskdteyh.aspx)
		__cpuidex(cpuInfo, dwFuncId, dwSubFuncId);
		ctx->gpRegs->rax = cpuInfo[0];
		ctx->gpRegs->rbx = cpuInfo[1];
		ctx->gpRegs->rcx = cpuInfo[2];
		ctx->gpRegs->rdx = cpuInfo[3];
		// Need to save the new context?
	}
	vmxStatus = VmxAdjustGuestIP(ctx->GuestRip);
	return (vmxStatus == VMX_STATUS_SUCCESS);
}


#pragma region Wrapper Functions
// A wrapper for vmx_vmread
SIZE_T VmxVmRead(SIZE_T Field) {
	size_t fieldValue = 0;
	VMX_STATUS vmxStatus = VMX_STATUS_SUCCESS;
	
	vmxStatus = (VMX_STATUS)__vmx_vmread(Field, &fieldValue);
	if (vmxStatus != VMX_STATUS_SUCCESS) 
		DbgPrint("Warning! VmxVmRead(0x%08x) failed with an error %d\r\n", Field, vmxStatus);
	
	return fieldValue;
}

// A wrapper for vmx_vmwrite
VMX_STATUS VmxVmWrite(SIZE_T Field, SIZE_T FieldValue) {
	VMX_STATUS vmxStatus = VMX_STATUS_SUCCESS;
	
	vmxStatus = (VMX_STATUS)__vmx_vmwrite(Field, FieldValue);
	if (vmxStatus != VMX_STATUS_SUCCESS)
		DbgPrint("Warning! VmxVmWrite(0x%08x) failed with an error %d\r\n", Field, vmxStatus);
	return vmxStatus;
}

// Adjust the Guest Instruction pointer after processing an exit opcode
VMX_STATUS VmxAdjustGuestIP(ULONG_PTR baseRip) {
	VMX_STATUS vmxStatus = VMX_STATUS_SUCCESS;
	size_t instrLen = 0;			// Last instruction length

	vmxStatus = (VMX_STATUS)__vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &instrLen);
	if (vmxStatus != VMX_STATUS_SUCCESS) return vmxStatus;

	vmxStatus = (VMX_STATUS)__vmx_vmwrite(GUEST_RIP, baseRip + instrLen);
	return vmxStatus;
}
#pragma endregion

#pragma region Utility Functions
// Selects a register to be used based on the Index (see Table 27-4. Exit Qualification for MOV DR)
ULONG_PTR *VmxSelectRegister(ULONG Index, VMX_HANDLER_CONTEXT *GuestContext) {
	ULONG_PTR *registerUsed = NULL;
	switch (Index) {
		case 0: registerUsed = &GuestContext->gpRegs->rax; break;
		case 1: registerUsed = &GuestContext->gpRegs->rcx; break;
		case 2: registerUsed = &GuestContext->gpRegs->rdx; break;
		case 3: registerUsed = &GuestContext->gpRegs->rbx; break;
		case 4: registerUsed = &GuestContext->gpRegs->rsp; break;
		case 5: registerUsed = &GuestContext->gpRegs->rbp; break;
		case 6: registerUsed = &GuestContext->gpRegs->rsi; break;
		case 7: registerUsed = &GuestContext->gpRegs->rdi; break;
		case 8: registerUsed = &GuestContext->gpRegs->r8; break;
		case 9: registerUsed = &GuestContext->gpRegs->r9; break;
		case 10: registerUsed = &GuestContext->gpRegs->r10; break;
		case 11: registerUsed = &GuestContext->gpRegs->r11; break;
		case 12: registerUsed = &GuestContext->gpRegs->r12; break;
		case 13: registerUsed = &GuestContext->gpRegs->r13; break;
		case 14: registerUsed = &GuestContext->gpRegs->r14; break;
		case 15: registerUsed = &GuestContext->gpRegs->r15; break;
		default: DbgBreak(); break;
	}
	return registerUsed;
}


// Get the segment descripto Access right and convert it to a VMM format (section 24.4.1)
DWORD GetSegmentARForVmm(WORD wSegSelector) {
	//X86_SEG_SELECTOR x86Seg = { wSegSelector };
	VMX_SEG_DESCRIPTOR_ACCESS_RIGHT vmxSegDesc = { 0 };
	DWORD ar = AsmLoadAccessRightsByte(wSegSelector);
	// The usage of LAR opcode is faster because it automatically grabs the correct descriptor from IDT, GDT or LDT
	ar = ar >> 8;			// Delete the unused 8 bits (see Intel Manual LAR opcode - Instruction Set reference)

	if (wSegSelector) {
		// Return a fully-compiled access right 
		vmxSegDesc.All = ar;
		vmxSegDesc.Fields.Reserved1 = 0;			// From Intel Manual of LAR instruction: Bits 19:16 are undefined - Must BE cleared to 0
		vmxSegDesc.Fields.Reserved2 = 0;
		vmxSegDesc.Fields.Unusable = FALSE;
	}
	else
		vmxSegDesc.Fields.Unusable = TRUE;
	return (DWORD)vmxSegDesc.All;
}

ULONG_PTR GetSegmentBaseByDescriptor(const X86_SEG_DESCRIPTOR *SegDesc) {
	ULONG_PTR baseAddr = NULL;

	// Get the 32-bit base address
	baseAddr = ((ULONG_PTR)SegDesc->Fields.BaseLow |
		(ULONG_PTR)SegDesc->Fields.BaseMid << 16 |
		(ULONG_PTR)SegDesc->Fields.BaseHi << 24) & 0xFFFFFFFF;

	if (!SegDesc->Fields.System) {
		// 64-bit System segment (see 5.8.3.1 IA-32e Mode Call Gates)
		// Don't confuse this with the L bit
		ULONG_PTR baseUpper32 = ((X86_SEG_DESCRIPTOR64*)SegDesc)->BaseUpper32;
		baseAddr |= (baseUpper32 << 32);
	}
	return baseAddr;
}

// Get a segment base address from its selector
ULONG_PTR GetSegmentBaseAddr(WORD wSegSelector) {
	X86_SEG_SELECTOR x86Seg = { wSegSelector };
	X86_SEG_DESCRIPTOR * targetDesc = NULL;				// Target Segment descriptor 
	X86_SEG_DESCRIPTOR * ldtDesc = NULL;				// LocalDescriptorTable segment descriptor
	ULONG_PTR ldtBaseAddr = NULL;						// LDT Base address
	ULONG_PTR baseAddr = NULL;

	// Read the GDT address
	GDTR gdtr = { 0 };
	AsmReadGDT(&gdtr);

	// Act as a processor here, access the memory and grab the right descriptor
	if (x86Seg.Fields.TI == 0) {
		// Access the GDT
		targetDesc = (X86_SEG_DESCRIPTOR*)
			((LPBYTE)gdtr.Address + (x86Seg.Fields.Index * sizeof(X86_SEG_DESCRIPTOR)));
		// Grab the Base Address
		baseAddr = GetSegmentBaseByDescriptor(targetDesc);
		return baseAddr;
	}
	else {
		// Access the LDT
		WORD ldtIndex = AsmReadLDTR();
		DbgBreak();
		// Grab the LDT descriptor
		ldtDesc = (X86_SEG_DESCRIPTOR*)
			((LPBYTE)gdtr.Address + (ldtIndex * sizeof(X86_SEG_DESCRIPTOR)));
		// Get LDT base address
		ldtBaseAddr = GetSegmentBaseByDescriptor(ldtDesc);

		// Then redo it again to grab the real segment base address
		targetDesc = (X86_SEG_DESCRIPTOR*)
			(ldtBaseAddr + (x86Seg.Fields.Index * sizeof(X86_SEG_DESCRIPTOR)));
		baseAddr = GetSegmentBaseByDescriptor(targetDesc);
	}
	return baseAddr;
}
#pragma endregion
#pragma code_seg()
