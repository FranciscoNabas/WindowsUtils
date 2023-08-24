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
		if (_underlyingSocket == INVALID_SOCKET)
			_connectResult = WuResult(WSAGetLastError(), __FILEW__, __LINE__);
		else {
			// Setting the IO mode to non-blocking.
			u_long mode = 1;
			ioctlsocket(_underlyingSocket, FIONBIO, &mode);

			// Connecting.
			int result = connect(_underlyingSocket, addressInfo->ai_addr, (int)addressInfo->ai_addrlen);
			if (result == SOCKET_ERROR) {
				result = WSAGetLastError();
				if (result != ERROR_SUCCESS && result != WSAEWOULDBLOCK)
					_connectResult = WuResult(WSAGetLastError(), __FILEW__, __LINE__);
				else
					_connectResult = WuResult();
			}
			else {
				_connectResult = WuResult();
			}
		}
		
		_addressInfo = addressInfo;
	}

	Network::EphemeralSocket::~EphemeralSocket()
	{
		if (_underlyingSocket != INVALID_SOCKET) {
			shutdown(_underlyingSocket, SD_SEND);
			closesocket(_underlyingSocket);
		}
	}

	const SOCKET Network::EphemeralSocket::GetSocket() const { return _underlyingSocket; }
	const WuResult& Network::EphemeralSocket::ConnectResult() const { return _connectResult; }

	WuResult Network::EphemeralSocket::TrySend(bool force)
	{
		int result;
		if (force) {
			result = send(_underlyingSocket, "tits", 4, 0);
			if (result == SOCKET_ERROR)
				return WuResult(WSAGetLastError(), __FILEW__, __LINE__);
		}
		else {
			result = send(_underlyingSocket, NULL, 0, 0);
			if (result == SOCKET_ERROR)
				return WuResult(WSAGetLastError(), __FILEW__, __LINE__);
		}

		return WuResult();
	}

	/*
	*	~ Tcping form ~
	*/

	Network::TcpingForm::TcpingForm(
		const WWuString& destination,
		DWORD port,
		DWORD count,
		DWORD timeout,
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
	) :_destination(destination), _port(port), _count(count), _timeout(timeout), _secondsInterval(secondsInterval), _preferredIpProtocol(ipProt),
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

		_instance = this;
		_ctrlCHit = false;
		_ctrlCHitCount = 0;
		SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);
	}

	Network::TcpingForm::~TcpingForm()
	{
		WSACleanup();
		SetConsoleCtrlHandler(CtrlHandlerRoutine, FALSE);
		_ctrlCHit = false;
	}

	Network::TcpingForm* Network::TcpingForm::_instance = { nullptr };
	Network::TcpingForm* Network::TcpingForm::GetForm() { return _instance; }

	inline BOOL WINAPI Network::TcpingForm::CtrlHandlerRoutine(DWORD fdwCtrlType) {
		Network::TcpingForm* instance = GetForm();
		switch (fdwCtrlType) {
			case CTRL_C_EVENT:
			{
				// Loved so much in tcping.exe that I had to bring it over =D.
				if (instance->_ctrlCHit && instance->_ctrlCHitCount < 2) {
					printf_s("Wow, you really mean it. I'm sorry... I'll stop now. :(");
					instance->_ctrlCHitCount++;
					return TRUE;
				}

				instance->_ctrlCHit = true;
				instance->_ctrlCHitCount++;

				return TRUE;
			}

			default:
				return FALSE;
		}
	}

	inline const bool Network::TcpingForm::IsCtrlCHit() {
		Network::TcpingForm* instance = GetForm();
		if (instance == nullptr)
			return false;

		return instance->_ctrlCHit;
	}

	inline WuStopWatch& Network::TcpingForm::StopWatch() { return _stopwatch; }
	inline PCSTR Network::TcpingForm::PortAsString() const { return _portString; }

	inline const WWuString& Network::TcpingForm::Destination() const { return _destination; }
	inline const DWORD Network::TcpingForm::Count() const { return _count; }
	inline const DWORD Network::TcpingForm::Timeout() const { return _timeout; }
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
		// Defining environment.
		int intResult;
		ADDRINFO hints = { 0 }, * addressInfo, * singleInfo;

		double milliseconds = 0.0;
		double totalMilliseconds = 0.0;
		double jitter = 0.0;
		DWORD successCount = 0;
		DWORD failedCount = 0;

		auto prefIpProto = workForm.PreferredIpProtocol();

		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_UNSPEC;

		// Attempting to get destination address information.
		WuString narrowDest = WWuStringToNarrow(workForm.Destination());
		intResult = getaddrinfo(narrowDest.GetBuffer(), workForm.PortAsString(), &hints, &addressInfo);
		if (intResult != 0)
			return WuResult(intResult, __FILEW__, __LINE__);

		WuResult lastResult;
		for (singleInfo = addressInfo; singleInfo != NULL; singleInfo = singleInfo->ai_next) {
			CHECKIFCTRLC;
			
			if ((singleInfo->ai_family == AF_UNSPEC && prefIpProto == None) ||
				(singleInfo->ai_family == AF_INET && prefIpProto != IPv6) ||
				(singleInfo->ai_family == AF_INET6 && prefIpProto != IPv4)
				)
				break;
			else
				return WuResult(WSAHOST_NOT_FOUND, __FILEW__, __LINE__);
		}

		// Main loop. Here the 'ping' will happen for 'count' times.
		if (workForm.IsContinuous()) {
			/*while (!_CtrlCHit_) {

			}*/

			while (true) {

			}
		}
		else {
			for (DWORD i = 0; i < workForm.Count(); i++) {
				CHECKIFCTRLC;

				WuString destText = FormatIp(addressInfo);
				WWuString outputText;

				workForm.StopWatch().Restart();
				WuResult sendResult = PerformSocketConnectSend(singleInfo, &workForm);
				workForm.StopWatch().Stop();
				if (sendResult.Result == WSAETIMEDOUT) {
					CHECKIFCTRLC;
					outputText = WWuString::Format(L"Probing %s - TCP:%d - No response - time=%dms", destText.GetBuffer(), workForm.Port(), (workForm.Timeout() * 1000));
					
					// Testing before using 'NativeWriteInformation'.
					wprintf(L"%ws\n", outputText.GetBuffer());

					continue;
				}
				if (sendResult.Result == ERROR_CANCELLED)
					return WuResult(ERROR_CANCELLED, __FILEW__, __LINE__);

				// DONT FORGET THE PRINTFQDN OPTION!!!!!!!
				outputText = WWuString::Format(L"Probing %s - TCP:%d - Port is open - time=%.2fms", destText.GetBuffer(), workForm.Port(), workForm.StopWatch().Elapsed().TotalMilliseconds);
				
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
		}
		
		
		// Fun fact, during testing I forgot this little fellow over here,
		// and was getting access violation on trying to destroy a WuString.
		// Ah C++... S2.
		return WuResult();
	}

	WuResult PerformSocketConnectSend(ADDRINFO* addressInfo, Network::TcpingForm* workForm)
	{
		// Defining environment.
		
		WuStopWatch spw;

		// Creating an ephemeral socket.
		Network::EphemeralSocket ephSocket(addressInfo);
		if (ephSocket.ConnectResult().Result != ERROR_SUCCESS) {
			// If it fails to connect, we bail.
			return ephSocket.ConnectResult();
		}

		// Trying to send until we succeed or timeout.
		WuResult sendResult;
		spw.Start();
		while (!Network::TcpingForm::IsCtrlCHit()) {
			sendResult = ephSocket.TrySend(workForm->IsForce());
			if (sendResult.Result == ERROR_SUCCESS)
				return sendResult;

			if (spw.Elapsed().TotalSeconds >= workForm->Timeout())
				break;

			Sleep(1);
		}

		// At this point we failed.
		return WuResult(WSAETIMEDOUT, __FILEW__, __LINE__);
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