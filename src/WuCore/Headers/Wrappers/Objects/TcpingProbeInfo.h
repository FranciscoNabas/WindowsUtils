#pragma once
#pragma unmanaged

#include "../../Engine/Network.h"

#pragma managed

namespace WindowsUtils
{
	public enum class PortStatus
	{
		Open,
		Closed,
		Timeout
	};

	public ref class TcpingProbeInfo
	{
	public:
		property String^ Destination { String^ get() { return gcnew String(m_wrapper->Destination); } }
		property String^ ResolvedAddress { String^ get() { return gcnew String(m_wrapper->DestAddress); } }
		property DateTime^ Timestamp {
			DateTime^ get()
			{
				::FILETIME fileTime = m_wrapper->Timestamp;
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
}