// stdafx.h : file di inclusione per file di inclusione di sistema standard
// o file di inclusione specifici del progetto utilizzati di frequente, ma
// modificati raramente
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Escludere gli elementi utilizzati di rado dalle intestazioni di Windows
// File di intestazione di Windows:
#include <windows.h>

// File di intestazione Runtime C
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define COUNTOF(x) sizeof(x) / sizeof(x[0])

enum ConsoleColor { DARKBLUE = 1, DARKGREEN, DARKTEAL, DARKRED, DARKPINK, DARKYELLOW, 
	GRAY, DARKGRAY, BLUE, GREEN, TEAL, RED, PINK, YELLOW, WHITE };

// Set console text Color
void SetConsoleColor(ConsoleColor c);
// Set console background color
void SetConsoleBk(ConsoleColor c);
// Color WPrintf 
void cl_wprintf(ConsoleColor c, LPTSTR string, LPVOID arg1 = NULL, LPVOID arg2 = NULL, LPVOID arg3 = NULL, LPVOID arg4 = NULL);
