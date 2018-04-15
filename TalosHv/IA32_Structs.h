/**********************************************************************
*   Talos Tiny Hypervisor
*	Filename: IA32_Structs.h
*	X64 General Intel Data structures
*	Last revision: 01/30/2016
*
*  Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
*	All right reserved
**********************************************************************/
#pragma once

// CPUID context structure
typedef struct _CPUID_CONTEXT {
	DWORD eax;
	DWORD ebx;
	DWORD ecx;
	DWORD edx;
} CPUID_CONTEXT;

// MODEL-SPECIFIC REGISTERS (MSRS)
enum MSR_CODE : unsigned int {
	IA32_FEATURE_CONTROL = 0x03A,
	IA32_SYSENTER_CS = 0x174,
	IA32_SYSENTER_ESP = 0x175,
	IA32_SYSENTER_EIP = 0x176,
	IA32_DEBUGCTL = 0x1D9,
	IA32_VMX_BASIC = 0x480,
	IA32_VMX_PINBASED_CTLS = 0x481,
	IA32_VMX_PROCBASED_CTLS = 0x482,
	IA32_VMX_EXIT_CTLS = 0x483,
	IA32_VMX_ENTRY_CTLS = 0x484,
	IA32_VMX_MISC = 0x485,
	IA32_VMX_CR0_FIXED0 = 0x486,
	IA32_VMX_CR0_FIXED1 = 0x487,
	IA32_VMX_CR4_FIXED0 = 0x488,
	IA32_VMX_CR4_FIXED1 = 0x489,
	IA32_VMX_VMCS_ENUM = 0x48A,
	IA32_VMX_PROCBASED_CTLS2 = 0x48B,
	IA32_VMX_EPT_VPID_CAP = 0x48C,

	IA32_EFER = 0xC0000080,
	IA32_STAR = 0xC0000081,
	IA32_LSTAR = 0xC0000082,
	IA32_FMASK = 0xC0000084,
	IA32_FS_BASE = 0xC0000100,
	IA32_GS_BASE = 0xC0000101,
	IA32_KERNEL_GS_BASE = 0xC0000102,
	IA32_TSC_AUX = 0xC0000103,
};

//
// Control Registers
//
union CR0_REG {
	ULONG_PTR All;
	struct {
		unsigned PE : 1;          // [0] Protected Mode Enabled
		unsigned MP : 1;          // [1] Monitor Coprocessor FLAG
		unsigned EM : 1;          // [2] Emulate FLAG
		unsigned TS : 1;          // [3] Task Switched FLAG
		unsigned ET : 1;          // [4] Extension Type FLAG
		unsigned NE : 1;          // [5] Numeric Error
		unsigned Reserved1 : 10;  // [6-15]
		unsigned WP : 1;          // [16] Write Protect
		unsigned Reserved2 : 1;   // [17]
		unsigned AM : 1;          // [18] Alignment Mask
		unsigned Reserved3 : 10;  // [19-28]
		unsigned NW : 1;          // [29] Not Write-Through
		unsigned CD : 1;          // [30] Cache Disable
		unsigned PG : 1;          // [31] Paging Enabled
	} Fields;
};
static_assert(sizeof(CR0_REG) == sizeof(void*), "Size check");

union CR4_REG {
	ULONG_PTR All;
	struct {
		unsigned TWA : 1;			// 0 - Virtual Mode Extensions - If set, enables support for the virtual interrupt flag (VIF) in virtual-8086 mode.
		unsigned FYI : 1;			// 1 - Protected-Mode Virtual Interrupts
		unsigned TSD : 1;			// 2 - Time Stamp Disable
		unsigned OF : 1;			// 3 - Debugging Extensions
		unsigned PSE : 1;			// 4 - Page Size Extensions
		unsigned EAP : 1;			// 5 - Physical Address Extension
		unsigned MCE : 1;			// 6 - Machine Check Enable
		unsigned PGE : 1;			// 7 - Enable Global Page
		unsigned PCE : 1;			// 8 - Performance-Monitoring Counter Enable
		unsigned OSFXSR : 1;		// 9 - OS Support for FXSAVE / FXRSTOR
		unsigned OSXMMEXCPT : 1;	// 10 - OS Support for Unmasked SIMD Floating-Point Exceptions
		unsigned Reserved1 : 2;		// 11
		unsigned VMXE : 1;			// 13 - Enabled Virtual Machine Extensions
		unsigned SMXE : 2;			// 14 - Safer Mode Extensions Enable (TXT)
		unsigned FSGSBASE : 1;		// 16 - Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE.
		unsigned PCIDE : 1;			// 17 - If set, enables process-context identifiers (PCIDs).
		unsigned OSXSAVE : 2;		// 18 - XSAVE and Processor Extended States Enable
		unsigned SMEP : 1;			// 20 - If set, execution of code in a higher ring generates a fault
		unsigned SMAP : 1;			// 21 - If set, access of data in a higher ring generates a fault[1]
		unsigned PKE : 1;			// 22 - Protection Key Enable
		unsigned Reserved2 : 10;
	} Fields;
};

// IDTR/GDTR
// MEMORY-MANAGEMENT REGISTERS
#include <pshpack1.h>
struct IDTR {
	unsigned short Limit;
	ULONG_PTR Address;
};
typedef IDTR GDTR;
static_assert(sizeof(IDTR) == 10, "Size check");

// x86 Segment Selector (Section 3.4.2)
union X86_SEG_SELECTOR {
	unsigned short All;
	struct {
		unsigned short RPL : 2;  // Requested Privilege Level
		unsigned short TI : 1;   // Table Indicator: 0 - GDT; 1 - LDT
		unsigned short Index : 13;
	} Fields;
};
static_assert(sizeof(X86_SEG_SELECTOR) == 2, "Size check");

// Segment Desctiptor (Section 3.4.5)
union X86_SEG_DESCRIPTOR {
	unsigned __int64 All;
	struct {
		unsigned LimitLow : 16;			// [0-15]
		unsigned BaseLow : 16;			// [16-31]
		unsigned BaseMid : 8;			// [32-39]
		unsigned Type : 4;				// [40-43]
		unsigned System : 1;			// [44] Descriptor type: 0 - System; 1 - Code or Data
		unsigned DPL : 2;				// [45-46]
		unsigned Present : 1;
		unsigned LimitHi : 4;
		unsigned AVL : 1;
		unsigned L : 1;					// [53] 64-bit code segment (IA-32e mode only)
		unsigned DB : 1;
		unsigned Gran : 1;
		unsigned BaseHi : 8;
	} Fields;
};
static_assert(sizeof(X86_SEG_DESCRIPTOR) == 8, "Size check");

struct X86_SEG_DESCRIPTOR64 {
	X86_SEG_DESCRIPTOR Descriptor;
	DWORD BaseUpper32;
	DWORD Reserved;
};
static_assert(sizeof(X86_SEG_DESCRIPTOR64) == 16, "Size check");

#include <poppack.h>

//
// EFLAGS
//  2.3 SYSTEM FLAGS AND FIELDS IN THE EFLAGS REGISTER
//
union EFLAGS {
	ULONG_PTR All;
	struct {
		unsigned CF : 1;          // [0] Carry flag
		unsigned Reserved1 : 1;   // [1]  Always 1
		unsigned PF : 1;          // [2] Parity flag
		unsigned Reserved2 : 1;   // [3] Always 0
		unsigned AF : 1;          // [4] Borrow flag
		unsigned Reserved3 : 1;   // [5] Always 0
		unsigned ZF : 1;          // [6] Zero flag
		unsigned SF : 1;          // [7] Sign flag
		unsigned TF : 1;          // [8] Trap flag
		unsigned IF : 1;          // [9] Interrupt flag
		unsigned DF : 1;          // [10]
		unsigned OF : 1;          // [11]
		unsigned IOPL : 2;        // [12-13] I/O privilege level
		unsigned NT : 1;          // [14] Nested task flag
		unsigned Reserved4 : 1;   // [15] Always 0
		unsigned RF : 1;          // [16] Resume flag
		unsigned VM : 1;          // [17] Virtual 8086 mode
		unsigned AC : 1;          // [18] Alignment check
		unsigned VIF : 1;         // [19] Virtual interrupt flag
		unsigned VIP : 1;         // [20] Virtual interrupt pending
		unsigned ID : 1;          // [21] Identification flag
		unsigned Reserved5 : 10;  // [22-31] Always 0
	} Fields;
};
static_assert(sizeof(EFLAGS) == sizeof(void*), "Size check");

//
// For PUSHAQ
//
struct GP_REGISTERS {
	ULONG_PTR r15;
	ULONG_PTR r14;
	ULONG_PTR r13;
	ULONG_PTR r12;
	ULONG_PTR r11;
	ULONG_PTR r10;
	ULONG_PTR r9;
	ULONG_PTR r8;
	ULONG_PTR rdi;
	ULONG_PTR rsi;
	ULONG_PTR rbp;
	ULONG_PTR rsp;
	ULONG_PTR rdx;
	ULONG_PTR rcx;
	ULONG_PTR rbx;
	ULONG_PTR rax;
};

//
// For the sequence of pushfq, PUSHAQ
//
struct ALL_REGISTERS {
	GP_REGISTERS gpReg;
	EFLAGS rflags;
};