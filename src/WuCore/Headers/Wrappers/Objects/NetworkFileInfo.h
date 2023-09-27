#pragma once
#pragma unmanaged

#include "../../Engine/Network.h"

#include <LMShare.h>
#include <LMAccess.h>

#pragma managed

namespace WindowsUtils
{
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

		NetworkFileInfo(const Core::NETWORK_FILE_INFO info, String^ computerName)
		{
			m_wrapper = new Core::NETWORK_FILE_INFO(
				info.Id,
				info.Permissions,
				info.LockCount,
				info.Path,
				info.UserName
			);

			m_sessionName = nullptr;
			m_computerName = computerName;
		}

		NetworkFileInfo(const Core::NETWORK_FILE_INFO info, const WWuString& sessionName, String^ computerName)
		{
			m_wrapper = new Core::NETWORK_FILE_INFO(
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
		Core::PNETWORK_FILE_INFO m_wrapper;
		String^ m_sessionName;
		String^ m_computerName;
	};
}