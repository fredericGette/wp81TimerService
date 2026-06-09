/**
 * Context: 
 * win32 console application, 
 * linked with Msvcr110.dll, 
 * architecture ARM 32bit little endian. 
 * Use secure functions when possible, example: _snprintf_s instead of _snprintf
 */

#include <windows.h>
#include <winsvc.h>
#include <strsafe.h>
#include <stdio.h>
#include <string>
#include "Win32Api.h"

#define TRACE_LEVEL_NONE        0   // Tracing is not on
#define TRACE_LEVEL_CRITICAL    1   // Abnormal exit or termination
#define TRACE_LEVEL_FATAL       1   // Deprecated name for Abnormal exit or termination
#define TRACE_LEVEL_ERROR       2   // Severe errors that need logging
#define TRACE_LEVEL_WARNING     3   // Warnings such as allocation failure
#define TRACE_LEVEL_INFORMATION 4   // Includes non-error cases(e.g.,Entry-Exit)
#define TRACE_LEVEL_VERBOSE     5   // Detailed traces from intermediate steps

// ─── Configuration ────────────────────────────────────────────────────────────
static const wchar_t* SERVICE_NAME    = L"wp81TimerService";
static const wchar_t* SERVICE_DISPLAY = L"Timer Task Service";
static const wchar_t* SERVICE_DESC    = L"Executes a configured command every 5 minutes.";
static const wchar_t* TASK_COMMAND        = L"ping -n 1 -w 1000 192.168.1.1"; // <-- change this
static const DWORD    DEFAULT_INTERVAL_MS = 5 * 60 * 1000;   // 5 minutes
static const DWORD    MIN_INTERVAL_MS     = 1 * 60 * 1000;   // 1 minute
// ─────────────────────────────────────────────────────────────────────────────

static DWORD g_IntervalMs = DEFAULT_INTERVAL_MS;

static SERVICE_STATUS        g_Status       = {};
static SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
static HANDLE                g_StopEvent    = nullptr;

static Win32Api api;

// ─── FileLogger ───────────────────────────────────────────────────────────────
namespace FileLogger
{
    static wchar_t s_logPath[MAX_PATH] = {};   // empty = logging disabled

    // Read LogFile from registry; call once at service start.
    void Init()
    {
        wchar_t keyPath[256];
        StringCchPrintfW(keyPath, _countof(keyPath),
            L"SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters", SERVICE_NAME);

        HKEY hKey;
        if (api.RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return;

        DWORD type = REG_SZ;
        DWORD size = sizeof(s_logPath);
        api.RegQueryValueExW(hKey, L"LogFile", nullptr, &type,
            reinterpret_cast<LPBYTE>(s_logPath), &size);
        api.RegCloseKey(hKey);
    }

    static const wchar_t* LevelTag(UCHAR level)
    {
        switch (level)
        {
        case 1:  return L"CRIT ";
        case 2:  return L"ERROR";
        case 3:  return L"WARN ";
        case 4:  return L"INFO ";
        case 5:  return L"VERB ";
        default: return L"?    ";
        }
    }

    void LogEvent(UCHAR level, const wchar_t* message)
    {
        if (s_logPath[0] == L'\0') return;   // logging not configured

        SYSTEMTIME st;
        GetLocalTime(&st);

        wchar_t line[1280];
        StringCchPrintfW(line, _countof(line),
            L"%04d-%02d-%02d %02d:%02d [%s] %s\r\n",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute,
            LevelTag(level),
            message);

        int needed = WideCharToMultiByte(CP_UTF8, 0, line, -1, nullptr, 0, nullptr, nullptr);
        if (needed <= 0) return;

        char* buf = new char[needed];
        WideCharToMultiByte(CP_UTF8, 0, line, -1, buf, needed, nullptr, nullptr);
        int byteCount = needed - 1;

        HANDLE hFile = CreateFileW(
            s_logPath,
            FILE_APPEND_DATA,
            FILE_SHARE_READ,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD written;
            WriteFile(hFile, buf, static_cast<DWORD>(byteCount), &written, nullptr);
            CloseHandle(hFile);
        }

        delete[] buf;
    }

    void LogEventF(UCHAR level, const wchar_t* fmt, ...)
    {
        if (s_logPath[0] == L'\0') return;   // logging not configured

        wchar_t msg[1024];
        va_list args;
        va_start(args, fmt);
        _vsnwprintf_s(msg, _countof(msg), _TRUNCATE, fmt, args);
        va_end(args);
        LogEvent(level, msg);
    }
}
// ─────────────────────────────────────────────────────────────────────────────

// ── Read optional interval override from registry ────────────────────────────
// Returns interval in ms; falls back to DEFAULT_INTERVAL_MS, minimum MIN_INTERVAL_MS.
static DWORD GetIntervalFromRegistry()
{
    wchar_t keyPath[256];
    StringCchPrintfW(keyPath, _countof(keyPath),
        L"SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters", SERVICE_NAME);

    HKEY hKey;
    if (api.RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return DEFAULT_INTERVAL_MS;

    DWORD minutes = 0;
    DWORD type    = REG_DWORD;
    DWORD size    = sizeof(minutes);
    bool  ok      = (api.RegQueryValueExW(hKey, L"IntervalMinutes", nullptr, &type,
                         reinterpret_cast<LPBYTE>(&minutes), &size) == ERROR_SUCCESS);
    api.RegCloseKey(hKey);

    if (!ok || minutes == 0)
        return DEFAULT_INTERVAL_MS;

    DWORD ms = minutes * 60 * 1000;
    return ms < MIN_INTERVAL_MS ? MIN_INTERVAL_MS : ms;
}

// ── Read optional override from registry ─────────────────────────────────────
bool GetTaskCommandFromRegistry(wchar_t* buf, DWORD bufChars)
{
    wchar_t keyPath[256];
    StringCchPrintfW(keyPath, 256,
        L"SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters", SERVICE_NAME);

    HKEY hKey;
    if (api.RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;

    DWORD type = REG_SZ;
    DWORD size = bufChars * sizeof(wchar_t);
    bool  ok   = (api.RegQueryValueExW(hKey, L"TaskCommand", nullptr, &type,
                      reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS);
    api.RegCloseKey(hKey);
    return ok;
}

// ── Helper: read all data from a pipe handle into a wstring ──────────────────
static std::wstring ReadPipeToWString(HANDLE hPipe)
{
    std::wstring result;
    char         buf[4096];
    DWORD        bytesRead = 0;

    while (ReadFile(hPipe, buf, sizeof(buf), &bytesRead, nullptr) && bytesRead > 0)
    {
        // The child writes bytes; convert from current ANSI/UTF-8 to wide.
        int wlen = MultiByteToWideChar(CP_ACP, 0, buf, (int)bytesRead, nullptr, 0);
        if (wlen > 0)
        {
            std::wstring chunk((size_t)wlen, L'\0');
            MultiByteToWideChar(CP_ACP, 0, buf, (int)bytesRead, &chunk[0], wlen);
            result += chunk;
        }
    }

    return result;
}

// ── Helper: log each non-empty line from a captured output block ──────────────
static void LogOutputLines(BYTE traceLevel, const wchar_t* prefix, const std::wstring& text)
{
    if (text.empty())
        return;

    std::wstring line;
    for (size_t i = 0; i < text.size(); ++i)
    {
        wchar_t c = text[i];
        if (c == L'\r')
            continue;                          // skip CR in CRLF pairs
        if (c == L'\n')
        {
            if (!line.empty())
            {
                FileLogger::LogEventF(traceLevel, L"%s%s", prefix, line.c_str());
                line.clear();
            }
        }
        else
        {
            line += c;
        }
    }
    if (!line.empty())                         // flush last line (no trailing newline)
        FileLogger::LogEventF(traceLevel, L"%s%s", prefix, line.c_str());
}

// ── Run the task using CreateProcess ─────────────────────────────────────────
void RunTask()
{
    wchar_t command[1024] = {};

    if (!GetTaskCommandFromRegistry(command, _countof(command)))
        StringCchCopyW(command, _countof(command), TASK_COMMAND);

    FileLogger::LogEventF(TRACE_LEVEL_INFORMATION, L"Launching: %s", command);

    // ── Create pipes for stdout and stderr ───────────────────────────────────
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength              = sizeof(sa);
    sa.bInheritHandle       = TRUE;   // pipe ends must be inheritable
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hStdoutRd = nullptr, hStdoutWr = nullptr;
    HANDLE hStderrRd = nullptr, hStderrWr = nullptr;

    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0) ||
        !CreatePipe(&hStderrRd, &hStderrWr, &sa, 0))
    {
        FileLogger::LogEventF(TRACE_LEVEL_ERROR,
            L"CreatePipe failed. Error: %lu", GetLastError());
        // Close whatever got created before returning
        if (hStdoutRd) CloseHandle(hStdoutRd);
        if (hStdoutWr) CloseHandle(hStdoutWr);
        if (hStderrRd) CloseHandle(hStderrRd);
        if (hStderrWr) CloseHandle(hStderrWr);
        return;
    }

    // The read ends must NOT be inherited by the child.
    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStderrRd, HANDLE_FLAG_INHERIT, 0);

    // ── Spawn the child ──────────────────────────────────────────────────────
    STARTUPINFOW        si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput  = hStdoutWr;
    si.hStdError   = hStderrWr;
    si.hStdInput   = nullptr;          // child inherits no stdin

    BOOL launched = api.CreateProcessW(
        nullptr,
        command,
        nullptr, nullptr,
        TRUE,              // inherit handles so the pipe write ends reach the child
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi);

    // Close the write ends in THIS process immediately after CreateProcess.
    // If we keep them open, ReadFile on the read end will never see EOF.
    CloseHandle(hStdoutWr); hStdoutWr = nullptr;
    CloseHandle(hStderrWr); hStderrWr = nullptr;

    if (!launched)
    {
        FileLogger::LogEventF(TRACE_LEVEL_ERROR,
            L"CreateProcess failed for '%s'. Error: %lu", command, GetLastError());
        CloseHandle(hStdoutRd);
        CloseHandle(hStderrRd);
        return;
    }

    // ── Wait for the process ─────────────────────────────────────────────────
    DWORD processTimeout = g_IntervalMs > 60000 ? g_IntervalMs - 60000 : g_IntervalMs / 2;
    DWORD waitResult = api.WaitForSingleObject(pi.hProcess, processTimeout);
    if (waitResult == WAIT_TIMEOUT)
    {
        FileLogger::LogEvent(TRACE_LEVEL_WARNING, L"Task exceeded time limit - terminating process.");
        api.TerminateProcess(pi.hProcess, 1);
    }
    else
    {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode))
            FileLogger::LogEventF(exitCode == 0 ? TRACE_LEVEL_INFORMATION : TRACE_LEVEL_WARNING,
                L"Task exited with code %lu.", exitCode);
        else
            FileLogger::LogEventF(TRACE_LEVEL_WARNING,
                L"Task finished but GetExitCodeProcess failed. Error: %lu", GetLastError());
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // ── Drain and log captured output ────────────────────────────────────────
    // The child has exited (or been killed), so both pipe write ends are closed
    // and ReadFile will return all buffered data then signal EOF cleanly.
    std::wstring stdoutText = ReadPipeToWString(hStdoutRd);
    std::wstring stderrText = ReadPipeToWString(hStderrRd);

    CloseHandle(hStdoutRd);
    CloseHandle(hStderrRd);

    LogOutputLines(TRACE_LEVEL_INFORMATION, L"[stdout] ", stdoutText);
    LogOutputLines(TRACE_LEVEL_ERROR,       L"[stderr] ", stderrText);
}

// ── Report service status helper ──────────────────────────────────────────────
void ReportStatus(DWORD currentState, DWORD waitHint = 0)
{
    static DWORD checkPoint = 1;
    g_Status.dwCurrentState = currentState;
    g_Status.dwWaitHint     = waitHint;
    g_Status.dwCheckPoint   = (currentState == SERVICE_RUNNING ||
                                currentState == SERVICE_STOPPED) ? 0 : checkPoint++;
    api.SetServiceStatus(g_StatusHandle, &g_Status);
}

// ── Service control handler ───────────────────────────────────────────────────
DWORD WINAPI ServiceCtrlHandlerEx(DWORD ctrl, DWORD eventType, LPVOID /*eventData*/, LPVOID /*ctx*/)
{
    if (ctrl == SERVICE_CONTROL_STOP || ctrl == SERVICE_CONTROL_SHUTDOWN)
    {
        FileLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"Service stop requested.");
        ReportStatus(SERVICE_STOP_PENDING, 5000);
        SetEvent(g_StopEvent);
    }
    else if (ctrl == SERVICE_CONTROL_POWEREVENT)
    {
        if (eventType == PBT_APMSUSPEND)
            FileLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"System going to sleep.");
        else if (eventType == PBT_APMRESUMEAUTOMATIC || eventType == PBT_APMRESUMESUSPEND)
            FileLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"System resumed from sleep.");
    }
    return NO_ERROR;
}

// ── Main service logic ────────────────────────────────────────────────────────
VOID WINAPI ServiceMain(DWORD /*argc*/, LPWSTR* /*argv*/)
{
    g_StatusHandle = api.RegisterServiceCtrlHandlerExW(SERVICE_NAME, ServiceCtrlHandlerEx, nullptr);
    if (!g_StatusHandle) return;

    FileLogger::Init();
    g_IntervalMs = GetIntervalFromRegistry();

    g_Status.dwServiceType     = SERVICE_WIN32_OWN_PROCESS;
    g_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_POWEREVENT;
    g_Status.dwCurrentState    = SERVICE_START_PENDING;
    api.SetServiceStatus(g_StatusHandle, &g_Status);

    g_StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_StopEvent)
    {
        ReportStatus(SERVICE_STOPPED);
        return;
    }

    ReportStatus(SERVICE_RUNNING);
    FileLogger::LogEventF(TRACE_LEVEL_INFORMATION, L"Service started. Timer interval: %lu minutes.", g_IntervalMs / 60000);

    // Run immediately on start, then every INTERVAL_MS
    RunTask();

    while (true)
    {
        DWORD result = api.WaitForSingleObject(g_StopEvent, g_IntervalMs);
        if (result == WAIT_OBJECT_0)
            break;   // stop signal received
        RunTask();
    }

    CloseHandle(g_StopEvent);
    FileLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"Service stopped.");
    ReportStatus(SERVICE_STOPPED);
}

// ── Install / Uninstall helpers ───────────────────────────────────────────────
bool InstallService()
{
    wchar_t path[MAX_PATH];
    if (!api.GetModuleFileNameW(nullptr, path, MAX_PATH))
    {
        wprintf(L"GetModuleFileName failed: %lu\n", GetLastError());
        return false;
    }

    SC_HANDLE hSCM = api.OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!hSCM) { wprintf(L"OpenSCManager failed: %lu\n", GetLastError()); return false; }

    SC_HANDLE hSvc = api.CreateServiceW(
        hSCM, SERVICE_NAME, SERVICE_DISPLAY,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        path, nullptr, nullptr, nullptr, nullptr, nullptr);

    if (!hSvc)
    {
        wprintf(L"CreateService failed: %lu\n", GetLastError());
        api.CloseServiceHandle(hSCM);
        return false;
    }

    // Set description
    SERVICE_DESCRIPTIONW sd = { const_cast<wchar_t*>(SERVICE_DESC) };
    api.ChangeServiceConfig2W(hSvc, SERVICE_CONFIG_DESCRIPTION, &sd);

    wprintf(L"Service installed successfully.\n");
    wprintf(L"To set the task command via registry:\n");
    wprintf(L"  reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters\" "
            L"/v TaskCommand /t REG_SZ /d \"C:\\path\\to\\your\\script.bat\" /f\n", SERVICE_NAME);
    wprintf(L"To set the timer interval (default 5 min, minimum 1 min):\n");
    wprintf(L"  reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters\" "
            L"/v IntervalMinutes /t REG_DWORD /d 10 /f\n", SERVICE_NAME);
    wprintf(L"To enable file logging (disabled by default):\n");
    wprintf(L"  reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters\" "
            L"/v LogFile /t REG_SZ /d \"C:\\path\\to\\logfile.log\" /f\n", SERVICE_NAME);

    api.CloseServiceHandle(hSvc);
    api.CloseServiceHandle(hSCM);
    return true;
}

bool UninstallService()
{
    SC_HANDLE hSCM = api.OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) { wprintf(L"OpenSCManager failed: %lu\n", GetLastError()); return false; }

    SC_HANDLE hSvc = api.OpenServiceW(hSCM, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (!hSvc)
    {
        wprintf(L"OpenService failed: %lu\n", GetLastError());
        api.CloseServiceHandle(hSCM);
        return false;
    }

    SERVICE_STATUS ss;
    api.ControlService(hSvc, SERVICE_CONTROL_STOP, &ss); // try to stop first
    Sleep(1000);

    if (!api.DeleteService(hSvc))
        wprintf(L"DeleteService failed: %lu\n", GetLastError());
    else
        wprintf(L"Service uninstalled successfully.\n");

    api.CloseServiceHandle(hSvc);
    api.CloseServiceHandle(hSCM);
    return true;
}

// ── Entry point ───────────────────────────────────────────────────────────────
int wmain(int argc, wchar_t* argv[])
{
    if (argc > 1)
    {
        if (_wcsicmp(argv[1], L"install") == 0)   return InstallService()   ? 0 : 1;
        if (_wcsicmp(argv[1], L"uninstall") == 0) return UninstallService() ? 0 : 1;
        wprintf(L"Unknown argument: %s\n", argv[1]);
        return 1;
    }

    // No arguments → hand control to the SCM
    SERVICE_TABLE_ENTRYW table[] = {
        { const_cast<wchar_t*>(SERVICE_NAME), ServiceMain },
        { nullptr, nullptr }
    };
    api.StartServiceCtrlDispatcherW(table);
    return 0;
}