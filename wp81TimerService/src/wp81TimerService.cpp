#include <windows.h>
#include <winsvc.h>
#include <strsafe.h>
#include <stdio.h>
#include "Win32Api.h"
#include "EtwLogger.h"

// ─── Configuration ────────────────────────────────────────────────────────────
static const wchar_t* SERVICE_NAME = L"wp81TimerService";
static const wchar_t* SERVICE_DISPLAY = L"Timer Task Service";
static const wchar_t* SERVICE_DESC = L"Executes a configured command every 5 minutes.";
static const wchar_t* TASK_COMMAND = L"C:\\Scripts\\mytask.bat"; // <-- change this
static const DWORD    INTERVAL_MS = 5 * 60 * 1000;             // 5 minutes
// ─────────────────────────────────────────────────────────────────────────────

static SERVICE_STATUS          g_Status = {};
static SERVICE_STATUS_HANDLE   g_StatusHandle = nullptr;
static HANDLE                  g_StopEvent = nullptr;

static Win32Api api;

static const GUID kProviderGuid = { /* 1f5d7938-0031-43cc-a059-1f691396d6de */
	0x1f5d7938,
	0x0031,
	0x43cc,
	{ 0xa0, 0x59, 0x1f, 0x69, 0x13, 0x96, 0xd6, 0xde }
};


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
	bool  ok = (api.RegQueryValueExW(hKey, L"TaskCommand", nullptr, &type,
		reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS);
	api.RegCloseKey(hKey);
	return ok;
}

// ── Run the task using CreateProcess ─────────────────────────────────────────
void RunTask()
{
	wchar_t command[1024] = {};

	if (!GetTaskCommandFromRegistry(command, _countof(command)))
		StringCchCopyW(command, _countof(command), TASK_COMMAND);

	wchar_t logMsg[1100];
	StringCchPrintfW(logMsg, _countof(logMsg), L"Launching: %s", command);
	EtwLogger::LogEvent(TRACE_LEVEL_INFORMATION, logMsg);

	STARTUPINFOW        si = { sizeof(si) };
	PROCESS_INFORMATION pi = {};

	// Run hidden, no window
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	if (!api.CreateProcessW(
		nullptr,   // no module name – use command line
		command,   // command line (may be modified by CreateProcess)
		nullptr, nullptr,
		FALSE,
		CREATE_NO_WINDOW,
		nullptr, nullptr,
		&si, &pi))
	{
		wchar_t err[256];
		StringCchPrintfW(err, _countof(err),
			L"CreateProcess failed for '%s'. Error: %lu", command, GetLastError());
		EtwLogger::LogEvent(TRACE_LEVEL_ERROR, err);
		return;
	}

	// Wait up to 4 minutes for the process; kill it if it hangs.
	DWORD waitResult = api.WaitForSingleObject(pi.hProcess, INTERVAL_MS - 60000);
	if (waitResult == WAIT_TIMEOUT) {
		EtwLogger::LogEvent(TRACE_LEVEL_WARNING, L"Task exceeded time limit – terminating process.");
		api.TerminateProcess(pi.hProcess, 1);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

// ── Report service status helper ──────────────────────────────────────────────
void ReportStatus(DWORD currentState, DWORD waitHint = 0)
{
	static DWORD checkPoint = 1;
	g_Status.dwCurrentState = currentState;
	g_Status.dwWaitHint = waitHint;
	g_Status.dwCheckPoint = (currentState == SERVICE_RUNNING ||
		currentState == SERVICE_STOPPED) ? 0 : checkPoint++;
	api.SetServiceStatus(g_StatusHandle, &g_Status);
}

// ── Service control handler ───────────────────────────────────────────────────
DWORD WINAPI ServiceCtrlHandlerEx(DWORD ctrl, DWORD eventType, LPVOID /*eventData*/, LPVOID /*ctx*/)
{
	if (ctrl == SERVICE_CONTROL_STOP || ctrl == SERVICE_CONTROL_SHUTDOWN) {
		EtwLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"Service stop requested.");
		ReportStatus(SERVICE_STOP_PENDING, 5000);
		SetEvent(g_StopEvent);
	}
	else if (ctrl == SERVICE_CONTROL_POWEREVENT) {
		if (eventType == PBT_APMSUSPEND)
			EtwLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"System going to sleep.");
		else if (eventType == PBT_APMRESUMEAUTOMATIC || eventType == PBT_APMRESUMESUSPEND)
			EtwLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"System resumed from sleep.");
	}
	return NO_ERROR;
}

// ── Main service logic ────────────────────────────────────────────────────────
VOID WINAPI ServiceMain(DWORD /*argc*/, LPWSTR* /*argv*/)
{
	EtwLogger::Init(kProviderGuid);

	g_StatusHandle = api.RegisterServiceCtrlHandlerExW(SERVICE_NAME, ServiceCtrlHandlerEx, nullptr);
	if (!g_StatusHandle) return;

	g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_POWEREVENT;
	g_Status.dwCurrentState = SERVICE_START_PENDING;
	api.SetServiceStatus(g_StatusHandle, &g_Status);

	g_StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!g_StopEvent) {
		ReportStatus(SERVICE_STOPPED);
		EtwLogger::Cleanup();
		return;
	}

	ReportStatus(SERVICE_RUNNING);
	EtwLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"Service started. Timer interval: 5 minutes.");

	// Run immediately on start, then every INTERVAL_MS
	RunTask();

	while (true) {
		DWORD result = api.WaitForSingleObject(g_StopEvent, INTERVAL_MS);
		if (result == WAIT_OBJECT_0)
			break;   // stop signal received
		RunTask();
	}

	CloseHandle(g_StopEvent);
	EtwLogger::LogEvent(TRACE_LEVEL_INFORMATION, L"Service stopped.");
	ReportStatus(SERVICE_STOPPED);

	EtwLogger::Cleanup();
}

// ── Install / Uninstall helpers ───────────────────────────────────────────────
bool InstallService()
{
	wchar_t path[MAX_PATH];
	if (!api.GetModuleFileNameW(nullptr, path, MAX_PATH)) {
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

	if (!hSvc) {
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

	api.CloseServiceHandle(hSvc);
	api.CloseServiceHandle(hSCM);
	return true;
}

bool UninstallService()
{
	SC_HANDLE hSCM = api.OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hSCM) { wprintf(L"OpenSCManager failed: %lu\n", GetLastError()); return false; }

	SC_HANDLE hSvc = api.OpenServiceW(hSCM, SERVICE_NAME, SERVICE_STOP | DELETE);
	if (!hSvc) {
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
	if (argc > 1) {
		if (_wcsicmp(argv[1], L"install") == 0)   return InstallService() ? 0 : 1;
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