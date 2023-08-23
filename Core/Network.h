#pragma once

#pragma unmanaged

#include "Common.h"
#include "String.h"
#include "Expressions.h"
#include "Notification.h"

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

		typedef enum _TCPING_SUPPORTED_HTTP_METHOD
		{
			GET,
			POST,
			HEAD
		} TCPING_SUPPORTED_HTTP_METHOD;

		class EphemeralSocket
		{
		public:
			const SOCKET GetSocket() const;
			const WuResult& Result() const;

			EphemeralSocket(ADDRINFO* addressInfo);
			~EphemeralSocket();

		private:
			SOCKET _underlyingSocket;
			ADDRINFO* _addressInfo;
			WuResult _connectSendResult;

			WuResult ConnectSend();
		};

		// This class represents a Tcping job request. It contains the data necessary
		// to carry out the job, plus some tidbits to make it easier, like a
		// centralized stopwatch and sockets. This allows us to implement RAII
		// for these raw sockets.
		class TcpingForm
		{
		public:
			inline WuStopWatch& StopWatch();
			inline PCSTR PortAsString() const;

			inline const WWuString& Destination() const;					// The destination. Either an IP or hostname.
			inline const DWORD Count() const;								// The ping count, analogous to '-n'. Default is 4.
			inline const DWORD SecondsInterval() const;						// Interval between each ping in seconds. Default is 1.
			inline const PREFERRED_IP_PROTOCOL PreferredIpProtocol() const;	// Preferred IP protocol.
			inline const int FailedCountThreshold() const;					// Number of failing attempts before giving up. Default is the same as 'count'.
			inline const DWORD Port() const;								// TCP port. Default is 80.
			inline const bool IsContinuous() const;							// Pings continuously. Analogous to '-t'
			inline const bool IncludeJitter() const;						// Include jitter test results.
			inline const bool IncludeDateTime() const;						// Includes date and time on each line.
			inline const bool PrintFqdn() const;							// Prints the Fully Qualified Domain Name on each line, when available.
			inline const bool IsForce() const;								// Forces sending 4 bytes.

			inline const bool OutputToFile() const;							// Output result to a file. Must include the file name.
			inline const WWuString* FileName() const;						// The file name. Only works with 'outputToFile'.
			inline const bool Append() const;								// Append result to the file, instead of overwriting. Only works with 'outputToFile'.

			inline const bool IsHttpMode() const;							// HTTP mode. Must include 'httpSettings'.
			inline const bool PrintUrl() const;								// Prints URL on each line. Only works with 'httpMode'.
			inline const TCPING_SUPPORTED_HTTP_METHOD* HttpMethod() const;	// HTTP Method. Default is 'GET'.

			TcpingForm(
				const WWuString& destination,
				DWORD port,
				DWORD count,
				DWORD secondsInterval,
				PREFERRED_IP_PROTOCOL ipProt,
				DWORD failedThres,
				bool continuous,
				bool includeJitter,
				bool includeDateTime,
				bool printFqdn,
				bool force,
				bool outputFile,
				const WWuString& fileName,
				bool append,
				bool httpMode,
				bool printUrl,
				TCPING_SUPPORTED_HTTP_METHOD httpMethod
			);

			~TcpingForm();

		private:
			WuStopWatch _stopwatch;
			wuvector<EphemeralSocket> _sockets;
			CHAR _portString[6];

			WWuString _destination;
			DWORD _count;
			DWORD _secondsInterval;
			PREFERRED_IP_PROTOCOL _preferredIpProtocol;
			int _failedCountThreshold;
			DWORD _port;
			bool _continuous;
			bool _includeJitter;
			bool _includeDateTime;
			bool _printFqdn;
			bool _force;

			bool _outputToFile;
			WWuString _fileFullName;
			bool _append;

			bool _httpMode;
			bool _printUrl;
			TCPING_SUPPORTED_HTTP_METHOD _httpMethod;
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
		//	This function "pings" a destination server using TCP sockets, or HTTP.
		//
		////////////////////////////////////////////////////////////////////////////////////////

		WuResult StartTcpPinging(TcpingForm& workForm, Notification::PNATIVE_CONTEXT context);
	};

	WuString FormatIp(ADDRINFO* address);
}