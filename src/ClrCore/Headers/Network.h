#pragma once

#pragma unmanaged

#include "IO.h"
#include "Common.h"
#include "String.h"
#include "Expressions.h"
#include "Notification.h"

#define CHECKIFCTRLC if (Network::TcpingForm::IsCtrlCHit()) { finalResult = ERROR_CANCELLED; goto END; }

// #define _TCPING_TEST

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) Network
	{
	public:
		typedef enum _PREFERRED_IP_PROTOCOL
		{
			None,
			IPv4,
			IPv6
		} PREFERRED_IP_PROTOCOL;

		typedef enum _TCPING_STATUS : WORD
		{
			Open,
			Closed,
			Timeout
		} TCPING_STATUS;

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
					MinJitter(0.00), MaxJitter(0.00), AvgJitter(0.00), TotalJitter(0.00), TotalMilliseconds(0.00) { }

		} TCPING_STATISTICS, *PTCPING_STATISTICS;

		typedef struct _TCPING_OUTPUT
		{
			FILETIME Timestamp;
			LPWSTR Destination;
			LPWSTR DestAddress;
			DWORD Port;
			TCPING_STATUS Status;
			double RoundTripTime;
			double Jitter;

			_TCPING_OUTPUT(FILETIME timestamp, const LPWSTR dest, LPWSTR destAddr, DWORD port, TCPING_STATUS stat, double rtt, double jitter)
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

		} TCPING_OUTPUT, *PTCPING_OUTPUT;

		class EphemeralSocket
		{
		public:
			SOCKET UnderlyingSocket;
			
			EphemeralSocket(ADDRINFO* addressInfo);
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
			CHAR PortAsString[6] = { 0 };

			WWuString Destination;						// The destination. Either an IP or hostname.
			DWORD Count;								// The ping count, analogous to '-n'. Default is 4.
			DWORD Timeout;								// The timeout in seconds. Default is 2.
			DWORD SecondsInterval;						// Interval between each ping in seconds. Default is 1.
			PREFERRED_IP_PROTOCOL PreferredIpProtocol;	// Preferred IP protocol.
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
				PREFERRED_IP_PROTOCOL ipProt,
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
	};

	void PerformSingleTestProbe(ADDRINFO* singleInfo, Network::TcpingForm* workForm, const WWuString& displayName, Network::PTCPING_STATISTICS statistics, WuNativeContext* context, DWORD& result);
	void PerformSocketConnectSend(ADDRINFO* addressInfo, Network::TcpingForm* workForm, DWORD& result);
	void ProcessStatistics(const Network::PTCPING_STATISTICS statistics, const WWuString& displayName, Network::TcpingForm* workForm, WuNativeContext* context);
	void PrintHeader(Network::TcpingForm* workForm, const WWuString& displayText, WuNativeContext* context);
	void FormatIp(ADDRINFO* address, WWuString& ipString);
	void ReverseIp(WWuString& ip);
	void ReverseIPv6(WWuString& ip);
	void ResolveIpToDomainName(const WWuString& ip, WWuString& domainName);
	void ResolveIpv6ToDomainName(WWuString& ip, WWuString& reverse);
}