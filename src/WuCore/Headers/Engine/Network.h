#pragma once
#pragma unmanaged

#include "../../Headers/Support/String.h"
#include "../../Headers/Support/CoreUtils.h"
#include "../../Headers/Support/Notification.h"

#include <WinSock2.h>
#include <ws2def.h>

namespace WindowsUtils::Core
{
	enum class PortStatus : WORD
	{
		Open,
		Closed,
		Timeout
	};

	enum class TestPortProtocol : WORD
	{
		Tcp,
		Udp
	};

	typedef struct _TCPING_STATISTICS
	{
		DWORD Sent;
		DWORD Successful;
		DWORD Failed;
		double FailedPercent;
		double MinRtt;
		double MaxRtt;
		double AvgRtt;
		double MinJitter;
		double MaxJitter;
		double AvgJitter;
		double TotalJitter;
		double TotalMilliseconds;

		_TCPING_STATISTICS()
			: Sent(0), Successful(0), Failed(0), FailedPercent(0), MinRtt(0.00), MaxRtt(0.00), AvgRtt(0.00),
			MinJitter(0.00), MaxJitter(0.00), AvgJitter(0.00), TotalJitter(0.00), TotalMilliseconds(0.00)
		{ }

		_TCPING_STATISTICS(DWORD sent, DWORD success, DWORD failed, double failPercent, double minRtt, double maxRtt, double avgRtt,
			double minJitter, double maxJitter, double avgJitter, double totalJitter, double totalMilliseconds
		)
			: Sent(sent), Successful(success), Failed(failed), FailedPercent(FailedPercent), MinRtt(minRtt), MaxRtt(maxRtt), AvgRtt(AvgRtt),
			MinJitter(MinJitter), MaxJitter(maxJitter), AvgJitter(avgJitter), TotalJitter(totalJitter), TotalMilliseconds(TotalMilliseconds)
		{ }

	} TCPING_STATISTICS, * PTCPING_STATISTICS;

	typedef struct _TCPING_OUTPUT
	{
		::FILETIME Timestamp;
		LPWSTR Destination;
		LPWSTR DestAddress;
		DWORD Port;
		PortStatus Status;
		double RoundTripTime;
		double Jitter;

		_TCPING_OUTPUT(::FILETIME timestamp, const LPWSTR dest, LPWSTR destAddr, DWORD port, PortStatus stat, double rtt, double jitter)
			: Timestamp(timestamp), Port(port), Status(stat), RoundTripTime(rtt), Jitter(jitter)
		{
			if (dest != NULL) {
				size_t destLen = wcslen(dest) + 1;
				Destination = new WCHAR[destLen];
				wcscpy_s(Destination, destLen, dest);
			}
			else
				Destination = NULL;

			if (destAddr != NULL) {
				size_t destAddrLen = wcslen(destAddr) + 1;
				DestAddress = new WCHAR[destAddrLen];
				wcscpy_s(DestAddress, destAddrLen, destAddr);
			}
			else
				DestAddress = NULL;
		}

		~_TCPING_OUTPUT()
		{
			if (Destination != NULL) {
				delete[] Destination;
			}

			if (DestAddress != NULL) {
				delete[] DestAddress;
			}
		}

	} TCPING_OUTPUT, * PTCPING_OUTPUT;

	typedef struct _TESTPORT_OUTPUT
	{
		::FILETIME Timestamp;
		LPWSTR Destination;
		LPWSTR DestAddress;
		DWORD Port;
		PortStatus Status;

		_TESTPORT_OUTPUT(::FILETIME timestamp, const LPWSTR destination, const LPWSTR destAddress, DWORD port, PortStatus status)
		{
			if (destination != NULL) {
				size_t destLen = wcslen(destination) + 1;
				Destination = new WCHAR[destLen];
				wcscpy_s(Destination, destLen, destination);
			}
			else
				Destination = NULL;

			if (destAddress != NULL) {
				size_t destAddrLen = wcslen(destAddress) + 1;
				DestAddress = new WCHAR[destAddrLen];
				wcscpy_s(DestAddress, destAddrLen, destAddress);
			}
			else
				DestAddress = NULL;

			Timestamp = timestamp;
			Port = port;
			Status = status;
		}

		~_TESTPORT_OUTPUT()
		{
			if (Destination != NULL)
				delete[] Destination;

			if (DestAddress != NULL)
				delete[] DestAddress;
		}

	} TESTPORT_OUTPUT, * PTESTPORT_OUTPUT;

	class EphemeralSocket
	{
	public:
		SOCKET UnderlyingSocket;

		EphemeralSocket(ADDRINFOW* addressInfo);
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

	} QUEUED_DATA, * PQUEUED_DATA;

	typedef struct _TCPING_WORKER_DATA
	{
		TcpingForm* WorkForm;
		wuqueue<QUEUED_DATA>* Queue;
		bool IsComplete;
	} TCPING_WORKER_DATA, * PTCPING_WORKER_DATA;

	// Get-NetworkFile

	typedef struct _NETWORK_FILE_INFO
	{
		DWORD Id;
		DWORD Permissions;
		DWORD LockCount;
		WWuString Path;
		WWuString UserName;

		_NETWORK_FILE_INFO(DWORD id, DWORD perms, DWORD locks, const WWuString& path, const WWuString& userName);
		~_NETWORK_FILE_INFO();

	} NETWORK_FILE_INFO, * PNETWORK_FILE_INFO;

	typedef struct _NETWORK_SESSION_INFO
	{
		WWuString ComputerSessionName;
		WWuString UserName;
		DWORD OpenIoCount;

		_NETWORK_SESSION_INFO(const WWuString& sessName, const WWuString& userName, DWORD ioCount);
		~_NETWORK_SESSION_INFO();

	} NETWORK_SESSION_INFO, * PNETWORK_SESSION_INFO;

	class TestPortForm
	{
	public:
		const WWuString& Destination() const;
		const DWORD Port() const;
		const TestPortProtocol Protocol() const;
		const DWORD Timeout() const;

		LPCWSTR PortAsString() const;

		TestPortForm(const WWuString& destination, DWORD port, TestPortProtocol protocol, DWORD timeoutSec);
		~TestPortForm();

	private:
		WWuString m_destination;
		DWORD m_port;
		TestPortProtocol m_protocol;
		DWORD m_timeoutSec;
		WCHAR m_portAsString[6];
	};

	extern "C" public class __declspec(dllexport) Network
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

		void StartTcpPing(TcpingForm& workForm, WuNativeContext* context);

		// Get-NetworkFile (PsFile)

		void ListNetworkFiles(const WWuString& computerName, const WWuString& basePath, const WWuString& userName, wuvector<NETWORK_FILE_INFO>& result);
		void ListNetworkFiles(const WWuString& computerName, const WWuString& basePath, const WWuString& userName, wuvector<NETWORK_FILE_INFO>& fileInfo, wuvector<NETWORK_SESSION_INFO>& sessionInfo);

		// Close-NetworkFile (PsFile)

		void CloseNetworkFile(const WWuString& computerName, DWORD fileId);

		// Test-Port

		void TestNetworkPort(const TestPortForm& workForm, WuNativeContext* context);
	};

	void PerformSingleTestProbe(ADDRINFOW* singleInfo, TcpingForm* workForm, const WWuString& displayName,
		PTCPING_STATISTICS statistics, wuqueue<QUEUED_DATA>* infoQueue, DWORD& result);

	void ProcessStatistics(TcpingForm* workForm, WuNativeContext* context);

	DWORD WINAPI StartTcpingWorker(PVOID params);
	void PrintHeader(TcpingForm* workForm, const WWuString& displayText);
	void PrintQueueData(const QUEUED_DATA& data, WuNativeContext* context);
	void FormatIp(ADDRINFOW* address, WWuString& ipString);
	void ReverseIp(WWuString& ip);
	void ReverseIPv6(WWuString& ip);
	void ResolveIpToDomainName(const WWuString& ip, WWuString& domainName);
	void ResolveIpv6ToDomainName(WWuString& ip, WWuString& reverse);
}