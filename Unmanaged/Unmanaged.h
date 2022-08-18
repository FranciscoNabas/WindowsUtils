#pragma once

#include <WtsApi32.h>
#include <string>
#include <vector>
#include <stdio.h>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

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
		int						SessionId;
		std::wstring			UserName;
		std::wstring			SessionName;
		WtsSessionState	SessionState;
	}SessionEnumOutput, *PSessionEnumOutput;

	std::vector<Unmanaged::SessionEnumOutput> GetEnumeratedSession(HANDLE session, BOOL onlyActive, BOOL excludeSystemSessions);
	std::vector<DWORD> InvokeMessage(LPWSTR pTitle, LPWSTR pMessage, DWORD style, DWORD timeout, BOOL bWait, std::vector<DWORD> sessionId, HANDLE session);
};