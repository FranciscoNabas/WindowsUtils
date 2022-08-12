#include "pch.h"
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
	inner.UserName = ppBuffer;
	inner.SessionName = sessionName;
	inner.SessionState = (Unmanaged::WtsSessionState)innerSes.State;

	WTSFreeMemory(ppBuffer);
	return inner;
}

 vector<Unmanaged::SessionEnumOutput> Unmanaged::GetEnumeratedSession(
	LPWSTR computerName = NULL,
	BOOL onlyActive = 0,
	BOOL excludeSystemSessions = 0
)
{
	HANDLE session;
	BOOL enumResult;
	DWORD pCount = 0;
	DWORD pLevel = 1;
	vector<SessionEnumOutput> output;

	PWTS_SESSION_INFO sessionInfo = (PWTS_SESSION_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WTS_SESSION_INFO));

	if (computerName != NULL)
	{
		session = WTSOpenServer(computerName);
		if (session == NULL) { goto END; }
	}
	else { session = WTS_CURRENT_SERVER_HANDLE; }

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
	if (session != NULL) { WTSCloseServer(session); }
	if (pCount > 0) { WTSFreeMemory(sessionInfo); }
	return output;
}