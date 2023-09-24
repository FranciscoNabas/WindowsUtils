#pragma once
#pragma managed

#include "TcpingProbeInfo.h"

namespace WindowsUtils
{
	public enum class TransportProtocol
	{
		Tcp,
		Udp
	};

	public ref class TestPortInfo
	{
	public:
		property String^ Destination { String^ get() { return gcnew String(m_wrapper->Destination); } }
		property String^ Address { String^ get() { return gcnew String(m_wrapper->DestAddress); } }
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

		TestPortInfo(const Core::TESTPORT_OUTPUT& info) { m_wrapper = new Core::TESTPORT_OUTPUT(info); }
		~TestPortInfo() { delete m_wrapper; }

	protected:
		!TestPortInfo() { delete m_wrapper; }

	private:
		Core::PTESTPORT_OUTPUT m_wrapper;
	};
}