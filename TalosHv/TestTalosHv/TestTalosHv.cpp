// TestTalosHv.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TestTalosHv.h"
#include <intrin.h>

DWORD GetProcessorSpeed()
{
	LARGE_INTEGER qwWait, qwStart, qwCurrent;
	QueryPerformanceCounter(&qwStart);
	QueryPerformanceFrequency(&qwWait);
	qwWait.QuadPart >>= 5;
	unsigned __int64 Start = __rdtsc();
	do
	{
		QueryPerformanceCounter(&qwCurrent);
	} while (qwCurrent.QuadPart - qwStart.QuadPart < qwWait.QuadPart);
	DWORD dwCPUSpeedMHz = (DWORD)(((__rdtsc() - Start) << 5) / 1000000.0);
	return dwCPUSpeedMHz;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CHAR cpuIdStr[64] = { 0 };				// CPU ID string
	CHAR hvType[64] = { 0 };				// Hypervisor ID string
	int cpuInfo[4] = { 0 };
	DWORD dwMaxBasicId = 0,					// Maximum Input Value for Basic CPUID Information
		dwMaxProcId = 0;					// Maximum Input Value for Extended Function CPUID Information 
	int invalidCpuInfo[4] = { 0 };

	// Get the processor Speed in Mhz
	DWORD dwSpeed = GetProcessorSpeed();

	// Get the CPU basic ID string
	__cpuidex(cpuInfo, 0, 0);
	dwMaxBasicId = cpuInfo[0];
	__cpuidex(invalidCpuInfo, dwMaxBasicId + 10, 0);
	RtlZeroMemory(cpuInfo, sizeof(cpuInfo));


	// Get max Extended Fucntion CPUID:
	__cpuidex(cpuInfo, 0x80000000, 0);
	dwMaxProcId = cpuInfo[0];
	RtlZeroMemory(cpuInfo, sizeof(cpuInfo));

	__cpuidex(cpuInfo, 0x80000002, 0);
	strncpy_s(cpuIdStr, sizeof(cpuIdStr), (LPSTR)cpuInfo, 16);

	__cpuidex(cpuInfo, 0x80000003, 0);
	strncpy_s(cpuIdStr + 16, sizeof(cpuIdStr) - 16, (LPSTR)cpuInfo, 16);

	__cpuidex(cpuInfo, 0x80000004, 0);
	strncpy_s(cpuIdStr + 32, sizeof(cpuIdStr) - 32, (LPSTR)cpuInfo, 16);
	RtlZeroMemory(cpuInfo, sizeof(cpuInfo));

	//// Emit a CPUID information with funcId 0x16
	//__cpuidex(cpuInfo, 0x16, 0x00);
	//dwBaseSpeed = cpuInfo[1] & 0xFFFF;
	//sprintf_s(cpuIdStr, 64, "%s at %i Mhz.", cpuIdStr, dwBaseSpeed);

	// Now get if we are under AaLl86 Hypervisor
	DWORD dwCode = TALOSHV_CPUID_CODE;
	__cpuidex(cpuInfo, TALOSHV_CPUID_CODE, 0x00);
	if ((cpuInfo[0] != 0 && cpuInfo[1] != 0) &&
		(cpuInfo[0] != invalidCpuInfo[0] && cpuInfo[1] != invalidCpuInfo[1])) {
		strncpy_s(hvType, sizeof(hvType), (LPSTR)cpuInfo, 16);
		__cpuidex(cpuInfo, TALOSHV_CPUID_CODE, 0x01);
		hvType[16] = ' ';
		strncpy_s(hvType + 17, sizeof(hvType) - 17, (LPSTR)cpuInfo, 16);
	}
	else
		strcpy_s(hvType, sizeof(hvType), "None Detected!");

	TCHAR lpMexBoxText[255] = { 0 };
	swprintf_s(lpMexBoxText, 255, L"CPU Information:\r\n   %S\r\nProcessor Speed: %i Mhz\r\nHypervisor Type: \r\n   %S.", cpuIdStr, dwSpeed, hvType);
	MessageBox(NULL, lpMexBoxText, L"AaLl86 RedPill", MB_ICONINFORMATION);


}

