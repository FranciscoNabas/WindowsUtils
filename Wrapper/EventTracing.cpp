#include "pch.h"
#include "EventTracing.h"
#include <Wmistr.h>
#include <evntrace.h>
#include <strsafe.h>

// For testing only. Remove after implementing callback.
#include <conio.h >

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

#define CHECKPOINTER(ptr) if (NULL == ptr) { goto CLEANUP; }

namespace Unmanaged::WindowsEventTracing
{
	static void WINAPI EventCallback(PEVENT_TRACE pevttrace)
	{


		PBYTE mofdata = (PBYTE)(pevttrace->MofData);
		PBYTE endmofdata = ((PBYTE)(pevttrace->MofData) + pevttrace->MofLength);

		for (UINT i = 0; i < 5; i++)
		{

		}

		EVENT_TRACE evttrace = *pevttrace;
	}

	ULONG StartProcessingTrace(LPWSTR loggername)
	{
		ULONG result;

		PEVENT_TRACE_LOGFILEW traceinfo = (PEVENT_TRACE_LOGFILEW)LocalAlloc(LMEM_ZEROINIT, sizeof(EVENT_TRACE_LOGFILEW));
		if (nullptr != traceinfo)
		{
			EventDecoder context(nullptr);
			TraceHandles handles;

			traceinfo->LoggerName = loggername;
			traceinfo->ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME;
			traceinfo->EventCallback = &EventCallback;
			traceinfo->Context = &context;

			result = handles.OpenTraceW(traceinfo);
			if (result != 0)
				return result;

			result = handles.ProcessTrace(nullptr, nullptr);
		}

		return result;
	}

	DWORD EventTracing::StartRegistryTrace(GUID &traceid, LPWSTR psessname)
	{
		DWORD result = 0;
		ULONG bufferSize = 0;
		TRACEHANDLE sessionhandle = 0;
		const GUID GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

		result = CoCreateGuid(&traceid);

		ULONG sessnamesize = (ULONG)(sizeof(WCHAR) * (wcslen(psessname) + 1));
		ULONG dummyfnamesize = (ULONG)(sizeof(WCHAR) * (wcslen(L"C:\\Windows\\dummyfname.etl") + 1));
		bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sessnamesize + dummyfnamesize;
		
		PEVENT_TRACE_PROPERTIES pevntprop = (PEVENT_TRACE_PROPERTIES)LocalAlloc(LMEM_ZEROINIT, bufferSize);
		CHECKPOINTER(pevntprop);

		if (GUID_NULL != traceid)
			pevntprop->Wnode.Guid = traceid;
		
		pevntprop->Wnode.BufferSize = bufferSize;
		pevntprop->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

		pevntprop->BufferSize = 8;
		pevntprop->MinimumBuffers = 4;
		pevntprop->MaximumBuffers = 4;
		pevntprop->FlushTimer = 7;

		pevntprop->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE;
		pevntprop->EnableFlags = EVENT_TRACE_FLAG_REGISTRY | EVENT_TRACE_FLAG_NO_SYSCONFIG;
		pevntprop->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
		pevntprop->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + dummyfnamesize;

		if (StartTrace(&sessionhandle, psessname, pevntprop) == ERROR_ALREADY_EXISTS)
		{
			result = ControlTraceW(0, psessname, pevntprop, EVENT_TRACE_CONTROL_STOP);
			goto CLEANUP;
		}
			
		
		result = StartProcessingTrace(psessname);
		
	CLEANUP:
		if (pevntprop)
			LocalFree(pevntprop);

		return result;
	}

	DWORD EventTracing::StopRegistryTrace(GUID traceid, LPWSTR psessname)
	{
		DWORD result = 0;
		ULONG bufferSize = 0;
		TRACEHANDLE sessionhandle = 0;

		ULONG sessnamesize = (ULONG)(sizeof(WCHAR) * (wcslen(psessname) + 1));
		bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sessnamesize;
		
		PEVENT_TRACE_PROPERTIES pevntprop = (PEVENT_TRACE_PROPERTIES)LocalAlloc(LMEM_ZEROINIT, bufferSize);
		CHECKPOINTER(pevntprop);

		pevntprop->Wnode.Guid = traceid;
		pevntprop->Wnode.BufferSize = bufferSize;
		pevntprop->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
		pevntprop->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

		result = ControlTraceW(0, psessname, pevntprop, EVENT_TRACE_CONTROL_STOP);

	CLEANUP:
		if (pevntprop)
			LocalFree(pevntprop);

		return result;
	}
}