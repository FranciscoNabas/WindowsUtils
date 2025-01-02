#include "../../pch.h"

#include "../../Headers/Engine/TerminalServices.h"

namespace WindowsUtils::Core
{
	/*
	*	~ WU_COMPUTER_SESSION ~
	*/

	_WU_COMPUTER_SESSION::_WU_COMPUTER_SESSION() { }

	_WU_COMPUTER_SESSION::_WU_COMPUTER_SESSION(INT sessionId, const WWuString& userName, const WWuString& sessionName, LARGE_INTEGER lastInpTime,
		LARGE_INTEGER logonTime, WTS_CONNECTSTATE_CLASS sessionState, const WWuString& computerName)
		: SessionId(sessionId), UserName(userName), SessionName(sessionName), LastInputTime(lastInpTime), LogonTime(logonTime), SessionState(sessionState), ComputerName(computerName) { }


	/*
	*	~ WU_MESSAGE_RESPONSE ~
	*/

	_WU_MESSAGE_RESPONSE::_WU_MESSAGE_RESPONSE()
		: SessionId(0), Response(0) { }

	_WU_MESSAGE_RESPONSE::_WU_MESSAGE_RESPONSE(DWORD sessid, DWORD resp)
		: SessionId(sessid), Response(resp) { }


	// Invoke-RemoteMessage
	void TerminalServices::SendMessage(
		WWuString& title,									// The message box title.
		WWuString& message,									// The message text.
		DWORD style,										// A bitwise-or combination of UINT-defined MessageBox styles (See function MessageBox).
		DWORD timeout,										// Timeout, in seconds, for the function to wait a response.
		BOOL wait,											// TRUE for the function wait a response. If FALSE, the function returns immediately with code 32001 (0x7D01), IDASYNC.
		std::vector<DWORD>& sessionIdList,					// The target session IDs.
		std::vector<WU_MESSAGE_RESPONSE>& responseList,		// A vector to receive the message responses.
		HANDLE hServer										// A handle to a WTS session.
	)
	{
		DWORD dwResult { ERROR_SUCCESS };

		for (const DWORD dwSession : sessionIdList) {
			WU_MESSAGE_RESPONSE response;

			if (!WTSSendMessageW(
				hServer,
				dwSession,
				title.Raw(),
				static_cast<DWORD>(title.Length() * 2),
				message.Raw(),
				static_cast<DWORD>(message.Length() * 2),
				style,
				timeout,
				&response.Response,
				wait
			)) {
				dwResult = GetLastError();

				// We don't want to break if the session is not found, or cannot receive messages
				if (dwResult != ERROR_FILE_NOT_FOUND)
					_WU_RAISE_NATIVE_EXCEPTION(dwResult, L"WTSSendMessageW", WriteErrorCategory::ProtocolError);
				else
					dwResult = ERROR_SUCCESS;
			}
			if (dwSession == WTS_CURRENT_SESSION) {
				DWORD sessid;
				ProcessIdToSessionId(GetCurrentProcessId(), &sessid);
				response.SessionId = sessid;
			}
			else
				response.SessionId = dwSession;

			responseList.push_back(response);
		}
	}

	void TerminalServices::SendMessage(
		WWuString& title,									// The message box title.
		WWuString& message,									// The message text.
		DWORD style,										// A bitwise-or combination of UINT-defined MessageBox styles (See function MessageBox).
		DWORD timeout,										// Timeout, in seconds, for the function to wait a response.
		BOOL wait,											// TRUE for the function wait a response. If FALSE, the function returns immediately with code 32001 (0x7D01), IDASYNC.
		std::vector<WU_MESSAGE_RESPONSE>& responseList,		// A vector to receive the message responses.
		HANDLE hServer										// A handle to a WTS session.
	)
	{
		DWORD dwResult { ERROR_SUCCESS };
		DWORD sessionCount { };
		PWTS_SESSION_INFOW sessionInfo;

		if (!WTSEnumerateSessionsW(hServer, 0, 1, &sessionInfo, &sessionCount))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"WTSEnumerateSessionsW", WriteErrorCategory::ProtocolError);

		for (DWORD i = 0; i < sessionCount; i++) {
			WU_MESSAGE_RESPONSE response;

			// Session 0 is not an interactive session
			if (sessionInfo[i].SessionId != 0) {
				if (!WTSSendMessageW(
					hServer,
					sessionInfo[i].SessionId,
					title.Raw(),
					static_cast<DWORD>(title.Length() * 2),
					message.Raw(),
					static_cast<DWORD>(message.Length() * 2),
					style,
					timeout,
					&response.Response,
					wait
				)) {
					dwResult = GetLastError();

					// We don't want to break if the session is not found, or cannot receive messages
					if (dwResult != ERROR_FILE_NOT_FOUND) {
						WTSFreeMemory(sessionInfo);
						_WU_RAISE_NATIVE_EXCEPTION(dwResult, L"WTSSendMessageW", WriteErrorCategory::ProtocolError);
					}
					else
						dwResult = ERROR_SUCCESS;
				}

				response.SessionId = sessionInfo[i].SessionId;
				responseList.push_back(response);
			}
		}

		WTSFreeMemory(sessionInfo);
	}

	// Get-ComputerSession
	void TerminalServices::GetEnumeratedSession(
		std::vector<WU_COMPUTER_SESSION>& sessionInfoList,	// A vector with the output computer session data.
		HANDLE hServer = WTS_CURRENT_SERVER_HANDLE,			// A handle to a WTS server session.
		BOOL activeOnly = FALSE,							// Returns only sessions with SessionState = Active.
		BOOL includeSystemSession = FALSE					// Includes sessions without an assigned user name.
	)
	{
		DWORD dwResult { ERROR_SUCCESS };
		PWTS_SESSION_INFOW sessionInfo;
		DWORD sessionCount;

		if (!WTSEnumerateSessionsW(hServer, 0, 1, &sessionInfo, &sessionCount))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"WTSEnumerateSessionsW", WriteErrorCategory::ProtocolError);

		switch (activeOnly) {
			case TRUE:
				for (DWORD i = 0; i < sessionCount; i++) {
					if (sessionInfo[i].State == WTSActive) {
						WU_COMPUTER_SESSION computerSession;
						try {
							GetSessionOutput(computerSession, hServer, sessionInfo[i]);
						}
						catch (const WuNativeException& ex) {
							dwResult = ex.ErrorCode();
							goto CLEANUP;
						}

						sessionInfoList.push_back(computerSession);
					}
				}
				break;

				// 'System' sessions are never 'WTSActive'
			default:
				if (includeSystemSession == TRUE) {
					for (DWORD i = 0; i < sessionCount; i++) {
						WU_COMPUTER_SESSION computerSession;
						try {
							GetSessionOutput(computerSession, hServer, sessionInfo[i]);
						}
						catch (const WuNativeException& ex) {
							dwResult = ex.ErrorCode();
							goto CLEANUP;
						}

						sessionInfoList.push_back(computerSession);
					}
				}
				else {
					for (DWORD i = 0; i < sessionCount; i++) {
						WU_COMPUTER_SESSION computerSession;
						try {
							GetSessionOutput(computerSession, hServer, sessionInfo[i]);
						}
						catch (const WuNativeException& ex) {
							dwResult = ex.ErrorCode();
							goto CLEANUP;
						}

						if (!WWuString::IsNullOrEmpty(computerSession.UserName))
							sessionInfoList.push_back(computerSession);
					}
				}
				break;
		}

	CLEANUP:

		WTSFreeMemory(sessionInfo);

		if (dwResult != ERROR_SUCCESS)
			_WU_RAISE_NATIVE_EXCEPTION(dwResult, L"GetSessionOutput", WriteErrorCategory::InvalidResult);
	}

	void TerminalServices::GetEnumeratedSession(WWuString& computerName, std::vector<WU_COMPUTER_SESSION>& sessionList, bool onlyActive, bool includeSystemSessions)
	{
		HANDLE hServer;
		PWTS_SESSION_INFOW sessionInfo;
		DWORD sessionCount;

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
			hServer = WTSOpenServer(computerName.Raw());
		}

		if (!WTSEnumerateSessionsW(hServer, 0, 1, &sessionInfo, &sessionCount))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"WTSEnumerateSessionsW", WriteErrorCategory::ProtocolError);

		switch (onlyActive) {
			case true:
				for (DWORD i = 0; i < sessionCount; i++) {
					if (sessionInfo[i].State == WTSActive) {
						WU_COMPUTER_SESSION computerSession;
						try {
							GetSessionOutput(computerSession, hServer, sessionInfo[i]);
						}
						catch (const WuNativeException& ex) {
							WTSFreeMemory(sessionInfo);
							throw ex;
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
						WU_COMPUTER_SESSION computerSession;

						try {
							GetSessionOutput(computerSession, hServer, sessionInfo[i]);
						}
						catch (const WuNativeException& ex) {
							WTSFreeMemory(sessionInfo);
							throw ex;
						}

						computerSession.ComputerName = computerName;

						sessionList.push_back(computerSession);
					}
				}
				else {
					for (DWORD i = 0; i < sessionCount; i++) {
						WU_COMPUTER_SESSION computerSession;

						try {
							GetSessionOutput(computerSession, hServer, sessionInfo[i]);
						}
						catch (const WuNativeException& ex) {
							WTSFreeMemory(sessionInfo);
							throw ex;
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
	void TerminalServices::DisconnectSession(
		HANDLE hServer = WTS_CURRENT_SERVER_HANDLE,		// A handle to a WTS server session.
		DWORD sessionId = WTS_CURRENT_SESSION,			// The session to disconnect.
		BOOL wait = FALSE								// Wait for the logoff operation to finish.
	)
	{
		if (!WTSLogoffSession(hServer, sessionId, wait))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"WTSLogoffSession", WriteErrorCategory::ProtocolError);
	}

	/*========================================
	==		Utility function definition		==
	==========================================*/

	/*
	* Helper function to get extra WTS session information.
	* There is a known bug with 'WTSEnumerateSessionsEx', and 'WTSEnumerateSessions' does not bring all the information we want.
	*/
	void TerminalServices::GetSessionOutput(WU_COMPUTER_SESSION& computerSession, HANDLE hServer, const WTS_SESSION_INFO& sessionInfo)
	{
		PWTSINFOW infoBuffer = NULL;
		DWORD bytesNeeded;

		if (!WTSQuerySessionInformationW(hServer, sessionInfo.SessionId, WTSSessionInfo, (LPWSTR*)&infoBuffer, &bytesNeeded) || NULL == infoBuffer)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"WTSQuerySessionInformationW", WriteErrorCategory::ProtocolError);

		computerSession.SessionName = infoBuffer->WinStationName;
		computerSession.SessionId = sessionInfo.SessionId;
		computerSession.SessionState = sessionInfo.State;
		computerSession.LogonTime = infoBuffer->LogonTime;

		if (wcslen(infoBuffer->UserName) > 1) {
			std::unique_ptr<WCHAR[]> unameBuff = std::make_unique<WCHAR[]>(UNLEN);
			DWORD bufferSize = UNLEN;

			if (!GetUserNameW(unameBuff.get(), &bufferSize))
				_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetUserName", WriteErrorCategory::InvalidResult);

			WWuString currentProcUser(unameBuff.get());

			if (currentProcUser == infoBuffer->UserName && hServer == WTS_CURRENT_SERVER_HANDLE)
				computerSession.LastInputTime.QuadPart = 0;
			else
				computerSession.LastInputTime = infoBuffer->LastInputTime;

			if (wcslen(infoBuffer->Domain) > 1)
				computerSession.UserName = WWuString::Format(L"%ws\\%ws", infoBuffer->Domain, infoBuffer->UserName);
			else
				computerSession.UserName = infoBuffer->UserName;
		}
		else
			computerSession.LastInputTime = infoBuffer->LastInputTime;

		if (NULL != infoBuffer)
			WTSFreeMemory(infoBuffer);
	}
}