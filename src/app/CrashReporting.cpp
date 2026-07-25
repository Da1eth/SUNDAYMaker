#include "CrashReporting.h"

#include <dbghelp.h>
#include <shlobj.h>

#include <exception>

namespace
{
constexpr DWORD kCppTerminateException = 0xE0000001;

TCHAR gatCrashDirectory[MAX_PATH]{};
char gacCppTerminateDetails[1024]{};
volatile LONG glCrashReportStarted = 0;

bool DirectoryIsUsable(LPCTSTR path) noexcept
{
    if (CreateDirectory(path, nullptr))
        return true;

    if (GetLastError() != ERROR_ALREADY_EXISTS)
        return false;

    const DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool SetCrashDirectory(LPCTSTR parent, LPCTSTR child,
                       LPCTSTR grandchild = nullptr) noexcept
{
    TCHAR path[MAX_PATH]{};
    if (FAILED(StringCchCopy(path, MAX_PATH, parent)) ||
        FAILED(PathAppend(path, child)) ||
        !DirectoryIsUsable(path))
    {
        return false;
    }

    if (grandchild &&
        (FAILED(PathAppend(path, grandchild)) || !DirectoryIsUsable(path)))
    {
        return false;
    }

    return SUCCEEDED(StringCchCopy(gatCrashDirectory, MAX_PATH, path));
}

const char *ExceptionReason(DWORD code) noexcept
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        return "Access violation";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "Array bounds exceeded";
    case EXCEPTION_BREAKPOINT:
        return "Breakpoint";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "Misaligned data access";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "Floating-point division by zero";
    case EXCEPTION_FLT_INVALID_OPERATION:
        return "Invalid floating-point operation";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "Illegal instruction";
    case EXCEPTION_IN_PAGE_ERROR:
        return "Memory page could not be loaded";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "Integer division by zero";
    case EXCEPTION_INT_OVERFLOW:
        return "Integer overflow";
    case EXCEPTION_STACK_OVERFLOW:
        return "Stack overflow";
    case kCppTerminateException:
        return "C++ termination (uncaught exception or noexcept violation)";
    default:
        return "Unknown fatal exception";
    }
}

void WideToUtf8(LPCWSTR source, char *destination,
                int destinationSize) noexcept
{
    if (!source ||
        WideCharToMultiByte(CP_UTF8, 0, source, -1, destination,
                            destinationSize, nullptr, nullptr) == 0)
    {
        if (destinationSize > 0)
            destination[0] = '\0';
    }
}

void WriteCrashReport(EXCEPTION_POINTERS *exceptionPointers) noexcept
{
    if (InterlockedCompareExchange(&glCrashReportStarted, 1, 0) != 0)
        return;

    SYSTEMTIME time{};
    GetLocalTime(&time);

    const DWORD processId = GetCurrentProcessId();
    const DWORD threadId = GetCurrentThreadId();

    TCHAR baseName[96]{};
    StringCchPrintf(baseName, ARRAYSIZE(baseName),
                    TEXT("SundayMaker_%04u%02u%02u_%02u%02u%02u_%03u_%lu"),
                    time.wYear, time.wMonth, time.wDay, time.wHour,
                    time.wMinute, time.wSecond, time.wMilliseconds, processId);

    TCHAR dumpPath[MAX_PATH]{};
    TCHAR logPath[MAX_PATH]{};
    StringCchCopy(dumpPath, MAX_PATH, gatCrashDirectory);
    StringCchCopy(logPath, MAX_PATH, gatCrashDirectory);
    PathAppend(dumpPath, baseName);
    PathAppend(logPath, baseName);
    StringCchCat(dumpPath, MAX_PATH, TEXT(".dmp"));
    StringCchCat(logPath, MAX_PATH, TEXT(".log"));

    bool dumpWritten = false;
    DWORD dumpError = ERROR_SUCCESS;
    HANDLE dumpFile = CreateFile(dumpPath, GENERIC_WRITE, FILE_SHARE_READ,
                                 nullptr, CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dumpFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};
        exceptionInfo.ThreadId = threadId;
        exceptionInfo.ExceptionPointers = exceptionPointers;
        exceptionInfo.ClientPointers = FALSE;

        dumpWritten =
            MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFile,
                              MiniDumpNormal,
                              exceptionPointers ? &exceptionInfo : nullptr,
                              nullptr, nullptr) != FALSE;
        if (!dumpWritten)
            dumpError = GetLastError();
        FlushFileBuffers(dumpFile);
        CloseHandle(dumpFile);
    }
    else
    {
        dumpError = GetLastError();
    }

    DWORD exceptionCode = kCppTerminateException;
    ULONG_PTR exceptionAddress = 0;
    ULONG_PTR accessKind = 0;
    ULONG_PTR accessAddress = 0;
    bool hasAccessDetails = false;

    if (exceptionPointers && exceptionPointers->ExceptionRecord)
    {
        const EXCEPTION_RECORD *record = exceptionPointers->ExceptionRecord;
        exceptionCode = record->ExceptionCode;
        exceptionAddress =
            reinterpret_cast<ULONG_PTR>(record->ExceptionAddress);
        if ((exceptionCode == EXCEPTION_ACCESS_VIOLATION ||
             exceptionCode == EXCEPTION_IN_PAGE_ERROR) &&
            record->NumberParameters >= 2)
        {
            accessKind = record->ExceptionInformation[0];
            accessAddress = record->ExceptionInformation[1];
            hasAccessDetails = true;
        }
    }

    TCHAR modulePath[MAX_PATH]{};
    ULONG_PTR moduleBase = 0;
    if (exceptionAddress != 0)
    {
        MEMORY_BASIC_INFORMATION memoryInfo{};
        if (VirtualQuery(reinterpret_cast<LPCVOID>(exceptionAddress),
                         &memoryInfo, sizeof(memoryInfo)) != 0)
        {
            moduleBase = reinterpret_cast<ULONG_PTR>(memoryInfo.AllocationBase);
            GetModuleFileName(reinterpret_cast<HMODULE>(memoryInfo.AllocationBase),
                              modulePath, MAX_PATH);
        }
    }

    char modulePathUtf8[MAX_PATH * 3]{};
    char dumpPathUtf8[MAX_PATH * 3]{};
    WideToUtf8(modulePath, modulePathUtf8, ARRAYSIZE(modulePathUtf8));
    WideToUtf8(dumpPath, dumpPathUtf8, ARRAYSIZE(dumpPathUtf8));

    const char *accessOperation = "unknown";
    if (accessKind == 0)
        accessOperation = "read";
    else if (accessKind == 1)
        accessOperation = "write";
    else if (accessKind == 8)
        accessOperation = "execute";

    char accessDetails[256]{};
    if (hasAccessDetails)
    {
        StringCchPrintfA(
            accessDetails, ARRAYSIZE(accessDetails),
            "Memory operation: %s\r\nMemory address: 0x%llX\r\n",
            accessOperation, static_cast<unsigned long long>(accessAddress));
    }

    char report[8192]{};
    StringCchPrintfA(
        report, ARRAYSIZE(report),
        "SundayMaker crash report\r\n"
        "========================\r\n"
        "Time (local): %04u-%02u-%02u %02u:%02u:%02u.%03u\r\n"
        "Process ID: %lu\r\n"
        "Thread ID: %lu\r\n"
        "Exception code: 0x%08lX\r\n"
        "Reason: %s\r\n"
        "%s"
        "Exception address: 0x%llX\r\n"
        "Module: %s\r\n"
        "Module offset: 0x%llX\r\n"
        "%s"
        "Minidump: %s\r\n"
        "Minidump status: %s (error %lu)\r\n"
        "\r\n"
        "Keep this .log file together with its .dmp file when reporting "
        "the crash.\r\n",
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
        time.wSecond, time.wMilliseconds, processId, threadId, exceptionCode,
        ExceptionReason(exceptionCode), gacCppTerminateDetails,
        static_cast<unsigned long long>(exceptionAddress), modulePathUtf8,
        static_cast<unsigned long long>(
            moduleBase ? exceptionAddress - moduleBase : 0),
        accessDetails, dumpPathUtf8, dumpWritten ? "written" : "failed",
        dumpError);

    HANDLE logFile = CreateFile(logPath, GENERIC_WRITE, FILE_SHARE_READ,
                                nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                nullptr);
    if (logFile != INVALID_HANDLE_VALUE)
    {
        size_t reportSize = 0;
        StringCchLengthA(report, ARRAYSIZE(report), &reportSize);
        DWORD written = 0;
        WriteFile(logFile, report, static_cast<DWORD>(reportSize), &written,
                  nullptr);
        FlushFileBuffers(logFile);
        CloseHandle(logFile);
    }
}

LONG WINAPI UnhandledExceptionHandler(
    EXCEPTION_POINTERS *exceptionPointers) noexcept
{
    WriteCrashReport(exceptionPointers);
    return EXCEPTION_EXECUTE_HANDLER;
}

[[noreturn]] void CppTerminateHandler() noexcept
{
    try
    {
        const std::exception_ptr activeException = std::current_exception();
        if (activeException)
        {
            try
            {
                std::rethrow_exception(activeException);
            }
            catch (const std::exception &exception)
            {
                StringCchPrintfA(gacCppTerminateDetails,
                                 ARRAYSIZE(gacCppTerminateDetails),
                                 "C++ exception message: %s\r\n",
                                 exception.what());
            }
            catch (...)
            {
                StringCchCopyA(gacCppTerminateDetails,
                               ARRAYSIZE(gacCppTerminateDetails),
                               "C++ exception message: non-standard "
                               "exception\r\n");
            }
        }
        else
        {
            StringCchCopyA(gacCppTerminateDetails,
                           ARRAYSIZE(gacCppTerminateDetails),
                           "C++ exception message: no active exception\r\n");
        }
    }
    catch (...)
    {
        StringCchCopyA(gacCppTerminateDetails,
                       ARRAYSIZE(gacCppTerminateDetails),
                       "C++ exception message: unavailable\r\n");
    }

    CONTEXT context{};
    RtlCaptureContext(&context);

    EXCEPTION_RECORD record{};
    record.ExceptionCode = kCppTerminateException;
    record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
#if defined(_M_X64) || defined(__x86_64__)
    record.ExceptionAddress = reinterpret_cast<PVOID>(context.Rip);
#elif defined(_M_IX86) || defined(__i386__)
    record.ExceptionAddress = reinterpret_cast<PVOID>(context.Eip);
#endif

    EXCEPTION_POINTERS pointers{};
    pointers.ExceptionRecord = &record;
    pointers.ContextRecord = &context;
    WriteCrashReport(&pointers);
    TerminateProcess(GetCurrentProcess(), kCppTerminateException);
#ifdef _MSC_VER
    __assume(0);
#else
    __builtin_unreachable();
#endif
}
} // namespace

void CrashReportingInitialise(LPCTSTR executableDirectory) noexcept
{
    if (executableDirectory &&
        SetCrashDirectory(executableDirectory, TEXT("CrashLogs")))
    {
        // Keep portable installations self-contained.
    }
    else
    {
        TCHAR documentsPath[MAX_PATH]{};
        if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr,
                                      SHGFP_TYPE_CURRENT, documentsPath)) &&
            SetCrashDirectory(documentsPath, TEXT("SundayMaker"),
                              TEXT("CrashLogs")))
        {
            // Use a user-visible fallback when the executable directory is
            // read-only.
        }
        else
        {
            TCHAR desktopPath[MAX_PATH]{};
            if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_DESKTOPDIRECTORY,
                                          nullptr, SHGFP_TYPE_CURRENT,
                                          desktopPath)) &&
                SetCrashDirectory(desktopPath,
                                  TEXT("SundayMakerCrashLogs")))
            {
                // The desktop is the final user-visible fallback.
            }
            else
            {
                TCHAR temporaryPath[MAX_PATH]{};
                if (GetTempPath(MAX_PATH, temporaryPath) != 0)
                    SetCrashDirectory(temporaryPath,
                                      TEXT("SundayMakerCrashLogs"));
            }
        }
    }

    if (gatCrashDirectory[0] == TEXT('\0'))
        return;

    ULONG stackGuarantee = 64 * 1024;
    SetThreadStackGuarantee(&stackGuarantee);
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
    std::set_terminate(CppTerminateHandler);
}
