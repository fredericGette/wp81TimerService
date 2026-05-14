#pragma once

#include <windows.h>
#include <evntprov.h>

#define TRACE_LEVEL_NONE        0   // Tracing is not on
#define TRACE_LEVEL_CRITICAL    1   // Abnormal exit or termination
#define TRACE_LEVEL_FATAL       1   // Deprecated name for Abnormal exit or termination
#define TRACE_LEVEL_ERROR       2   // Severe errors that need logging
#define TRACE_LEVEL_WARNING     3   // Warnings such as allocation failure
#define TRACE_LEVEL_INFORMATION 4   // Includes non-error cases(e.g.,Entry-Exit)
#define TRACE_LEVEL_VERBOSE     5   // Detailed traces from intermediate steps

FORCEINLINE
VOID
EventDescCreate(
	_Out_ PEVENT_DESCRIPTOR EventDescriptor,
	_In_ USHORT Id,
	_In_ UCHAR Version,
	_In_ UCHAR Channel,
	_In_ UCHAR Level,
	_In_ USHORT Task,
	_In_ UCHAR Opcode,
	_In_ ULONGLONG Keyword
)
{
	EventDescriptor->Id = Id;
	EventDescriptor->Version = Version;
	EventDescriptor->Channel = Channel;
	EventDescriptor->Level = Level;
	EventDescriptor->Task = Task;
	EventDescriptor->Opcode = Opcode;
	EventDescriptor->Keyword = Keyword;
	return;
}

namespace EtwLogger
{
	// Initializes the ETW provider with a specific GUID.
	// Call this once at the start of your application.
	//
	// To generate a new GUID:
	// "C:\Program Files (x86)\Windows Kits\8.1\bin\x64\uuidgen.exe" -s
	void Init(const GUID& providerId);

	// Unregisters the ETW provider.
	// Call this before your application exits.
	void Cleanup();

	// Writes an event to the ETW session.
	void LogEvent(UCHAR level, const wchar_t* msg);
}