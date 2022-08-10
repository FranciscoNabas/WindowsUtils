#include "pch.h"
#include "Unmanaged.h"
#include <Windows.h>
#include <string>

using namespace std;

Unmanaged::PSessionEnumOutput Unmanaged::GetEnumeratedSession(
	OUT DWORD* rCount,
	LPWSTR computerName = NULL,
	BOOL onlyActive = 0,
	BOOL excludeSystemSessions = 0
)
{
	HANDLE session;
	BOOL enumResult;
	DWORD pCount = 0;
	DWORD pLevel = 1;
	PWTS_SESSION_INFO_1 sessionInfo = (PWTS_SESSION_INFO_1)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WTS_SESSION_INFO_1));

	if (computerName != NULL)
	{
		session = WTSOpenServer(computerName);
		if (session == NULL) { goto END; }
	}
	else { session = WTS_CURRENT_SERVER_HANDLE; }

	enumResult = WTSEnumerateSessionsEx(session, &pLevel, 0, &sessionInfo, &pCount);
	if (enumResult == 0) { goto END; }

	rCount = &pCount;
	static PSessionEnumOutput output = new SessionEnumOutput[pCount];
	switch (onlyActive)
	{
	case 1:
		for (DWORD i = 0; i < pCount; i++)
		{
			WTS_SESSION_INFO_1 innerSes = sessionInfo[i];
			if (innerSes.State == WTSActive)
			{
				wchar_t  sessionName;
				wchar_t sessUserName;
				SessionEnumOutput outObj;

				if (innerSes.pUserName == NULL) { sessUserName = *L"System"; }
				else { sessUserName = *innerSes.pUserName; }
				if (innerSes.pSessionName == NULL) { sessionName = *L""; }
				else { sessionName = *innerSes.pSessionName; }

				WtsSessionState sessionState = (WtsSessionState)innerSes.State;
				output[i].UserName = sessUserName;
				output[i].SessionName = sessionName;
				output[i].SessionState = sessionState;
			}
		}
		break;

	default:
		if (excludeSystemSessions == 0)
		{
			for (DWORD i = 0; i < pCount; i++)
			{
				WTS_SESSION_INFO_1 innerSes = sessionInfo[i];
				wchar_t  sessionName;
				wchar_t sessUserName;
				SessionEnumOutput outObj;

				if (innerSes.pUserName == NULL) { sessUserName = *L"System"; }
				else { sessUserName = *innerSes.pUserName; }
				if (innerSes.pSessionName == NULL) { sessionName = *L""; }
				else { sessionName = *innerSes.pSessionName; }

				WtsSessionState sessionState = (WtsSessionState)innerSes.State;
				output[i].UserName = sessUserName;
				output[i].SessionName = sessionName;
				output[i].SessionState = sessionState;
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
					wchar_t innerUser = *(wchar_t*)sessUserName.c_str();
					wchar_t sessionName;
					SessionEnumOutput outObj;
					WTS_SESSION_INFO_1 innerSes = sessionInfo[i];

					if (innerSes.pSessionName == NULL) { sessionName = *L""; }
					else { sessionName = *innerSes.pSessionName; }

					WtsSessionState sessionState = (WtsSessionState)innerSes.State;
					output[i].UserName = innerUser;
					output[i].SessionName = sessionName;
					output[i].SessionState = sessionState;
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