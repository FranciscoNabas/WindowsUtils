#pragma once

#include <WtsApi32.h>
#include <string>
#include <vector>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

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
	};

	typedef struct SessionEnumOutput {
		SessionEnumOutput() {};
		wstring			UserName;
		wstring			SessionName;
		WtsSessionState	SessionState;
	}SessionEnumOutput, *PSessionEnumOutput;

	vector<Unmanaged::SessionEnumOutput> GetEnumeratedSession(LPWSTR computerName, BOOL onlyActive, BOOL excludeSystemSessions);
};