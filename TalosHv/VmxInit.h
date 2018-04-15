/**********************************************************************
*   Talos Tiny Hypervisor
*	Filename: VmxInit.h
*	Implement Driver Entry point and startup functions
*	Last revision: 01/30/2016
*
*  Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
*	All right reserved
**********************************************************************/

#pragma once
#include "IA32_Structs.h"
#include "VmxStructs.h"
#include "asm64.h"

// The UNLOAD VmCall code (reserved)
#define VMX_UNLOAD_CALL_CODE 0x67F0

// Check if the current processor support VMX
NTSTATUS CheckVmxSupport();

// Initialize and launch a VM
#define InitializeVM() InitializeVmStub() 
EXTERN_C NTSTATUS InitializeVM_internal(LPVOID lpGuestRsp);

// Enter the VMX mode for a particular CPU
NTSTATUS VmEnableVmxMode(PER_PROCESSOR_VM_DATA * lpCpuVmData);

// Initialize the VMCS structure for a particular CPU
NTSTATUS VmInitVmcs(PER_PROCESSOR_VM_DATA * lpCpuVmData);

// Setup the VMCS to virtualize the guest as the host (see the Hypervisor Abiss documentation)
NTSTATUS VmSetupVmcs(PER_PROCESSOR_VM_DATA * lpCpuVmData, LPVOID lpGuestRIP, LPVOID lpGuestRsp);

// Launch the VM of the current processor
NTSTATUS VmLaunch(PER_PROCESSOR_VM_DATA * lpCpuVmData);


