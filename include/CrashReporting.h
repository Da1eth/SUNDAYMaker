// CrashReporting.h - process-wide fatal error reporting
#pragma once

#include <windows.h>

// Installs the unhandled-exception and C++ terminate handlers. Crash reports are
// written to the CrashLogs directory beside the executable when possible.
void CrashReportingInitialise(LPCTSTR executableDirectory) noexcept;
