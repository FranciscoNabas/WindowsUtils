#include "pch.h"
#include "TerminalServices.h"

namespace WindowsUtils::Core
{
	/*========================================
	==		 Main function definition		==
	==========================================*/

	// Invoke-RemoteMessage
	DWORD TerminalServices::InvokeMessage(
		LPWSTR lptitle														// The message box title.
		, LPWSTR lpmessage													// The message text.
		, DWORD dwstyle														// A bitwise combination of UINT-defined messagebox styles (See function MessageBox).
		, DWORD dwtimeout													// Timeout, in seconds, for the function to wait a response.
		, BOOL bwait														// TRUE for the function wait a response. If FALSE, the function returns imediately with code 32001 (0x7D01), IDASYNC.
		, std::vector<DWORD>& rvecdwsessid									// The target session IDs.
		, std::vector<TerminalServices::WU_MESSAGE_RESPONSE>& rvecmessres	// A vector to receive the message responses.
		, HANDLE	hserversession = WTS_CURRENT_SERVER_HANDLE				// A handle to a WTS session.
	)
	{
		DWORD result = ERROR_SUCCESS;

		if (rvecdwsessid.empty())
		{
			DWORD dwsesscount = 0;
			PWTS_SESSION_INFOW pwtssessinfo;

			if (!::WTSEnumerateSessionsW(hserversession, 0, 1, &pwtssessinfo, &dwsesscount))
				return ::GetLastError();

			for (DWORD i = 0; i < dwsesscount; i++)
			{
				WU_MESSAGE_RESPONSE mrsingle;

				// Session 0 is not an interactive session
				if (pwtssessinfo[i].SessionId != 0)
				{
					if (!::WTSSendMessageW(
						hserversession
						, pwtssessinfo[i].SessionId
						, lptitle
						, (DWORD)wcslen(lptitle) * 2
						, lpmessage
						, (DWORD)wcslen(lpmessage) * 2
						, dwstyle
						, dwtimeout
						, &mrsingle.Response
						, bwait
					))
					{
						result = ::GetLastError();
						// We don't want to break if the session is not found, or cannot receive messages
						if (ERROR_FILE_NOT_FOUND != result)
							return result;
						else
							result = ERROR_SUCCESS;
					}

					mrsingle.SessionId = pwtssessinfo[i].SessionId;
					rvecmessres.push_back(mrsingle);
				}
			}
		}
		else
		{
			for (size_t i = 0; i < rvecdwsessid.size(); i++)
			{
				WU_MESSAGE_RESPONSE mrsingle;

				if (!::WTSSendMessageW(
					hserversession
					, rvecdwsessid.at(i)
					, lptitle
					, (DWORD)wcslen(lptitle) * 2
					, lpmessage
					, (DWORD)wcslen(lpmessage) * 2
					, dwstyle
					, dwtimeout
					, &mrsingle.Response
					, bwait
				))
				{
					result = ::GetLastError();
					// We don't want to break if the session is not found, or cannot receive messages
					if (ERROR_FILE_NOT_FOUND != result)
						return result;
					else
						result = ERROR_SUCCESS;
				}
				if (rvecdwsessid.at(i) == WTS_CURRENT_SESSION)
				{
					DWORD sessid;
					::ProcessIdToSessionId(GetCurrentProcessId(), &sessid);
					mrsingle.SessionId = sessid;
				}
				else
					mrsingle.SessionId = rvecdwsessid.at(i);
				
				rvecmessres.push_back(mrsingle);
			}
		}

		return result;
	}

	// Get-ComputerSession
	DWORD TerminalServices::GetEnumeratedSession(
		std::vector<TerminalServices::WU_COMPUTER_SESSION>& rveccompsess	// A vector with the output computer session data.
		, HANDLE hserversession = WTS_CURRENT_SERVER_HANDLE					// A handle to a WTS server session.
		, BOOL bonlyactive = FALSE											// Returns only sessions with SessionState = Active.
		, BOOL bincsystemsession = FALSE									// Includes sessions without an assigned user name.
	)
	{
		DWORD result = ERROR_SUCCESS;
		PWTS_SESSION_INFOW pwtssessinfo;
		DWORD dwsinfocount;

		if (!::WTSEnumerateSessionsW(hserversession, 0, 1, &pwtssessinfo, &dwsinfocount))
			return GetLastError();

		switch (bonlyactive)
		{
		case TRUE:
			for (DWORD i = 0; i < dwsinfocount; i++)
			{
				if (pwtssessinfo[i].State == WTSActive)
				{
					TerminalServices::WU_COMPUTER_SESSION compsessingle;
					result = GetSessionOutput(compsessingle, hserversession, pwtssessinfo[i]);
					if (ERROR_SUCCESS != result)
						goto FINALLY;

					rveccompsess.push_back(compsessingle);
				}
			}
			break;

			// 'System' sessions are never 'WTSActive'
		default:
			if (bincsystemsession == TRUE)
			{
				for (DWORD i = 0; i < dwsinfocount; i++)
				{
					TerminalServices::WU_COMPUTER_SESSION compsessingle;
					result = GetSessionOutput(compsessingle, hserversession, pwtssessinfo[i]);
					if (ERROR_SUCCESS != result)
						goto FINALLY;

					rveccompsess.push_back(compsessingle);
				}
			}
			else
			{
				for (DWORD i = 0; i < dwsinfocount; i++)
				{
					TerminalServices::WU_COMPUTER_SESSION compsessingle;
					result = GetSessionOutput(compsessingle, hserversession, pwtssessinfo[i]);
					if (ERROR_SUCCESS != result)
						goto FINALLY;

					if (compsessingle.UserName)
						rveccompsess.push_back(compsessingle);
				}
			}
			break;
		}

	FINALLY:

		WTSFreeMemory(pwtssessinfo);

		return result;

	}

	// Disconnect-Session
	DWORD TerminalServices::DisconnectSession(
		HANDLE hserversession = WTS_CURRENT_SERVER_HANDLE	// A handle to a WTS server session.
		, DWORD dwsessionid = WTS_CURRENT_SESSION			// The session to disconnect.
		, BOOL bwait = FALSE								// Wait for the logoff operation to finish.
	)
	{
		if (!::WTSLogoffSession(hserversession, dwsessionid, bwait))
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
	DWORD GetSessionOutput(TerminalServices::WU_COMPUTER_SESSION& rwucompsess, HANDLE hserversession, WTS_SESSION_INFO wtssessinfo)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD dwbytereturn;
		PWTSINFOW pwtssessex;

		if (!::WTSQuerySessionInformationW(hserversession, wtssessinfo.SessionId, WTSSessionInfo, (LPWSTR*)&pwtssessex, &dwbytereturn) || NULL == pwtssessex)
			return ::GetLastError();

		size_t szusrname = wcslen(pwtssessex->UserName) + 1;
		size_t szdomain = wcslen(pwtssessex->Domain) + 1;
		size_t szsessname = wcslen(pwtssessex->WinStationName) + 1;

		if (szsessname > 0)
		{
			rwucompsess.SessionName = new WCHAR[szsessname];
			wcscpy_s(rwucompsess.SessionName, szsessname, pwtssessex->WinStationName);
		}

		rwucompsess.SessionId = wtssessinfo.SessionId;
		rwucompsess.SessionState = wtssessinfo.State;
		rwucompsess.LogonTime = pwtssessex->LogonTime;

		if (szusrname > 1)
		{
			WCHAR currprocusername[1 << 10] = { 0 };
			DWORD szgetunbuff = 1 << 10;

			if (FALSE == GetUserNameW(currprocusername, &szgetunbuff))
				return ::GetLastError();

			if (wcscmp(currprocusername, pwtssessex->UserName) == 0 && hserversession == WTS_CURRENT_SERVER_HANDLE)
				rwucompsess.LastInputTime.QuadPart = 0;
			else
				rwucompsess.LastInputTime = pwtssessex->LastInputTime;

			if (szdomain > 1)
			{
				size_t sztotal = szdomain + szusrname;
				rwucompsess.UserName = new WCHAR[sztotal];
				wcscpy_s(rwucompsess.UserName, szdomain, pwtssessex->Domain);
				wcscat_s(rwucompsess.UserName, sztotal, L"\\");
				wcscat_s(rwucompsess.UserName, sztotal, pwtssessex->UserName);
			}
			else
			{
				rwucompsess.UserName = new WCHAR[szusrname];
				wcscpy_s(rwucompsess.UserName, szusrname, pwtssessex->UserName);
			}
		}
		else
		{
			rwucompsess.UserName = nullptr;
			rwucompsess.LastInputTime = pwtssessex->LastInputTime;
		}

		if (NULL != pwtssessex)
			::WTSFreeMemory(pwtssessex);

		return result;
	}
}