/**********************************************************************
*  Talos Tiny Hypervisor
*	Filename: DriverEntry.cpp
*	Implement VMX Startup/Shutdown Functions
*	Last revision: 01/30/2016
*
*  Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
*	All right reserved
**********************************************************************/

#include "stdafx.h"
#include "DriverEntry.h"
#include "Debug.h"
#include "VmxInit.h"
#include "VmxMain.h"
#include <intrin.h>

#define STATUS_VMX_DISABLED       ((NTSTATUS)0xC0001001L)
#pragma code_seg(".nonpaged")

// Check if the current processor support VMX
NTSTATUS CheckVmxSupport() {
	CPUID_CONTEXT cpuid_ctx = { 0 };
	IA32_FEATURE_CONTROL_MSR controlMsr = { 0 };
	CPUID_FEATURE_INFO & cpuid_feature = (CPUID_FEATURE_INFO &)cpuid_ctx.ecx;

	// Get the CPUID feature list
	InvokeCpuid(1, &cpuid_ctx);
	if (cpuid_feature.VMX != 1) return STATUS_NOT_SUPPORTED;

	// Read the IA32_FEATURE_CONTROL MSR
	controlMsr.All = __readmsr(IA32_FEATURE_CONTROL);
	if (controlMsr.Fields.Lock != 1 || controlMsr.Fields.EnableVmxon != 1)
		return STATUS_VMX_DISABLED;

	// Return the compatibility - success
	return STATUS_SUCCESS;
}

// Initialize and launch a VM
NTSTATUS InitializeVM_internal(LPVOID lpGuestRsp) {
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PER_PROCESSOR_VM_DATA * pProc_vm_data = NULL;		// Per processor Virtual machine data
	PHYSICAL_ADDRESS maxPhysAddr = { (ULONG)-1 };		// Higher address of Continuous 
	DWORD dwVmcsSize = 4096;							// VMCS and VMXON region size
	const DWORD dwStackSize = 4096 * 2;					// The VMX stack size

	// Initialize the maximum memory address
	maxPhysAddr.QuadPart = (QWORD)-1i64;
	
	ntStatus = CheckVmxSupport();
	if (!NT_SUCCESS(ntStatus)) return ntStatus;

	// Allocate the Per processor Data
	pProc_vm_data = (PER_PROCESSOR_VM_DATA*)ExAllocatePoolWithTag(NonPagedPool,
		sizeof(PER_PROCESSOR_VM_DATA), MEMTAG);
	if (!pProc_vm_data) return STATUS_INSUFFICIENT_RESOURCES;
	RtlZeroMemory(pProc_vm_data, sizeof(PER_PROCESSOR_VM_DATA));

	// allocate memory for each structures (MUST BE in physical nonpaged uncached memory)
	// Before executing VMXON, software should allocate a naturally aligned 4 - KByte region of memory that a logical processor may use to support VMX operation.
	pProc_vm_data->dwVmcsRegionSize = dwVmcsSize;
	pProc_vm_data->VmxonRegion = (VMCS*)MmAllocateContiguousMemory(dwVmcsSize, maxPhysAddr);
	pProc_vm_data->VmcsRegion = (VMCS*)MmAllocateContiguousMemory(dwVmcsSize, maxPhysAddr);
	// Allocate memory for the MSR Bitmap (4 Bitmaps 1 KB wide)
	pProc_vm_data->MsrBitmap = (LPBYTE)MmAllocateContiguousMemory(4096, maxPhysAddr);
	// Allocate memory for the VMM Exit point stack
	pProc_vm_data->VmmStackTop = (LPVOID)MmAllocateContiguousMemory(dwStackSize, maxPhysAddr);
	pProc_vm_data->dwProcNumber = KeGetCurrentProcessorNumber();


	/* We need to allocate and compile a Stack for the VM Exit Handler
	 * The stack should include the VM_PER_PROCESSOR data pointer and a copy of all registers at 
	 * the time the VM exits. Save all the GP registers is the duty of the VmxVmEntryPointAsm functions.
	 * We need to save the VM_PER_PROCESSOR structure:
	 *  |-----------------------|		<-- Bottom or Base (StackPtr + StackSize)
	 *	| ProcessorVmData		|
	 *  | 0xF0F0F0F0F0F0F0F0	|		|
	 *	|-----------------------|		|  GROWs downwards
	 *	|		Vm Stack		|		|
	 *  |						|		?
	 *	|-----------------------|		<-- Top (StackPtr)
	 */

	pProc_vm_data->VmmStackBase = (LPVOID)((ULONG_PTR)pProc_vm_data->VmmStackTop + dwStackSize);
	ULONG_PTR * pStackBase = (ULONG_PTR*)pProc_vm_data->VmmStackBase;
	*(pStackBase - 1) = (ULONG_PTR)pProc_vm_data;
	*(pStackBase - 2) = 0xF0F0F0F0F0F0F0F0;

	// 1. Enter VMX mode
	ntStatus = VmEnableVmxMode(pProc_vm_data);
	
	// 2. Initialize the VMCS structure
	ntStatus = VmInitVmcs(pProc_vm_data);

	// 3. Setup the VMCS
	ntStatus = VmSetupVmcs(pProc_vm_data, (LPVOID)VmxVmEntryPointAsm, lpGuestRsp);

	// 4. Launch the VM
	ntStatus = VmLaunch(pProc_vm_data);
	return ntStatus;
}

// Enter the VMX mode for a particular CPU
NTSTATUS VmEnableVmxMode(PER_PROCESSOR_VM_DATA * lpCpuVmData) {
	IA32_VMX_BASIC_MSR vmxBasic = { 0 };				// Content of IA32_VMX_BASIC Model Specific Register

	if (!lpCpuVmData || !lpCpuVmData->VmxonRegion)
		return STATUS_INVALID_PARAMETER;

	// 1. Read the IA32_VMX_BASIC
	vmxBasic.All = __readmsr(IA32_VMX_BASIC);

	// From http://wiki.osdev.org/VMX:
	// In order to see what your OS needs to support, you need to read the lower 32-bits of IA32_VMX_CR0_FIXED0 and IA32_VMX_CR4_FIXED0. 
	const CR0_REG cr0Fixed0 = { __readmsr(IA32_VMX_CR0_FIXED0) };
	const CR4_REG cr4Fixed0 = { __readmsr(IA32_VMX_CR4_FIXED0) };
	// The bits in those MSR's are what need to be set (and supported) in their corresponding control registers (CR0/CR4). If any of those bits are not enabled, a GPF will occur. 
	// Also, to see what extra bits can be set, check the IA32_VMX_CR0_FIXED1 and IA32_VMX_CR4_FIXED1. 
	const CR0_REG cr0Fixed1 = { __readmsr(IA32_VMX_CR0_FIXED1) };
	const CR4_REG cr4Fixed1 = { __readmsr(IA32_VMX_CR4_FIXED1) };
	// It's basically a mask of bits, if a bit is set to 1, then it "can" be enabled, but if it's a 0, you may generate an exception if you enable the corresponding bit in the control registers. 

	// 2. Read and modify CR0, CR4 if needed
	CR0_REG cpu_cr0 = { __readcr0() };							// CR0 register of current processor
	CR4_REG cpu_cr4 = { __readcr4() };							// CR4 Register of current processor
	DbgPrint("TalosHv!VmEnterVmxMode - Original CR0 register: 0x%08X - Original CR4 register: 0x%08X.\r\n", cpu_cr0.All, cpu_cr4.All);

	cpu_cr0.All |= cr0Fixed0.All;
	cpu_cr0.All &= cr0Fixed1.All;			// Apply the "CAN-BE-SET" bitmask
	cpu_cr4.All |= cr4Fixed0.All;
	cpu_cr4.All &= cr4Fixed1.All;			// Apply the "CAN-BE-SET" bitmask

	// Write the new 2 control registers:
	__writecr0(cpu_cr0.All);				// Theoretically NE (Numeric Error) flags will be changed (PG and PE are already enabled)
	__writecr4(cpu_cr4.All);				// Theoretically only VMXE will be changed
	DbgPrint("TalosHv!VmEnterVmxMode - New CR0 register: 0x%08X - New CR4 register: 0x%08X.\r\n", cpu_cr0.All, cpu_cr4.All);

	// Store the RevisionID in the region
	lpCpuVmData->VmxonRegion->RevisionIdentifier = vmxBasic.Fields.RevisionIdentifier;
	PHYSICAL_ADDRESS lpVmxOnPhysAddr = MmGetPhysicalAddress(lpCpuVmData->VmxonRegion);

	// From intel Manual: Check successful execution of VMXON by checking if RFLAGS.CF = 0
	if (__vmx_on((UINT64*)&lpVmxOnPhysAddr.QuadPart))
		return STATUS_UNSUCCESSFUL;
	else
		return STATUS_SUCCESS;
}

// Initialize the VMCS structure for a particular CPU
NTSTATUS VmInitVmcs(PER_PROCESSOR_VM_DATA * lpCpuVmData) {
	IA32_VMX_BASIC_MSR vmxBasic = 				// Content of IA32_VMX_BASIC Model Specific Register
	{ __readmsr(IA32_VMX_BASIC) };
	PHYSICAL_ADDRESS physAddr = { 0 };
	BOOL bRetVal = FALSE;
	ULONG dwProcNum = KeGetCurrentProcessorNumber();

	if (!lpCpuVmData->VmcsRegion) return STATUS_INVALID_PARAMETER;

	// Grab the initialization RevID
	RtlZeroMemory(lpCpuVmData->VmcsRegion, lpCpuVmData->dwVmcsRegionSize);
	lpCpuVmData->VmcsRegion->RevisionIdentifier = vmxBasic.Fields.RevisionIdentifier;

	// Get the physical address of the VMCS
	physAddr = MmGetPhysicalAddress((LPVOID)lpCpuVmData->VmcsRegion);

	// Clear the VMCS structure
	bRetVal = (__vmx_vmclear((QWORD*)&physAddr.QuadPart) == 0);

	// Make this VMCS as current for current processor. That address is loaded into the current-VMCS pointer
	if (bRetVal)
		bRetVal = (__vmx_vmptrld((QWORD*)&physAddr.QuadPart) == 0);

	if (bRetVal) {
		DbgPrint("TalosHv!VmInitVmcs - VMCS for processor %i initialized successfully!\r\n", dwProcNum);
		return STATUS_SUCCESS;
	} else {
		DbgPrint("TalosHv!VmInitVmcs - Unable to initialize the VMCS for processor %i.\r\n", dwProcNum);
		return STATUS_UNSUCCESSFUL;
	}
}

// Fix the VM Control register with the allowed and disallowed values (see Appendix A.5 of Intel System Programming Guide volume3)
DWORD FixVmControlValue(DWORD MsrNumber, ULONG orgValue) {
	// See part 2 of the "Hypervisor Abyss" document (ReadMsr routine)
	LARGE_INTEGER msrValue = { 0 };
	msrValue.QuadPart = __readmsr(MsrNumber);
	DWORD dwRetVal = orgValue;

	// Bits 31:0 indicate the allowed 0-settings of these controls. VM entry allows control X 
	// (bit X of the VM-entry controls) to be 0 if bit X in the MSR is cleared to 0; 
	// if bit X in the MSR is set to 1, VM entry fails if control X is 0.
	dwRetVal |= msrValue.LowPart;

	// Bits 31:0 indicate the allowed 0-settings of these controls. VM entry allows control X 
	// (bit X of the VM-entry controls) to be 0 if bit X in the MSR is cleared to 0; 
	// if bit X in the MSR is set to 1, VM entry fails if control X is 0.
	dwRetVal &= msrValue.HighPart;
	return dwRetVal;
}


// Setup the VMCS to virtualize the guest as the host (see the Hypervisor Abiss documentation)
NTSTATUS VmSetupVmcs(PER_PROCESSOR_VM_DATA * lpCpuVmData, LPVOID lpGuestRip, LPVOID lpGuestRsp) {
	BOOLEAN bError = FALSE;
	GDTR gdtr = { 0 };						// Host GDTR
	IDTR idtr = { 0 };						// Host IDTR
	PHYSICAL_ADDRESS physAddr = { 0 };		// A physical memory address

	// READ needed host GDTR and IDTR
	AsmReadGDT(&gdtr);
	__sidt((LPVOID)&idtr);

	/* 1. Use VMWRITEs to set up the various VM-exit control fields, VM-entry
	 *  control fields, and VM execution control fields in the VMCS.
	 *	Care should be taken to make sure the settings of individualfields match the allowed
	 *	0 and 1 settings for the respective controls as reported by the VMX capability MSRs.
	 *	Any settings inconsistent with the settings reported by the capability MSRs will cause VM entries to fail.
	 */
	// VM Entries - Intel System Programming Guide - Section 24.8.1
	VMX_VM_ENTER_CONTROLS vmEnterCtrl = { 0 };
	vmEnterCtrl.Fields.IA32eModeGuest = TRUE;		// We need to enter our VM in 64-bit mode
	vmEnterCtrl.Fields.LoadDebugControls = FALSE;	// This control determines whether DR7 and the IA32_DEBUGCTL MSR are loaded on VM entry.
	// Consult the IA32_VMX_ENTRY_CTLS MSR and fix the value of reserved bits
	vmEnterCtrl.All = FixVmControlValue(IA32_VMX_ENTRY_CTLS, vmEnterCtrl.All);

	// VM Exits - Intel System Programming Guide - Section 24.7.1
	VMX_VM_EXIT_CONTROLS vmExitCtl = { 0 };
	vmExitCtl.Fields.AcknowledgeInterruptOnExit = TRUE;
	vmExitCtl.Fields.HostAddressSpaceSize = TRUE;	// We need to re-exit in 64-bit long mode
	// Consult the IA32_VMX_EXIT_CTLS MSR and fix the value of reserved bits
	vmExitCtl.All = FixVmControlValue(IA32_VMX_EXIT_CTLS, vmExitCtl.All);

	// PIN based and CPU based VM_Execution controls (section 24.6.1 and 24.6.2)
	VMX_PIN_BASED_CONTROLS vmPinCtl = { FixVmControlValue(IA32_VMX_PINBASED_CTLS, 0) };
	VMX_CPU_BASED_CONTROLS vmCpuCtl = {};
	vmCpuCtl.Fields.RDTSCExiting = FALSE;		// Exit with RDTSC and RDTSCP
	vmCpuCtl.Fields.CR3LoadExiting = FALSE;		// MOV to CR3
	vmCpuCtl.Fields.CR8LoadExiting = FALSE;		// MOV to CR8
	vmCpuCtl.Fields.MovDRExiting = FALSE;		// MOV DRX, YYY
	if (lpCpuVmData->MsrBitmap)
		vmCpuCtl.Fields.UseMSRBitmaps = TRUE;		// This control determines whether MSR bitmaps are used to control execution of the RDMSR and WRMSR instructions
	if (lpCpuVmData->IoBitmap)						// NB. We DON'T Use IO Bitmaps
		vmCpuCtl.Fields.UseIOBitmaps = TRUE;		
	vmCpuCtl.Fields.ActivateSecondaryControl = TRUE;			// Activate the Secondary Processor based VM Execution Controls (section 24.6.2)
	vmCpuCtl.All = { FixVmControlValue(IA32_VMX_PROCBASED_CTLS, vmCpuCtl.All) };

	// Secondary CPU based VM_Execution controls (section 24.6.2)
	VMX_SECONDARY_CPU_BASED_CONTROLS vmCpuCtl2 = {};
	vmCpuCtl2.Fields.EnableRDTSCP = TRUE;
	vmCpuCtl2.Fields.DescriptorTableExiting = FALSE; // This control determines whether executions of LGDT, LIDT, LLDT, LTR, SGDT, SIDT, SLDT, and STR cause VM exits
	vmCpuCtl2.All = { FixVmControlValue(IA32_VMX_PROCBASED_CTLS2, vmCpuCtl2.All) };

	// The MSR bitmaps - Section 24.6.9 of System Programming Guide Volume 3
	if (lpCpuVmData->MsrBitmap) {
		// Read bitmap for low MSRs - one bit for each MSR address in the range 0 to 0x1FFF
		memset(lpCpuVmData->MsrBitmap, 0xFF, 1024);			// 0x1FFF bit = 8191 bit -> 1024 bytes
		// Read bitmap for high MSRs - one bit for each MSR address in the range 0xC0000000 to 0xC0001FFF
		memset(lpCpuVmData->MsrBitmap + 1024, 0xFF, 1024);	// 0x1FFF bit = 8191 bit -> 1024 bytes
		// Write bitmap for low MSRs - one bit for each MSR address in the range 0 to 0x1FFF
		memset(lpCpuVmData->MsrBitmap + 2048, 0xFF, 1024);	// 0x1FFF bit = 8191 bit -> 1024 bytes
		// Write bitmap for high MSRs - one bit for each MSR address in the range 0xC0000000 to 0xC0001FFF
		memset(lpCpuVmData->MsrBitmap + 3072, 0xFF, 1024);	// 0x1FFF bit = 8191 bit -> 1024 bytes

		// Ignore IA32_MPERF (000000e7) and IA32_APERF (000000e8)
		lpCpuVmData->MsrBitmap[0x1C] &= (~0x80);		// 0xE0 / 8 = 0x1C
		lpCpuVmData->MsrBitmap[0x1D] &= (~0x01);		// 0xE8 / 8 = 0x1D
		lpCpuVmData->MsrBitmap[0x1C + 0x800] &= (~0x80);		// Write access
		lpCpuVmData->MsrBitmap[0x1D + 0x800] &= (~0x01);		// Write access

		// Ignore IA32_GS_BASE (c0000101) and IA32_KERNEL_GS_BASE (c0000102)
		lpCpuVmData->MsrBitmap[0x20 + 0x400] &= (~0x02);		// 0x101 / 8 = 0x20
		lpCpuVmData->MsrBitmap[0x20 + 0x400] &= (~0x04);
		lpCpuVmData->MsrBitmap[0x20 + 0xC00] &= (~0x02);		// Write access
		lpCpuVmData->MsrBitmap[0x20 + 0xC00] &= (~0x04);		// Write access

		// DEBUG: We don't need to catch any MSR
		RtlZeroMemory(lpCpuVmData->MsrBitmap, 4096);
	}

	physAddr = MmGetPhysicalAddress(lpCpuVmData->MsrBitmap);

	// 24.6.6 - Guest/Host Masks and Read Shadows for CR0 and CR4
	CR0_REG cr0_mask = { 0 };			// In general, bits set to 1 in a guest/host mask correspond to bits “owned” by the host
	cr0_mask.Fields.WP = 1;				// Guest attempts to set them (using CLTS, LMSW, or MOV to CR) to values differing from the corresponding
										//bits in the corresponding read shadow cause VM exits.
	CR4_REG cr4_mask = { 0 };			// Bits cleared to 0 correspond to bits “owned” by the guest; guest attempts to modify them succeed and 
	cr4_mask.Fields.PGE = 1;			// guest reads return values for these bits from the control register itself.

	// Now save the control fields in the VMCS:
	// 64-bit control fields: 
	bError |= __vmx_vmwrite(IO_BITMAP_A, 0);			
	bError |= __vmx_vmwrite(IO_BITMAP_B, 0);
	bError |= __vmx_vmwrite(MSR_BITMAP, physAddr.QuadPart);
	bError |= __vmx_vmwrite(TSC_OFFSET, 0);
	// 32-bit control fields:
	bError |= __vmx_vmwrite(PIN_BASED_VM_EXEC_CONTROL, vmPinCtl.All);
	bError |= __vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL, vmCpuCtl.All);
	bError |= __vmx_vmwrite(SECONDARY_VM_EXEC_CONTROL, vmCpuCtl2.All);
	bError |= __vmx_vmwrite(EXCEPTION_BITMAP, 0);				// <-- Set here the exception bitmap
	bError |= __vmx_vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
	bError |= __vmx_vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);
	bError |= __vmx_vmwrite(CR3_TARGET_COUNT, 0);
	bError |= __vmx_vmwrite(VM_EXIT_CONTROLS, vmExitCtl.All);
	bError |= __vmx_vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
	bError |= __vmx_vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
	bError |= __vmx_vmwrite(VM_ENTRY_CONTROLS, vmEnterCtrl.All);
	bError |= __vmx_vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
	bError |= __vmx_vmwrite(VM_ENTRY_INTR_INFO_FIELD, 0);
	// Natural-Width Control Fields (B.4.1):
	bError |= __vmx_vmwrite(CR0_GUEST_HOST_MASK, cr0_mask.All);
	bError |= __vmx_vmwrite(CR4_GUEST_HOST_MASK, cr4_mask.All);
	bError |= __vmx_vmwrite(CR0_READ_SHADOW, __readcr0());
	bError |= __vmx_vmwrite(CR4_READ_SHADOW, __readcr4());
	bError |= __vmx_vmwrite(CR3_TARGET_VALUE0, 0);
	bError |= __vmx_vmwrite(CR3_TARGET_VALUE1, 0);
	bError |= __vmx_vmwrite(CR3_TARGET_VALUE2, 0);
	bError |= __vmx_vmwrite(CR3_TARGET_VALUE3, 0);

	if (bError) {
		DbgPrint("VmSetupVmcs - Error while setting the Vm-Entry and VM-Exit controls in the target VMCS for processor %i.\r\n", lpCpuVmData->dwProcNumber);
		return STATUS_UNSUCCESSFUL;
	}


	/* 2. Issue a sequence of VMWRITEs to initialize various host-state area fields
	 *	in the working VMCS.The initialization sets up the context and entry -
	 *	points to the VMM upon subsequent VM exits from the guest. */
	// Control registers CR0, CR3, and CR4 (64 bits each)
	bError |= __vmx_vmwrite(HOST_CR0, __readcr0());
	bError |= __vmx_vmwrite(HOST_CR3, __readcr3());
	bError |= __vmx_vmwrite(HOST_CR4, __readcr4());
	// RSP and RIP
	bError |= __vmx_vmwrite(HOST_RSP, (QWORD)lpCpuVmData->VmmStackBase);
	//
	// Set here the VMM Exit Entry point:
	bError |= __vmx_vmwrite(HOST_RIP, reinterpret_cast<size_t>(VmxExitHandlerAsm));
	//
	// Selector fields (16 bits each) for the segment registers CS, SS, DS, ES, FS, GS, and TR.
	bError |= __vmx_vmwrite(HOST_ES_SELECTOR, AsmReadES() & 0xf8); // RPL and TI 
	bError |= __vmx_vmwrite(HOST_CS_SELECTOR, AsmReadCS() & 0xf8); // have to be 0
	bError |= __vmx_vmwrite(HOST_SS_SELECTOR, AsmReadSS() & 0xf8);
	bError |= __vmx_vmwrite(HOST_DS_SELECTOR, AsmReadDS() & 0xf8);
	bError |= __vmx_vmwrite(HOST_FS_SELECTOR, AsmReadFS() & 0xf8);
	bError |= __vmx_vmwrite(HOST_GS_SELECTOR, AsmReadGS() & 0xf8);
	bError |= __vmx_vmwrite(HOST_TR_SELECTOR, AsmReadTR() & 0xf8);
	// Base-address fields for FS, GS, TR, GDTR, and IDTR
	bError |= __vmx_vmwrite(HOST_FS_BASE, __readmsr(IA32_FS_BASE));
	bError |= __vmx_vmwrite(HOST_GS_BASE, __readmsr(IA32_GS_BASE));
	bError |= __vmx_vmwrite(HOST_TR_BASE, GetSegmentBaseAddr(AsmReadTR()));
	bError |= __vmx_vmwrite(HOST_GDTR_BASE, gdtr.Address);
	bError |= __vmx_vmwrite(HOST_IDTR_BASE, idtr.Address);
	// The MSRs (section 24.5):
	bError |= __vmx_vmwrite(HOST_IA32_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));			// SYSENTER and SYSEXIT are the x86 versions of the AMD64 SYSCALL instruction
	bError |= __vmx_vmwrite(HOST_IA32_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));			// See the NTDLL!KiFastSystemCall
	bError |= __vmx_vmwrite(HOST_IA32_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));

	if (bError) {
		DbgPrint("VmSetupVmcs - Error while setting the Host state area in the VMCS of processor %i.\r\n", lpCpuVmData->dwProcNumber);
		return STATUS_UNSUCCESSFUL;
	}

	 /* 3. Use VMWRITE to initialize various guest-state area fields in the working
	  *	VMCS. This sets up the context and entry-point for guest execution upon VM entry.
	  * (section 24.4)   */
	 // Control registers CR0, CR3, and CR4 (64 bits each)
	bError |= __vmx_vmwrite(GUEST_CR0, __readcr0());
	bError |= __vmx_vmwrite(GUEST_CR3, __readcr3());
	bError |= __vmx_vmwrite(GUEST_CR4, __readcr4());
	// Debug register DR7
	bError |= __vmx_vmwrite(GUEST_DR7, __readdr(7));
	// RSP, RIP, and RFLAGS
	bError |= __vmx_vmwrite(GUEST_RSP, (size_t)lpGuestRsp);
	bError |= __vmx_vmwrite(GUEST_RIP, (size_t)lpGuestRip);
	bError |= __vmx_vmwrite(GUEST_RFLAGS, __readeflags());
	// selector fields for the segment registers(CS, SS, DS, ES, FS, GS, LDTR and TR),
	bError |= __vmx_vmwrite(GUEST_ES_SELECTOR, AsmReadES());
	bError |= __vmx_vmwrite(GUEST_CS_SELECTOR, AsmReadCS());
	bError |= __vmx_vmwrite(GUEST_SS_SELECTOR, AsmReadSS());
	bError |= __vmx_vmwrite(GUEST_DS_SELECTOR, AsmReadDS());
	bError |= __vmx_vmwrite(GUEST_FS_SELECTOR, AsmReadFS());
	bError |= __vmx_vmwrite(GUEST_GS_SELECTOR, AsmReadGS());
	bError |= __vmx_vmwrite(GUEST_LDTR_SELECTOR, AsmReadLDTR());
	bError |= __vmx_vmwrite(GUEST_TR_SELECTOR, AsmReadTR());
	// base-addressfields
	bError |= __vmx_vmwrite(GUEST_CS_BASE, 0);
	bError |= __vmx_vmwrite(GUEST_SS_BASE, 0);
	bError |= __vmx_vmwrite(GUEST_DS_BASE, 0);
	bError |= __vmx_vmwrite(GUEST_ES_BASE, 0);
	bError |= __vmx_vmwrite(GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
	bError |= __vmx_vmwrite(GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
	bError |= __vmx_vmwrite(GUEST_LDTR_BASE, GetSegmentBaseAddr(AsmReadLDTR()));
	bError |= __vmx_vmwrite(GUEST_TR_BASE, GetSegmentBaseAddr(AsmReadTR()));
	// segment limits
	bError |= __vmx_vmwrite(GUEST_CS_LIMIT, GetSegmentLimit(AsmReadCS()));
	bError |= __vmx_vmwrite(GUEST_SS_LIMIT, GetSegmentLimit(AsmReadSS()));
	bError |= __vmx_vmwrite(GUEST_DS_LIMIT, GetSegmentLimit(AsmReadDS()));
	bError |= __vmx_vmwrite(GUEST_ES_LIMIT, GetSegmentLimit(AsmReadES()));
	bError |= __vmx_vmwrite(GUEST_FS_LIMIT, GetSegmentLimit(AsmReadFS()));
	bError |= __vmx_vmwrite(GUEST_GS_LIMIT, GetSegmentLimit(AsmReadGS()));
	bError |= __vmx_vmwrite(GUEST_LDTR_LIMIT, GetSegmentLimit(AsmReadLDTR()));
	bError |= __vmx_vmwrite(GUEST_TR_LIMIT, GetSegmentLimit(AsmReadTR()));
	// access rights
	bError |= __vmx_vmwrite(GUEST_CS_AR_BYTES, GetSegmentARForVmm(AsmReadCS()));
	bError |= __vmx_vmwrite(GUEST_SS_AR_BYTES, GetSegmentARForVmm(AsmReadSS()));
	bError |= __vmx_vmwrite(GUEST_DS_AR_BYTES, GetSegmentARForVmm(AsmReadDS()));
	bError |= __vmx_vmwrite(GUEST_ES_AR_BYTES, GetSegmentARForVmm(AsmReadES()));
	bError |= __vmx_vmwrite(GUEST_FS_AR_BYTES, GetSegmentARForVmm(AsmReadFS()));
	bError |= __vmx_vmwrite(GUEST_GS_AR_BYTES, GetSegmentARForVmm(AsmReadGS()));
	bError |= __vmx_vmwrite(GUEST_LDTR_AR_BYTES, GetSegmentARForVmm(AsmReadLDTR()));
	bError |= __vmx_vmwrite(GUEST_TR_AR_BYTES, GetSegmentARForVmm(AsmReadTR()));
	// The base address and limit of GDTR and IDTR
	bError |= __vmx_vmwrite(GUEST_GDTR_BASE, gdtr.Address);
	bError |= __vmx_vmwrite(GUEST_IDTR_BASE, idtr.Address);
	bError |= __vmx_vmwrite(GUEST_GDTR_LIMIT, gdtr.Limit);
	bError |= __vmx_vmwrite(GUEST_IDTR_LIMIT, idtr.Limit);
	// The Guest MSRs (section 24.4.1)
	bError |= __vmx_vmwrite(GUEST_IA32_DEBUGCTL, __readmsr(IA32_DEBUGCTL));
	bError |= __vmx_vmwrite(GUEST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));		
	bError |= __vmx_vmwrite(GUEST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
	bError |= __vmx_vmwrite(GUEST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));

	// The guest non-register state (section 24.4.2):  
	bError |= __vmx_vmwrite(GUEST_ACTIVITY_STATE, 0);				// 0 means Active
	bError |= __vmx_vmwrite(GUEST_INTERRUPTIBILITY_INFO, 0);		// 0 means to no block any interrupt
	bError |= __vmx_vmwrite(GUEST_PENDING_DBG_EXCEPTIONS, 0);
	bError |= __vmx_vmwrite(VMCS_LINK_POINTER, 0xffffffffffffffff);	// VMCS link pointer set to -1 - No shadowing
	// For the extended EPT set EPT_POINTER

	if (bError) {
		DbgPrint("VmSetupVmcs - Error while setting the Guest State area in the VMCS of processor %i.\r\n", lpCpuVmData->dwProcNumber);
		return STATUS_UNSUCCESSFUL;
	}
	else {
		DbgPrint("VmSetupVmcs - VMCS of processor %i initialized successfully.\r\n", lpCpuVmData->dwProcNumber);
		return STATUS_SUCCESS;
	}
}

// Launch the VM of the current processor
NTSTATUS VmLaunch(PER_PROCESSOR_VM_DATA * lpCpuVmData) {
	QWORD qwErrCode = 0;							// VMX error code
	VMX_STATUS vmxStatus = VMX_STATUS_SUCCESS;		// VMX status
	DWORD dwProcNumber = 0;							// Current processor number

	DbgBreak();
	dwProcNumber = KeGetCurrentProcessorNumber();
	if (lpCpuVmData && lpCpuVmData->dwProcNumber != dwProcNumber) {
		DbgPrint("VmLaunch - Error! The current processor number is not the same as the PER_PROCESSOR struct.\r\n");
		return STATUS_INVALID_PARAMETER;
	}

	vmxStatus = (VMX_STATUS)__vmx_vmread(VM_INSTRUCTION_ERROR, &qwErrCode);
	if (vmxStatus != VMX_STATUS_SUCCESS) {
		// Log last error
		DbgPrint("VmLaunch - Warning, last VMX error: %i\r\n", (DWORD)qwErrCode);
	}

	vmxStatus = (VMX_STATUS)__vmx_vmlaunch();

	// If I am here it means that some errors happens,
	// Otherwise execution control is transferred to GUEST_RIP for a VM Entry, and HOST_RIP for a VM Exit
	if (vmxStatus == VMX_ERROR_WITH_STATUS) {
		__vmx_vmread(VM_INSTRUCTION_ERROR, &qwErrCode);
		DbgPrint("VmLaunch - Error while trying to launch the VM for processor %i - %i\r\n", dwProcNumber, (DWORD)qwErrCode);
	}
	else if (vmxStatus == VMX_ERROR_WITHOUT_STATUS) 
		DbgPrint("VmLaunch - Unknown error while trying to launch the VM for processor %i", dwProcNumber);

	return STATUS_UNSUCCESSFUL;
}
#pragma code_seg()
