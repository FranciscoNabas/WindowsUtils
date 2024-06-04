#pragma once
#pragma managed

#include "CmdletContextProxy.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;
	using namespace System::Runtime::InteropServices;

	public ref class NetworkWrapper
	{
	public:
		// Start-Tcping
		void StartTcpPing(String^ destination, Int32 port, Int32 count, Int32 timeout, Int32 interval, Int32 failThreshold, bool continuous,
			bool jitter, bool fqdn, bool force, bool single, String^ outFile, bool append, Core::CmdletContextProxy^ context, [Out] bool% isCancel);

		// Get-NetworkFile
		List<NetworkFileInfo^>^ GetNetworkFile(String^ computerName, String^ basePath, String^ userName, bool includeSessionName);

		// Close-NetworkFile
		void CloseNetworkFile(String^ computerName, Int32 fileId);

		// Test-Port
		void TestNetworkPort(String^ destination, UInt32 port, TransportProtocol protocol, UInt32 timeout, Core::CmdletContextProxy^ context);

	private:
		Core::Network* m_ntw;
	};
}