#include "pch.h"
#include "Unmanaged.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

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

	PWTS_SESSION_INFO_1 sessionInfo = (PWTS_SESSION_INFO_1)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WTS_SESSION_INFO_1));

	if (computerName != NULL)
	{
		session = WTSOpenServer(computerName);
		if (session == NULL) { goto END; }
	}
	else { session = WTS_CURRENT_SERVER_HANDLE; }

	enumResult = WTSEnumerateSessionsEx(session, &pLevel, 0, &sessionInfo, &pCount);
	if (enumResult == 0) { goto END; }

	switch (onlyActive)
	{
	case 1:
		for (DWORD i = 0; i < pCount; i++)
		{
			WTS_SESSION_INFO_1 innerSes = sessionInfo[i];
			if (innerSes.State == WTSActive)
			{
				wchar_t *sessionName;
				wchar_t *sessUserName;
				SessionEnumOutput outObj;

				if (innerSes.pUserName == NULL) { sessUserName = L"System"; }
				else { sessUserName = innerSes.pUserName; }
				if (innerSes.pSessionName == NULL) { sessionName = L""; }
				else { sessionName = innerSes.pSessionName; }

				Unmanaged::SessionEnumOutput inner;
				inner.UserName = sessUserName;
				inner.SessionName = sessionName;
				inner.SessionState = (WtsSessionState)innerSes.State;

				output.push_back(inner);
			}
		}
		break;

	default:
		if (excludeSystemSessions == 0)
		{
			for (DWORD i = 0; i < pCount; i++)
			{
				WTS_SESSION_INFO_1 innerSes = sessionInfo[i];
				wchar_t* sessionName;
				wchar_t* sessUserName;
				SessionEnumOutput outObj;

				if (innerSes.pUserName == NULL) { sessUserName = L"System"; }
				else { sessUserName = innerSes.pUserName; }
				if (innerSes.pSessionName == NULL) { sessionName = L""; }
				else { sessionName = innerSes.pSessionName; }

				Unmanaged::SessionEnumOutput inner;
				inner.UserName = sessUserName;
				inner.SessionName = sessionName;
				inner.SessionState = (WtsSessionState)innerSes.State;

				output.push_back(inner);

			}
		}
		else
		{
			for (DWORD i = 0; i < pCount; i++)
			{
				WTS_SESSION_INFO_1 innerSes = sessionInfo[i];
				wstring sessUserName;
				if (innerSes.pUserName == NULL) { sessUserName = L""; }
				else { sessUserName = innerSes.pUserName; }

				if (sessUserName.length() > 0)
				{
					wchar_t *innerUser = (wchar_t*)sessUserName.c_str();
					wchar_t *sessionName;
					SessionEnumOutput outObj;
					WTS_SESSION_INFO_1 innerSes = sessionInfo[i];

					if (innerSes.pSessionName == NULL) { sessionName = L""; }
					else { sessionName = innerSes.pSessionName; }

					Unmanaged::SessionEnumOutput inner;
					inner.UserName = innerUser;
					inner.SessionName = sessionName;
					inner.SessionState = (WtsSessionState)innerSes.State;

					output.push_back(inner);
				}
			}
		}
		break;
	}



END:
	if (session != NULL) { WTSCloseServer(session); }
	if (pCount > 0) { WTSFreeMemoryEx(WTSTypeSessionInfoLevel1, sessionInfo, pCount); }
	return output;
}