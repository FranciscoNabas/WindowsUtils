#include "pch.h"
#include "Unmanaged.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

using namespace std;

namespace Unmanaged::WindowsTerminalServices
{
	vector<DWORD> TerminalServices::InvokeMessage (
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
			shared_ptr<PWTS_SESSION_INFO> sessInfo = make_shared<PWTS_SESSION_INFO>();
			enumResult = WTSEnumerateSessions(session, 0, 1, &*sessInfo, &pCount);

			for (DWORD i = 0; i < pCount; i++)
			{
				DWORD response;
				mesResult = WTSSendMessageW(
					session,
					(*sessInfo)[i].SessionId,
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

	TerminalServices::SessionEnumOutput GetSessionOutput(HANDLE session, WTS_SESSION_INFO info)
	{
		DWORD pBytesReturned = 0;
		LPWSTR ppBuffer = L"";
		WTSQuerySessionInformation(session, info.SessionId, WTSUserName, &ppBuffer, &pBytesReturned);

		if (wcslen(ppBuffer) > 0) {
			TerminalServices::SessionEnumOutput result = TerminalServices::SessionEnumOutput(ppBuffer, info.pWinStationName);
			result.SessionId = info.SessionId;
			result.SessionState = (TerminalServices::WtsSessionState)info.State;
			return result;
		}
		else {
			TerminalServices::SessionEnumOutput result = TerminalServices::SessionEnumOutput(L"System", info.pWinStationName);
			result.SessionId = info.SessionId;
			result.SessionState = (TerminalServices::WtsSessionState)info.State;
			return result;
		}
	}

	void TerminalServices::GetEnumeratedSession(
		vector<TerminalServices::SessionEnumOutput>& ppOutVec,
		HANDLE session = WTS_CURRENT_SERVER_HANDLE,
		BOOL onlyActive = 0,
		BOOL excludeSystemSessions = 0
	)
	{
		BOOL enumResult = 0;
		DWORD pCount = 0;
		DWORD pLevel = 1;

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
					TerminalServices::SessionEnumOutput result = GetSessionOutput(session, innerSes);
					ppOutVec.push_back(result);
				}
			}
			break;

		default:
			if (excludeSystemSessions == 0)
			{
				for (DWORD i = 0; i < pCount; i++)
				{
					WTS_SESSION_INFO innerSes = sessionInfo[i];
					TerminalServices::SessionEnumOutput result = GetSessionOutput(session, innerSes);
					ppOutVec.push_back(result);
				}
			}
			else
			{
				for (DWORD i = 0; i < pCount; i++)
				{
					WTS_SESSION_INFO innerSes = sessionInfo[i];
					TerminalServices::SessionEnumOutput result = GetSessionOutput(session, innerSes);
					if (result.UserName != L"System") { ppOutVec.push_back(result); }
				}
			}
			break;
		}

	END:
		if (pCount > 0) { WTSFreeMemory(sessionInfo); }
	}
}