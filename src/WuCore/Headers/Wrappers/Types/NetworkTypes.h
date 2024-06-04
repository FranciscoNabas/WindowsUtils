#pragma once
#pragma unmanaged

#include <LMShare.h>
#include <LMAccess.h>

#include "../../Engine/Network.h"

#pragma managed

namespace WindowsUtils
{
	using namespace System;

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

	public enum class PortStatus
	{
		Open,
		Closed,
		Timeout
	};

	public enum class TransportProtocol
	{
		Tcp,
		Udp
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

	public ref class TcpingProbeInfo
	{
	public:
		property String^ Destination { String^ get() { return gcnew String(m_wrapper->Destination); } }
		property String^ ResolvedAddress { String^ get() { return gcnew String(m_wrapper->DestAddress); } }
		property DateTime^ Timestamp {
			DateTime^ get()
			{
				FILETIME fileTime = m_wrapper->Timestamp;
				__int64 timeQuadPart = ULARGE_INTEGER { fileTime.dwLowDateTime, fileTime.dwHighDateTime }.QuadPart;
				return DateTime::FromFileTime(timeQuadPart);
			}
		}
		property UInt32 Port { UInt32 get() { return m_wrapper->Port; } }
		property PortStatus Status { PortStatus get() { return static_cast<PortStatus>(m_wrapper->Status); } }
		property Double RoundTripTime { Double get() { return m_wrapper->RoundTripTime; } }
		property Double Jitter { Double get() { return m_wrapper->Jitter; } }

		TcpingProbeInfo(const Core::TCPING_OUTPUT& info) { m_wrapper = new Core::TCPING_OUTPUT(info); }
		~TcpingProbeInfo() { delete m_wrapper; }

	protected:
		!TcpingProbeInfo() { delete m_wrapper; }

	private:
		Core::PTCPING_OUTPUT m_wrapper;
	};

	public ref class TcpingStatistics
	{
	public:
		property UInt32 Sent { UInt32 get() { return m_wrapper->Sent; } }
		property UInt32 Succeeded { UInt32 get() { return m_wrapper->Successful; } }
		property UInt32 Failed { UInt32 get() { return m_wrapper->Failed; } }
		property Double MinRtt { Double get() { return m_wrapper->MinRtt; } }
		property Double MaxRtt { Double get() { return m_wrapper->MaxRtt; } }
		property Double AvgRtt { Double get() { return m_wrapper->AvgRtt; } }
		property Double MinJitter { Double get() { return m_wrapper->MinJitter; } }
		property Double MaxJitter { Double get() { return m_wrapper->MaxJitter; } }
		property Double AvgJitter { Double get() { return m_wrapper->AvgJitter; } }
		property Double TotalMilliseconds { Double get() { return m_wrapper->TotalMilliseconds; } }
		property Double TotalJitter { Double get() { return m_wrapper->TotalJitter; } }

		TcpingStatistics(const Core::TCPING_STATISTICS& info) { m_wrapper = new Core::TCPING_STATISTICS(info); }
		~TcpingStatistics() { delete m_wrapper; }

	protected:
		!TcpingStatistics() { delete m_wrapper; }

	private:
		Core::PTCPING_STATISTICS m_wrapper;
	};

	public ref class TestPortInfo
	{
	public:
		property String^ Destination { String^ get() { return gcnew String(m_wrapper->Destination); } }
		property String^ Address { String^ get() { return gcnew String(m_wrapper->DestAddress); } }
		property DateTime^ Timestamp {
			DateTime^ get()
			{
				FILETIME fileTime = m_wrapper->Timestamp;
				__int64 timeQuadPart = ULARGE_INTEGER { fileTime.dwLowDateTime, fileTime.dwHighDateTime }.QuadPart;
				return DateTime::FromFileTime(timeQuadPart);
			}
		}
		property UInt32 Port { UInt32 get() { return m_wrapper->Port; } }
		property PortStatus Status { PortStatus get() { return static_cast<PortStatus>(m_wrapper->Status); } }

		TestPortInfo(const Core::TESTPORT_OUTPUT& info) { m_wrapper = new Core::TESTPORT_OUTPUT(info); }
		~TestPortInfo() { delete m_wrapper; }

	protected:
		!TestPortInfo() { delete m_wrapper; }

	private:
		Core::PTESTPORT_OUTPUT m_wrapper;
	};
}