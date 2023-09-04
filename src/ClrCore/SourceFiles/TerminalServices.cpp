#include "..\pch.h"

#include "..\Headers\TerminalServices.h"

namespace WindowsUtils::Core
{
	/*========================================
	==		 Main function definition		==
	==========================================*/

	// Invoke-RemoteMessage
	WuResult TerminalServices::SendMessage(
		WWuString& title,													// The message box title.
		WWuString& message,													// The message text.
		DWORD style,														// A bitwise-or combination of UINT-defined MessageBox styles (See function MessageBox).
		DWORD timeout,														// Timeout, in seconds, for the function to wait a response.
		BOOL wait,															// TRUE for the function wait a response. If FALSE, the function returns immediately with code 32001 (0x7D01), IDASYNC.
		wuvector<DWORD>* sessionIdList,										// The target session IDs.
		wuvector<TerminalServices::WU_MESSAGE_RESPONSE>* responseList,		// A vector to receive the message responses.
		HANDLE hServer														// A handle to a WTS session.
	) {
		DWORD dwResult = ERROR_SUCCESS;

		for (DWORD dwSession : *sessionIdList)
		{
			WU_MESSAGE_RESPONSE response;

			if (!WTSSendMessageW(
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
				dwResult = GetLastError();
				
				// We don't want to break if the session is not found, or cannot receive messages
				if (dwResult != ERROR_FILE_NOT_FOUND)
					return WuResult(dwResult, __FILEW__, __LINE__);
				else
					dwResult = ERROR_SUCCESS;
			}
			if (dwSession == WTS_CURRENT_SESSION)
			{
				DWORD sessid;
				ProcessIdToSessionId(GetCurrentProcessId(), &sessid);
				response.SessionId = sessid;
			}
			else
				response.SessionId = dwSession;

			responseList->push_back(response);
		}
		
		return WuResult();
	}

	WuResult TerminalServices::SendMessage(
		WWuString& title,													// The message box title.
		WWuString& message,													// The message text.
		DWORD style,														// A bitwise-or combination of UINT-defined MessageBox styles (See function MessageBox).
		DWORD timeout,														// Timeout, in seconds, for the function to wait a response.
		BOOL wait,															// TRUE for the function wait a response. If FALSE, the function returns immediately with code 32001 (0x7D01), IDASYNC.
		wuvector<TerminalServices::WU_MESSAGE_RESPONSE>* responseList,		// A vector to receive the message responses.
		HANDLE hServer														// A handle to a WTS session.
	) {
		DWORD dwResult = ERROR_SUCCESS;
		DWORD sessionCount = 0;
		PWTS_SESSION_INFOW sessionInfo;

		if (!WTSEnumerateSessionsW(hServer, 0, 1, &sessionInfo, &sessionCount))
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		for (DWORD i = 0; i < sessionCount; i++)
		{
			WU_MESSAGE_RESPONSE response;

			// Session 0 is not an interactive session
			if (sessionInfo[i].SessionId != 0)
			{
				if (!WTSSendMessageW(
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
					dwResult = GetLastError();
					
					// We don't want to break if the session is not found, or cannot receive messages
					if (dwResult != ERROR_FILE_NOT_FOUND)
					{
						WTSFreeMemory(sessionInfo);
						return WuResult(dwResult, __FILEW__, __LINE__);
					}
					else
						dwResult = ERROR_SUCCESS;
				}

				response.SessionId = sessionInfo[i].SessionId;
				responseList->push_back(response);
			}
		}
		
		WTSFreeMemory(sessionInfo);

		return WuResult();
	}

	// Get-ComputerSession
	WuResult TerminalServices::GetEnumeratedSession(
		wuvector<TerminalServices::WU_COMPUTER_SESSION>* sessionInfoList,		// A vector with the output computer session data.
		HANDLE hServer = WTS_CURRENT_SERVER_HANDLE,								// A handle to a WTS server session.
		BOOL activeOnly = FALSE,												// Returns only sessions with SessionState = Active.
		BOOL includeSystemSession = FALSE										// Includes sessions without an assigned user name.
	)
	{
		WuResult result;
		DWORD dwResult = ERROR_SUCCESS;
		PWTS_SESSION_INFOW sessionInfo;
		DWORD sessionCount;

		if (!WTSEnumerateSessionsW(hServer, 0, 1, &sessionInfo, &sessionCount))
			return WuResult(GetLastError(), __FILEW__, __LINE__);

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
					if (result.Result != ERROR_SUCCESS)
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
					if (result.Result != ERROR_SUCCESS)
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
					if (result.Result != ERROR_SUCCESS)
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

	void TerminalServices::GetEnumeratedSession(WWuString& computerName, wuvector<WU_COMPUTER_SESSION>& sessionList, bool onlyActive, bool includeSystemSessions)
	{
		HANDLE hServer;
		PWTS_SESSION_INFOW sessionInfo;
		DWORD sessionCount;
		WuResult result;

		if (computerName.Length() == 0) {
			hServer = WTS_CURRENT_SERVER_HANDLE;

			WCHAR computerNameRaw[MAX_PATH] = { 0 };
			DWORD pcNameSize = MAX_PATH;
			if (GetComputerName(computerNameRaw, &pcNameSize))
				computerName = WWuString(computerNameRaw);
		}
		else {
			// If the function succeeds you get what you want, if it fails
			// you also get what you want, but is invalid. wth??
			hServer = WTSOpenServer(computerName.GetBuffer());
		}

		if (!WTSEnumerateSessionsW(hServer, 0, 1, &sessionInfo, &sessionCount))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		switch (onlyActive) {
			case true:
				for (DWORD i = 0; i < sessionCount; i++) {
					if (sessionInfo[i].State == WTSActive) {
						TerminalServices::WU_COMPUTER_SESSION computerSession;

						result = GetSessionOutput(&computerSession, hServer, sessionInfo[i]);
						if (result.Result != ERROR_SUCCESS) {
							WTSFreeMemory(sessionInfo);
							throw WuStdException(result.Result, __FILEW__, __LINE__);
						}

						computerSession.ComputerName = computerName;

						sessionList.push_back(computerSession);
					}
				}
				break;

				// 'System' sessions are never 'WTSActive'
			default:
				if (includeSystemSessions == true) {
					for (DWORD i = 0; i < sessionCount; i++) {
						TerminalServices::WU_COMPUTER_SESSION computerSession;

						result = GetSessionOutput(&computerSession, hServer, sessionInfo[i]);
						if (result.Result != ERROR_SUCCESS) {
							WTSFreeMemory(sessionInfo);
							throw WuStdException(result.Result, __FILEW__, __LINE__);
						}
						
						computerSession.ComputerName = computerName;

						sessionList.push_back(computerSession);
					}
				}
				else {
					for (DWORD i = 0; i < sessionCount; i++) {
						TerminalServices::WU_COMPUTER_SESSION computerSession;

						result = GetSessionOutput(&computerSession, hServer, sessionInfo[i]);
						if (result.Result != ERROR_SUCCESS) {
							WTSFreeMemory(sessionInfo);
							throw WuStdException(result.Result, __FILEW__, __LINE__);
						}

						computerSession.ComputerName = computerName;

						if (!WWuString::IsNullOrEmpty(computerSession.UserName))
							sessionList.push_back(computerSession);
					}
				}
				break;
		}

		WTSFreeMemory(sessionInfo);
	}

	// Disconnect-Session
	WuResult TerminalServices::DisconnectSession(
		HANDLE hServer = WTS_CURRENT_SERVER_HANDLE,		// A handle to a WTS server session.
		DWORD sessionId = WTS_CURRENT_SESSION,			// The session to disconnect.
		BOOL wait = FALSE								// Wait for the logoff operation to finish.
	)
	{
		if (!WTSLogoffSession(hServer, sessionId, wait))
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		return WuResult();
	}

	/*========================================
	==		Utility function definition		==
	==========================================*/

	/*
	* Helper function to get extra WTS session information.
	* There is a known bug with 'WTSEnumerateSessionsEx', and 'WTSEnumerateSessions' does not bring all the information we want.
	*/
	WuResult GetSessionOutput(TerminalServices::PWU_COMPUTER_SESSION computerSession, HANDLE hServer, const WTS_SESSION_INFO& sessionInfo)
	{
		PWTSINFOW infoBuffer = NULL;
		DWORD bytesNeeded;

		if (!WTSQuerySessionInformationW(hServer, sessionInfo.SessionId, WTSSessionInfo, (LPWSTR*)&infoBuffer, &bytesNeeded) || NULL == infoBuffer)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		computerSession->SessionName = infoBuffer->WinStationName;
		computerSession->SessionId = sessionInfo.SessionId;
		computerSession->SessionState = sessionInfo.State;
		computerSession->LogonTime = infoBuffer->LogonTime;

		if (wcslen(infoBuffer->UserName) > 1)
		{
			wuunique_ha_ptr<WCHAR> unameBuff = make_wuunique_ha<WCHAR>(UNLEN * 2);
			DWORD bufferSize = UNLEN;

			if (FALSE == GetUserNameW(unameBuff.get(), &bufferSize))
				return WuResult(GetLastError(), __FILEW__, __LINE__);

			WWuString currentProcUser(unameBuff.get());

			if (currentProcUser == infoBuffer->UserName && hServer == WTS_CURRENT_SERVER_HANDLE)
				computerSession->LastInputTime.QuadPart = 0;
			else
				computerSession->LastInputTime = infoBuffer->LastInputTime;

			if (wcslen(infoBuffer->Domain) > 1)
				computerSession->UserName = WWuString::Format(L"%ws\\%ws", infoBuffer->Domain, infoBuffer->UserName);
			else
				computerSession->UserName = infoBuffer->UserName;
		}
		else
			computerSession->LastInputTime = infoBuffer->LastInputTime;

		if (NULL != infoBuffer)
			WTSFreeMemory(infoBuffer);

		return WuResult();
	}
}