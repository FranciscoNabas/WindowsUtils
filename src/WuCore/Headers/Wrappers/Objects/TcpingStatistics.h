#pragma once
#pragma unmanaged

#include "../../Engine/Network.h"

#pragma managed

namespace WindowsUtils
{
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
}