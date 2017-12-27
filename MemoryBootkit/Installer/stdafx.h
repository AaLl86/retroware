#pragma once

// Set Target version
#ifndef _WIN32_WINNT            // Specifica che la piattaforma minima richiesta è Windows Vista SP0
#define _WIN32_WINNT 0x0600     // Modificare il valore con quello appropriato per altre versioni di Windows.
#endif

#include <stdio.h>
#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "wincore.h"

typedef unsigned __int64 QWORD;
#define STATUS_NOT_IMPLEMENTED 0xC0000002

// Definizione per far sentire il compilatore contento
#pragma warning(disable: 4005)
#pragma warning (disable: 4201)				// Used non-standard struct/union without name
#define DECLSPEC_IMPORT extern "C" __declspec(dllimport)
#define DDKBUILD extern "C"
#pragma warning(default: 4005)
//#pragma comment(lib, "ntdll.lib")

// GDI Plus Header and Library
#include <gdiplus.h>
#pragma comment (lib, "gdiplus.lib")
#include "resource.h"

#ifdef _DEBUG
#define DbgBreak() __debugbreak();
#else
#define DbgBreak() __noop();
#endif

// Get number elements of a STACK array
#define COUNTOF(x) sizeof(x) / sizeof(x[0])

// Title of this application
#define APPTITLE L"X86 Memory Bootkit"
#define COMPANYNAME L"Saferbytes"

// Global background color of all application windows
#define GLOBAL_DLGS_BKCOLOR RGB(255,255,238)

// Standard disk sector size
#define SECTOR_SIZE 0x200

// Get if a file Exist
bool FileExist(LPTSTR fileName);