#include "pch.h"

#include "Network.h"

namespace WindowsUtils::Core
{
	/*
	*	~ Ephemeral socket ~
	*/

	Network::EphemeralSocket::EphemeralSocket(ADDRINFO* addressInfo)
	{
		_underlyingSocket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
		_addressInfo = addressInfo;

		_connectSendResult = ConnectSend();
	}

	Network::EphemeralSocket::~EphemeralSocket()
	{
		if (_underlyingSocket != INVALID_SOCKET) {
			shutdown(_underlyingSocket, SD_SEND);
			closesocket(_underlyingSocket);
		}
	}

	const SOCKET Network::EphemeralSocket::GetSocket() const { return _underlyingSocket; }
	const WuResult& Network::EphemeralSocket::Result() const { return _connectSendResult; }

	WuResult Network::EphemeralSocket::ConnectSend()
	{
		if (_underlyingSocket == INVALID_SOCKET)
			return WuResult(WSAGetLastError(), __FILEW__, __LINE__);

		int result = connect(_underlyingSocket, _addressInfo->ai_addr, (int)_addressInfo->ai_addrlen);
		if (result == SOCKET_ERROR)
			return WuResult(WSAGetLastError(), __FILEW__, __LINE__);

		result = send(_underlyingSocket, "tits", 4, NULL);
		if (result == SOCKET_ERROR)
			return WuResult(WSAGetLastError(), __FILEW__, __LINE__);

		return WuResult();
	}

	/*
	*	~ Tcping form ~
	*/

	Network::TcpingForm::TcpingForm(
		const WWuString& destination,
		DWORD port,
		DWORD count,
		DWORD secondsInterval,
		PREFERRED_IP_PROTOCOL ipProt,
		DWORD failedThreshold,
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
	) :_destination(destination), _port(port), _count(count), _secondsInterval(secondsInterval), _preferredIpProtocol(ipProt),
		_failedCountThreshold(failedThreshold), _continuous(continuous), _includeJitter(includeJitter), _includeDateTime(includeDateTime),
		_printFqdn(printFqdn), _force(force), _outputToFile(outputFile), _fileFullName(fileName), _append(append), _httpMode(httpMode),
		_printUrl(printUrl), _httpMethod(httpMethod)
	{
		WSADATA wsaData;
		WORD reqVersion = MAKEWORD(2, 2);
		int result;

		if ((result = WSAStartup(reqVersion, &wsaData)) != 0)
			throw result;

		_ui64toa_s(_port, _portString, 6, 10);
	}

	Network::TcpingForm::~TcpingForm()
	{
		WSACleanup();
	}

	inline WuStopWatch& Network::TcpingForm::StopWatch() { return _stopwatch; }
	inline PCSTR Network::TcpingForm::PortAsString() const { return _portString; }

	inline const WWuString& Network::TcpingForm::Destination() const { return _destination; }
	inline const DWORD Network::TcpingForm::Count() const { return _count; }
	inline const DWORD Network::TcpingForm::SecondsInterval() const { return _secondsInterval; }
	inline const Network::PREFERRED_IP_PROTOCOL Network::TcpingForm::PreferredIpProtocol() const { return _preferredIpProtocol; }
	inline const int Network::TcpingForm::FailedCountThreshold() const { return _failedCountThreshold; }
	inline const DWORD Network::TcpingForm::Port() const { return _port; }
	inline const bool Network::TcpingForm::IsContinuous() const { return _continuous; }
	inline const bool Network::TcpingForm::IncludeJitter() const { return _includeJitter; }
	inline const bool Network::TcpingForm::IncludeDateTime() const { return _includeDateTime; }
	inline const bool Network::TcpingForm::PrintFqdn() const { return _printFqdn; }
	inline const bool Network::TcpingForm::IsForce() const { return _force; }
	inline const bool Network::TcpingForm::OutputToFile() const { return _outputToFile; }
	inline const WWuString* Network::TcpingForm::FileName() const { return &_fileFullName; }
	inline const bool Network::TcpingForm::Append() const { return _append; }
	inline const bool Network::TcpingForm::IsHttpMode() const { return _httpMode; }
	inline const bool Network::TcpingForm::PrintUrl() const { return _printUrl; }
	inline const Network::TCPING_SUPPORTED_HTTP_METHOD* Network::TcpingForm::HttpMethod() const { return &_httpMethod; }

	/*
	*	~ Function definition ~
	*/

	WuResult Network::StartTcpPinging(TcpingForm& workForm, Notification::PNATIVE_CONTEXT context)
	{
		int intResult;
		ADDRINFO hints = { 0 }, * addressInfo, * singleInfo;

		double milliseconds = 0.0;
		double totalMilliseconds = 0.0;
		double jitter = 0.0;
		DWORD successCount = 0;

		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		WuString narrowDest = WWuStringToNarrow(workForm.Destination());
		intResult = getaddrinfo(narrowDest.GetBuffer(), workForm.PortAsString(), &hints, &addressInfo);

		WuResult lastResult;
		bool found = false;
		for (singleInfo = addressInfo; singleInfo != NULL; singleInfo = singleInfo->ai_next) {
			SOCKET tempSocket = socket(singleInfo->ai_family, singleInfo->ai_socktype, singleInfo->ai_protocol);
			if (tempSocket == INVALID_SOCKET)
				return WuResult(WSAGetLastError(), __FILEW__, __LINE__);

			intResult = connect(tempSocket, singleInfo->ai_addr, (int)singleInfo->ai_addrlen);
			if (intResult == SOCKET_ERROR) {
				closesocket(tempSocket);
				lastResult = WuResult(WSAGetLastError(), __FILEW__, __LINE__);
				continue;
			}

			found = true;
			break;
		}

		if (!found)
			return lastResult;

		for (DWORD i = 0; i < workForm.Count(); i++) {
			workForm.StopWatch().Restart();
			EphemeralSocket ephSocket(singleInfo);
			workForm.StopWatch().Stop();
			
			if (ephSocket.Result().Result != ERROR_SUCCESS) {
				// HANDLE IT! PRINT BAD RESULT.
				continue;
			}

			// DONT FORGET THE PRINTFQDN OPTION!!!!!!!
			WuString destText = FormatIp(singleInfo);
			WWuString outputText = WWuString::Format(L"Probing %s:%d(tcp) - Port is open - time=%.2fms", destText.GetBuffer(), workForm.Port(), workForm.StopWatch().Elapsed().TotalMilliseconds);
			if (workForm.IncludeJitter() && successCount >= 1) {
				double currentJitter = workForm.StopWatch().Elapsed().TotalMilliseconds - (totalMilliseconds / successCount);
				currentJitter = abs(currentJitter);

				outputText = WWuString::Format(L"%ws jitter=%.2fms", outputText.GetBuffer(), currentJitter);
			}

			successCount++;
			totalMilliseconds += workForm.StopWatch().Elapsed().TotalMilliseconds;

			// Testing before using 'NativeWriteInformation'.
			wprintf(L"%ws\n", outputText.GetBuffer());

			Sleep(workForm.SecondsInterval() * 1000);
		}

		return WuResult();
	}

	// 100% from tcping.exe
	WuString FormatIp(ADDRINFO* address)
	{
		char buffer[46];
		DWORD ret;

		switch (address->ai_family) {
			case AF_INET:
			{
				struct sockaddr_in* sockaddr_ipv4;
				sockaddr_ipv4 = (struct sockaddr_in*)address->ai_addr;
			} break;
			case AF_INET6:
				ret = getnameinfo(address->ai_addr, (int)address->ai_addrlen, buffer, sizeof(buffer), NULL, 0, NI_NUMERICHOST);
				break;
		}

		WuString abuffer(buffer);

		return abuffer;
	}
}