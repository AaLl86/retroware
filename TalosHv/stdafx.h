/**********************************************************************
*   Talos Tiny Hypervisor
*	Filename: stdafx.h
*	X64 precompiled header file
*	Last revision: 01/30/2016
*
*  Copyright© 2016 Andrea Allievi - TALOS Security and Research Group
*	All right reserved
**********************************************************************/

#pragma once
#include <ntddk.h>

//Set target Windows Version for Driver
#undef NTDDI_VERSION 
#undef _WIN32_WINNT
#define NTDDI_VERSION NTDDI_WINXPSP2
#define _WIN32_WINNT 0x0501

// Common data types
typedef unsigned char BYTE, *LPBYTE, *PBYTE;
typedef unsigned long DWORD, UINT, *LPDWORD, *PDWORD;
typedef unsigned __int64 QWORD, *LPQWORD, *PQWORD;
typedef unsigned short WORD, *LPWORD, *PWORD;
typedef int BOOL, *PBOOL;
typedef unsigned char BOOLEAN, *PBOOLEAN, *LPBOOLEAN;
typedef void * LPVOID;

// Definizione per far sentire il compilatore contento
#pragma warning(disable: 4005)
#define DECLSPEC_IMPORT extern "C" __declspec(dllimport)
#define DDKBUILD extern "C"
#pragma warning(default: 4005)

// Disable unreferenced parameters warning
#pragma warning(disable:4100)

// Default Memory Tag
#define MEMTAG (ULONG)'epyH'

#ifdef _DEBUG
#define DbgBreak() __debugbreak()
#else
#define DbgBreak() __noop()
#endif
