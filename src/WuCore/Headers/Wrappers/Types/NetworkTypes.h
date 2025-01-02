#pragma once
#pragma unmanaged

#include <LMShare.h>
#include <LMAccess.h>

#include "../../Engine/Network.h"

#pragma managed

/*
*	Important note about types:
* 
*	PowerShell for some reason lists the properties in the reverse order than declared here.
*	An easy way to deal with it is to declare properties 'upside-down'.
*	This avoids us having to mess with 'format'/'types' files.
* 
*	For classes with inheritance, it seems to list first the child properties, then
*	the parent ones.
* 
*	Remember: what matters is what shows up to the user.
*/

namespace WindowsUtils::Network
{
	using namespace System;

	[Flags]
	public enum class NetworkFilePermission
	{
		Read              = PERM_FILE_READ,
		Write             = PERM_FILE_WRITE,
		Create            = PERM_FILE_CREATE,
		Execute           = ACCESS_EXEC,
		Delete            = ACCESS_DELETE,
		ChangeAttribute   = ACCESS_ATRIB,
		ChangePermission  = ACCESS_PERM,
	};

	public enum class PortProbeStatus
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

	public enum class GetNetStatProtocol
	{
		Tcp,
		Tcpv6,
		Udp,
		Udpv6,
		Ipv4,
		Ipv6,
		Icmp,
		Icmpv6,
		Ethernet
	};

	public enum class PortState
	{
		None = 0,
		Closed = 1,
		Listen,
		SynSent,
		SynReceived,
		Established,
		FinWait1,
		FinWait2,
		CloseWait,
		Closing,
		LastAck,
		TimeWait,
		DeleteTcb
	};

	public ref class NetworkFileInfo sealed
	{
	public:
		property Int32 Id { Int32 get() { return m_wrapper->Id; } }
		property String^ Path { String^ get() { return gcnew String(m_wrapper->Path.Raw()); } }
		property NetworkFilePermission Permissions { NetworkFilePermission get() { return static_cast<NetworkFilePermission>(m_wrapper->Permissions); } }
		property Int32 LockCount { Int32 get() { return m_wrapper->LockCount; } }
		property String^ UserName { String^ get() { return gcnew String(m_wrapper->UserName.Raw()); } }
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

			m_sessionName = gcnew String(sessionName.Raw());
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

	public ref class TcpingProbeInfo sealed
	{
	public:
		property String^ Destination { String^ get() { return gcnew String(m_wrapper->Destination.Raw()); } }
		property String^ ResolvedAddress { String^ get() { return gcnew String(m_wrapper->DestAddress.Raw()); } }
		property DateTime^ Timestamp {
			DateTime^ get()
			{
				::FILETIME fileTime = m_wrapper->Timestamp;
				__int64 timeQuadPart = ULARGE_INTEGER { fileTime.dwLowDateTime, fileTime.dwHighDateTime }.QuadPart;
				return DateTime::FromFileTime(timeQuadPart);
			}
		}
		property UInt32 Port { UInt32 get() { return m_wrapper->Port; } }
		property PortProbeStatus Status { PortProbeStatus get() { return static_cast<PortProbeStatus>(m_wrapper->Status); } }
		property Double RoundTripTime { Double get() { return m_wrapper->RoundTripTime; } }
		property Double Jitter { Double get() { return m_wrapper->Jitter; } }

		TcpingProbeInfo(const Core::TCPING_OUTPUT& info) { m_wrapper = new Core::TCPING_OUTPUT(info); }
		~TcpingProbeInfo() { delete m_wrapper; }

	protected:
		!TcpingProbeInfo() { delete m_wrapper; }

	private:
		Core::PTCPING_OUTPUT m_wrapper;
	};

	public ref class TcpingStatistics sealed
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
		property String^ Destination { String^ get() { return gcnew String(m_wrapper->Destination.Raw()); } }
		property String^ Address { String^ get() { return gcnew String(m_wrapper->DestAddress.Raw()); } }
		property DateTime^ Timestamp {
			DateTime^ get()
			{
				::FILETIME fileTime = m_wrapper->Timestamp;
				__int64 timeQuadPart = ULARGE_INTEGER { fileTime.dwLowDateTime, fileTime.dwHighDateTime }.QuadPart;
				return DateTime::FromFileTime(timeQuadPart);
			}
		}
		property UInt32 Port { UInt32 get() { return m_wrapper->Port; } }
		property PortProbeStatus Status { PortProbeStatus get() { return static_cast<PortProbeStatus>(m_wrapper->Status); } }

		TestPortInfo(const Core::TESTPORT_OUTPUT& info) { m_wrapper = new Core::TESTPORT_OUTPUT(info); }
		~TestPortInfo() { delete m_wrapper; }

	protected:
		!TestPortInfo() { delete m_wrapper; }

	private:
		Core::PTESTPORT_OUTPUT m_wrapper;
	};

	public ref class TransportTableInfo sealed
	{
	public:
		property String^ ModuleName { String^ get() { return gcnew String(m_wrapper->ModuleName.Raw()); } }
		property Int32 ProcessId { Int32 get() { return m_wrapper->ProcessId; } }
		property PortState State { PortState get() { return static_cast<PortState>(m_wrapper->State); } }
		property Int32 RemotePort { Int32 get() { return m_wrapper->RemotePort; } }
		property String^ RemoteAddress { String^ get() { return gcnew String(m_wrapper->RemoteAddress.Raw()); } }
		property Int32 LocalPort { Int32 get() { return m_wrapper->LocalPort; } }
		property String^ LocalAddress { String^ get() { return gcnew String(m_wrapper->LocalAddress.Raw()); } }
		property TransportProtocol Protocol { TransportProtocol get() { return static_cast<TransportProtocol>(m_wrapper->Protocol); } }

		TransportTableInfo(const Core::GETNETSTAT_MAIN_OUTPUT& info) { m_wrapper = new Core::GETNETSTAT_MAIN_OUTPUT(info); }
		~TransportTableInfo() { delete m_wrapper; }

	protected:
		!TransportTableInfo() { delete m_wrapper; }

	private:
		Core::PGETNETSTAT_MAIN_OUTPUT m_wrapper;
	};

	public ref class InterfaceStatistics
	{

	};

	public ref class InterfaceStatisticsSingle sealed : public InterfaceStatistics
	{
	public:
		property Int64 UnknownProtocolsReceived { Int64 get() { return m_wrapper->InUnknownProtos; } }
		property Int64 ErrorsReceived { Int64 get() { return m_wrapper->InErrors; } }
		property Int64 ErrorsSent { Int64 get() { return m_wrapper->OutErrors; } }
		property Int64 DiscardsReceived { Int64 get() { return m_wrapper->InDiscards; } }
		property Int64 DiscardsSent { Int64 get() { return m_wrapper->OutDiscards; } }
		property Int64 NonUnicastReceived { Int64 get() { return m_wrapper->InNUcastPkts; } }
		property Int64 NonUnicastSent { Int64 get() { return m_wrapper->OutNUcastPkts; } }
		property Int64 UnicastReceived { Int64 get() { return m_wrapper->InUcastPkts; } }
		property Int64 UnicastSent { Int64 get() { return m_wrapper->OutUcastPkts; } }
		property Int64 BytesReceived { Int64 get() { return m_wrapper->InOctets; } }
		property Int64 BytesSent { Int64 get() { return m_wrapper->OutOctets; } }
		property Int64 OutQueueLength { Int64 get() { return m_wrapper->OutQLen; } }
		property Int64 ReceiveLinkSpeed { Int64 get() { return m_wrapper->ReceiveLinkSpeed; } }
		property Int64 TransmitLinkSpeed { Int64 get() { return m_wrapper->TransmitLinkSpeed; } }
		property Int32 Mtu { Int32 get() { return m_wrapper->Mtu; } }
		property String^ InterfaceDescription { String^ get() { return m_interfaceDescription; } }
		property String^ InterfaceAlias { String^ get() { return m_interfaceAlias; } }
		property Int32 InterfaceIndex { Int32 get() { return m_wrapper->InterfaceIndex; } }

		InterfaceStatisticsSingle(const MIB_IF_ROW2& info)
			: m_wrapper(new MIB_IF_ROW2(info))
		{
			m_interfaceAlias = gcnew String(m_wrapper->Alias);
			m_interfaceDescription = gcnew String(m_wrapper->Description);
		}

		~InterfaceStatisticsSingle() { delete m_wrapper; }

	protected:
		!InterfaceStatisticsSingle() { delete m_wrapper; }

	private:
		String^ m_interfaceAlias;
		String^ m_interfaceDescription;

		PMIB_IF_ROW2 m_wrapper;
	};

	public ref class IpStatistics sealed
	{
	public:
		property UInt32 FragmentsCreated { UInt32 get() { return m_wrapper->dwFragCreates; } }
		property UInt32 DatagramsFailingFragmentation { UInt32 get() { return m_wrapper->dwFragFails; } }
		property UInt32 DatagramsSuccessfullyFragmented { UInt32 get() { return m_wrapper->dwFragOks; } }
		property UInt32 ReassemblyFailures { UInt32 get() { return m_wrapper->dwReasmFails; } }
		property UInt32 ReassemblySuccessful { UInt32 get() { return m_wrapper->dwReasmOks; } }
		property UInt32 ReassemblyRequired { UInt32 get() { return m_wrapper->dwReasmReqds; } }
		property UInt32 OutputDatagramNoRoute { UInt32 get() { return m_wrapper->dwOutNoRoutes; } }
		property UInt32 DiscardedOutputDatagrams { UInt32 get() { return m_wrapper->dwOutDiscards; } }
		property UInt32 RoutingDiscards { UInt32 get() { return m_wrapper->dwRoutingDiscards; } }
		property UInt32 OutputRequests { UInt32 get() { return m_wrapper->dwOutRequests; } }
		property UInt32 ReceivedDatagramsDelivered { UInt32 get() { return m_wrapper->dwInDelivers; } }
		property UInt32 ReceivedDatagramsDiscarded { UInt32 get() { return m_wrapper->dwInDiscards; } }
		property UInt32 UnknownProtocolsReceived { UInt32 get() { return m_wrapper->dwInUnknownProtos; } }
		property UInt32 DatagramsForwarded { UInt32 get() { return m_wrapper->dwForwDatagrams; } }
		property UInt32 ReceivedAddressErrors { UInt32 get() { return m_wrapper->dwInAddrErrors; } }
		property UInt32 ReceivedHeaderErrors { UInt32 get() { return m_wrapper->dwInHdrErrors; } }
		property UInt32 DatagramsReceived { UInt32 get() { return m_wrapper->dwInReceives; } }
		property GetNetStatProtocol Protocol { GetNetStatProtocol get() { return m_protocol; } }
		
		IpStatistics(Core::GetNetStatProtocol proto, const MIB_IPSTATS& info)
			: m_protocol(static_cast<GetNetStatProtocol>(proto)), m_wrapper(new MIB_IPSTATS(info)) { }

		~IpStatistics() { delete m_wrapper; }

	protected:
		!IpStatistics() { delete m_wrapper; }

	private:
		GetNetStatProtocol m_protocol;
		PMIB_IPSTATS m_wrapper;
	};

	public ref class TcpStatistics sealed
	{
	public:
		property UInt32 SegmentsRetransmitted { UInt32 get() { return m_wrapper->dwRetransSegs; } }
		property UInt32 SegmentsSent { UInt32 get() { return m_wrapper->dwOutSegs; } }
		property UInt32 SegmentsReceived { UInt32 get() { return m_wrapper->dwInSegs; } }
		property UInt32 CurrentConnections { UInt32 get() { return m_wrapper->dwCurrEstab; } }
		property UInt32 ResetConnections { UInt32 get() { return m_wrapper->dwEstabResets; } }
		property UInt32 FailedConnectionAttempts { UInt32 get() { return m_wrapper->dwAttemptFails; } }
		property UInt32 PassiveOpens { UInt32 get() { return m_wrapper->dwPassiveOpens; } }
		property UInt32 ActiveOpens { UInt32 get() { return m_wrapper->dwActiveOpens; } }
		property GetNetStatProtocol Protocol { GetNetStatProtocol get() { return m_protocol; } }

		TcpStatistics(Core::GetNetStatProtocol proto, const MIB_TCPSTATS& info)
			: m_protocol(static_cast<GetNetStatProtocol>(proto)), m_wrapper(new MIB_TCPSTATS(info)) { }
		
		~TcpStatistics() { delete m_wrapper; }

	protected:
		!TcpStatistics() { delete m_wrapper; }

	private:
		GetNetStatProtocol m_protocol;
		PMIB_TCPSTATS m_wrapper;
	};

	public ref class UdpStatistics sealed
	{
	public:
		property UInt32 DatagramsSent { UInt32 get() { return m_wrapper->dwOutDatagrams; } }
		property UInt32 ReceiveErrors { UInt32 get() { return m_wrapper->dwInErrors; } }
		property UInt32 NoPorts { UInt32 get() { return m_wrapper->dwNoPorts; } }
		property UInt32 DatagramsReceived { UInt32 get() { return m_wrapper->dwInDatagrams; } }
		property GetNetStatProtocol Protocol { GetNetStatProtocol get() { return m_protocol; } }

		UdpStatistics(Core::GetNetStatProtocol proto, const MIB_UDPSTATS& info)
			: m_protocol(static_cast<GetNetStatProtocol>(proto)), m_wrapper(new MIB_UDPSTATS(info)) { }

		~UdpStatistics() { delete m_wrapper; }

	protected:
		!UdpStatistics() { delete m_wrapper; }

	private:
		GetNetStatProtocol m_protocol;
		PMIB_UDPSTATS m_wrapper;
	};

	public ref class IcmpStatistics
	{
	public:
		property UInt32 EchoRequestReceived { UInt32 get() { return m_echoRequestReceived; } }
		property UInt32 EchoRequestSent { UInt32 get() { return m_echoRequestSent; } }
		property UInt32 EchoReplyReceived { UInt32 get() { return m_echoReplyReceived; } }
		property UInt32 EchoReplySent { UInt32 get() { return m_echoReplySent; } }
		property UInt32 RedirectReceived { UInt32 get() { return m_redirectReceived; } }
		property UInt32 RedirectSent { UInt32 get() { return m_redirectSent; } }
		property UInt32 RouterAdvertReceived { UInt32 get() { return m_routerAdvertReceived; } }
		property UInt32 RouterAdvertSent { UInt32 get() { return m_routerAdvertSent; } }
		property UInt32 RouterSolicitReceived { UInt32 get() { return m_routerSolicitReceived; } }
		property UInt32 RouterSolicitSent { UInt32 get() { return m_routerSolicitSent; } }
		property UInt32 TimeExceededReceived { UInt32 get() { return m_timeExceededReceived; } }
		property UInt32 TimeExceededSent { UInt32 get() { return m_timeExceededSent; } }
		property UInt32 ParameterProblemReceived { UInt32 get() { return m_parameterProblemReceived; } }
		property UInt32 ParameterProblemSent { UInt32 get() { return m_parameterProblemSent; } }
		property UInt32 DestinationUnreachableReceived { UInt32 get() { return m_destinationUnreachableReceived; } }
		property UInt32 DestinationUnreachableSent { UInt32 get() { return m_destinationUnreachableSent; } }
		property UInt32 ErrorsReceived { UInt32 get() { return m_errorsReceived; } }
		property UInt32 ErrorsSent { UInt32 get() { return m_errorsSent; } }
		property UInt32 MessagesReceived { UInt32 get() { return m_messagesReceived; } }
		property UInt32 MessagesSent { UInt32 get() { return m_messagesSent; } }
		property GetNetStatProtocol Protocol { GetNetStatProtocol get() { return m_protocol; } }

		IcmpStatistics(Core::GetNetStatProtocol proto, const MIB_ICMP_EX& info)
		{
			m_protocol = static_cast<GetNetStatProtocol>(proto);
			m_messagesSent = info.icmpOutStats.dwMsgs;
			m_messagesReceived = info.icmpInStats.dwMsgs;
			m_errorsSent = info.icmpOutStats.dwErrors;
			m_errorsReceived = info.icmpInStats.dwErrors;

			if (proto == Core::GetNetStatProtocol::Icmp) {
				m_destinationUnreachableSent = info.icmpOutStats.rgdwTypeCount[ICMP4_DST_UNREACH];
				m_parameterProblemSent = info.icmpOutStats.rgdwTypeCount[ICMP4_PARAM_PROB];
				m_timeExceededSent = info.icmpOutStats.rgdwTypeCount[ICMP4_TIME_EXCEEDED];
				m_routerSolicitSent = info.icmpOutStats.rgdwTypeCount[ICMP4_ROUTER_SOLICIT];
				m_routerAdvertSent = info.icmpOutStats.rgdwTypeCount[ICMP4_ROUTER_ADVERT];
				m_redirectSent = info.icmpOutStats.rgdwTypeCount[ICMP4_REDIRECT];
				m_echoReplySent = info.icmpOutStats.rgdwTypeCount[ICMP4_ECHO_REPLY];
				m_echoRequestSent = info.icmpOutStats.rgdwTypeCount[ICMP4_ECHO_REQUEST];
				m_destinationUnreachableReceived = info.icmpInStats.rgdwTypeCount[ICMP4_DST_UNREACH];
				m_parameterProblemReceived = info.icmpInStats.rgdwTypeCount[ICMP4_PARAM_PROB];
				m_timeExceededReceived = info.icmpInStats.rgdwTypeCount[ICMP4_TIME_EXCEEDED];
				m_routerSolicitReceived = info.icmpInStats.rgdwTypeCount[ICMP4_ROUTER_SOLICIT];
				m_routerAdvertReceived = info.icmpInStats.rgdwTypeCount[ICMP4_ROUTER_ADVERT];
				m_redirectReceived = info.icmpInStats.rgdwTypeCount[ICMP4_REDIRECT];
				m_echoReplyReceived = info.icmpInStats.rgdwTypeCount[ICMP4_ECHO_REPLY];
				m_echoRequestReceived = info.icmpInStats.rgdwTypeCount[ICMP4_ECHO_REQUEST];
			}
			else if (proto == Core::GetNetStatProtocol::Icmpv6) {
				m_destinationUnreachableSent = info.icmpOutStats.rgdwTypeCount[ICMP6_DST_UNREACH];
				m_parameterProblemSent = info.icmpOutStats.rgdwTypeCount[ICMP6_PARAM_PROB];
				m_timeExceededSent = info.icmpOutStats.rgdwTypeCount[ICMP6_TIME_EXCEEDED];
				m_routerSolicitSent = info.icmpOutStats.rgdwTypeCount[ND_ROUTER_SOLICIT];
				m_routerAdvertSent = info.icmpOutStats.rgdwTypeCount[ND_ROUTER_ADVERT];
				m_redirectSent = info.icmpOutStats.rgdwTypeCount[ND_REDIRECT];
				m_echoReplySent = info.icmpOutStats.rgdwTypeCount[ICMP6_ECHO_REPLY];
				m_echoRequestSent = info.icmpOutStats.rgdwTypeCount[ICMP6_ECHO_REQUEST];
				m_destinationUnreachableReceived = info.icmpInStats.rgdwTypeCount[ICMP6_DST_UNREACH];
				m_parameterProblemReceived = info.icmpInStats.rgdwTypeCount[ICMP6_PARAM_PROB];
				m_timeExceededReceived = info.icmpInStats.rgdwTypeCount[ICMP6_TIME_EXCEEDED];
				m_routerSolicitReceived = info.icmpInStats.rgdwTypeCount[ND_ROUTER_SOLICIT];
				m_routerAdvertReceived = info.icmpInStats.rgdwTypeCount[ND_ROUTER_ADVERT];
				m_redirectReceived = info.icmpInStats.rgdwTypeCount[ND_REDIRECT];
				m_echoReplyReceived = info.icmpInStats.rgdwTypeCount[ICMP6_ECHO_REPLY];
				m_echoRequestReceived = info.icmpInStats.rgdwTypeCount[ICMP6_ECHO_REQUEST];
			}
			else {
				throw gcnew ArgumentException("Invalid protocol: " + m_protocol.ToString() + ".");
			}
		}

	private:
		GetNetStatProtocol m_protocol;
		UInt32 m_messagesSent;
		UInt32 m_messagesReceived;
		UInt32 m_errorsSent;
		UInt32 m_errorsReceived;
		UInt32 m_destinationUnreachableSent;
		UInt32 m_destinationUnreachableReceived;
		UInt32 m_parameterProblemSent;
		UInt32 m_parameterProblemReceived;
		UInt32 m_timeExceededSent;
		UInt32 m_timeExceededReceived;
		UInt32 m_routerSolicitSent;
		UInt32 m_routerSolicitReceived;
		UInt32 m_routerAdvertSent;
		UInt32 m_routerAdvertReceived;
		UInt32 m_redirectSent;
		UInt32 m_redirectReceived;
		UInt32 m_echoReplySent;
		UInt32 m_echoReplyReceived;
		UInt32 m_echoRequestSent;
		UInt32 m_echoRequestReceived;
		
	};

	public ref class Icmpv4Statistics sealed : public IcmpStatistics
	{
	public:
		property UInt32 SourceQuenchReceived { UInt32 get() { return m_sourceQuenchReceived; } }
		property UInt32 SourceQuenchSent { UInt32 get() { return m_sourceQuenchSent; } }
		property UInt32 MaskRequestReceived { UInt32 get() { return m_maskRequestReceived; } }
		property UInt32 MaskRequestSent { UInt32 get() { return m_maskRequestSent; } }
		property UInt32 MaskReplyReceived { UInt32 get() { return m_maskReplyReceived; } }
		property UInt32 MaskReplySent { UInt32 get() { return m_maskReplySent; } }
		property UInt32 TimestampRequestReceived { UInt32 get() { return m_timestampRequestReceived; } }
		property UInt32 TimestampRequestSent { UInt32 get() { return m_timestampRequestSent; } }
		property UInt32 TimestampReplyReceived { UInt32 get() { return m_timestampReplyReceived; } }
		property UInt32 TimestampReplySent { UInt32 get() { return m_timestampReplySent; } }

		Icmpv4Statistics(Core::GetNetStatProtocol proto, const MIB_ICMP_EX& info)
			: IcmpStatistics(proto, info)
		{
			m_timestampReplySent = info.icmpOutStats.rgdwTypeCount[ICMP4_TIMESTAMP_REPLY];
			m_timestampRequestSent = info.icmpOutStats.rgdwTypeCount[ICMP4_TIMESTAMP_REQUEST];
			m_maskReplySent = info.icmpOutStats.rgdwTypeCount[ICMP4_MASK_REPLY];
			m_maskRequestSent = info.icmpOutStats.rgdwTypeCount[ICMP4_MASK_REQUEST];
			m_sourceQuenchSent = info.icmpOutStats.rgdwTypeCount[ICMP4_SOURCE_QUENCH];
			m_timestampReplyReceived = info.icmpInStats.rgdwTypeCount[ICMP4_TIMESTAMP_REPLY];
			m_timestampRequestReceived = info.icmpInStats.rgdwTypeCount[ICMP4_TIMESTAMP_REQUEST];
			m_maskReplyReceived = info.icmpInStats.rgdwTypeCount[ICMP4_MASK_REPLY];
			m_maskRequestReceived = info.icmpInStats.rgdwTypeCount[ICMP4_MASK_REQUEST];
			m_sourceQuenchReceived = info.icmpInStats.rgdwTypeCount[ICMP4_SOURCE_QUENCH];
		}

	private:
		UInt32 m_timestampReplySent;
		UInt32 m_timestampReplyReceived;
		UInt32 m_timestampRequestSent;
		UInt32 m_timestampRequestReceived;
		UInt32 m_maskReplySent;
		UInt32 m_maskReplyReceived;
		UInt32 m_maskRequestSent;
		UInt32 m_maskRequestReceived;
		UInt32 m_sourceQuenchSent;
		UInt32 m_sourceQuenchReceived;
	};

	public ref class Icmpv6Statistics sealed : public IcmpStatistics
	{
	public:
		property UInt32 MembershipReportV2Received { UInt32 get() { return m_membershipReportV2Received; } }
		property UInt32 MembershipReportV2Sent { UInt32 get() { return m_membershipReportV2Sent; } }
		property UInt32 NeighborAdvertReceived { UInt32 get() { return m_neighborAdvertReceived; } }
		property UInt32 NeighborAdvertSent { UInt32 get() { return m_neighborAdvertSent; } }
		property UInt32 NeighborSolicitReceived { UInt32 get() { return m_neighborSolicitReceived; } }
		property UInt32 NeighborSolicitSent { UInt32 get() { return m_neighborSolicitSent; } }
		property UInt32 MembershipReductionReceived { UInt32 get() { return m_membershipReductionReceived; } }
		property UInt32 MembershipReductionSent { UInt32 get() { return m_membershipReductionSent; } }
		property UInt32 MembershipReportReceived { UInt32 get() { return m_membershipReportReceived; } }
		property UInt32 MembershipReportSent { UInt32 get() { return m_membershipReportSent; } }
		property UInt32 MembershipQueryReceived { UInt32 get() { return m_membershipQueryReceived; } }
		property UInt32 MembershipQuerySent { UInt32 get() { return m_membershipQuerySent; } }
		property UInt32 PacketToBigReceived { UInt32 get() { return m_packetToBigReceived; } }
		property UInt32 PacketToBigSent { UInt32 get() { return m_packetToBigSent; } }

		Icmpv6Statistics(Core::GetNetStatProtocol proto, const MIB_ICMP_EX& info)
			: IcmpStatistics(proto, info)
		{
			m_packetToBigSent = info.icmpOutStats.rgdwTypeCount[ICMP6_PACKET_TOO_BIG];
			m_membershipQuerySent = info.icmpOutStats.rgdwTypeCount[ICMP6_MEMBERSHIP_QUERY];
			m_membershipReportSent = info.icmpOutStats.rgdwTypeCount[ICMP6_MEMBERSHIP_REPORT];
			m_membershipReductionSent = info.icmpOutStats.rgdwTypeCount[ICMP6_MEMBERSHIP_REDUCTION];
			m_neighborSolicitSent = info.icmpOutStats.rgdwTypeCount[ND_NEIGHBOR_SOLICIT];
			m_neighborAdvertSent = info.icmpOutStats.rgdwTypeCount[ND_NEIGHBOR_ADVERT];
			m_membershipReportV2Sent = info.icmpOutStats.rgdwTypeCount[ICMP6_V2_MEMBERSHIP_REPORT];
			m_packetToBigReceived = info.icmpInStats.rgdwTypeCount[ICMP6_PACKET_TOO_BIG];
			m_membershipQueryReceived = info.icmpInStats.rgdwTypeCount[ICMP6_MEMBERSHIP_QUERY];
			m_membershipReportReceived = info.icmpInStats.rgdwTypeCount[ICMP6_MEMBERSHIP_REPORT];
			m_membershipReductionReceived = info.icmpInStats.rgdwTypeCount[ICMP6_MEMBERSHIP_REDUCTION];
			m_neighborSolicitReceived = info.icmpInStats.rgdwTypeCount[ND_NEIGHBOR_SOLICIT];
			m_neighborAdvertReceived = info.icmpInStats.rgdwTypeCount[ND_NEIGHBOR_ADVERT];
			m_membershipReportV2Received = info.icmpInStats.rgdwTypeCount[ICMP6_V2_MEMBERSHIP_REPORT];
		}

	private:
		UInt32 m_packetToBigSent;
		UInt32 m_packetToBigReceived;
		UInt32 m_membershipQuerySent;
		UInt32 m_membershipQueryReceived;
		UInt32 m_membershipReportSent;
		UInt32 m_membershipReportReceived;
		UInt32 m_membershipReductionSent;
		UInt32 m_membershipReductionReceived;
		UInt32 m_neighborSolicitSent;
		UInt32 m_neighborSolicitReceived;
		UInt32 m_neighborAdvertSent;
		UInt32 m_neighborAdvertReceived;
		UInt32 m_membershipReportV2Sent;
		UInt32 m_membershipReportV2Received;
	};

	public ref class IpRoute sealed
	{
	public:
		property Boolean Persistent { Boolean get() { return m_wrapper->Persistent; } }
		property UInt32 Metric { UInt32 get() { return m_wrapper->Metric; } }
		property String^ Gateway { String^ get() { return m_gateway; } }
		property String^ NetworkMask { String^ get() { return m_ntwMask; } }
		property String^ NetworkDestination { String^ get() { return m_ntwDestination; } }
		property UInt32 InterfaceIndex { UInt32 get() { return m_wrapper->InterfaceIndex; } }

		IpRoute(const Core::WU_IP_ROUTE& info)
			: m_wrapper(new Core::WU_IP_ROUTE(info)), m_ntwDestination(gcnew String(info.NetworkDestination.Raw())),
			m_ntwMask(gcnew String(info.NetworkMask.Raw())), m_gateway(gcnew String(info.Gateway.Raw())) { }

		~IpRoute() { delete m_wrapper; }

	protected:
		!IpRoute() { delete m_wrapper; }

	private:
		String^ m_ntwDestination;
		String^ m_ntwMask;
		String^ m_gateway;

		Core::PWU_IP_ROUTE m_wrapper;
	};
}