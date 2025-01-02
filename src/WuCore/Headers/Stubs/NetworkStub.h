#pragma once
#pragma unmanaged

#include "StubUtils.h"
#include "../Engine/Network.h"

namespace WindowsUtils
{
	enum class NetworkOperation
	{
		Tcping,
		ListFiles,
		CloseFile,
		TestPort,
		TcpTables,
		UdpTables,
		IfStats,
		RouteTable,
		Ipv4Stats,
		Ipv6Stats,
		Icmpv4Stats,
		Icmpv6Stats,
		Tcpv4Stats,
		Tcpv6Stats,
		Udpv4Stats,
		Udpv6Stats,
	};
}

namespace WindowsUtils::Stubs
{
	class Network
	{
	public:
		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Tcping, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::StartTcpPing(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::ListFiles, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::ListNetworkFiles(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::CloseFile, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::CloseNetworkFile(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::TestPort, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::TestNetworkPort(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::TcpTables, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetTcpTables(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::UdpTables, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetUdpTables(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::IfStats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetInterfaceStatistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::RouteTable, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetIpRouteTable(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Ipv4Stats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetIpv4Statistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Ipv6Stats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetIpv6Statistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Icmpv4Stats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetIcmpv4Statistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Icmpv6Stats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetIcmpv6Statistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Tcpv4Stats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetTcpv4Statistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Tcpv6Stats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetTcpv6Statistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Udpv4Stats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetUdpv4Statistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <NetworkOperation Opr, std::enable_if_t<Opr == NetworkOperation::Udpv6Stats, int> = 0, class... TArgs>
		static void Dispatch(Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Network::GetUdpv6Statistics(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}
	};
}