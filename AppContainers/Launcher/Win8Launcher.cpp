/*
 *  Copyright (C) 2013 Saferbytes Srl - Andrea Allievi
 *	Filename: Win8Launcher.cpp
 *	Implements a Windows 8 Security test application
 *	Last revision: 05/30/2013
 *
 */
#include "stdafx.h"
#include "Win8Launcher.h"
#include "resource.h"

// GUI Test thread
DWORD _stdcall GuiThread(LPVOID param);
DWORD g_iGuiThrLastError = 0;
BOOLEAN g_bWindowCreated = FALSE;

int _tmain(int argc, _TCHAR* argv[])
{
	BOOL retVal = FALSE;
	DWORD dwThrId = 0;
	HMODULE hMod = GetModuleHandle(NULL);
	HANDLE hGuiThread = NULL;
	HANDLE hCurProc = GetCurrentProcess();
	PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY sysMit = {0};
	DWORD exitCode = 0;

	DWORD test = PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR_FAKE;

	//LUID pLuid = {0};
	//LookupPrivilegeValue(NULL, SE_INC_WORKING_SET_NAME, &pLuid);
	DoSecurityTests();
	
	TestAppContainer();
	system("cls");
	wprintf(L"|-------------------------------------------------------------|\r\n");
	wprintf(L"|                AaLl86 Windows 8 Security TEST               |\r\n");
	wprintf(L"|-------------------------------------------------------------|\r\n");
	wprintf(L"\r\n");

	retVal = GetProcessMitigationPolicy(hCurProc, ProcessSystemCallDisablePolicy, (LPVOID)&sysMit, sizeof(sysMit));
	if (retVal) 
		wprintf(L"Current process SysCall mitigation Settings: %s\r\n", sysMit.DisallowWin32kSystemCalls ? L"ON": L"OFF");
	
	if (!sysMit.DisallowWin32kSystemCalls) {
		TCHAR answer[10] = {0};
		wprintf(L"Would you like to enable SysCall mitigation for current process? [Y/N] ");
		rewind(stdin);
		wscanf_s(L"%s", answer, 10);

		if ((answer[0] | 0x20) == L'y') {
			sysMit.DisallowWin32kSystemCalls = TRUE;
			retVal = SetProcessMitigationPolicy(ProcessSystemCallDisablePolicy, (LPVOID)&sysMit, sizeof(sysMit));
			if (retVal) wprintf(L"\tSuccess!\r\n");
		}
		wprintf(L"\r\n");

	}
		
	wprintf(L"Creating GUI thread...\r\n");
	hGuiThread = CreateThread(NULL, 0, GuiThread, hMod, 0, &dwThrId);
	WaitForSingleObject(hGuiThread, INFINITE);
	retVal = GetExitCodeThread(hGuiThread, &exitCode);
	if (!g_bWindowCreated)
		wprintf(L"Gui Thread has failed to create ALL its Windows!\r\n");
	else
		wprintf(L"Win32 window was created and then closed.\r\n");
	wprintf(L"Gui Thread is exited with code: %i. Last GUI thread error: %i\r\n", exitCode, g_iGuiThrLastError);
	wprintf(L"Press Enter to exit....\r\n");
	rewind(stdin);
	getwchar();
	return 0;
}

// GUI Test thread
DWORD _stdcall GuiThread(LPVOID param) {
	HINSTANCE hInst = (HINSTANCE)param;
	int retVal = 
		_tWinMain(hInst, NULL, NULL, SW_SHOW);
	return (DWORD)retVal;
}

#pragma region GUI Stuff
#define MAX_LOADSTRING 100

// Variabili globali:
HINSTANCE hInst;								// istanza corrente
TCHAR szTitle[MAX_LOADSTRING];					// Testo della barra del titolo
TCHAR szWindowClass[MAX_LOADSTRING];			// nome della classe di finestre principale

// Dichiarazioni con prototipo delle funzioni incluse in questo modulo di codice:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: inserire qui il codice.
	MSG msg;
	HACCEL hAccelTable;

	// Inizializzare le stringhe globali
	wcscpy_s(szTitle, MAX_LOADSTRING, L"Test Win32 Window");
	wcscpy_s(szWindowClass, MAX_LOADSTRING, L"AaLl86");
	MyRegisterClass(hInstance);

	// Eseguire l'inizializzazione dall'applicazione:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));

	// Ciclo di messaggi principale:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
	}

	return (int) msg.wParam;
}


//
//  FUNZIONE: MyRegisterClass()
//
//  SCOPO: registra la classe di finestre.
//
//  COMMENTI:
//
//    Questa funzione e il relativo utilizzo sono necessari solo se si desidera che il codice
//    sia compatibile con i sistemi Win32 precedenti alla funzione 'RegisterClassEx'
//    aggiunta a Windows 95. È importante chiamare questa funzione
//    in modo da ottenere piccole icone in formato corretto associate
//    all'applicazione.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICO));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDR_MENU);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICO));

	ATOM retAtom = RegisterClassEx(&wcex);
	DWORD lastErr = GetLastError();
   if (lastErr == ERROR_SUCCESS && !retAtom) lastErr = ERROR_NOT_GUI_PROCESS;

	g_iGuiThrLastError = lastErr;
	return retAtom;
}

//
//   FUNZIONE: InitInstance(HINSTANCE, int)
//
//   SCOPO: salva l'handle di istanza e crea la finestra principale
//
//   COMMENTI:
//
//        In questa funzione l'handle di istanza viene salvato in una variabile globale e
//        la finestra di programma principale viene creata e visualizzata.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Memorizzare l'handle di istanza nella variabile globale

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
   DWORD lastErr = GetLastError();
   if (lastErr == ERROR_SUCCESS && !hWnd) lastErr = ERROR_NOT_GUI_PROCESS;
   
   if (!hWnd)
   {
	  g_iGuiThrLastError = lastErr;
      return FALSE;
   }

   g_bWindowCreated = TRUE;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNZIONE: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  SCOPO:  elabora i messaggi per la finestra principale.
//
//  WM_COMMAND	- elabora il menu dell'applicazione
//  WM_PAINT	- disegna la finestra principale
//  WM_DESTROY	- inserisce un messaggio di uscita e restituisce un risultato
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Analizzare le selezioni di menu:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_FILE_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: aggiungere qui il codice per il disegno...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Gestore dei messaggi della finestra Informazioni su.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
#pragma endregion

#pragma region Test Code showed in companion Analysis article
/*
BOOL IsCapabilityPresent(PSID neededCapSid, SID_AND_ATTRIBUTES * capArray, DWORD capCount);
DWORD NtStatusToWin32Error(NTSTATUS ntStatus);

DWORD TestSuca() {

	HANDLE hToken = NULL;
	BOOL ntStatus = FALSE,
		retVal = FALSE;
	BOOL bIsAppContainer = FALSE;
	DWORD retLength = 0;
	DWORD buffSize = 0;
	PSID pNeededCapSid = NULL;

// Get if token is an AppContainer
ntStatus = ZwQueryInformationToken(hToken, TokenIsAppContainer, &bIsAppContainer, sizeof(BOOL), &retLength);
if (!NT_SUCCESS(ntStatus))
	return NtStatusToWin32Error(ntStatus)

if (bIsAppContainer)  {
	// Query LowBox token Capabilities
	PTOKEN_CAPABILITIES pTokCap = NULL;

	// Allocate enough buffer to contains Array (call ZwQueryInformationToken with a NULL buffer to get
	// exactly the needed size)
	pTokCap = (PTOKEN_CAPABILITIES)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buffSize);
	retVal = TRUE;
	ntStatus = ZwQueryInformationToken(hToken, TokenCapabilities, (PVOID)pTokCap, buffSize, &retLength);
	if (NT_SUCCESS(ntStatus)) 
		// Get if needed capability is present in 
		// PSID pNeededCapSid is needed capability
		retVal = IsCapabilityPresent(pNeededCapSid, pTokCap->Capabilities, pTokCap->CapabilityCount);
		
	if (!retVal | NT_ERROR(ntStatus))
		return ERROR_ACCESS_DENIED;
}

// Main function body
}
*/
#pragma endregion