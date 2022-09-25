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

	DWORD GetSessionOutput(TerminalServices::SessionEnumOutput& poutput, HANDLE session, WTS_SESSION_INFO info)
	{
		DWORD pBytesReturned = 0;
		DWORD result = ERROR_SUCCESS;
		PWTSINFOW sessinfoex;

		BOOL queryres = WTSQuerySessionInformationW(session, info.SessionId, WTSSessionInfo, (LPWSTR*)&sessinfoex, &pBytesReturned);
		if (queryres == 0)
			return GetLastError();

		size_t usrnsize = wcslen(sessinfoex->UserName) + 1;
		size_t sessnamesize = wcslen(info.pWinStationName) + 1;

		if (usrnsize > 1) {
			poutput.UserName = new WCHAR[usrnsize];
			poutput.SessionName = new WCHAR[sessnamesize];

			wcscpy_s(poutput.UserName, usrnsize, sessinfoex->UserName);
			wcscpy_s(poutput.SessionName, sessnamesize, info.pWinStationName);

			poutput.LogonTime = sessinfoex->LogonTime;
			poutput.IdleTime = sessinfoex->LastInputTime;
			poutput.SessionId = info.SessionId;
			poutput.SessionState = (TerminalServices::WtsSessionState)info.State;

			WTSFreeMemory(sessinfoex);
			return result;
		}
		else {
			poutput.UserName = new WCHAR[1];
			poutput.SessionName = new WCHAR[sessnamesize];

			wcscpy_s(poutput.UserName, 1, L"");
			wcscpy_s(poutput.SessionName, sessnamesize, info.pWinStationName);

			poutput.LogonTime = sessinfoex->LogonTime;
			poutput.IdleTime = sessinfoex->LastInputTime;
			poutput.SessionId = info.SessionId;
			poutput.SessionState = (TerminalServices::WtsSessionState)info.State;
			return result;
		}
	}

	DWORD TerminalServices::GetEnumeratedSession(
		vector<TerminalServices::SessionEnumOutput>& ppOutVec,
		HANDLE session = WTS_CURRENT_SERVER_HANDLE,
		BOOL onlyActive = 0,
		BOOL includeSystemSessions = 0
	)
	{
		BOOL enumResult = 0;
		DWORD pCount = 0;
		DWORD pLevel = 1;
		DWORD result = ERROR_SUCCESS;

		PWTS_SESSION_INFOW sessionInfo = (PWTS_SESSION_INFOW)LocalAlloc(LMEM_ZEROINIT, sizeof(PWTS_SESSION_INFOW));
		if (nullptr == sessionInfo)
		{
			result = ERROR_NOT_ENOUGH_MEMORY;
			goto END;
		}

		enumResult = WTSEnumerateSessionsW(session, 0, 1, &sessionInfo, &pCount);
		if (enumResult == 0)
		{
			result = GetLastError();
			goto END;
		}

		switch (onlyActive)
		{
		case 1:
			for (DWORD i = 0; i < pCount; i++)
			{
				WTS_SESSION_INFO innerSes = sessionInfo[i];
				if (innerSes.State == WTSActive)
				{
					TerminalServices::SessionEnumOutput single;
					result = GetSessionOutput(single, session, innerSes);
					if (result != ERROR_SUCCESS)
						goto END;

					ppOutVec.push_back(single);
				}
			}
			break;

		default:
			if (includeSystemSessions == 1)
			{
				for (DWORD i = 0; i < pCount; i++)
				{
					WTS_SESSION_INFO innerSes = sessionInfo[i];
					TerminalServices::SessionEnumOutput single;
					result = GetSessionOutput(single, session, innerSes);
					if (result != ERROR_SUCCESS)
						goto END;

					ppOutVec.push_back(single);
				}
			}
			else
			{
				for (DWORD i = 0; i < pCount; i++)
				{
					WTS_SESSION_INFO innerSes = sessionInfo[i];
					TerminalServices::SessionEnumOutput single;
					result = GetSessionOutput(single, session, innerSes);
					if (result != ERROR_SUCCESS)
						goto END;

					if (wcslen(single.UserName) > 0) { ppOutVec.push_back(single); }
				}
			}
			break;
		}

	END:
		if (pCount > 0) { WTSFreeMemory(sessionInfo); }
		return result;
	}

	DWORD TerminalServices::DisconnectSession(
		HANDLE session = WTS_CURRENT_SERVER_HANDLE,
		DWORD sessionid = WTS_CURRENT_SESSION,
		BOOL wait = 0
	)
	{
		DWORD result = 0;
		BOOL call = WTSLogoffSession(session, sessionid, wait);
		if (call == 0)
			result = GetLastError();

		return result;
	}
}