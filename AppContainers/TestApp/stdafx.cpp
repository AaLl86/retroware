// stdafx.cpp : file di origine che include solo le inclusioni standard
// TestApp.pch sarà l'intestazione precompilata
// stdafx.obj conterrà le informazioni sui tipi precompilati

#include "stdafx.h"

// Set console color
void SetConsoleColor(ConsoleColor c){
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hCon, (WORD)c);
}

// Set console background color
void SetConsoleBk(ConsoleColor c){
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hCon, ((WORD)c) << 4);
}

// Color WPrintf 
void cl_wprintf(ConsoleColor c, LPTSTR string, LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4) {
	SetConsoleColor(c);
	wprintf(string, arg1, arg2, arg3, arg4);
	SetConsoleColor(GRAY);
}