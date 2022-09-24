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
		LPWSTR ppBuffer = L"";
		DWORD result = ERROR_SUCCESS;

		BOOL queryres = WTSQuerySessionInformationW(session, info.SessionId, WTSUserName, &ppBuffer, &pBytesReturned);
		if (queryres == 0)
			return GetLastError();

		if (wcslen(ppBuffer) > 0) {
			size_t buffsize = wcslen(ppBuffer) + 1;
			size_t sessnamesize = wcslen(info.pWinStationName) + 1;

			poutput.UserName = new WCHAR[buffsize];
			poutput.SessionName = new WCHAR[sessnamesize];

			wcscpy_s(poutput.UserName, buffsize, ppBuffer);
			wcscpy_s(poutput.SessionName, sessnamesize, info.pWinStationName);

			poutput.SessionId = info.SessionId;
			poutput.SessionState = (TerminalServices::WtsSessionState)info.State;

			WTSFreeMemory(ppBuffer);
			return result;
		}
		else {
			size_t sessnamesize = wcslen(info.pWinStationName) + 1;

			poutput.UserName = new WCHAR[7];
			poutput.SessionName = new WCHAR[sessnamesize];

			wcscpy_s(poutput.UserName, 7, L"System");
			wcscpy_s(poutput.SessionName, sessnamesize, info.pWinStationName);

			poutput.SessionId = info.SessionId;
			poutput.SessionState = (TerminalServices::WtsSessionState)info.State;
			return result;
		}
	}

	DWORD TerminalServices::GetEnumeratedSession(
		vector<TerminalServices::SessionEnumOutput>& ppOutVec,
		HANDLE session = WTS_CURRENT_SERVER_HANDLE,
		BOOL onlyActive = 0,
		BOOL excludeSystemSessions = 0
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
			if (excludeSystemSessions == 0)
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

					if (single.UserName != L"System") { ppOutVec.push_back(single); }
				}
			}
			break;
		}

	END:
		if (pCount > 0) { WTSFreeMemory(sessionInfo); }
		return result;
	}
}