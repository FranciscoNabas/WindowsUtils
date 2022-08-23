#pragma once
#pragma unmanaged

#include <WtsApi32.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <Windows.h>
#include <iostream>
#include <memory>

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

	class SessionEnumOutput
	{
	public:
		int				SessionId;
		LPWSTR			UserName;
		LPWSTR			SessionName;
		WtsSessionState	SessionState;
	
		SessionEnumOutput(LPWSTR usrName, LPWSTR sessName) {
			size_t usrSz = wcslen(usrName) + 1;
			size_t sesSz = wcslen(sessName) + 1;

			UserName = new wchar_t[usrSz];
			SessionName = new wchar_t[sesSz];

			wcscpy_s(UserName, usrSz, usrName);
			wcscpy_s(SessionName, sesSz, sessName);
		};
		~SessionEnumOutput() {};
	}*PSessionEnumOutput;

	typedef struct MessageDumpOutput {
		MessageDumpOutput() {};
		std::wstring	Id;
		std::wstring	Message;
	}MessageDumpOutput, *PMessageDumpOutput;

	std::vector<Unmanaged::MessageDumpOutput> GetResourceMessageTable(LPWSTR libName);
	void GetEnumeratedSession(std::vector<Unmanaged::SessionEnumOutput> &ppOutVec, HANDLE session, BOOL onlyActive, BOOL excludeSystemSessions);
	std::vector<DWORD> InvokeMessage(LPWSTR pTitle, LPWSTR pMessage, DWORD style, DWORD timeout, BOOL bWait, std::vector<DWORD> sessionId, HANDLE session);
};