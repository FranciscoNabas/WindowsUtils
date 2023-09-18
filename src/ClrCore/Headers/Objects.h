#pragma once

#pragma unmanaged

#include "..\pch.h"

#include "Network.h"
#include "TerminalServices.h"

#pragma managed

using namespace System;

namespace WindowsUtils
{
	// Get-ComputerSession
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
		property Int32 SessionId { Int32 get() { return wrapper->SessionId; } }
		property String^ UserName { String^ get() { return gcnew String(wrapper->UserName.GetBuffer()); } }
		property String^ SessionName { String^ get() { return gcnew String(wrapper->SessionName.GetBuffer()); } }
		property TimeSpan IdleTime {
			TimeSpan get()
			{
				if (wrapper->LastInputTime.QuadPart == 0)
					return TimeSpan::Zero;
				else {
					::FILETIME datePivot;
					datePivot.dwLowDateTime = wrapper->LastInputTime.LowPart;
					datePivot.dwHighDateTime = wrapper->LastInputTime.HighPart;

				}

				return DateTime::Now - DateTime::FromFileTime(wrapper->LastInputTime.QuadPart);
			}
		}
		property DateTime^ LogonTime {
			DateTime^ get()
			{
				if (wrapper->LogonTime.QuadPart == 0)
					return nullptr;

				return DateTime::FromFileTime(wrapper->LogonTime.QuadPart);
			}
		}
		property SessionState State { SessionState get() { return static_cast<SessionState>(wrapper->SessionState); } }
		property String^ ComputerName { String^ get() { return gcnew String(wrapper->ComputerName.GetBuffer()); } }

		ComputerSession()
			: wrapper(new Core::TerminalServices::WU_COMPUTER_SESSION) { }

		ComputerSession(const Core::TerminalServices::WU_COMPUTER_SESSION& computerSession)
			: wrapper(new Core::TerminalServices::WU_COMPUTER_SESSION(computerSession)) { }
		
		~ComputerSession() { delete wrapper; }

	protected:
		!ComputerSession() { delete wrapper; }

	private:
		Core::TerminalServices::PWU_COMPUTER_SESSION wrapper;
	};

	// Get-ObjectHandle
	public enum class ObjectHandleType
	{
		FileSystem = 0,
		Registry = 1
	};

	// Get-NetworkFile
	[Flags]
	public enum class NetworkFilePermission
	{
		Read = PERM_FILE_READ,
		Write = PERM_FILE_WRITE,
		Create = PERM_FILE_CREATE,
		Execute = ACCESS_EXEC,
		Delete = ACCESS_DELETE,
		ChangeAttribute = ACCESS_ATRIB,
		ChangePermission = ACCESS_PERM
	};

	public ref class NetworkFileInfo
	{
	public:
		property Int32 Id { Int32 get() { return m_wrapper->Id; } }
		property String^ Path { String^ get() { return gcnew String(m_wrapper->Path.GetBuffer()); } }
		property NetworkFilePermission Permissions { NetworkFilePermission get() { return static_cast<NetworkFilePermission>(m_wrapper->Permissions); } }
		property Int32 LockCount { Int32 get() { return m_wrapper->LockCount; } }
		property String^ UserName { String^ get() { return gcnew String(m_wrapper->UserName.GetBuffer()); } }
		property String^ SessionName { String^ get() { return m_sessionName; } }
		property String^ ComputerName { String^ get() { return m_computerName; }}

		NetworkFileInfo(const Core::Network::NETWORK_FILE_INFO info, String^ computerName)
		{
			m_wrapper = new Core::Network::NETWORK_FILE_INFO(
				info.Id,
				info.Permissions,
				info.LockCount,
				info.Path,
				info.UserName
			);

			m_sessionName = nullptr;
			m_computerName = computerName;
		}

		NetworkFileInfo(const Core::Network::NETWORK_FILE_INFO info, const WWuString& sessionName, String^ computerName)
		{
			m_wrapper = new Core::Network::NETWORK_FILE_INFO(
				info.Id,
				info.Permissions,
				info.LockCount,
				info.Path,
				info.UserName
			);

			m_sessionName = gcnew String(sessionName.GetBuffer());
			m_computerName = computerName;
		}

		~NetworkFileInfo() { delete m_wrapper; }

	protected:
		!NetworkFileInfo() { delete m_wrapper; }

	private:
		Core::Network::PNETWORK_FILE_INFO m_wrapper;
		String^ m_sessionName;
		String^ m_computerName;
	};
}