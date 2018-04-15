/**********************************************************************
 *  Driver Model 2012
 *	Filename: Debug.h
 *	Implement Driver Debug function prototypes
 *	Last revision: 05/07/2013
 *
 *  Copyright© 2012 Andrea Allievi
 *	All right reserved
 **********************************************************************/

#pragma once

// Enable debug output for DPFLTR_DEFAULT_ID  component filter mask 
NTSTATUS EnableDebugOutput();

// Revert to default Debug Settings
VOID RevertToDefaultDbgSettings();

// Allocate Debug Memory  with auditing
PVOID DbgAllocateMemory(IN POOL_TYPE  PoolType, IN SIZE_T  NumberOfBytes, IN ULONG  Tag);

// Free Allocated Debug Memory
VOID DbgFreeMemory(PVOID pMem);
