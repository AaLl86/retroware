#pragma once

#include "resource.h"

// Test Windows connection
bool TestConnection(LPSTR serverName, bool bPrintRecvData = true);

// Test Registry
bool TestRegistry(bool bDisplayData = true);