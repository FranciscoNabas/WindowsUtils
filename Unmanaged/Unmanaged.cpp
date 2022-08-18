﻿#include "pch.h"
#include "Unmanaged.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

using namespace std;

vector<DWORD> Unmanaged::InvokeMessage(
	LPWSTR pTitle,
	LPWSTR pMessage,
	DWORD style,
	DWORD timeout,
	BOOL bWait,
	vector<DWORD> sessionId,
	HANDLE session = WTS_CURRENT_SERVER_HANDLE
)
{
	vector<DWORD> output;
	if (sessionId.empty())
	{
		DWORD pCount;
		BOOL enumResult;
		BOOL mesResult;
		PWTS_SESSION_INFO sessionInfo = (PWTS_SESSION_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WTS_SESSION_INFO));
		enumResult = WTSEnumerateSessions(session, 0, 1, &sessionInfo, &pCount);

		for (DWORD i = 0; i < pCount; i++)
		{
			DWORD response;
			mesResult = WTSSendMessageW(
				session,
				sessionInfo[i].SessionId,
				pTitle,
				(DWORD)wcslen(pTitle) * 2,
				pMessage,
				(DWORD)wcslen(pMessage) * 2,
				style,
				timeout,
				&response,
				bWait
			);

			output.push_back(response);
		}
		if (pCount > 0) { WTSFreeMemory(sessionInfo); }
	}
	else
	{
		BOOL mesResult;

		for (size_t i = 0; i < sessionId.size(); i++)
		{
			DWORD response;
			mesResult = WTSSendMessageW(
				session,
				sessionId.at(i),
				pTitle,
				(DWORD)wcslen(pTitle) * 2,
				pMessage,
				(DWORD)wcslen(pMessage) * 2,
				style,
				timeout,
				&response,
				bWait
			);

			output.push_back(response);
		}
	}

	return output;
}

Unmanaged::SessionEnumOutput GetOutputObject(HANDLE session, WTS_SESSION_INFO innerSes)
{
	wstring sessionName;
	wstring sessUserName;
	LPWSTR ppBuffer;
	DWORD pBytesReturned;
	BOOL thisResult;

	thisResult = WTSQuerySessionInformation(session, innerSes.SessionId, WTSUserName, &ppBuffer, &pBytesReturned);

	if (innerSes.pWinStationName == NULL) { sessionName = L""; }
	else { sessionName = innerSes.pWinStationName; }

	Unmanaged::SessionEnumOutput inner;
	inner.SessionId = innerSes.SessionId;
	inner.UserName = ppBuffer;
	inner.SessionName = sessionName;
	inner.SessionState = (Unmanaged::WtsSessionState)innerSes.State;

	WTSFreeMemory(ppBuffer);
	return inner;
}

 vector<Unmanaged::SessionEnumOutput> Unmanaged::GetEnumeratedSession(
	HANDLE session = WTS_CURRENT_SERVER_HANDLE,
	BOOL onlyActive = 0,
	BOOL excludeSystemSessions = 0
)
{
	BOOL enumResult;
	DWORD pCount = 0;
	DWORD pLevel = 1;
	vector<SessionEnumOutput> output;

	PWTS_SESSION_INFO sessionInfo = (PWTS_SESSION_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WTS_SESSION_INFO));

	enumResult = WTSEnumerateSessions(session, 0, 1, &sessionInfo, &pCount);
	if (enumResult == 0) { goto END; }

	switch (onlyActive)
	{
	case 1:
		for (DWORD i = 0; i < pCount; i++)
		{
			WTS_SESSION_INFO innerSes = sessionInfo[i];
			if (innerSes.State == WTSActive)
			{
				Unmanaged::SessionEnumOutput inner = GetOutputObject(session, innerSes);
				if (inner.UserName.empty()) { inner.UserName = L"System"; }
				output.push_back(inner);
			}
		}
		break;

	default:
		if (excludeSystemSessions == 0)
		{
			for (DWORD i = 0; i < pCount; i++)
			{
				WTS_SESSION_INFO innerSes = sessionInfo[i];
				Unmanaged::SessionEnumOutput inner = GetOutputObject(session, innerSes);
				if (inner.UserName.empty()) { inner.UserName = L"System"; }
				output.push_back(inner);
			}
		}
		else
		{
			for (DWORD i = 0; i < pCount; i++)
			{
				WTS_SESSION_INFO innerSes = sessionInfo[i];
				Unmanaged::SessionEnumOutput inner = GetOutputObject(session, innerSes);
				if (!inner.UserName.empty()) { output.push_back(inner); }
			}
		}
		break;
	}



END:
	if (pCount > 0) { WTSFreeMemory(sessionInfo); }
	return output;
}