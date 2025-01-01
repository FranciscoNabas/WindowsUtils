#pragma once
#pragma unmanaged

#include "../Support/Nt/NtUtilities.h"
#include "../Stubs/NetworkStub.h"
#include "../Stubs/ProcessAndThreadStub.h"

#pragma managed

#include "WrapperBase.h"
#include "NativeException.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;
	using namespace System::Management::Automation;
	using namespace System::Runtime::InteropServices;
	using namespace WindowsUtils::Network;

	public ref class NetworkWrapper : public WrapperBase
	{
	public:
		NetworkWrapper(Core::CmdletContextProxy^ context)
			: WrapperBase(context) { }

		// Start-Tcping
		void StartTcpPing(String^ destination, Int32 port, Int32 count, Int32 timeout, Int32 interval, Int32 failThreshold, bool continuous,
			bool jitter, bool fqdn, bool force, bool single, String^ outFile, bool append, [Out] bool% isCancel);

		// Get-NetworkFile
		List<NetworkFileInfo^>^ GetNetworkFile(String^ computerName, String^ basePath, String^ userName, bool includeSessionName);

		// Close-NetworkFile
		void CloseNetworkFile(String^ computerName, int fileId);

		// Test-Port
		void TestNetworkPort(String^ destination, UInt32 port, TransportProtocol protocol, UInt32 timeout);

		// Get-NetworkStatistics
		void GetIpRouteTable();
		void GetTransportTables(bool all, bool includeModuleName);
		void GetInterfaceStatistics();
		void GetInterfaceStatistics([Out] List<InterfaceStatisticsSingle^>^ output);
		void GetIpv4Statistics([Out] List<IpStatistics^>^ output);
		void GetIpv6Statistics([Out] List<IpStatistics^>^ output);
		void GetIcmpv4Statistics([Out] List<IcmpStatistics^>^ output);
		void GetIcmpv6Statistics([Out] List<IcmpStatistics^>^ output);
		void GetTcpv4Statistics([Out] List<TcpStatistics^>^ output);
		void GetTcpv6Statistics([Out] List<TcpStatistics^>^ output);
		void GetUdpv4Statistics([Out] List<UdpStatistics^>^ output);
		void GetUdpv6Statistics([Out] List<UdpStatistics^>^ output);
	};
}