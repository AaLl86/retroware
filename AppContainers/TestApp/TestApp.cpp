/*
 *  Copyright (C) 2013 Saferbytes Srl - Andrea Allievi
 *	Filename: TestApp.cpp
 *	Implements a test application runned under an AppContainer Environment
 *	Last revision: 05/30/2013
 *
 */

#include "stdafx.h"
#include "TestApp.h"
#include "AppContainer.h"
#include <Winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <iostream>

// Create console screen buffer and set it to application
bool SetConsoleBuffers() {
	HANDLE hNewScreenBuffer = FALSE;
	BOOL retVal = FALSE;

	FILE * fOut = NULL, *fIn = NULL, *fErr = NULL;
	freopen_s(&fOut, "CON", "w", stdout ) ;
	freopen_s(&fIn, "CON", "r", stdin ) ;
	freopen_s(&fErr, "CON", "w", stderr ) ;
	std::cin.clear();
	std::cout.clear();
	std::cerr.clear();
	std::ios::sync_with_stdio();

	rewind(stdout);
	rewind(stdin);
	//wprintf(L"\r\n");
	//WCHAR buff[10];
	//wscanf_s(L"%s", buff, 10);
	return (fOut != NULL);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	DWORD lastErr = 0;	
	LPTSTR appContFileName = L"D:\\test\\prova.txt";			// APP Container fileName
	LPTSTR docFileName = L"d:\\Documenti\\Test.txt";			// Document library file name
	LPTSTR stdFileName = L"C:\\Python27\\python.exe";			// Standard file name
	TCHAR resStr[0x200] = {0};
	BOOL retVal = FALSE;
	DWORD bytesRead = 0;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MessageBox(NULL, L"Starting AaLl86 AppContainer test Application....", L"AaLl86 Test", MB_ICONINFORMATION);
	retVal = AllocConsole(); SetConsoleBuffers();
	SetConsoleTitle(L"AaLl86 AppContainer Test Application");
	//system("color f0");
	Sleep(400);
	wprintf(L"\t\tLOWBOX TOKEN TEST\r\n");
 	retVal = GetTokenAppContainerState(NULL);

	wprintf(L"\r\n\t\tCONNECTION TEST\r\n");
	retVal = TestConnection("www.google.com", false);
	//Sleep(1250);
	//system("cls");
	//if (retVal) {
	//	wprintf(L"Connection test completed with success!\r\n");
	//} else
	//	wprintf(L"Connection test failed with error %i.\r\n", WSAGetLastError());

	wprintf(L"\r\n\t\tFILE I/O TEST\r\n");

	for (int i = 0; i < 3; i++) {
		LPTSTR fileName = NULL;
		HANDLE hFile = NULL;
		BYTE smallBuff[1024] = {0};

		if (i == 0) {
			// App Container file
			fileName = appContFileName;
			wprintf(L"File with AppContainer ACE test (\"%s\")... ", fileName);		// (wcsrchr(fileName, L'\\') + 1)
		} else if (i == 1) {
			// User document file
			fileName = docFileName;
			wprintf(L"User document-library file test (\"%s\")... ", fileName);
		} else {
			// User document file
			fileName = stdFileName;
			wprintf(L"Standard file test (\"%s\")... ", fileName);
		}
		// Try to open a file
		hFile = CreateFile(fileName, FILE_GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) 
			retVal = ReadFile(hFile, smallBuff, sizeof(smallBuff), &bytesRead, NULL);
		lastErr = GetLastError();

		if (hFile != INVALID_HANDLE_VALUE && retVal) 
			cl_wprintf(GREEN, L"Success!\r\n");
		else { 
			cl_wprintf(RED,L"Failed ");
			wprintf(L"(error %i)\r\n", lastErr);
		}
		//MessageBox(NULL, resStr, L"AaLl86 AppContainer test", MB_ICONEXCLAMATION);
		if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	}
	rewind(stdin);
	_getwch();

	wprintf(L"\r\n\t\tREGISTRY I/O TEST\r\n");
	retVal = TestRegistry(true);

//Cleanup:
    WSACleanup();

	rewind(stdin);
	wprintf(L"\r\nPress any key to exit...");
	_getwch();
}

// Test Windows connection
bool TestConnection(LPSTR serverName, bool bPrintRecvData) {
	//----------------------
    // Initialize Winsock
	WSADATA wsaData = {0};
	DWORD res = 0;
	struct addrinfo hints = {0};
	struct addrinfo *result = NULL;
	 
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        wprintf(L"WSAStartup function failed with error: %d\n", iResult);
        return 1;
    }
    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket;
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    // Setup the hints address info structure
    // which is passed to the getaddrinfo() function
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

	// Get dns ip address
	res = getaddrinfo(serverName, "http", &hints, &result);
	if (res != 0) {
        wprintf(L"Getaddrinfo function failed with error: %ld\n", WSAGetLastError());
        iResult = closesocket(ConnectSocket);
        if (iResult == SOCKET_ERROR)
            wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
        return false;
	}

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.S_un.S_addr = inet_addr("192.168.0.1");
    clientService.sin_port = htons(80);

    //----------------------
    // Connect to server.
	iResult = connect(ConnectSocket, (SOCKADDR *)result->ai_addr, sizeof(SOCKADDR));
    if (iResult == SOCKET_ERROR) {
        wprintf(L"connect function failed with error: %ld\n", WSAGetLastError());
        iResult = closesocket(ConnectSocket);
        if (iResult == SOCKET_ERROR)
            wprintf(L"closesocket function failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return false;
    }
    //wprintf(L"Connected to server \"%S\".\n", serverName);

    //----------------------
    // Send an initial buffer
	LPSTR sendbuf = "GET / HTTP/1.1\nHost: www.andrea-allievii.com\n\n";
    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        wprintf(L"send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return false;
    }

    printf("Bytes Sent: %d\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return false;
    }

    // Receive until the peer closes the connection
	DWORD recvbuflen = 0x1000;
	BYTE * recvbuf = new BYTE[recvbuflen];
	DWORD totalBytesReceived = 0;

    do {
        iResult = recv(ConnectSocket, (char*)recvbuf, recvbuflen-1, 0);
		recvbuf[iResult] = 0;
		if ( iResult > 0) {
			if (bPrintRecvData) 
				printf("%s", (char*)recvbuf);
		} else if ( iResult == 0 )
            wprintf(L"Connection closed\r\n");
        else
            wprintf(L"recv failed with error: %d\n", WSAGetLastError());
		totalBytesReceived += iResult;
    } while( iResult > 0 );

	wprintf(L"Total received bytes: %i\r\n", totalBytesReceived);

    // close the socket
    iResult = closesocket(ConnectSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"close failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return false;
    }
    return true;
}
