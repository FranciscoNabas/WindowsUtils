#pragma unmanaged

#include "../../Headers/Support/WuException.h"

#pragma managed

#include "../../Headers/Wrappers/NetworkWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	// Start-Tcping
	void NetworkWrapper::StartTcpPing(String^ destination, Int32 port, Int32 count, Int32 timeout, Int32 interval, Int32 failThreshold, bool continuous,
		bool jitter, bool fqdn, bool force, bool single, String^ outFile, bool append, [Out] bool% isCancel)
	{
		bool isFile = false;
		WWuString wrappedOutFile;
		if (!String::IsNullOrEmpty(outFile)) {
			wrappedOutFile = UtilitiesWrapper::GetWideStringFromSystemString(outFile);
			isFile = true;
		}

		WWuString wrappedDest = UtilitiesWrapper::GetWideStringFromSystemString(destination);

		Core::TcpingForm form { wrappedDest, static_cast<DWORD>(port), static_cast<DWORD>(count), static_cast<DWORD>(timeout), static_cast<DWORD>(interval), static_cast<DWORD>(failThreshold),
			continuous, jitter, fqdn, force, single, isFile, wrappedOutFile, append };

		try {
			Stubs::Network::Dispatch<NetworkOperation::Tcping>(Context->GetUnderlyingContext(), form);
		}
		catch (NativeException^ ex) {
			if (ex->ErrorCode != ERROR_CANCELLED) {
				Context->WriteError(ex->Record);
				throw;
			}
		}

		isCancel = form.IsCtrlCHit();
	}

	// Get-NetworkFile
	List<NetworkFileInfo^>^ NetworkWrapper::GetNetworkFile(String^ computerName, String^ basePath, String^ userName, bool includeSessionName)
	{
		WWuString wrappedPcName    = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		WWuString wrappedBasePath  = UtilitiesWrapper::GetWideStringFromSystemString(basePath);
		WWuString wrappedUserName  = UtilitiesWrapper::GetWideStringFromSystemString(userName);

		WuList<Core::NETWORK_FILE_INFO> result;
		auto output = gcnew List<NetworkFileInfo^>(0);

		if (includeSessionName) {
			WuList<Core::NETWORK_SESSION_INFO> sessionInfo;
			_WU_START_TRY
				Stubs::Network::Dispatch<NetworkOperation::ListFiles>(Context->GetUnderlyingContext(), wrappedPcName, wrappedBasePath, wrappedUserName, result, sessionInfo);
			_WU_MANAGED_CATCH

			for (Core::NETWORK_FILE_INFO& info : result) {
				WWuString sessName;
				for (Core::NETWORK_SESSION_INFO& sessInfo : sessionInfo) {
					if (sessInfo.UserName == info.UserName) {
						sessName = sessInfo.ComputerSessionName;
						break;
					}
				}

				output->Add(gcnew NetworkFileInfo(info, sessName, computerName));
			}
		}
		else {
			_WU_START_TRY
				Stubs::Network::Dispatch<NetworkOperation::ListFiles>(Context->GetUnderlyingContext(), wrappedPcName, wrappedBasePath, wrappedUserName, result);
				for (Core::NETWORK_FILE_INFO& info : result)
					output->Add(gcnew NetworkFileInfo(info, computerName));
			_WU_MANAGED_CATCH
		}

		return output;
	}

	// Close-NetworkFile
	void NetworkWrapper::CloseNetworkFile(String^ computerName, int fileId)
	{
		WWuString wrappedPcName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::CloseFile>(Context->GetUnderlyingContext(), wrappedPcName, fileId);
		_WU_MANAGED_CATCH
	}

	// Test-Port
	void NetworkWrapper::TestNetworkPort(String^ destination, UInt32 port, TransportProtocol protocol, UInt32 timeout)
	{
		WWuString wrappedDest = UtilitiesWrapper::GetWideStringFromSystemString(destination);
		Core::TestPortForm workForm(
			wrappedDest,
			port,
			static_cast<Core::TransportProtocol>(protocol),
			timeout
		);

		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::TestPort>(Context->GetUnderlyingContext(), workForm);
		_WU_MANAGED_CATCH
	}

	// Get-NetworkStatistics
	void NetworkWrapper::GetTransportTables(bool all, bool includeModuleName)
	{
		WuList<Core::GETNETSTAT_MAIN_OUTPUT> output;
		std::unordered_map<DWORD, WWuString> processList;

		const auto nativeContext = Context->GetUnderlyingContext();
		if (includeModuleName) {
			try {
				Stubs::ProcessAndThread::Dispatch<PatOperation::ListProcess>(nativeContext, processList);
			}
			catch (NativeException^ ex) {
				Context->WriteError(ex->Record);
			}
		}

		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::TcpTables>(nativeContext, includeModuleName, output, processList);
			if (all)
				Stubs::Network::Dispatch<NetworkOperation::UdpTables>(nativeContext, includeModuleName, output, processList);
		_WU_MANAGED_CATCH

		for (const auto& row : output)
			Context->WriteObject(gcnew TransportTableInfo(row));
	}

	void NetworkWrapper::GetInterfaceStatistics()
	{
		WuList<MIB_IF_ROW2> output;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::IfStats>(Context->GetUnderlyingContext(), output);
		_WU_MANAGED_CATCH

		for (const auto& info : output)
			Context->WriteObject(gcnew InterfaceStatisticsSingle(info));
	}

	void NetworkWrapper::GetInterfaceStatistics([Out] List<InterfaceStatisticsSingle^>^ output)
	{
		WuList<MIB_IF_ROW2> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::IfStats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH

		for (const auto& info : rawOutput)
			output->Add(gcnew InterfaceStatisticsSingle(info));
	}

	void NetworkWrapper::GetIpRouteTable()
	{
		WuList<Core::WU_IP_ROUTE> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::RouteTable>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH
		
		for (const auto& info : rawOutput)
			Context->WriteObject(gcnew IpRoute(info));
	}

	inline void NetworkWrapper::GetIpv4Statistics([Out] List<IpStatistics^>^ output)
	{
		std::unordered_map<Core::GetNetStatProtocol, MIB_IPSTATS> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::Ipv4Stats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH

		for (const auto& info : rawOutput)
			output->Add(gcnew IpStatistics(info.first, info.second));
	}

	inline void NetworkWrapper::GetIpv6Statistics([Out] List<IpStatistics^>^ output)
	{
		std::unordered_map<Core::GetNetStatProtocol, MIB_IPSTATS> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::Ipv6Stats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH

		for (const auto& info : rawOutput)
			output->Add(gcnew IpStatistics(info.first, info.second));
	}

	inline void NetworkWrapper::GetIcmpv4Statistics([Out] List<IcmpStatistics^>^ output)
	{
		std::unordered_map<Core::GetNetStatProtocol, MIB_ICMP_EX> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::Icmpv4Stats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH

		for (const auto& info : rawOutput)
			output->Add(gcnew Icmpv4Statistics(info.first, info.second));
	}

	inline void NetworkWrapper::GetIcmpv6Statistics([Out] List<IcmpStatistics^>^ output)
	{
		std::unordered_map<Core::GetNetStatProtocol, MIB_ICMP_EX> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::Icmpv6Stats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH

		for (const auto& info : rawOutput)
			output->Add(gcnew Icmpv6Statistics(info.first, info.second));
	}

	inline void NetworkWrapper::GetTcpv4Statistics([Out] List<TcpStatistics^>^ output)
	{
		std::unordered_map<Core::GetNetStatProtocol, MIB_TCPSTATS> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::Tcpv4Stats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH

		for (const auto& info : rawOutput)
			output->Add(gcnew TcpStatistics(info.first, info.second));
	}

	inline void NetworkWrapper::GetTcpv6Statistics([Out] List<TcpStatistics^>^ output)
	{
		std::unordered_map<Core::GetNetStatProtocol, MIB_TCPSTATS> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::Tcpv6Stats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH
		
		for (const auto& info : rawOutput)
			output->Add(gcnew TcpStatistics(info.first, info.second));
	}

	inline void NetworkWrapper::GetUdpv4Statistics([Out] List<UdpStatistics^>^ output)
	{
		std::unordered_map<Core::GetNetStatProtocol, MIB_UDPSTATS> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::Udpv4Stats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH
		
		for (const auto& info : rawOutput)
			output->Add(gcnew UdpStatistics(info.first, info.second));
	}

	inline void NetworkWrapper::GetUdpv6Statistics([Out] List<UdpStatistics^>^ output)
	{
		std::unordered_map<Core::GetNetStatProtocol, MIB_UDPSTATS> rawOutput;
		_WU_START_TRY
			Stubs::Network::Dispatch<NetworkOperation::Udpv6Stats>(Context->GetUnderlyingContext(), rawOutput);
		_WU_MANAGED_CATCH

		for (const auto& info : rawOutput)
			output->Add(gcnew UdpStatistics(info.first, info.second));
	}
}