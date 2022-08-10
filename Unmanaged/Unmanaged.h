#pragma once

#include <WtsApi32.h>
#include <string>

using namespace std;

extern "C" public class __declspec(dllexport) Unmanaged
{
public:
	enum class WtsSessionState {
		Active = WTSActive,
		Connected = WTSConnected,
		ConnectQuery = WTSConnectQuery,
		Shadow = WTSShadow,
		Disconnected = WTSDisconnected,
		Idle = WTSIdle,
		Listen = WTSListen,
		Reset = WTSReset,
		Down = WTSDown,
		Init = WTSInit,
		Null = NULL
	};

	typedef struct SessionEnumOutput {
		SessionEnumOutput() {};
		wchar_t			UserName;
		wchar_t			SessionName;
		WtsSessionState	SessionState = WtsSessionState::Null;
	}SessionEnumOutput, *PSessionEnumOutput;

	PSessionEnumOutput GetEnumeratedSession(OUT DWORD* rCount, LPWSTR computerName, BOOL onlyActive, BOOL excludeSystemSessions);
};