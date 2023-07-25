#include "pch.h"

#include "TerminalServices.h"

namespace WindowsUtils::Core
{
	/*========================================
	==		 Main function definition		==
	==========================================*/

	// Invoke-RemoteMessage
	DWORD TerminalServices::SendMessage(
		WWuString& title,													// The message box title.
		WWuString& message,													// The message text.
		DWORD style,														// A bitwise-or combination of UINT-defined MessageBox styles (See function MessageBox).
		DWORD timeout,														// Timeout, in seconds, for the function to wait a response.
		BOOL wait,															// TRUE for the function wait a response. If FALSE, the function returns immediately with code 32001 (0x7D01), IDASYNC.
		wuvector<DWORD>* sessionIdList,										// The target session IDs.
		wuvector<TerminalServices::WU_MESSAGE_RESPONSE>* responseList,		// A vector to receive the message responses.
		HANDLE hServer														// A handle to a WTS session.
	) {
		DWORD result = ERROR_SUCCESS;

		for (DWORD dwSession : *sessionIdList)
		{
			WU_MESSAGE_RESPONSE response;

			if (!::WTSSendMessageW(
				hServer,
				dwSession,
				title.GetBuffer(),
				static_cast<DWORD>(title.Length() * 2),
				message.GetBuffer(),
				static_cast<DWORD>(message.Length() * 2),
				style,
				timeout,
				&response.Response,
				wait
			))
			{
				result = ::GetLastError();
				// We don't want to break if the session is not found, or cannot receive messages
				if (ERROR_FILE_NOT_FOUND != result)
					return result;
				else
					result = ERROR_SUCCESS;
			}
			if (dwSession == WTS_CURRENT_SESSION)
			{
				DWORD sessid;
				::ProcessIdToSessionId(GetCurrentProcessId(), &sessid);
				response.SessionId = sessid;
			}
			else
				response.SessionId = dwSession;

			responseList->push_back(response);
		}
		
		return result;
	}

	DWORD TerminalServices::SendMessage(
		WWuString& title,													// The message box title.
		WWuString& message,													// The message text.
		DWORD style,														// A bitwise-or combination of UINT-defined MessageBox styles (See function MessageBox).
		DWORD timeout,														// Timeout, in seconds, for the function to wait a response.
		BOOL wait,															// TRUE for the function wait a response. If FALSE, the function returns immediately with code 32001 (0x7D01), IDASYNC.
		wuvector<TerminalServices::WU_MESSAGE_RESPONSE>* responseList,		// A vector to receive the message responses.
		HANDLE hServer														// A handle to a WTS session.
	) {
		DWORD result = ERROR_SUCCESS;
		DWORD sessionCount = 0;
		PWTS_SESSION_INFOW sessionInfo;

		if (!::WTSEnumerateSessionsW(hServer, 0, 1, &sessionInfo, &sessionCount))
			return ::GetLastError();

		for (DWORD i = 0; i < sessionCount; i++)
		{
			WU_MESSAGE_RESPONSE response;

			// Session 0 is not an interactive session
			if (sessionInfo[i].SessionId != 0)
			{
				if (!::WTSSendMessageW(
					hServer,
					sessionInfo[i].SessionId,
					title.GetBuffer(),
					static_cast<DWORD>(title.Length() * 2),
					message.GetBuffer(),
					static_cast<DWORD>(message.Length() * 2),
					style,
					timeout,
					&response.Response,
					wait
				))
				{
					result = ::GetLastError();
					// We don't want to break if the session is not found, or cannot receive messages
					if (ERROR_FILE_NOT_FOUND != result)
					{
						WTSFreeMemory(sessionInfo);
						return result;
					}
					else
						result = ERROR_SUCCESS;
				}

				response.SessionId = sessionInfo[i].SessionId;
				responseList->push_back(response);
			}
		}
		
		WTSFreeMemory(sessionInfo);

		return result;
	}

	// Get-ComputerSession
	DWORD TerminalServices::GetEnumeratedSession(
		wuvector<TerminalServices::WU_COMPUTER_SESSION>* sessionInfoList,		// A vector with the output computer session data.
		HANDLE hServer = WTS_CURRENT_SERVER_HANDLE,								// A handle to a WTS server session.
		BOOL activeOnly = FALSE,												// Returns only sessions with SessionState = Active.
		BOOL includeSystemSession = FALSE										// Includes sessions without an assigned user name.
	)
	{
		DWORD result = ERROR_SUCCESS;
		PWTS_SESSION_INFOW sessionInfo;
		DWORD sessionCount;

		if (!::WTSEnumerateSessionsW(hServer, 0, 1, &sessionInfo, &sessionCount))
			return GetLastError();

		switch (activeOnly)
		{
		case TRUE:
			for (DWORD i = 0; i < sessionCount; i++)
			{
				if (sessionInfo[i].State == WTSActive)
				{
					std::wstring tits = L"Huge titties.";
					TerminalServices::WU_COMPUTER_SESSION computerSession;
					result = GetSessionOutput(&computerSession, hServer, sessionInfo[i]);
					if (ERROR_SUCCESS != result)
						goto CLEANUP;

					sessionInfoList->push_back(computerSession);
				}
			}
			break;

			// 'System' sessions are never 'WTSActive'
		default:
			if (includeSystemSession == TRUE)
			{
				for (DWORD i = 0; i < sessionCount; i++)
				{
					TerminalServices::WU_COMPUTER_SESSION computerSession;
					result = GetSessionOutput(&computerSession, hServer, sessionInfo[i]);
					if (ERROR_SUCCESS != result)
						goto CLEANUP;

					sessionInfoList->push_back(computerSession);
				}
			}
			else
			{
				for (DWORD i = 0; i < sessionCount; i++)
				{
					TerminalServices::WU_COMPUTER_SESSION computerSession;
					result = GetSessionOutput(&computerSession, hServer, sessionInfo[i]);
					if (ERROR_SUCCESS != result)
						goto CLEANUP;

					if (!WWuString::IsNullOrEmpty(computerSession.UserName))
						sessionInfoList->push_back(computerSession);
				}
			}
			break;
		}

	CLEANUP:

		WTSFreeMemory(sessionInfo);

		return result;
	}

	// Disconnect-Session
	DWORD TerminalServices::DisconnectSession(
		HANDLE hServer = WTS_CURRENT_SERVER_HANDLE,		// A handle to a WTS server session.
		DWORD sessionId = WTS_CURRENT_SESSION,			// The session to disconnect.
		BOOL wait = FALSE								// Wait for the logoff operation to finish.
	)
	{
		if (!::WTSLogoffSession(hServer, sessionId, wait))
			return GetLastError();

		return ERROR_SUCCESS;
	}

	/*========================================
	==		Utility function definition		==
	==========================================*/

	/*
	* Helper function to get extra WTS session information.
	* There is a known bug with 'WTSEnumerateSessionsEx', and 'WTSEnumerateSessions' does not bring all the information we want.
	*/
	DWORD GetSessionOutput(TerminalServices::PWU_COMPUTER_SESSION computerSession, HANDLE hServer, const WTS_SESSION_INFO& sessionInfo)
	{
		DWORD result = ERROR_SUCCESS;
		PWTSINFOW infoBuffer = NULL;
		DWORD bytesNeeded;

		if (!::WTSQuerySessionInformationW(hServer, sessionInfo.SessionId, WTSSessionInfo, (LPWSTR*)&infoBuffer, &bytesNeeded) || NULL == infoBuffer)
			return ::GetLastError();

		computerSession->SessionName = infoBuffer->WinStationName;
		computerSession->SessionId = sessionInfo.SessionId;
		computerSession->SessionState = sessionInfo.State;
		computerSession->LogonTime = infoBuffer->LogonTime;

		if (wcslen(infoBuffer->UserName) > 1)
		{
			wuunique_ha_ptr<WCHAR> unameBuff = make_wuunique_ha<WCHAR>(UNLEN * 2);
			DWORD bufferSize = UNLEN;

			if (FALSE == GetUserNameW(unameBuff.get(), &bufferSize))
				return ::GetLastError();

			WWuString currentProcUser(unameBuff.get());

			if (currentProcUser == infoBuffer->UserName && hServer == WTS_CURRENT_SERVER_HANDLE)
				computerSession->LastInputTime.QuadPart = 0;
			else
				computerSession->LastInputTime = infoBuffer->LastInputTime;

			if (wcslen(infoBuffer->Domain) > 1)
				computerSession->UserName.Format(L"%ws\\%ws", infoBuffer->Domain, infoBuffer->UserName);
			else
				computerSession->UserName = infoBuffer->UserName;
		}
		else
			computerSession->LastInputTime = infoBuffer->LastInputTime;

		if (NULL != infoBuffer)
			::WTSFreeMemory(infoBuffer);

		return result;
	}
}