#pragma once
#pragma unmanaged

#include "../../Engine/TerminalServices.h"

#pragma managed

/*
*	Important note about types:
*
*	PowerShell for some reason lists the properties in the reverse order than declared here.
*	An easy way to deal with it is to declare properties 'upside-down'.
*	This avoids us having to mess with 'format'/'types' files.
*
*	For classes with inheritance, it seems to list first the child properties, then
*	the parent ones.
*
*	Remember: what matters is what shows up to the user.
*/

namespace WindowsUtils
{
	using namespace System;

	public enum class SessionState
	{
		Active,
		Connected,
		ConnectQuery,
		Shadow,
		Disconnected,
		Idle,
		Listen,
		Reset,
		Down,
		Init
	};

	public ref class ComputerSession
	{
	public:
		property Int32 SessionId { Int32 get() { return m_wrapper->SessionId; } }
		property String^ UserName { String^ get() { return gcnew String(m_wrapper->UserName.Raw()); } }
		property String^ SessionName { String^ get() { return gcnew String(m_wrapper->SessionName.Raw()); } }
		property TimeSpan IdleTime {
			TimeSpan get()
			{
				if (m_wrapper->LastInputTime.QuadPart == 0)
					return TimeSpan::Zero;
				else {
					::FILETIME datePivot;
					datePivot.dwLowDateTime = m_wrapper->LastInputTime.LowPart;
					datePivot.dwHighDateTime = m_wrapper->LastInputTime.HighPart;

				}

				return DateTime::Now - DateTime::FromFileTime(m_wrapper->LastInputTime.QuadPart);
			}
		}
		property DateTime^ LogonTime {
			DateTime^ get()
			{
				if (m_wrapper->LogonTime.QuadPart == 0)
					return nullptr;

				return DateTime::FromFileTime(m_wrapper->LogonTime.QuadPart);
			}
		}
		property SessionState State { SessionState get() { return static_cast<SessionState>(m_wrapper->SessionState); } }
		property String^ ComputerName { String^ get() { return gcnew String(m_wrapper->ComputerName.Raw()); } }

		ComputerSession()
			: m_wrapper(new Core::WU_COMPUTER_SESSION)
		{ }

		ComputerSession(const Core::WU_COMPUTER_SESSION& computerSession)
			: m_wrapper(new Core::WU_COMPUTER_SESSION(computerSession))
		{ }

		~ComputerSession() { delete m_wrapper; }

	protected:
		!ComputerSession() { delete m_wrapper; }

	private:
		Core::PWU_COMPUTER_SESSION m_wrapper;
	};

	public ref class MessageResponse
	{
	public:
		property UInt32 SessionId { UInt32 get() { return m_wrapper->SessionId; } }
		property UInt32 Response { UInt32 get() { return m_wrapper->Response; } }

		MessageResponse()
			: m_wrapper { new Core::WU_MESSAGE_RESPONSE }
		{ }

		MessageResponse(const Core::WU_MESSAGE_RESPONSE& messageResponse)
			: m_wrapper { new Core::WU_MESSAGE_RESPONSE { messageResponse.SessionId, messageResponse.Response } }
		{ }

		~MessageResponse() { delete m_wrapper; }

	protected:
		!MessageResponse() { delete m_wrapper; }

	private:
		Core::PWU_MESSAGE_RESPONSE m_wrapper;
	};
}