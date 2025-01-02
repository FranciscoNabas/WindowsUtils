#pragma once
#pragma unmanaged

#include "../Support/WuString.h"
#include "../Support/Expressions.h"
#include "../Support/WuException.h"

#include <WtsApi32.h>
#include <vector>
#include <memory>

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

		_WU_COMPUTER_SESSION();
		_WU_COMPUTER_SESSION(INT sessionId, const WWuString& userName, const WWuString& sessionName, LARGE_INTEGER lastInpTime,
			LARGE_INTEGER logonTime, WTS_CONNECTSTATE_CLASS sessionState, const WWuString& computerName);

	} WU_COMPUTER_SESSION, * PWU_COMPUTER_SESSION;


	/*
	*	~ Invoke-RemoteMessage
	*/

	typedef struct _WU_MESSAGE_RESPONSE
	{
	public:
		DWORD	SessionId;	// Session ID from where the response came.
		DWORD	Response;	// Response. Later mapped as a WindowsUtils::MessageBoxReturn.

		_WU_MESSAGE_RESPONSE();
		_WU_MESSAGE_RESPONSE(DWORD sessid, DWORD resp);

	} WU_MESSAGE_RESPONSE, * PWU_MESSAGE_RESPONSE;


	/*
	*	~ Main class
	*/

	class TerminalServices
	{
	public:
		// Get-ComputerSession
		static void GetEnumeratedSession(std::vector<WU_COMPUTER_SESSION>& sessionList, HANDLE session, BOOL onlyActive, BOOL includeSystemSessions);
		static void GetEnumeratedSession(WWuString& computerName, std::vector<WU_COMPUTER_SESSION>& sessionList, bool onlyActive, bool includeSystemSessions);

		// Invoke-RemoteMessage
		static void SendMessage(WWuString& pTitle, WWuString& pMessage, DWORD style, DWORD timeout, BOOL wait, std::vector<DWORD>& sessionIdList, std::vector<WU_MESSAGE_RESPONSE>& responseList, HANDLE session);
		static void SendMessage(WWuString& pTitle, WWuString& pMessage, DWORD style, DWORD timeout, BOOL bWait, std::vector<WU_MESSAGE_RESPONSE>& responseList, HANDLE session);

		// Disconnect-Session
		static void DisconnectSession(HANDLE session, DWORD sessionid, BOOL wait);

	private:
		static void GetSessionOutput(WU_COMPUTER_SESSION& computerSession, HANDLE hServer, const WTS_SESSION_INFO& sessionInfo);
	};
}