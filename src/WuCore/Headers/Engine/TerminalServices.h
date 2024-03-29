#pragma once
#pragma unmanaged

#include "../Support/String.h"
#include "../Support/Expressions.h"

#include <WtsApi32.h>

namespace WindowsUtils::Core
{
	/*
	*	~ Get-ComputerSession
	*/

	typedef struct _WU_COMPUTER_SESSION
	{
		INT						SessionId;		// Session ID.
		WWuString				UserName;		// User name, on the format 'Domain\Username'.
		WWuString				SessionName;	// Session name.
		LARGE_INTEGER			LastInputTime;	// Last input time.
		LARGE_INTEGER			LogonTime;		// User logon time.
		WWuString				ComputerName;	// The computer name.
		WTS_CONNECTSTATE_CLASS	SessionState;	// Session state.

		_WU_COMPUTER_SESSION() { }
		_WU_COMPUTER_SESSION(INT sessionId, const WWuString& userName, const WWuString& sessionName, LARGE_INTEGER lastInpTime,
			LARGE_INTEGER logonTime, WTS_CONNECTSTATE_CLASS sessionState, const WWuString& computerName)
			: SessionId(sessionId), UserName(userName), SessionName(sessionName), LastInputTime(lastInpTime), LogonTime(logonTime), SessionState(sessionState), ComputerName(computerName)
		{ }

		~_WU_COMPUTER_SESSION() { }

	} WU_COMPUTER_SESSION, * PWU_COMPUTER_SESSION;

	/*
	*	~ Invoke-RemoteMessage
	*/

	typedef struct _WU_MESSAGE_RESPONSE
	{
	public:
		DWORD	SessionId;	// Session ID from where the response came.
		DWORD	Response;	// Response. Later mapped as a WindowsUtils::MessageBoxReturn.

		_WU_MESSAGE_RESPONSE()
			: SessionId(0), Response(0)
		{ }

		_WU_MESSAGE_RESPONSE(DWORD sessid, DWORD resp)
			: SessionId(sessid), Response(resp)
		{ }

	} WU_MESSAGE_RESPONSE, * PWU_MESSAGE_RESPONSE;

	/*
	*	~ Main class
	*/

	extern "C" public class __declspec(dllexport) TerminalServices
	{
	public:
		// Get-ComputerSession
		void GetEnumeratedSession(wuvector<WU_COMPUTER_SESSION>* sessionList, HANDLE session, BOOL onlyActive, BOOL includeSystemSessions);
		void GetEnumeratedSession(WWuString& computerName, wuvector<WU_COMPUTER_SESSION>& sessionList, bool onlyActive, bool includeSystemSessions);

		// Invoke-RemoteMessage
		void SendMessage(WWuString& pTitle, WWuString& pMessage, DWORD style, DWORD timeout, BOOL wait, wuvector<DWORD>* sessionIdList, wuvector<WU_MESSAGE_RESPONSE>* responseList, HANDLE session);
		void SendMessage(WWuString& pTitle, WWuString& pMessage, DWORD style, DWORD timeout, BOOL bWait, wuvector<WU_MESSAGE_RESPONSE>* responseList, HANDLE session);

		// Disconnect-Session
		void DisconnectSession(HANDLE session, DWORD sessionid, BOOL wait);
	};

	/*
	*	~ Utility functions
	*/

	void GetSessionOutput(PWU_COMPUTER_SESSION computerSession, HANDLE hServer, const WTS_SESSION_INFO& sessionInfo);
}