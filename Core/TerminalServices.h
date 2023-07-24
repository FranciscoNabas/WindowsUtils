#pragma once
#pragma unmanaged

#include "String.h"
#include "Expressions.h"

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
			WuString				UserName;		// User name, on the format 'Domain\Username'.
			WuString				SessionName;	// Session name.
			LARGE_INTEGER			LastInputTime;	// Last input time.
			LARGE_INTEGER			LogonTime;		// User logon time.
			WTS_CONNECTSTATE_CLASS	SessionState;	// Session state.

			_WU_COMPUTER_SESSION() { }
			_WU_COMPUTER_SESSION(INT sessionId, const WuString& userName, const WuString& sessionName, LARGE_INTEGER lastInpTime, LARGE_INTEGER logonTime, WTS_CONNECTSTATE_CLASS sessionState)
				: SessionId(sessionId), UserName(userName), SessionName(sessionName), LastInputTime(lastInpTime), LogonTime(logonTime), SessionState(sessionState)
			{ }

			~_WU_COMPUTER_SESSION() { }

		} WU_COMPUTER_SESSION, *PWU_COMPUTER_SESSION;

		// Invoke-RemoteMessage
		typedef struct _WU_MESSAGE_RESPONSE
		{
		public:
			DWORD	SessionId;	// Session ID from where the response came.
			DWORD	Response;	// Response. Later mapped as a WindowsUtils::MessageBoxReturn.

			_WU_MESSAGE_RESPONSE()
				: SessionId(0), Response(0) { }
			
			_WU_MESSAGE_RESPONSE(DWORD sessid, DWORD resp)
				: SessionId(sessid), Response(resp) { }

		} WU_MESSAGE_RESPONSE, *PWU_MESSAGE_RESPONSE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Get-ComputerSession
		DWORD GetEnumeratedSession(wuvector<WU_COMPUTER_SESSION>* sessionList, HANDLE session, BOOL onlyActive, BOOL includeSystemSessions);

		// Invoke-RemoteMessage
		DWORD SendMessage(const WuString& pTitle, const WuString& pMessage, DWORD style, DWORD timeout, BOOL wait, wuvector<DWORD>* sessionIdList, wuvector<WU_MESSAGE_RESPONSE>* responseList, HANDLE session);
		DWORD SendMessage(const WuString& pTitle, const WuString& pMessage, DWORD style, DWORD timeout, BOOL bWait, wuvector<WU_MESSAGE_RESPONSE>* responseList, HANDLE session);

		// Disconnect-Session
		DWORD DisconnectSession(HANDLE session, DWORD sessionid, BOOL wait);
	};

	/*========================================
	==	  Internal function identifiers		==
	==========================================*/

	// Helper fuction used with GetEnumeratedSession.
	DWORD GetSessionOutput(TerminalServices::PWU_COMPUTER_SESSION computerSession, HANDLE hServer, const WTS_SESSION_INFO& sessionInfo);
}