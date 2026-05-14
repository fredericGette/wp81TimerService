#include "EtwLogger.h"

namespace EtwLogger
{
	// Hidden inside the namespace and cpp file, preventing global scope pollution
	static REGHANDLE g_hProvider = 0;

	void Init(const GUID& providerId)
	{
		// Only register if we haven't already
		if (g_hProvider == 0) {
			EventRegister(&providerId, nullptr, nullptr, &g_hProvider);
		}
	}

	void Cleanup()
	{
		if (g_hProvider) {
			EventUnregister(g_hProvider);
			g_hProvider = 0;
		}
	}

	void LogEvent(UCHAR level, const wchar_t* msg)
	{
		// Fail gracefully if not initialized or if message is null
		if (!g_hProvider || !msg) return;

		// Pack the message string as a single ETW data descriptor
		EVENT_DESCRIPTOR desc;
		EventDescCreate(&desc,
			1,      // EventId
			0,      // Version
			0,      // Channel
			level,  // Level
			0,      // Task
			0,      // Opcode
			0);     // Keyword

		EVENT_DATA_DESCRIPTOR data;
		EventDataDescCreate(&data, msg,
			static_cast<ULONG>((wcslen(msg) + 1) * sizeof(wchar_t)));

		EventWrite(g_hProvider, &desc, 1, &data);
	}
}