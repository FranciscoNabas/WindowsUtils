#pragma once
#pragma unmanaged

#include "../../Engine/TerminalServices.h"

#pragma managed

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
		property String^ UserName { String^ get() { return gcnew String(m_wrapper->UserName.GetBuffer()); } }
		property String^ SessionName { String^ get() { return gcnew String(m_wrapper->SessionName.GetBuffer()); } }
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
		property String^ ComputerName { String^ get() { return gcnew String(m_wrapper->ComputerName.GetBuffer()); } }

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