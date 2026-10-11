#pragma once

typedef struct _SERVICE_STATUS {
	DWORD   dwServiceType;
	DWORD   dwCurrentState;
	DWORD   dwControlsAccepted;
	DWORD   dwWin32ExitCode;
	DWORD   dwServiceSpecificExitCode;
	DWORD   dwCheckPoint;
	DWORD   dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;
DECLARE_HANDLE(SERVICE_STATUS_HANDLE);

typedef LONG LSTATUS;

typedef ACCESS_MASK REGSAM;
#define HKEY_LOCAL_MACHINE (( HKEY ) (ULONG_PTR)((LONG)0x80000002) )

typedef struct _STARTUPINFOW {
	DWORD   cb;
	LPWSTR  lpReserved;
	LPWSTR  lpDesktop;
	LPWSTR  lpTitle;
	DWORD   dwX;
	DWORD   dwY;
	DWORD   dwXSize;
	DWORD   dwYSize;
	DWORD   dwXCountChars;
	DWORD   dwYCountChars;
	DWORD   dwFillAttribute;
	DWORD   dwFlags;
	WORD    wShowWindow;
	WORD    cbReserved2;
	LPBYTE  lpReserved2;
	HANDLE  hStdInput;
	HANDLE  hStdOutput;
	HANDLE  hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

typedef struct _PROCESS_INFORMATION {
	HANDLE hProcess;
	HANDLE hThread;
	DWORD dwProcessId;
	DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

#define STARTF_USESHOWWINDOW       0x00000001
#define STARTF_USESTDHANDLES       0x00000100

#define SERVICE_QUERY_CONFIG           0x0001
#define SERVICE_CHANGE_CONFIG          0x0002
#define SERVICE_QUERY_STATUS           0x0004
#define SERVICE_ENUMERATE_DEPENDENTS   0x0008
#define SERVICE_START                  0x0010
#define SERVICE_STOP                   0x0020
#define SERVICE_PAUSE_CONTINUE         0x0040
#define SERVICE_INTERROGATE            0x0080
#define SERVICE_USER_DEFINED_CONTROL   0x0100

#define SERVICE_STOPPED                        0x00000001
#define SERVICE_START_PENDING                  0x00000002
#define SERVICE_STOP_PENDING                   0x00000003
#define SERVICE_RUNNING                        0x00000004
#define SERVICE_CONTINUE_PENDING               0x00000005
#define SERVICE_PAUSE_PENDING                  0x00000006
#define SERVICE_PAUSED                         0x00000007

#define SERVICE_CONTROL_STOP                   0x00000001
#define SERVICE_CONTROL_PAUSE                  0x00000002
#define SERVICE_CONTROL_CONTINUE               0x00000003
#define SERVICE_CONTROL_INTERROGATE            0x00000004
#define SERVICE_CONTROL_SHUTDOWN               0x00000005
#define SERVICE_CONTROL_PARAMCHANGE            0x00000006
#define SERVICE_CONTROL_NETBINDADD             0x00000007
#define SERVICE_CONTROL_NETBINDREMOVE          0x00000008
#define SERVICE_CONTROL_NETBINDENABLE          0x00000009
#define SERVICE_CONTROL_NETBINDDISABLE         0x0000000A
#define SERVICE_CONTROL_DEVICEEVENT            0x0000000B
#define SERVICE_CONTROL_HARDWAREPROFILECHANGE  0x0000000C
#define SERVICE_CONTROL_POWEREVENT             0x0000000D
#define SERVICE_CONTROL_SESSIONCHANGE          0x0000000E
#define SERVICE_CONTROL_PRESHUTDOWN            0x0000000F
#define SERVICE_CONTROL_TIMECHANGE             0x00000010
#define SERVICE_CONTROL_TRIGGEREVENT           0x00000020

#define SERVICE_ACCEPT_STOP                    0x00000001
#define SERVICE_ACCEPT_PAUSE_CONTINUE          0x00000002
#define SERVICE_ACCEPT_SHUTDOWN                0x00000004
#define SERVICE_ACCEPT_PARAMCHANGE             0x00000008
#define SERVICE_ACCEPT_NETBINDCHANGE           0x00000010
#define SERVICE_ACCEPT_HARDWAREPROFILECHANGE   0x00000020
#define SERVICE_ACCEPT_POWEREVENT              0x00000040
#define SERVICE_ACCEPT_SESSIONCHANGE           0x00000080
#define SERVICE_ACCEPT_PRESHUTDOWN             0x00000100
#define SERVICE_ACCEPT_TIMECHANGE              0x00000200
#define SERVICE_ACCEPT_TRIGGEREVENT            0x00000400

#define SERVICE_CONFIG_DESCRIPTION              1
#define SERVICE_CONFIG_FAILURE_ACTIONS          2
#define SERVICE_CONFIG_DELAYED_AUTO_START_INFO  3
#define SERVICE_CONFIG_FAILURE_ACTIONS_FLAG     4
#define SERVICE_CONFIG_SERVICE_SID_INFO         5
#define SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO 6
#define SERVICE_CONFIG_PRESHUTDOWN_INFO         7
#define SERVICE_CONFIG_TRIGGER_INFO             8
#define SERVICE_CONFIG_PREFERRED_NODE           9
// reserved                                     10
// reserved                                     11
#define SERVICE_CONFIG_LAUNCH_PROTECTED         12

#define SC_MANAGER_CONNECT             0x0001
#define SC_MANAGER_CREATE_SERVICE      0x0002
#define SC_MANAGER_ENUMERATE_SERVICE   0x0004
#define SC_MANAGER_LOCK                0x0008
#define SC_MANAGER_QUERY_LOCK_STATUS   0x0010
#define SC_MANAGER_MODIFY_BOOT_CONFIG  0x0020

#define SC_MANAGER_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED      | \
                                        SC_MANAGER_CONNECT            | \
                                        SC_MANAGER_CREATE_SERVICE     | \
                                        SC_MANAGER_ENUMERATE_SERVICE  | \
                                        SC_MANAGER_LOCK               | \
                                        SC_MANAGER_QUERY_LOCK_STATUS  | \
                                        SC_MANAGER_MODIFY_BOOT_CONFIG)

#define SERVICE_QUERY_CONFIG           0x0001
#define SERVICE_CHANGE_CONFIG          0x0002
#define SERVICE_QUERY_STATUS           0x0004
#define SERVICE_ENUMERATE_DEPENDENTS   0x0008
#define SERVICE_START                  0x0010
#define SERVICE_STOP                   0x0020
#define SERVICE_PAUSE_CONTINUE         0x0040
#define SERVICE_INTERROGATE            0x0080
#define SERVICE_USER_DEFINED_CONTROL   0x0100

#define SERVICE_ALL_ACCESS             (STANDARD_RIGHTS_REQUIRED     | \
                                        SERVICE_QUERY_CONFIG         | \
                                        SERVICE_CHANGE_CONFIG        | \
                                        SERVICE_QUERY_STATUS         | \
                                        SERVICE_ENUMERATE_DEPENDENTS | \
                                        SERVICE_START                | \
                                        SERVICE_STOP                 | \
                                        SERVICE_PAUSE_CONTINUE       | \
                                        SERVICE_INTERROGATE          | \
                                        SERVICE_USER_DEFINED_CONTROL)

DECLARE_HANDLE(SC_HANDLE);

#define HANDLE_FLAG_INHERIT 0x00000001

typedef VOID(WINAPI *LPSERVICE_MAIN_FUNCTIONW)(
	DWORD   dwNumServicesArgs,
	LPWSTR  *lpServiceArgVectors
	);

typedef struct _SERVICE_TABLE_ENTRYW {
	LPWSTR                      lpServiceName;
	LPSERVICE_MAIN_FUNCTIONW    lpServiceProc;
}SERVICE_TABLE_ENTRYW, *LPSERVICE_TABLE_ENTRYW;

typedef struct _SERVICE_DESCRIPTIONW {
	LPWSTR      lpDescription;
} SERVICE_DESCRIPTIONW, *LPSERVICE_DESCRIPTIONW;

typedef VOID(WINAPI *LPHANDLER_FUNCTION)(
	DWORD    dwControl
	);

typedef DWORD(WINAPI *LPHANDLER_FUNCTION_EX)(
	DWORD   dwControl,
	DWORD   dwEventType,
	LPVOID  lpEventData,
	LPVOID  lpContext
	);

extern "C" {
	WINBASEAPI HMODULE WINAPI LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
	WINBASEAPI HMODULE WINAPI GetModuleHandleW(LPCWSTR lpModuleName);
	WINADVAPI LSTATUS APIENTRY RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
	WINADVAPI LSTATUS APIENTRY RegQueryValueExW(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
	WINADVAPI LSTATUS APIENTRY RegCloseKey(HKEY hKey);
	WINBASEAPI BOOL	WINAPI CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	WINBASEAPI DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
	WINBASEAPI BOOL WINAPI TerminateProcess(HANDLE hProcess, UINT uExitCode);
	WINADVAPI BOOL WINAPI SetServiceStatus(SERVICE_STATUS_HANDLE hServiceStatus, LPSERVICE_STATUS lpServiceStatus);
	WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerW(LPCWSTR lpServiceName, LPHANDLER_FUNCTION lpHandlerProc);
	WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExW(LPCWSTR lpServiceName, LPHANDLER_FUNCTION_EX lpHandlerProc, LPVOID lpContext);
	WINBASEAPI HANDLE WINAPI CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
	WINBASEAPI DWORD WINAPI GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
	WINADVAPI SC_HANDLE WINAPI OpenSCManagerW(LPCWSTR lpMachineName, LPCWSTR lpDatabaseName, DWORD dwDesiredAccess);
	WINADVAPI SC_HANDLE WINAPI CreateServiceW(SC_HANDLE hSCManager, LPCWSTR lpServiceName, LPCWSTR lpDisplayName, DWORD dwDesiredAccess, DWORD dwServiceType, DWORD dwStartType, DWORD dwErrorControl, LPCWSTR lpBinaryPathName, LPCWSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCWSTR lpDependencies, LPCWSTR lpServiceStartName, LPCWSTR lpPassword);
	WINADVAPI BOOL WINAPI CloseServiceHandle(SC_HANDLE hSCObject);
	WINADVAPI BOOL WINAPI ChangeServiceConfig2W(SC_HANDLE hService, DWORD dwInfoLevel, LPVOID lpInfo);
	WINADVAPI SC_HANDLE WINAPI OpenServiceW(SC_HANDLE hSCManager, LPCWSTR lpServiceName, DWORD dwDesiredAccess);
	WINADVAPI BOOL WINAPI ControlService(SC_HANDLE hService, DWORD dwControl, LPSERVICE_STATUS lpServiceStatus);
	WINADVAPI BOOL WINAPI DeleteService(SC_HANDLE hService);
	WINADVAPI BOOL WINAPI StartServiceCtrlDispatcherW(SERVICE_TABLE_ENTRYW *lpServiceStartTable);
	WINBASEAPI VOID WINAPI Sleep(DWORD dwMilliseconds);
	WINBASEAPI HANDLE WINAPI CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
	WINBASEAPI BOOL WINAPI GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode);
	WINBASEAPI BOOL WINAPI CreatePipe(PHANDLE hReadPipe, PHANDLE hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize);
	WINBASEAPI BOOL WINAPI SetHandleInformation(HANDLE hObject, DWORD dwMask, DWORD dwFlags);
	WINBASEAPI BOOL WINAPI PeekNamedPipe(HANDLE hNamedPipe, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesRead, LPDWORD lpTotalBytesAvail, LPDWORD lpBytesLeftThisMessage);
}

#define WIN32API_TOSTRING(x) #x

// Link exported function
#define WIN32API_INIT_PROC(Module, Name)  \
  Name(reinterpret_cast<decltype(&::Name)>( \
      ::GetProcAddress((Module), WIN32API_TOSTRING(Name))))

// Convenientmacro to declare function
#define WIN32API_DEFINE_PROC(Name) const decltype(&::Name) Name

class Win32Api {

private:
	// Returns a base address of KernelBase.dll
	static HMODULE GetKernelBase() {
		return GetBaseAddress(reinterpret_cast<void*>(&::DisableThreadLibraryCalls));
	}

	// Returns a base address of the given address
	static HMODULE GetBaseAddress(const void *Address) {
		MEMORY_BASIC_INFORMATION mbi = {};
		if (!::VirtualQuery(Address, &mbi, sizeof(mbi))) {
			return nullptr;
		}
		const auto mz = *reinterpret_cast<WORD *>(mbi.AllocationBase);
		if (mz != IMAGE_DOS_SIGNATURE) {
			return nullptr;
		}
		return reinterpret_cast<HMODULE>(mbi.AllocationBase);
	}

public:
	const HMODULE m_Kernelbase;
	WIN32API_DEFINE_PROC(LoadLibraryExW);
	WIN32API_DEFINE_PROC(GetModuleHandleW);
	WIN32API_DEFINE_PROC(RegOpenKeyExW);
	WIN32API_DEFINE_PROC(RegQueryValueExW);
	WIN32API_DEFINE_PROC(RegCloseKey);
	WIN32API_DEFINE_PROC(CreateProcessW);
	WIN32API_DEFINE_PROC(WaitForSingleObject);
	WIN32API_DEFINE_PROC(TerminateProcess);
	WIN32API_DEFINE_PROC(CreateEventW);
	WIN32API_DEFINE_PROC(GetModuleFileNameW);
	WIN32API_DEFINE_PROC(Sleep);
	WIN32API_DEFINE_PROC(PeekNamedPipe);
	const HMODULE m_SecHost;
	WIN32API_DEFINE_PROC(SetServiceStatus);
	WIN32API_DEFINE_PROC(RegisterServiceCtrlHandlerW);
	WIN32API_DEFINE_PROC(RegisterServiceCtrlHandlerExW);
	WIN32API_DEFINE_PROC(OpenSCManagerW);
	WIN32API_DEFINE_PROC(CreateServiceW);
	WIN32API_DEFINE_PROC(CloseServiceHandle);
	WIN32API_DEFINE_PROC(ChangeServiceConfig2W);
	WIN32API_DEFINE_PROC(OpenServiceW);
	WIN32API_DEFINE_PROC(ControlService);
	WIN32API_DEFINE_PROC(DeleteService);
	WIN32API_DEFINE_PROC(StartServiceCtrlDispatcherW);

	Win32Api()
		: m_Kernelbase(GetKernelBase()),
		WIN32API_INIT_PROC(m_Kernelbase, LoadLibraryExW),
		WIN32API_INIT_PROC(m_Kernelbase, GetModuleHandleW),
		WIN32API_INIT_PROC(m_Kernelbase, RegOpenKeyExW),
		WIN32API_INIT_PROC(m_Kernelbase, RegQueryValueExW),
		WIN32API_INIT_PROC(m_Kernelbase, RegCloseKey),
		WIN32API_INIT_PROC(m_Kernelbase, CreateProcessW),
		WIN32API_INIT_PROC(m_Kernelbase, WaitForSingleObject),
		WIN32API_INIT_PROC(m_Kernelbase, TerminateProcess),
		WIN32API_INIT_PROC(m_Kernelbase, CreateEventW),
		WIN32API_INIT_PROC(m_Kernelbase, GetModuleFileNameW),
		WIN32API_INIT_PROC(m_Kernelbase, Sleep),
		WIN32API_INIT_PROC(m_Kernelbase, PeekNamedPipe),
		m_SecHost(LoadLibraryExW(L"SECHOST.dll", NULL, NULL)),
		WIN32API_INIT_PROC(m_SecHost, SetServiceStatus),
		WIN32API_INIT_PROC(m_SecHost, RegisterServiceCtrlHandlerW),
		WIN32API_INIT_PROC(m_SecHost, RegisterServiceCtrlHandlerExW),
		WIN32API_INIT_PROC(m_SecHost, OpenSCManagerW),
		WIN32API_INIT_PROC(m_SecHost, CreateServiceW),
		WIN32API_INIT_PROC(m_SecHost, CloseServiceHandle),
		WIN32API_INIT_PROC(m_SecHost, ChangeServiceConfig2W),
		WIN32API_INIT_PROC(m_SecHost, OpenServiceW),
		WIN32API_INIT_PROC(m_SecHost, ControlService),
		WIN32API_INIT_PROC(m_SecHost, DeleteService),
		WIN32API_INIT_PROC(m_SecHost, StartServiceCtrlDispatcherW)

	{};

};