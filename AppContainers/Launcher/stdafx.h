// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


// TODO: reference additional headers your program requires here
#include <stdlib.h>


#ifdef _DEBUG
#define DbgBreak() __debugbreak()
#else
#define DbgBreak() __noop()
#endif