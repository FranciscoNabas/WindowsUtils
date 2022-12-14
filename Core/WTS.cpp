#include "pch.h"
#include "Unmanaged.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

namespace WindowsUtils::Core
{
	DWORD Unmanaged::InvokeMessage (
		LPWSTR pTitle,
		LPWSTR pMessage,
		DWORD style,
		DWORD timeout,
		BOOL bWait,
		std::vector<DWORD>& sessionId,
		std::vector<Unmanaged::MessageResponse>& pvecres,
		HANDLE session = WTS_CURRENT_SERVER_HANDLE
	)
	{
		DWORD result = ERROR_SUCCESS;
		Unmanaged::MessageResponse* psingle;
		
		if (sessionId.empty())
		{
			DWORD pCount;
			std::shared_ptr<PWTS_SESSION_INFO> sessInfo = std::make_shared<PWTS_SESSION_INFO>();
			if (FALSE == WTSEnumerateSessionsW(session, 0, 1, &*sessInfo, &pCount))
				return GetLastError();

			for (DWORD i = 0; i < pCount; i++)
			{
				DWORD dwinerr = ERROR_SUCCESS;
				psingle = (Unmanaged::MessageResponse*)LocalAlloc(LMEM_ZEROINIT, sizeof(Unmanaged::MessageResponse));
				ALLCHECK(psingle);

				// We're not allowed to send messages to the 'Services' session.
				if (0 != (*sessInfo)[i].SessionId)
				{
					if (FALSE == WTSSendMessageW(
						session,
						(*sessInfo)[i].SessionId,
						pTitle,
						(DWORD)wcslen(pTitle) * 2,
						pMessage,
						(DWORD)wcslen(pMessage) * 2,
						style,
						timeout,
						&psingle->Response,
						bWait
					))
					{
						dwinerr = GetLastError();
						if (dwinerr != ERROR_FILE_NOT_FOUND) // We don't wanna break if the session type cannot receive messages.
							return dwinerr;
					}

					psingle->SessionId = (*sessInfo)[i].SessionId;
					pvecres.push_back(*psingle);
				}
				
				if (NULL == LocalFree(psingle))
					psingle = NULL;
			}
		}
		else
		{
			for (size_t i = 0; i < sessionId.size(); i++)
			{
				DWORD dwinerr = ERROR_SUCCESS;
				psingle = (Unmanaged::MessageResponse*)LocalAlloc(LMEM_ZEROINIT, sizeof(Unmanaged::MessageResponse));
				ALLCHECK(psingle);

				if (FALSE == WTSSendMessageW(
					session,
					sessionId.at(i),
					pTitle,
					(DWORD)wcslen(pTitle) * 2,
					pMessage,
					(DWORD)wcslen(pMessage) * 2,
					style,
					timeout,
					&psingle->Response,
					bWait
				))
				{
					dwinerr = GetLastError();
					if (dwinerr != ERROR_FILE_NOT_FOUND) // We don't wanna break if the session type cannot receive messages.
						return dwinerr;
				}

				psingle->SessionId = sessionId.at(i);
				pvecres.push_back(*psingle);

				if (NULL == LocalFree(psingle))
					psingle = NULL;
			}
		}

		return result;
	}

	DWORD GetSessionOutput(Unmanaged::ComputerSession& poutput, HANDLE session, WTS_SESSION_INFO info)
	{
		DWORD pBytesReturned = 0;
		DWORD result = ERROR_SUCCESS;
		PWTSINFOW sessinfoex;

		BOOL queryres = WTSQuerySessionInformationW(session, info.SessionId, WTSSessionInfo, (LPWSTR*)&sessinfoex, &pBytesReturned);
		if (queryres == 0)
			return GetLastError();

		size_t usrnsize = wcslen(sessinfoex->UserName) + 1;
		size_t sessnamesize = wcslen(info.pWinStationName) + 1;
		size_t domainnsize = wcslen(sessinfoex->Domain) + 1;

		if (usrnsize > 1) {
			poutput.SessionId = info.SessionId;
			
			if (domainnsize > 1)
			{
				size_t tsize = domainnsize + usrnsize;
				LPWSTR fqdun = new WCHAR[tsize];
				wcscpy_s(fqdun, domainnsize, sessinfoex->Domain);
				wcscat_s(fqdun, tsize, L"\\");
				wcscat_s(fqdun, tsize, sessinfoex->UserName);

				poutput.UserName = new WCHAR[tsize];
				wcscpy_s(poutput.UserName, tsize, fqdun);
			}
			else
			{
				poutput.UserName = new WCHAR[usrnsize];
				wcscpy_s(poutput.UserName, usrnsize, sessinfoex->UserName);
			}
			
			poutput.SessionName = new WCHAR[sessnamesize];
			wcscpy_s(poutput.SessionName, sessnamesize, info.pWinStationName);

			poutput.IdleTime = sessinfoex->LastInputTime;
			poutput.SessionState = info.State;
			poutput.LogonTime = sessinfoex->LogonTime;
			
			WTSFreeMemory(sessinfoex);
			return result;
		}
		else {
			poutput.SessionId = info.SessionId;
			poutput.UserName = new WCHAR[1];
			poutput.SessionName = new WCHAR[sessnamesize];

			wcscpy_s(poutput.UserName, 1, L"");
			wcscpy_s(poutput.SessionName, sessnamesize, info.pWinStationName);

			poutput.IdleTime = sessinfoex->LastInputTime;
			poutput.SessionState = info.State;
			poutput.LogonTime = sessinfoex->LogonTime;
			
			return result;
		}
	}

	DWORD Unmanaged::GetEnumeratedSession(
		std::vector<Unmanaged::ComputerSession>& ppOutVec,
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
					Unmanaged::ComputerSession single;
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
					Unmanaged::ComputerSession single;
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
					Unmanaged::ComputerSession single;
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

	DWORD Unmanaged::DisconnectSession(
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