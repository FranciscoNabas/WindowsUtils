#pragma once
#pragma unmanaged

#include "pch.h"

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) TerminalServices
	{
	public:
		/*========================================
		==		Unmanaged object definition		==
		==========================================*/

		// Get-ComputerSession
		typedef struct _WU_COMPUTER_SESSION
		{
			INT						SessionId;		// Session ID.
			LPWSTR					UserName;		// User name, on the format 'Domain\Username'.
			LPWSTR					SessionName;	// Session name.
			LARGE_INTEGER			LastInputTime;	// Last input time.
			LARGE_INTEGER			LogonTime;		// User logon time.
			WTS_CONNECTSTATE_CLASS	SessionState;	// Session state.

			_WU_COMPUTER_SESSION() { }
			_WU_COMPUTER_SESSION(INT sessid, LPWSTR usrname, LPWSTR sessname, LARGE_INTEGER linptime, LARGE_INTEGER lgtime, WTS_CONNECTSTATE_CLASS sesstate)
				: SessionId(sessid), UserName(usrname), SessionName(sessname), LastInputTime(linptime), LogonTime(lgtime), SessionState(sesstate) { }
		}WU_COMPUTER_SESSION, * PWU_COMPUTER_SESSION;

		// Invoke-RemoteMessage
		typedef struct _WU_MESSAGE_RESPONSE
		{
		public:
			DWORD	SessionId;	// Session ID from where the response came.
			DWORD	Response;	// Response. Later mapped as a WindowsUtils::MessageBoxReturn.

			_WU_MESSAGE_RESPONSE() { }
			_WU_MESSAGE_RESPONSE(DWORD sessid, DWORD resp) : SessionId(sessid), Response(resp) { }
		}WU_MESSAGE_RESPONSE, * PWU_MESSAGE_RESPONSE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Get-ComputerSession
		DWORD GetEnumeratedSession(std::vector<WU_COMPUTER_SESSION>& ppOutVec, HANDLE session, BOOL onlyActive, BOOL includeSystemSessions);

		// Invoke-RemoteMessage
		DWORD SendMessage(const LPWSTR& pTitle, const LPWSTR& pMessage, DWORD const& style, DWORD const& timeout, BOOL const& bWait, std::vector<DWORD>& sessionId, std::vector<WU_MESSAGE_RESPONSE>& pvecres, HANDLE const& session);
		DWORD SendMessage(const LPWSTR& pTitle, const LPWSTR& pMessage, DWORD const& style, DWORD const& timeout, BOOL const& bWait, std::vector<WU_MESSAGE_RESPONSE>& pvecres, HANDLE const& session);

		// Disconnect-Session
		DWORD DisconnectSession(HANDLE session, DWORD sessionid, BOOL wait);
	};

	/*========================================
	==	  Internal function identifiers		==
	==========================================*/

	// Helper fuction used with GetEnumeratedSession.
	DWORD GetSessionOutput(TerminalServices::WU_COMPUTER_SESSION& rwucompsess, HANDLE hserversession, WTS_SESSION_INFO wtssessinfo);
}