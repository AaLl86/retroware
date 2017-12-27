// stdafx.h : file di inclusione per file di inclusione di sistema standard
// o file di inclusione specifici del progetto utilizzati di frequente, ma
// modificati raramente
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Debug Routines
#include <crtdbg.h> 

#include "resource.h"

#ifdef _DEBUG
#define DbgBreak __asm int 3;
#else
#define DbgBreak __asm nop;
#endif

typedef unsigned __int64 QWORD;
