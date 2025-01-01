#pragma once
#pragma unmanaged

#include "Registry.h"

#include "../Support/WuString.h"
#include "../Support/CoreUtils.h"
#include "../Support/Notification.h"
#include "../Support/Nt/NtUtilities.h"
#include "../Support/SafeHandle.h"
#include "../Support/IO.h"
#include "../Support/WuException.h"

#include <WinSock2.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <ip2string.h>
#include <unordered_map>
#include <queue>
#include <memory>
#include <WinDNS.h>
#include <lmcons.h>
#include <LMShare.h>
#include <lmerr.h>
#include <LMAPIbuf.h>
#include <WS2tcpip.h>
#include <cmath>

namespace WindowsUtils::Core
{
	/*
	* ~ General
	*/

	enum class PortProbeStatus : WORD
	{
		Open,
		Closed,
		Timeout,
	};

	enum class TransportProtocol : WORD
	{
		Tcp,
		Udp,
	};

	enum class PortState : WORD
	{
		None,
		Closed,
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
		DeleteTcb,
	};


	/*
	* ~ Start-Tcping
	*/

	typedef struct _TCPING_STATISTICS
	{
		DWORD   Sent;
		DWORD   Successful;
		DWORD   Failed;
		double  FailedPercent;
		double  MinRtt;
		double  MaxRtt;
		double  AvgRtt;
		double  MinJitter;
		double  MaxJitter;
		double  AvgJitter;
		double  TotalJitter;
		double  TotalMilliseconds;

		_TCPING_STATISTICS();

		_TCPING_STATISTICS(DWORD sent, DWORD success, DWORD failed, double failPercent, double minRtt, double maxRtt, double avgRtt,
			double minJitter, double maxJitter, double avgJitter, double totalJitter, double totalMilliseconds);

	} TCPING_STATISTICS, *PTCPING_STATISTICS;

	typedef struct _TCPING_OUTPUT
	{
		::FILETIME		 Timestamp;
		WWuString        Destination;
		WWuString        DestAddress;
		DWORD            Port;
		PortProbeStatus  Status;
		double           RoundTripTime;
		double           Jitter;

		_TCPING_OUTPUT(::FILETIME timestamp, const WWuString& dest, const WWuString& destAddr, DWORD port, PortProbeStatus stat, double rtt, double jitter);
		~_TCPING_OUTPUT();

	} TCPING_OUTPUT, *PTCPING_OUTPUT;

	typedef struct _QUEUED_DATA
	{
		WriteDataType Type;
		union
		{
			PMAPPED_INFORMATION_DATA InformationData;
			PMAPPED_PROGRESS_DATA ProgressData;
			struct
			{
				WriteOutputType ObjectType;
				PVOID Object;
			} ObjectData;
			WWuString WarningData;
		};

		_QUEUED_DATA(const _QUEUED_DATA& other);
		_QUEUED_DATA(
			__in WriteDataType type,
			__in PVOID data,
			__in_opt WriteOutputType* objectData
		);

		~_QUEUED_DATA();

	} QUEUED_DATA, *PQUEUED_DATA;

	class EphemeralSocket
	{
	public:
		SOCKET UnderlyingSocket;

		EphemeralSocket(ADDRINFOW* addressInfo, const bool block = false);
		~EphemeralSocket();
	};

	// This class represents a Tcping job request. It contains the data necessary
	// to carry out the job, plus some tidbits to make it easier, like a
	// centralized stopwatch and sockets. This allows us to implement RAII
	// for these raw sockets.
	class TcpingForm
	{
	public:
		static inline BOOL WINAPI CtrlHandlerRoutine(DWORD fdwCtrlType);
		static const bool IsCtrlCHit();

		WuStopWatch StopWatch;
		WCHAR PortAsString[6] = { 0 };
		WWuString DisplayName;
		TCPING_STATISTICS Statistics;

		WWuString Destination;						// The destination. Either an IP or hostname.
		DWORD Count;								// The ping count, analogous to '-n'. Default is 4.
		DWORD Timeout;								// The timeout in seconds. Default is 2.
		DWORD SecondsInterval;						// Interval between each ping in seconds. Default is 1.
		int FailedCountThreshold;					// Number of failing attempts before giving up. Default is the same as 'count'.
		DWORD Port;									// TCP port. Default is 80.
		bool IsContinuous;							// Pings continuously. Analogous to '-t'
		bool IncludeJitter;							// Include jitter test results.
		bool PrintFqdn;								// Prints the Fully Qualified Domain Name on each line, when available.
		bool IsForce;								// Forces sending 4 bytes.
		bool Single;								// Sends only one probe, and do not display statistics.

		bool OutputToFile;							// Output result to a file. Must include the file name.
		HANDLE File;								// The file name. Only works with 'outputToFile'.
		bool Append;								// Append result to the file, instead of overwriting. Only works with 'outputToFile'.

		TcpingForm(
			const WWuString& destination,
			DWORD port,
			DWORD count,
			DWORD timeout,
			DWORD secondsInterval,
			DWORD failedThres,
			bool continuous,
			bool includeJitter,
			bool printFqdn,
			bool force,
			bool single,
			bool outputFile,
			const WWuString& filePath,
			bool append
		);

		~TcpingForm();

	private:
		static TcpingForm* _instance;
		bool _ctrlCHit;
		int _ctrlCHitCount;

		static TcpingForm* GetForm();
	};

	typedef struct _TCPING_WORKER_DATA
	{
		TcpingForm* WorkForm;
		std::queue<QUEUED_DATA>* Queue;
		bool IsComplete;
	} TCPING_WORKER_DATA, * PTCPING_WORKER_DATA;


	/*
	* ~ Get-NetworkFile
	*/

	typedef struct _NETWORK_FILE_INFO
	{
		DWORD      Id;
		DWORD      Permissions;
		DWORD      LockCount;
		WWuString  Path;
		WWuString  UserName;

		_NETWORK_FILE_INFO(DWORD id, DWORD perms, DWORD locks, const WWuString& path, const WWuString& userName);
		~_NETWORK_FILE_INFO();

	} NETWORK_FILE_INFO, * PNETWORK_FILE_INFO;

	typedef struct _NETWORK_SESSION_INFO
	{
		WWuString  ComputerSessionName;
		WWuString  UserName;
		DWORD      OpenIoCount;

		_NETWORK_SESSION_INFO(const WWuString& sessName, const WWuString& userName, DWORD ioCount);
		~_NETWORK_SESSION_INFO();

	} NETWORK_SESSION_INFO, * PNETWORK_SESSION_INFO;

	
	/*
	* ~ Test-Port
	*/
	
	typedef struct _TESTPORT_OUTPUT
	{
		::FILETIME		 Timestamp;
		WWuString        Destination;
		WWuString        DestAddress;
		DWORD            Port;
		PortProbeStatus  Status;

		_TESTPORT_OUTPUT(::FILETIME timestamp, const WWuString& destination, const WWuString& destAddress, DWORD port, PortProbeStatus status);
		~_TESTPORT_OUTPUT();

	} TESTPORT_OUTPUT, * PTESTPORT_OUTPUT;

	class TestPortForm
	{
	public:
		const WWuString& Destination() const;
		const DWORD Port() const;
		const TransportProtocol Protocol() const;
		const DWORD Timeout() const;

		LPCWSTR PortAsString() const;

		TestPortForm(const WWuString& destination, DWORD port, TransportProtocol protocol, DWORD timeoutSec);
		~TestPortForm();

	private:
		WWuString m_destination;
		DWORD m_port;
		TransportProtocol m_protocol;
		DWORD m_timeoutSec;
		WCHAR m_portAsString[6];
	};


	/*
	* ~ Get-NetworkStatistics
	*/

	enum class GetNetStatProtocol
	{
		Tcp,
		Tcpv6,
		Udp,
		Udpv6,
		Ipv4,
		Ipv6,
		Icmp,
		Icmpv6,
	};

	typedef struct _GETNETSTAT_MAIN_OUTPUT
	{
		TransportProtocol  Protocol;
		WWuString          LocalAddress;
		DWORD              LocalPort;
		WWuString          RemoteAddress;
		DWORD              RemotePort;
		PortState          State;
		DWORD              ProcessId;
		WWuString          ModuleName;

	} GETNETSTAT_MAIN_OUTPUT, *PGETNETSTAT_MAIN_OUTPUT;

	typedef struct _WU_IP_ROUTE
	{
		DWORD      InterfaceIndex;
		WWuString  NetworkDestination;
		WWuString  NetworkMask;
		WWuString  Gateway;
		DWORD      Metric;
		bool       Persistent;

	} WU_IP_ROUTE, *PWU_IP_ROUTE;

	
	/*
	* ~ Main API
	*/

	class Network
	{
	public:
		////////////////////////////////////////////////////////////////////////////////////////
		//
		//	This function, and Cmdlet are based on the great 'tcping.exe' by Eli Fulkerson.
		//	This tool is among the ones I use the most in my sysadmin career.
		//	The implementation varies a little, and also uses techniques found in the MS docs.
		//
		//	If you never heard about 'tcping.exe' I highly recommend checking out his website.
		//	http://www.elifulkerson.com/projects/
		//
		//
		//	This function "pings" a destination server using TCP sockets.
		//
		////////////////////////////////////////////////////////////////////////////////////////

		static void StartTcpPing(TcpingForm& workForm, WuNativeContext* context);

		// Get-NetworkFile (PsFile)

		static void ListNetworkFiles(const WWuString& computerName, const WWuString& basePath, const WWuString& userName, WuList<NETWORK_FILE_INFO>& result);
		static void ListNetworkFiles(const WWuString& computerName, const WWuString& basePath, const WWuString& userName, WuList<NETWORK_FILE_INFO>& fileInfo, WuList<NETWORK_SESSION_INFO>& sessionInfo);

		// Close-NetworkFile (PsFile)

		static void CloseNetworkFile(const WWuString& computerName, DWORD fileId);

		// Test-Port

		static void TestNetworkPort(const TestPortForm& workForm, WuNativeContext* context);

		// Get-NetworkStatistics
		
		static void GetTcpTables(bool includeModuleName, WuList<GETNETSTAT_MAIN_OUTPUT>& output, std::unordered_map<DWORD, WWuString>& processList, WuNativeContext* context);
		static void GetUdpTables(bool includeModuleName, WuList<GETNETSTAT_MAIN_OUTPUT>& output, std::unordered_map<DWORD, WWuString>& processList, WuNativeContext* context);
		static void GetInterfaceStatistics(WuList<MIB_IF_ROW2>& output, WuNativeContext* context);
		static void GetIpRouteTable(WuList<WU_IP_ROUTE>& output, WuNativeContext* context);
		static void GetIpv4Statistics(std::unordered_map<GetNetStatProtocol, MIB_IPSTATS>& output, WuNativeContext* context);
		static void GetIpv6Statistics(std::unordered_map<GetNetStatProtocol, MIB_IPSTATS>& output, WuNativeContext* context);
		static void GetIcmpv4Statistics(std::unordered_map<GetNetStatProtocol, MIB_ICMP_EX>& output, WuNativeContext* context);
		static void GetIcmpv6Statistics(std::unordered_map<GetNetStatProtocol, MIB_ICMP_EX>& output, WuNativeContext* context);
		static void GetTcpv4Statistics(std::unordered_map<GetNetStatProtocol, MIB_TCPSTATS>& output, WuNativeContext* context);
		static void GetTcpv6Statistics(std::unordered_map<GetNetStatProtocol, MIB_TCPSTATS>& output, WuNativeContext* context);
		static void GetUdpv4Statistics(std::unordered_map<GetNetStatProtocol, MIB_UDPSTATS>& output, WuNativeContext* context);
		static void GetUdpv6Statistics(std::unordered_map<GetNetStatProtocol, MIB_UDPSTATS>& output, WuNativeContext* context);

	private:
		// Utilities
		static void PerformSingleTestProbe(ADDRINFOW* singleInfo, TcpingForm* workForm, const WWuString& displayName,
			PTCPING_STATISTICS statistics, std::queue<QUEUED_DATA>* infoQueue, DWORD& result);

		static void ProcessStatistics(TcpingForm* workForm, WuNativeContext* context);

		static DWORD WINAPI StartTcpingWorker(LPVOID params);
		static void PrintHeader(TcpingForm* workForm, const WWuString& displayText);
		static void PrintQueueData(const QUEUED_DATA& data, WuNativeContext* context);
		static void FormatIp(ADDRINFOW* address, WWuString& ipString);
		static void ReverseIp(WWuString& ip);
		static void ReverseIPv6(WWuString& ip);
		static void ResolveIpToDomainName(const WWuString& ip, WWuString& domainName);
		static void ResolveIpv6ToDomainName(WWuString& ip, WWuString& reverse);

		static constexpr USHORT UshortByteSwap(const USHORT value)
		{
			return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
		}
	};
}