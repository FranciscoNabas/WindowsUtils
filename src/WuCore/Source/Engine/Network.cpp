#include "../../pch.h"

#include "../../Headers/Engine/Network.h"

namespace WindowsUtils::Core
{
	/*
	*	~ TCPING_STATISTICS ~
	*/

	_TCPING_STATISTICS::_TCPING_STATISTICS()
		: Sent(0), Successful(0), Failed(0), FailedPercent(0), MinRtt(0.00), MaxRtt(0.00), AvgRtt(0.00),
		MinJitter(0.00), MaxJitter(0.00), AvgJitter(0.00), TotalJitter(0.00), TotalMilliseconds(0.00) { }

	_TCPING_STATISTICS::_TCPING_STATISTICS(DWORD sent, DWORD success, DWORD failed, double failPercent, double minRtt, double maxRtt, double avgRtt,
		double minJitter, double maxJitter, double avgJitter, double totalJitter, double totalMilliseconds)
		: Sent(sent), Successful(success), Failed(failed), FailedPercent(FailedPercent), MinRtt(minRtt), MaxRtt(maxRtt), AvgRtt(AvgRtt),
		MinJitter(MinJitter), MaxJitter(maxJitter), AvgJitter(avgJitter), TotalJitter(totalJitter), TotalMilliseconds(TotalMilliseconds) { }


	/*
	*	~ TCPING_OUTPUT ~
	*/

	_TCPING_OUTPUT::_TCPING_OUTPUT(FILETIME timestamp, const WWuString& dest, const WWuString& destAddr, DWORD port, PortProbeStatus stat, double rtt, double jitter)
		: Timestamp(timestamp), Port(port), Status(stat), RoundTripTime(rtt), Jitter(jitter), Destination(dest), DestAddress(destAddr) { }
	
	_TCPING_OUTPUT::~_TCPING_OUTPUT() { }


	/*
	*	~ Queued data ~
	*/

	_QUEUED_DATA::_QUEUED_DATA(const _QUEUED_DATA& other)
	{
		Type = other.Type;
		switch (other.Type) {
		case WriteDataType::Information:
			InformationData = new MAPPED_INFORMATION_DATA(
				other.InformationData->Computer,
				other.InformationData->NativeThreadId,
				other.InformationData->Text,
				other.InformationData->Source,
				other.InformationData->TimeGenerated,
				other.InformationData->User
			);
			break;

		case WriteDataType::Object:
		{
			ObjectData.ObjectType = other.ObjectData.ObjectType;
			switch (other.ObjectData.ObjectType) {

			case WriteOutputType::TcpingOutput:
			{
				auto objData = reinterpret_cast<PTCPING_OUTPUT>(other.ObjectData.Object);
				ObjectData.Object = new TCPING_OUTPUT(
					objData->Timestamp,
					objData->Destination,
					objData->DestAddress,
					objData->Port,
					objData->Status,
					objData->RoundTripTime,
					objData->Jitter
				);
			} break;

			case WriteOutputType::TcpingStatistics:
			{
				auto objData = reinterpret_cast<PTCPING_STATISTICS>(other.ObjectData.Object);
				ObjectData.Object = new TCPING_STATISTICS(
					objData->Sent,
					objData->Successful,
					objData->Failed,
					objData->FailedPercent,
					objData->MinRtt,
					objData->MaxRtt,
					objData->AvgRtt,
					objData->MinJitter,
					objData->MaxJitter,
					objData->AvgJitter,
					objData->TotalJitter,
					objData->TotalMilliseconds
				);
			} break;
			}
		} break;

		case WriteDataType::Progress:
			ProgressData = new MAPPED_PROGRESS_DATA(*other.ProgressData);
			break;

		case WriteDataType::Warning:
			WarningData = WWuString(other.WarningData);
			break;
		}
	}

	_QUEUED_DATA::_QUEUED_DATA(
		__in WriteDataType type,
		__in PVOID data,
		__in_opt WriteOutputType* objectData
	)
	{
		Type = type;
		switch (type) {
		case WriteDataType::Information:
		{
			auto infoData = reinterpret_cast<PMAPPED_INFORMATION_DATA>(data);
			InformationData = new MAPPED_INFORMATION_DATA(*infoData);
		} break;

		case WriteDataType::Object:
		{
			ObjectData.ObjectType = *objectData;
			switch (*objectData) {

			case WriteOutputType::TcpingOutput:
			{
				auto objData = reinterpret_cast<PTCPING_OUTPUT>(data);
				ObjectData.Object = new TCPING_OUTPUT(*objData);
			} break;

			case WriteOutputType::TcpingStatistics:
			{
				auto objData = reinterpret_cast<PTCPING_STATISTICS>(data);
				ObjectData.Object = new TCPING_STATISTICS(*objData);
			} break;
			}
		} break;

		case WriteDataType::Progress:
		{
			auto progData = reinterpret_cast<PMAPPED_PROGRESS_DATA>(data);
			ProgressData = new MAPPED_PROGRESS_DATA(*progData);
		} break;

		case WriteDataType::Warning:
		{
			auto warnData = reinterpret_cast<WWuString*>(data);
			WarningData = WWuString(*warnData);
		} break;
		}
	}

	_QUEUED_DATA::~_QUEUED_DATA()
	{
		switch (Type) {
		case WriteDataType::Information:
			if (InformationData != NULL)
				delete InformationData;
			break;

		case WriteDataType::Object:
			if (ObjectData.Object != NULL)
				delete ObjectData.Object;
			break;

		case WriteDataType::Progress:
			if (ProgressData != NULL)
				delete ProgressData;
			break;
		}
	}


	/*
	*	~ TESTPORT_OUTPUT ~
	*/

	_TESTPORT_OUTPUT::_TESTPORT_OUTPUT(::FILETIME timestamp, const WWuString& destination, const WWuString& destAddress, DWORD port, PortProbeStatus status)
		: Timestamp(timestamp), Destination(destination), DestAddress(destAddress), Port(port), Status(status) { }

	_TESTPORT_OUTPUT::~_TESTPORT_OUTPUT() { }


	/*
	*	~ Ephemeral socket ~
	*/

	EphemeralSocket::EphemeralSocket(ADDRINFOW* addressInfo, const bool block)
	{
		UnderlyingSocket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
		if (UnderlyingSocket == INVALID_SOCKET) {
			_WU_RAISE_NATIVE_EXCEPTION(WSAGetLastError(), L"socket", WriteErrorCategory::OpenError);
		}
		else {
			if (!block) {
				// Setting the IO mode to non-blocking.
				u_long mode = 1;
				ioctlsocket(UnderlyingSocket, FIONBIO, &mode);
			}
		}
	}

	EphemeralSocket::~EphemeralSocket()
	{
		if (UnderlyingSocket != INVALID_SOCKET) {
			shutdown(UnderlyingSocket, SD_SEND);
			closesocket(UnderlyingSocket);
		}
	}


	/*
	*	~ Tcping form ~
	*/

	TcpingForm::TcpingForm(
		const WWuString& destination,
		DWORD port,
		DWORD count,
		DWORD timeout,
		DWORD secondsInterval,
		DWORD failedThreshold,
		bool continuous,
		bool includeJitter,
		bool printFqdn,
		bool force,
		bool single,
		bool outputFile,
		const WWuString& filePath,
		bool append
	) : Destination(destination.Raw()), Port(port), Count(count), Timeout(timeout), SecondsInterval(secondsInterval),
		FailedCountThreshold(failedThreshold), IsContinuous(continuous), IncludeJitter(includeJitter), PrintFqdn(printFqdn),
		IsForce(force), Single(single), OutputToFile(outputFile), Append(append)
	{
		WSADATA wsaData;
		WORD reqVersion = MAKEWORD(2, 2);
		int result;

		if ((result = WSAStartup(reqVersion, &wsaData)) != 0)
			throw result;

		_ui64tow_s(Port, PortAsString, 6, 10);

		if (outputFile) {
			DWORD access;
			DWORD disposition;

			if (append) {
				access = FILE_APPEND_DATA;
				disposition = OPEN_ALWAYS;
			}
			else {
				access = GENERIC_WRITE;
				disposition = CREATE_ALWAYS;
			}

			File = CreateFile(
				filePath.Raw(),
				access,
				FILE_SHARE_READ,
				NULL,
				disposition,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);
			if (File == INVALID_HANDLE_VALUE)
				_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"CreateFile", WriteErrorCategory::OpenError);
		}
		else
			File = INVALID_HANDLE_VALUE;

		_instance = this;
		_ctrlCHit = false;
		_ctrlCHitCount = 0;
		SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);
	}

	TcpingForm::~TcpingForm()
	{
		WSACleanup();
		if (File != NULL && File != INVALID_HANDLE_VALUE)
			CloseHandle(File);

		SetConsoleCtrlHandler(CtrlHandlerRoutine, FALSE);
		_ctrlCHit = false;
	}

	TcpingForm* TcpingForm::_instance = { nullptr };
	TcpingForm* TcpingForm::GetForm() { return _instance; }

	inline BOOL WINAPI TcpingForm::CtrlHandlerRoutine(DWORD fdwCtrlType)
	{
		TcpingForm* instance = GetForm();
		switch (fdwCtrlType) {
			case CTRL_C_EVENT:
			{
				// Loved so much in tcping.exe that I had to bring it over =D.
				if (instance->_ctrlCHit && instance->_ctrlCHitCount < 2) {
					printf_s("Wow, you really mean it. I'm sorry... I'll stop now. :(\n");
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

	const bool TcpingForm::IsCtrlCHit()
	{
		TcpingForm* instance = GetForm();
		if (instance == nullptr)
			return false;

		return instance->_ctrlCHit;
	}


	/*
	*	~ TestPortForm
	*/

	TestPortForm::TestPortForm(const WWuString& destination, DWORD port, TransportProtocol protocol, DWORD timeoutSec)
	{
		WSADATA wsaData;
		WORD reqVersion = MAKEWORD(2, 2);
		int result;

		result = WSAStartup(reqVersion, &wsaData);
		if (result != ERROR_SUCCESS)
			_WU_RAISE_NATIVE_EXCEPTION(result, L"WSAStartup", WriteErrorCategory::DeviceError);

		_ui64tow_s(port, m_portAsString, 6, 10);

		m_destination = destination;
		m_port = port;
		m_protocol = protocol;
		m_timeoutSec = timeoutSec;
	}

	TestPortForm::~TestPortForm()
	{
		WSACleanup();
	}

	const WWuString& TestPortForm::Destination() const { return m_destination; }
	const DWORD TestPortForm::Port() const { return m_port; }
	const TransportProtocol TestPortForm::Protocol() const { return m_protocol; }
	const DWORD TestPortForm::Timeout() const { return m_timeoutSec; }

	LPCWSTR TestPortForm::PortAsString() const { return m_portAsString; }


	/*
	*	~ Start-Tcping
	*/

	void Network::StartTcpPing(TcpingForm& workForm, WuNativeContext* context)
	{
		// Creating notification queue and worker parameters.
		std::queue<QUEUED_DATA> queue;
		TCPING_WORKER_DATA threadArgs = {
			&workForm,
			&queue,
			false
		};

		// Creating worker thread.
		DWORD threadId;
		HANDLE hWorker = CreateThread(NULL, 0, StartTcpingWorker, &threadArgs, 0, &threadId);

		// da loop.
		do {
			// Checking the notification queue.
			if (queue.size() > 0) {

				// Printing from queue.
				QUEUED_DATA& data = queue.front();
				PrintQueueData(data, context);
				queue.pop();
			}

			// Checking the cancel flag.
			if (workForm.IsCtrlCHit()) {
				if (!threadArgs.IsComplete)
					TerminateThread(hWorker, ERROR_CANCELLED);

				break;
			}

			Sleep(1);

		} while (!threadArgs.IsComplete);

		DWORD workerExitCode;
		GetExitCodeThread(hWorker, &workerExitCode);
		if (workerExitCode != ERROR_SUCCESS && workerExitCode != ERROR_NO_MORE_ITEMS && workerExitCode != ERROR_CANCELLED)
			_WU_RAISE_NATIVE_EXCEPTION(workerExitCode, L"StartTcpingWorker", WriteErrorCategory::InvalidResult);

		// Printing remaining items on queue (if any).
		if (!workForm.IsCtrlCHit()) {
			while (queue.size() > 0) {
				QUEUED_DATA& data = queue.front();
				PrintQueueData(data, context);
				queue.pop();
			}
		}

		// Process and print statistics.
		if (!workForm.Single)
			ProcessStatistics(&workForm, context);
	}


	/*
	*	~ Get-NetworkFile
	*/

	_NETWORK_FILE_INFO::_NETWORK_FILE_INFO(DWORD id, DWORD perms, DWORD locks, const WWuString& path, const WWuString& userName)
		: Id(id), Permissions(perms), LockCount(locks), Path(path), UserName(userName)
	{ }

	_NETWORK_FILE_INFO::~_NETWORK_FILE_INFO() { }

	_NETWORK_SESSION_INFO::_NETWORK_SESSION_INFO(const WWuString& sessName, const WWuString& userName, DWORD ioCount)
		: ComputerSessionName(sessName), UserName(userName), OpenIoCount(ioCount)
	{ }

	_NETWORK_SESSION_INFO::~_NETWORK_SESSION_INFO() { }

	void Network::ListNetworkFiles(const WWuString& computerName, const WWuString& basePath, const WWuString& userName, WuList<NETWORK_FILE_INFO>& result)
	{
		LPBYTE buffer;
		DWORD entryCount;
		DWORD totalEntryCount;

		NET_API_STATUS status = NetFileEnum(
			(LPWSTR)computerName.Raw(),	// The computer name. NULL for the current computer.
			(LPWSTR)basePath.Raw(),		// A path prefix. If used, only paths that starts with this prefix are returned.
			(LPWSTR)userName.Raw(),		// Qualifier for user name or connection name. Results are limited by matches to this qualifier.
			3,							// Level of information data. 3 = FILE_INFO_3.
			&buffer,					// The buffer that receives the list.
			MAX_PREFERRED_LENGTH,		// Maximum preferred buffer length. MAX_PREFERRED_LENGTH = no limit.
			&entryCount,				// The number of entries returned in the buffer.
			&totalEntryCount,			// A hint to total number of entries if the operation is resumed.
			NULL						// Resume handle used in possible subsequent calls.
		);

		if (status != NERR_Success)
			_WU_RAISE_NATIVE_EXCEPTION(status, L"NetFileEnum", WriteErrorCategory::InvalidResult);

		size_t dataSize = sizeof(FILE_INFO_3);
		LPBYTE offset = buffer;
		for (DWORD i = 0; i < entryCount; i++) {
			auto currentInfo = reinterpret_cast<PFILE_INFO_3>(offset);
			result.Add(
				currentInfo->fi3_id,
				currentInfo->fi3_permissions,
				currentInfo->fi3_num_locks,
				currentInfo->fi3_pathname,
				currentInfo->fi3_username
			);

			offset += dataSize;
		}

		NetApiBufferFree(buffer);
	}

	void Network::ListNetworkFiles(const WWuString& computerName, const WWuString& basePath, const WWuString& userName, WuList<NETWORK_FILE_INFO>& fileInfo, WuList<NETWORK_SESSION_INFO>& sessionInfo)
	{
		LPBYTE buffer;
		DWORD entryCount;
		DWORD totalEntryCount;

		NET_API_STATUS status = NetFileEnum(
			(LPWSTR)computerName.Raw(),	// The computer name. NULL for the current computer.
			(LPWSTR)basePath.Raw(),		// A path prefix. If used, only paths that starts with this prefix are returned.
			(LPWSTR)userName.Raw(),		// Qualifier for user name or connection name. Results are limited by matches to this qualifier.
			3,							// Level of information data. 3 = FILE_INFO_3.
			&buffer,					// The buffer that receives the list.
			MAX_PREFERRED_LENGTH,		// Maximum preferred buffer length. MAX_PREFERRED_LENGTH = no limit.
			&entryCount,				// The number of entries returned in the buffer.
			&totalEntryCount,			// A hint to total number of entries if the operation is resumed.
			NULL						// Resume handle used in possible subsequent calls.
		);

		if (status != NERR_Success)
			_WU_RAISE_NATIVE_EXCEPTION(status, L"NetFileEnum", WriteErrorCategory::InvalidResult);

		size_t dataSize = sizeof(FILE_INFO_3);
		LPBYTE offset = buffer;
		for (DWORD i = 0; i < entryCount; i++) {
			auto currentInfo = reinterpret_cast<PFILE_INFO_3>(offset);
			fileInfo.Add(
				currentInfo->fi3_id,
				currentInfo->fi3_permissions,
				currentInfo->fi3_num_locks,
				currentInfo->fi3_pathname,
				currentInfo->fi3_username
			);

			offset += dataSize;
		}

		NetApiBufferFree(buffer);

		status = NetSessionEnum(
			(LPWSTR)computerName.Raw(),	// The computer name. NULL for the current computer.
			NULL,								// A filter for the computer session name where the session was initiated from.
			NULL,								// Qualifier for user name or connection name. Results are limited by matches to this qualifier.
			1,									// Level of information data. 1 = SESSION_INFO_1.
			&buffer,							// The buffer that receives the list.
			MAX_PREFERRED_LENGTH,				// Maximum preferred buffer length. MAX_PREFERRED_LENGTH = no limit.
			&entryCount,						// The number of entries returned in the buffer.
			&totalEntryCount,					// A hint to total number of entries if the operation is resumed.
			NULL								// Resume handle used in possible subsequent calls.
		);

		if (status != NERR_Success)
			_WU_RAISE_NATIVE_EXCEPTION(status, L"NetSessionEnum", WriteErrorCategory::InvalidResult);

		dataSize = sizeof(SESSION_INFO_1);
		offset = buffer;
		for (DWORD i = 0; i < entryCount; i++) {
			auto currentInfo = reinterpret_cast<PSESSION_INFO_1>(offset);
			sessionInfo.Add(
				currentInfo->sesi1_cname,
				currentInfo->sesi1_username,
				currentInfo->sesi1_num_opens
			);

			offset += dataSize;
		}

		NetApiBufferFree(buffer);
	}


	/*
	*	~ Close-NetworkFile
	*/

	void Network::CloseNetworkFile(const WWuString& computerName, DWORD fileId)
	{
		NET_API_STATUS status = NetFileClose((LPWSTR)computerName.Raw(), fileId);
		if (status != NERR_Success)
			_WU_RAISE_NATIVE_EXCEPTION(status, L"NetFileClose", WriteErrorCategory::CloseError);
	}


	/*
	*	~ Test-Port
	*/

	void Network::TestNetworkPort(const TestPortForm& workForm, WuNativeContext* context)
	{
		// Initial setup.
		int intResult;
		ADDRINFOW hints{ }, * addressInfo;

		if (workForm.Protocol() == TransportProtocol::Tcp) {
			hints.ai_socktype  = SOCK_STREAM;
			hints.ai_family    = AF_UNSPEC;
			hints.ai_protocol  = IPPROTO_TCP;
		}
		else {
			hints.ai_socktype  = SOCK_DGRAM;
			hints.ai_family    = AF_INET;
			hints.ai_protocol  = IPPROTO_UDP;
		}

		// Getting address info for destination.
		intResult = GetAddrInfoW(workForm.Destination().Raw(), workForm.PortAsString(), &hints, &addressInfo);
		if (intResult != 0)
			_WU_RAISE_NATIVE_EXCEPTION(intResult, L"GetAddrInfoW", WriteErrorCategory::InvalidResult);

		// Getting the display name from the resolved address, and optionally the FQDN.
		WWuString displayName;
		FormatIp(addressInfo, displayName);

		::FILETIME timestamp = { 0, 0 };
		TESTPORT_OUTPUT output(timestamp, (LPWSTR)workForm.Destination().Raw(), reinterpret_cast<const LPWSTR>(displayName.Raw()), workForm.Port(), PortProbeStatus::Timeout);

		EphemeralSocket ephSocket(addressInfo);
		long timeout = workForm.Timeout() * 1000;
		if (workForm.Protocol() == TransportProtocol::Tcp) {
			// Setting timeout for 'send'.
			setsockopt(ephSocket.UnderlyingSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

			// Attempting to connect. This here is a non blocking operation.
			intResult = connect(ephSocket.UnderlyingSocket, addressInfo->ai_addr, static_cast<int>(addressInfo->ai_addrlen));
			
			if (intResult == SOCKET_ERROR) {
				intResult = WSAGetLastError();
				if (intResult != WSAEWOULDBLOCK)
					_WU_RAISE_NATIVE_EXCEPTION(intResult, L"connect", WriteErrorCategory::ConnectionError);
			}

			// Checking if the connection was successful with timeout.
			fd_set set = { 0, 0 };
			FD_SET(ephSocket.UnderlyingSocket, &set);
			timeval timevalOut = { static_cast<long>(workForm.Timeout()), 0 };
			intResult = select(0, NULL, &set, NULL, &timevalOut);

			// 0: Timeout.
			if (intResult == 0) {
				GetSystemTimeAsFileTime(&timestamp);
				output.Timestamp = timestamp;
			}
			else {
				if (intResult == SOCKET_ERROR)
					_WU_RAISE_NATIVE_EXCEPTION(WSAGetLastError(), L"select", WriteErrorCategory::DeviceError);

				// Attempting to send.
				intResult = send(ephSocket.UnderlyingSocket, "tits", 4, 0);
				if (intResult == SOCKET_ERROR)
					_WU_RAISE_NATIVE_EXCEPTION(WSAGetLastError(), L"send", WriteErrorCategory::ProtocolError);
				else {
					GetSystemTimeAsFileTime(&timestamp);
					output.Timestamp = timestamp;
					output.Status = PortProbeStatus::Open;
				}
			}
		}
		else {
			// Setting the socket to blocking mode.
			u_long mode = 0;
			ioctlsocket(ephSocket.UnderlyingSocket, FIONBIO, &mode);

			// Setting the timeout for 'read'.
			setsockopt(ephSocket.UnderlyingSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

			// Attempting to connect.
			intResult = connect(ephSocket.UnderlyingSocket, addressInfo->ai_addr, static_cast<int>(addressInfo->ai_addrlen));
			if (intResult == SOCKET_ERROR)
				_WU_RAISE_NATIVE_EXCEPTION(WSAGetLastError(), L"connect", WriteErrorCategory::ConnectionError);

			// Attempting to send.
			intResult = send(ephSocket.UnderlyingSocket, "tits", 4, 0);
			if (intResult == SOCKET_ERROR)
				_WU_RAISE_NATIVE_EXCEPTION(WSAGetLastError(), L"send", WriteErrorCategory::ProtocolError);

			// Attempting to read.
			CHAR recBuffer[512] = { 0 };
			do {
				intResult = recv(ephSocket.UnderlyingSocket, recBuffer, 512, 0);

				// Received data, port is open.
				if (intResult > 0) {
					GetSystemTimeAsFileTime(&timestamp);
					output.Timestamp = timestamp;
					output.Status = PortProbeStatus::Open;
				}
				else {
					GetSystemTimeAsFileTime(&timestamp);
					output.Timestamp = timestamp;

					if (intResult != 0) {
						switch (WSAGetLastError()) {
							// Timed out, not forcibly closed. Port is open.
							case WSAETIMEDOUT:
								output.Status = PortProbeStatus::Open;
								break;

								// Forcibly closed.
							case WSAECONNRESET:
								output.Status = PortProbeStatus::Closed;
								break;

								// Default is already set to timeout.
						}
					}
				}

			} while (intResult > 0);
		}

		// Printing output.
		context->NativeWriteObject(&output, WriteOutputType::TestportOutput);
	}


	/*
	*	~ Get-NetworkStatistics
	*/

	void Network::GetTcpTables(bool includeModuleName, WuList<GETNETSTAT_MAIN_OUTPUT>& output, std::unordered_map<DWORD, WWuString>& processList, WuNativeContext* context)
	{
		ULONG bytesNeeded;
		ULONG result;
				
		// Getting the initial buffer size.
		if (result = GetTcpTable2(NULL, &bytesNeeded, TRUE) != ERROR_INSUFFICIENT_BUFFER)
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetTcpTable2", WriteErrorCategory::InvalidResult);

		// Getting the TCP table.
		std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(bytesNeeded);
		if (result = GetTcpTable2(reinterpret_cast<PMIB_TCPTABLE2>(buffer.get()), &bytesNeeded, TRUE) != NO_ERROR) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetTcpTable2", WriteErrorCategory::InvalidResult);
		}

		// Doing the same for IPv6.
		if (result = GetTcp6Table2(NULL, &bytesNeeded, TRUE) != ERROR_INSUFFICIENT_BUFFER) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetTcp6Table2", WriteErrorCategory::InvalidResult);
		}

		std::unique_ptr<BYTE[]> buffer6 = std::make_unique<BYTE[]>(bytesNeeded);
		if (result = GetTcp6Table2(reinterpret_cast<PMIB_TCP6TABLE2>(buffer6.get()), &bytesNeeded, TRUE) != NO_ERROR) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetTcp6Table2", WriteErrorCategory::InvalidResult);
		}

		in_addr localAddr { };
		in_addr remoteAddr { };
		auto tcpTable = reinterpret_cast<PMIB_TCPTABLE2>(buffer.get());
		for (DWORD i = 0; i < tcpTable->dwNumEntries; i++) {
			
			// Getting the IP address in string form.
			localAddr.S_un.S_addr = static_cast<u_long>(tcpTable->table[i].dwLocalAddr);
			remoteAddr.S_un.S_addr = static_cast<u_long>(tcpTable->table[i].dwRemoteAddr);

			WCHAR localAddrStr[16] { };
			WCHAR remoteAddrStr[16] { };

			if (tcpTable->table[i].dwLocalPort == 666)
				printf_s("Break");

			RtlIpv4AddressToString(&localAddr, localAddrStr);
			RtlIpv4AddressToString(&remoteAddr, remoteAddrStr);

			// Getting the module name.
			WWuString moduleName { };
			if (includeModuleName) {
				if (auto iterator = processList.find(tcpTable->table[i].dwOwningPid); iterator != processList.end()) {
					if (!WWuString::IsNullOrEmpty(iterator->second))
						moduleName = iterator->second.Split('\\').Back();
				}
			}
			
			output.Add(
				TransportProtocol::Tcp,
				localAddrStr,
				UshortByteSwap(static_cast<USHORT>(tcpTable->table[i].dwLocalPort)), // Need to convert the TCP/IP port to little-endian.
				remoteAddrStr,
				tcpTable->table[i].dwRemotePort,
				static_cast<PortState>(tcpTable->table[i].dwState),
				tcpTable->table[i].dwOwningPid,
				moduleName
			);
		}

		auto tcpTable6 = reinterpret_cast<PMIB_TCP6TABLE2>(buffer6.get());
		for (DWORD i = 0; i < tcpTable6->dwNumEntries; i++) {
			
			// Getting the IP address in string form.
			WCHAR localAddrStr[46] { };
			WCHAR remoteAddrStr[46] { };

			RtlIpv6AddressToString(&tcpTable6->table[i].LocalAddr, localAddrStr);
			RtlIpv6AddressToString(&tcpTable6->table[i].RemoteAddr, remoteAddrStr);

			// Getting the module name.
			WWuString moduleName { };
			if (includeModuleName) {
				if (auto iterator = processList.find(tcpTable6->table[i].dwOwningPid); iterator != processList.end()) {
					if (!WWuString::IsNullOrEmpty(iterator->second))
						moduleName = iterator->second.Split('\\').Back();
				}
			}

			output.Add(
				TransportProtocol::Tcp,
				localAddrStr,
				UshortByteSwap(static_cast<USHORT>(tcpTable6->table[i].dwLocalPort)), // Need to convert the TCP/IP port to little-endian.
				remoteAddrStr,
				tcpTable6->table[i].dwRemotePort,
				static_cast<PortState>(tcpTable->table[i].dwState),
				tcpTable6->table[i].dwOwningPid,
				moduleName
			);
		}
	}

	void Network::GetUdpTables(bool includeModuleName, WuList<GETNETSTAT_MAIN_OUTPUT>& output, std::unordered_map<DWORD, WWuString>& processList, WuNativeContext* context)
	{
		DWORD bytesNeeded;
		DWORD result;

		// Getting the buffer size.
		if (result = GetExtendedUdpTable(NULL, &bytesNeeded, TRUE, AF_INET, UDP_TABLE_CLASS::UDP_TABLE_OWNER_PID, 0) != ERROR_INSUFFICIENT_BUFFER) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetExtendedUdpTable", WriteErrorCategory::InvalidResult);
		}

		// Getting the TCP table.
		std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(bytesNeeded);
		if (result = GetExtendedUdpTable(reinterpret_cast<PVOID>(buffer.get()), &bytesNeeded, TRUE, AF_INET, UDP_TABLE_CLASS::UDP_TABLE_OWNER_PID, 0) != NO_ERROR) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetExtendedUdpTable", WriteErrorCategory::InvalidResult);
		}

		// Doing the same for IPv6.
		if (result = GetExtendedUdpTable(NULL, &bytesNeeded, TRUE, AF_INET6, UDP_TABLE_CLASS::UDP_TABLE_OWNER_PID, 0) != ERROR_INSUFFICIENT_BUFFER) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetExtendedUdpTable", WriteErrorCategory::InvalidResult);
		}

		std::unique_ptr<BYTE[]> buffer6 = std::make_unique<BYTE[]>(bytesNeeded);
		if (result = GetExtendedUdpTable(reinterpret_cast<PVOID>(buffer6.get()), &bytesNeeded, TRUE, AF_INET6, UDP_TABLE_CLASS::UDP_TABLE_OWNER_PID, 0) != NO_ERROR) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetExtendedUdpTable", WriteErrorCategory::InvalidResult);
		}

		in_addr localAddr { };
		auto udpTable = reinterpret_cast<PMIB_UDPTABLE_OWNER_PID>(buffer.get());
		for (DWORD i = 0; i < udpTable->dwNumEntries; i++) {
			
			// Getting the IP address in string form.
			WCHAR localAddrStr[16] { };
			localAddr.S_un.S_addr = static_cast<u_long>(udpTable->table[i].dwLocalAddr);
			RtlIpv4AddressToString(&localAddr, localAddrStr);

			// Getting the module name.
			WWuString moduleName { };
			if (includeModuleName) {
				if (auto iterator = processList.find(udpTable->table[i].dwOwningPid); iterator != processList.end()) {
					if (!WWuString::IsNullOrEmpty(iterator->second))
						moduleName = iterator->second.Split('\\').Back();
				}
			}

			output.Add(
				TransportProtocol::Udp,
				localAddrStr,
				UshortByteSwap(static_cast<USHORT>(udpTable->table[i].dwLocalPort)), // Need to convert the TCP/IP port to little-endian.
				L"",
				0,
				PortState::None,
				udpTable->table[i].dwOwningPid,
				moduleName
			);
		}

		in6_addr local6Addr { };
		auto udp6Table = reinterpret_cast<PMIB_UDP6TABLE_OWNER_PID>(buffer6.get());
		for (DWORD i = 0; i < udp6Table->dwNumEntries; i++) {

			// Getting the IP address in string form.
			WCHAR localAddrStr[46] { };
			RtlCopyMemory(local6Addr.u.Byte, udp6Table->table[i].ucLocalAddr, 16);
			RtlIpv6AddressToString(&local6Addr, localAddrStr);

			// Getting the module name.
			WWuString moduleName { };
			if (includeModuleName) {
				if (auto iterator = processList.find(udp6Table->table[i].dwOwningPid); iterator != processList.end()) {
					if (!WWuString::IsNullOrEmpty(iterator->second))
						moduleName = iterator->second.Split('\\').Back();
				}
			}

			output.Add(
				TransportProtocol::Udp,
				localAddrStr,
				UshortByteSwap(static_cast<USHORT>(udp6Table->table[i].dwLocalPort)), // Need to convert the TCP/IP port to little-endian.
				L"",
				0,
				PortState::None,
				udp6Table->table[i].dwOwningPid,
				moduleName
			);
		}
	}

	void Network::GetInterfaceStatistics(WuList<MIB_IF_ROW2>& output, WuNativeContext* context)
	{
		MibTablePointer<MIB_IF_TABLE2> interfaceTable;
		if (DWORD result = GetIfTable2(&interfaceTable) != NO_ERROR) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetIfTable2", WriteErrorCategory::InvalidResult);
		}

		for (ULONG i = 0; i < interfaceTable->NumEntries; i++)
			output.Add(interfaceTable->Table[i]);
	}

	void Network::GetIpRouteTable(WuList<WU_IP_ROUTE>& output, WuNativeContext* context)
	{
		MibTablePointer<MIB_IPFORWARD_TABLE2> forwardTable;
		if (DWORD result = GetIpForwardTable2(AF_UNSPEC, &forwardTable) != NO_ERROR) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetIpForwardTable2", WriteErrorCategory::InvalidResult);
		}

		for (ULONG i = 0; i < forwardTable->NumEntries; i++) {
			ADDRESS_FAMILY family;
			WWuString destinationAddress;
			WWuString gatewayAddress;
			WWuString maskAddress;
			switch (forwardTable->Table[i].DestinationPrefix.Prefix.si_family) {
				case AF_INET: case AF_UNSPEC:
				{
					WCHAR destinationStr[16] { };
					RtlIpv4AddressToString(&forwardTable->Table[i].DestinationPrefix.Prefix.Ipv4.sin_addr, destinationStr);
					destinationAddress = destinationStr;

					// Calculating the mask.
					ULONG mask { };
					in_addr maskInAddr { };
					if (ConvertLengthToIpv4Mask(forwardTable->Table[i].DestinationPrefix.PrefixLength, &mask) == NO_ERROR) {
						WCHAR maskStr[16] { };
						maskInAddr.S_un.S_addr = static_cast<u_long>(mask);
						RtlIpv4AddressToString(&maskInAddr, maskStr);
						maskAddress = maskStr;
					}

					family = AF_INET;

				} break;

				case AF_INET6:
				{
					WCHAR destinationStr[46] { };
					RtlIpv6AddressToString(&forwardTable->Table[i].DestinationPrefix.Prefix.Ipv6.sin6_addr, destinationStr);
					destinationAddress = destinationStr;

					family = AF_INET6;
				} break;
			}
			switch (forwardTable->Table[i].NextHop.si_family) {
				case AF_INET: case AF_UNSPEC:
				{
					WCHAR gatewayStr[16] { };
					RtlIpv4AddressToString(&forwardTable->Table[i].NextHop.Ipv4.sin_addr, gatewayStr);
					gatewayAddress = gatewayStr;
				} break;

				case AF_INET6:
				{
					WCHAR gatewayStr[46] { };
					RtlIpv6AddressToString(&forwardTable->Table[i].NextHop.Ipv6.sin6_addr, gatewayStr);
					gatewayAddress = gatewayStr;
				} break;
			}

			// Getting the metric.
			MIB_IPINTERFACE_ROW ifInfo { };
			ifInfo.InterfaceIndex = forwardTable->Table[i].InterfaceIndex;
			ifInfo.Family = family;
			if (DWORD result = GetIpInterfaceEntry(&ifInfo)) {
				_WU_RAISE_NATIVE_EXCEPTION(result, L"GetIpInterfaceEntry", WriteErrorCategory::InvalidResult);
			}

			DWORD metric = forwardTable->Table[i].Metric + ifInfo.Metric;

			output.Add(
				forwardTable->Table[i].InterfaceIndex,
				destinationAddress,
				maskAddress,
				gatewayAddress,
				metric,
				false
			);
		}

		// Checking for persistent routes.
		WuList<WWuString> persistentRoutes(10);
		RegistryHandle regHandle{ HKEY_LOCAL_MACHINE, false };
		Registry::GetRegistryKeyValueNames(regHandle, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\PersistentRoutes", persistentRoutes);
		
		for (const auto& routeInfo : persistentRoutes) {
			auto infoSplit = routeInfo.Split(',');
			if (infoSplit.Count() >= 3)
				output.Add(
					0,
					infoSplit[0],
					infoSplit[1],
					infoSplit[2],
					static_cast<DWORD>(_wtoi(infoSplit[3].Raw())),
					true
				);
		}
	}

	void Network::GetIpv4Statistics(std::unordered_map<GetNetStatProtocol, MIB_IPSTATS>& output, WuNativeContext* context)
	{
		MIB_IPSTATS buffer { };
		if (ULONG result = GetIpStatisticsEx(&buffer, AF_INET) != NO_ERROR) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetIpStatisticsEx", WriteErrorCategory::InvalidResult);
		}

		output.emplace(GetNetStatProtocol::Ipv4, buffer);
	}

	void Network::GetIpv6Statistics(std::unordered_map<GetNetStatProtocol, MIB_IPSTATS>& output, WuNativeContext* context)
	{
		MIB_IPSTATS buffer { };
		if (ULONG result = GetIpStatisticsEx(&buffer, AF_INET6) != NO_ERROR) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetIpStatisticsEx", WriteErrorCategory::InvalidResult);
		}

		output.emplace(GetNetStatProtocol::Ipv6, buffer);
	}

	void Network::GetIcmpv4Statistics(std::unordered_map<GetNetStatProtocol, MIB_ICMP_EX>& output, WuNativeContext* context)
	{
		MIB_ICMP_EX buffer { };
		if (ULONG result = GetIcmpStatisticsEx(&buffer, AF_INET) != NO_ERROR)
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetIcmpStatisticsEx", WriteErrorCategory::InvalidResult);

		output.emplace(GetNetStatProtocol::Icmp, buffer);
	}

	void Network::GetIcmpv6Statistics(std::unordered_map<GetNetStatProtocol, MIB_ICMP_EX>& output, WuNativeContext* context)
	{
		MIB_ICMP_EX buffer { };
		if (ULONG result = GetIcmpStatisticsEx(&buffer, AF_INET6) != NO_ERROR)
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetIcmpStatisticsEx", WriteErrorCategory::InvalidResult);

		output.emplace(GetNetStatProtocol::Icmpv6, buffer);
	}

	void Network::GetTcpv4Statistics(std::unordered_map<GetNetStatProtocol, MIB_TCPSTATS>& output, WuNativeContext* context)
	{
		MIB_TCPSTATS buffer { };
		if (ULONG result = GetTcpStatisticsEx(&buffer, AF_INET) != NO_ERROR)
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetTcpStatisticsEx", WriteErrorCategory::InvalidResult);

		output.emplace(GetNetStatProtocol::Tcp, buffer);
	}

	void Network::GetTcpv6Statistics(std::unordered_map<GetNetStatProtocol, MIB_TCPSTATS>& output, WuNativeContext* context)
	{
		MIB_TCPSTATS buffer { };
		if (ULONG result = GetTcpStatisticsEx(&buffer, AF_INET6) != NO_ERROR)
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetTcpStatisticsEx", WriteErrorCategory::InvalidResult);

		output.emplace(GetNetStatProtocol::Tcpv6, buffer);
	}

	void Network::GetUdpv4Statistics(std::unordered_map<GetNetStatProtocol, MIB_UDPSTATS>& output, WuNativeContext* context)
	{
		MIB_UDPSTATS buffer { };
		if (ULONG result = GetUdpStatisticsEx(&buffer, AF_INET) != NO_ERROR)
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetUdpStatisticsEx", WriteErrorCategory::InvalidResult);

		output.emplace(GetNetStatProtocol::Udp, buffer);
	}

	void Network::GetUdpv6Statistics(std::unordered_map<GetNetStatProtocol, MIB_UDPSTATS>& output, WuNativeContext* context)
	{
		MIB_UDPSTATS buffer { };
		if (ULONG result = GetUdpStatisticsEx(&buffer, AF_INET6) != NO_ERROR)
			_WU_RAISE_NATIVE_EXCEPTION(result, L"GetUdpStatisticsEx", WriteErrorCategory::InvalidResult);

		output.emplace(GetNetStatProtocol::Udpv6, buffer);
	}


	/*
	*	~ Utility functions
	*/

	void Network::PrintQueueData(const QUEUED_DATA& data, WuNativeContext* context)
	{
		switch (data.Type) {
			case WriteDataType::Information:
				context->NativeWriteInformation(data.InformationData);
				break;

			case WriteDataType::Object:
			{
				switch (data.ObjectData.ObjectType) {

					case WriteOutputType::TcpingOutput:
					{
						auto objData = reinterpret_cast<PTCPING_OUTPUT>(data.ObjectData.Object);
						context->NativeWriteObject(objData, data.ObjectData.ObjectType);
					} break;

					case WriteOutputType::TcpingStatistics:
					{
						auto objData = reinterpret_cast<PTCPING_STATISTICS>(data.ObjectData.Object);
						context->NativeWriteObject(objData, data.ObjectData.ObjectType);
					} break;
				}
			} break;

			case WriteDataType::Progress:
				context->NativeWriteProgress(data.ProgressData);
				break;

			case WriteDataType::Warning:
				context->NativeWriteWarning(data.WarningData);
				break;
		}
	}

	DWORD WINAPI Network::StartTcpingWorker(LPVOID params)
	{
		// Defining environment.
		auto threadArgs = reinterpret_cast<PTCPING_WORKER_DATA>(params);
		TcpingForm* workForm = threadArgs->WorkForm;
		std::queue<QUEUED_DATA>* infoQueue = threadArgs->Queue;

		int intResult;
		ADDRINFOW hints = { 0 }, * addressInfo, * singleInfo;

		double milliseconds = 0.0;
		double totalMilliseconds = 0.0;
		double jitter = 0.0;
		DWORD successCount = 0;
		int failedCount = 0;

		WWuString header;
		WWuString destText;

		if (workForm->Single)
			workForm->Count = 1;

		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_UNSPEC;

		// Attempting to get destination address information.
		WuString narrowDest = workForm->Destination.ToNarrow();
		intResult = GetAddrInfoW(workForm->Destination.Raw(), workForm->PortAsString, &hints, &addressInfo);
		if (intResult != 0) {
			threadArgs->IsComplete = true;
			return intResult;
		}

		singleInfo = addressInfo;

		FormatIp(addressInfo, destText);
		if (workForm->PrintFqdn) {
			if (singleInfo->ai_family == AF_INET6) {
				WWuString ipStr = WWuString(destText);
				ResolveIpv6ToDomainName(ipStr, workForm->DisplayName);
				if (workForm->DisplayName.Length() == 0)
					workForm->DisplayName = destText;
			}
			else {
				ResolveIpToDomainName(destText, workForm->DisplayName);
				if (workForm->DisplayName.Length() == 0)
					workForm->DisplayName = destText;
			}
		}
		else
			workForm->DisplayName = destText;

		// Header
		if (workForm->OutputToFile)
			PrintHeader(workForm, workForm->DisplayName);

		// Main loop. Here the 'ping' will happen for 'count' times.
		if (workForm->IsContinuous) {
			while (!TcpingForm::IsCtrlCHit()) {
				DWORD testResult = ERROR_SUCCESS;
				try {
					PerformSingleTestProbe(singleInfo, workForm, workForm->DisplayName, &workForm->Statistics, infoQueue, testResult);
				}
				catch (const WuNativeException& ex) {
					threadArgs->IsComplete = true;
					return ex.ErrorCode();
				}

				if (testResult != ERROR_SUCCESS && testResult != WSAETIMEDOUT) {
					if (testResult == ERROR_CANCELLED)
						goto END;

					threadArgs->IsComplete = true;
					return testResult;
				}
				// We don't wanna sleep on the last one.
				else if (testResult == ERROR_SUCCESS && (workForm->Statistics.Sent < workForm->Count || workForm->IsContinuous))
					Sleep(workForm->SecondsInterval * 1000);
			}
		}
		else {
			for (DWORD i = 0; i < workForm->Count; i++) {
				if (static_cast<int>(workForm->Statistics.Failed) >= workForm->FailedCountThreshold)
					goto END;

				DWORD testResult = ERROR_SUCCESS;
				try {
					PerformSingleTestProbe(singleInfo, workForm, workForm->DisplayName, &workForm->Statistics, infoQueue, testResult);
				}
				catch (const WuNativeException& ex) {
					threadArgs->IsComplete = true;
					return ex.ErrorCode();
				}

				if (testResult != ERROR_SUCCESS && testResult != WSAETIMEDOUT) {
					if (testResult == ERROR_CANCELLED)
						goto END;

					threadArgs->IsComplete = true;
					return testResult;
				}
				else if (testResult == ERROR_SUCCESS && (workForm->Statistics.Sent < workForm->Count || workForm->IsContinuous))
					Sleep(workForm->SecondsInterval * 1000);
			}
		}

	END:

		threadArgs->IsComplete = true;
		return ERROR_SUCCESS;
	};


	//////////////////////////////////////////////////////////////////////
	//
	// ~ Important note:
	//
	// Since this is running in native and managed, code performance
	// is very important. At first everything was nice and structured
	// with return types, and separate functions, but that was 
	// affecting the results.
	//
	//////////////////////////////////////////////////////////////////////

	void Network::PerformSingleTestProbe(ADDRINFOW* singleInfo, TcpingForm* workForm, const WWuString& displayName,
		PTCPING_STATISTICS statistics, std::queue<QUEUED_DATA>* infoQueue, DWORD& result)
	{
		DWORD finalResult = ERROR_SUCCESS;
		DWORD sendResult = ERROR_SUCCESS;
		WWuString outputText;
		WuStopWatch timeoutSpw;
		FILETIME timestamp;

		workForm->StopWatch.Restart();
		bool timedOut = false;

		// Creating an ephemeral socket.
		try {
			EphemeralSocket ephSocket { singleInfo };

			// Connecting.
			int connResult = connect(ephSocket.UnderlyingSocket, singleInfo->ai_addr, (int)singleInfo->ai_addrlen);
			if (connResult == SOCKET_ERROR) {
				connResult = WSAGetLastError();
				if (connResult != ERROR_SUCCESS && connResult != WSAEWOULDBLOCK)
					_WU_RAISE_NATIVE_EXCEPTION(WSAGetLastError(), L"connect", WriteErrorCategory::ConnectionError);
			}

			// Trying to send until we succeed or timeout.
			timeoutSpw.Start();
			while (!TcpingForm::IsCtrlCHit()) {
				try {
					if (workForm->IsForce) {
						connResult = send(ephSocket.UnderlyingSocket, "tits", 4, 0);
						if (connResult == SOCKET_ERROR)
							_WU_RAISE_NATIVE_EXCEPTION(WSAGetLastError(), L"send", WriteErrorCategory::ProtocolError);
					}
					else {
						connResult = send(ephSocket.UnderlyingSocket, NULL, 0, 0);
						if (connResult == SOCKET_ERROR)
							_WU_RAISE_NATIVE_EXCEPTION(WSAGetLastError(), L"send", WriteErrorCategory::ProtocolError);
					}
					break;
				}
				catch (const WuNativeException&) { }

				if ((timeoutSpw.ElapsedMilliseconds() / 1000) >= workForm->Timeout) {
					timedOut = true;
					break;
				}

				Sleep(1);
			}
			workForm->StopWatch.Stop();
		}
		catch (const WuNativeException& ex) {
			sendResult = ex.ErrorCode();
			return;
		}

		if (TcpingForm::IsCtrlCHit()) {
			result = ERROR_CANCELLED;
			return;
		}

		statistics->Sent++;

		if (timedOut) {
			outputText += WWuString::Format(L"%ws%ws - TCP:%d - No response - time=%dms", outputText.Raw(), displayName.Raw(), workForm->Port, (workForm->Timeout * 1000));

			if (workForm->OutputToFile) {
				// Instead of printing the same output as the one in the file
				// We show a nice progress bar with condensed information.
				int percentage = 0;
				WWuString action = WWuString::Format(L"Probing %ws", displayName.Raw());
				WWuString status;

				MAPPED_PROGRESS_DATA progressRecord;
				if (workForm->IsContinuous) {
					status = WWuString::Format(L"TCP:%d - No response - time %dms - press Ctrl + C to stop.", workForm->Port, (workForm->Timeout * 1000));
					progressRecord = MAPPED_PROGRESS_DATA(
						action.Raw(),
						0,
						(LPWSTR)NULL,
						-1,
						0,
						ProgressRecordType::Processing,
						-1,
						status.Raw()
					);

					QUEUED_DATA queueData(WriteDataType::Progress, &progressRecord, NULL);
					infoQueue->push(queueData);
				}
				else {
					status = WWuString::Format(L"TCP:%d - No response - time %dms", workForm->Port, (workForm->Timeout * 1000));
					percentage = std::lround((static_cast<float>(statistics->Sent) / workForm->Count) * 100);
					progressRecord = MAPPED_PROGRESS_DATA(
						action.Raw(),
						0,
						(LPWSTR)NULL,
						-1,
						percentage,
						ProgressRecordType::Processing,
						-1,
						status.Raw()
					);

					QUEUED_DATA queueData(WriteDataType::Progress, &progressRecord, NULL);
					infoQueue->push(queueData);
				}

				IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", outputText.Raw()));
			}
			else {
#if defined(_TCPING_TEST)
				wprintf(L"%ws\n", outputText.Raw());
#else
				GetSystemTimeAsFileTime(&timestamp);
				TCPING_OUTPUT tcpingOut(
					timestamp,
					(LPWSTR)workForm->Destination.Raw(),
					(LPWSTR)displayName.Raw(),
					workForm->Port,
					PortProbeStatus::Timeout,
					static_cast<double>(workForm->Timeout * 1000),
					-1.00
				);

				auto objType = WriteOutputType::TcpingOutput;
				QUEUED_DATA queueData(WriteDataType::Object, &tcpingOut, &objType);
				infoQueue->push(queueData);

				/*LPWSTR tags[1] = { L"PSHOST" };
				Notification::MAPPED_INFORMATION_DATA report(
					(LPWSTR)NULL, GetCurrentThreadId(), outputText.Raw(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
				);

				context->NativeWriteInformation(&report);*/
#endif
			}

			statistics->Failed++;

			result = WSAETIMEDOUT;
			return;
		}
		if (sendResult == ERROR_CANCELLED) {
			result = ERROR_CANCELLED;
			return;
		}

		double currentMilliseconds = workForm->StopWatch.ElapsedMilliseconds();
		outputText = WWuString::Format(L"%ws%ws - TCP:%d - Port is open - time=%.2fms", outputText.Raw(), displayName.Raw(), workForm->Port, currentMilliseconds);

		if (currentMilliseconds > statistics->MaxRtt)
			statistics->MaxRtt = currentMilliseconds;
		else if (currentMilliseconds < statistics->MinRtt || statistics->MinRtt == 0)
			statistics->MinRtt = currentMilliseconds;

		double currentJitter = -1.00;
		if (workForm->IncludeJitter && statistics->Successful >= 1) {
			currentJitter = currentMilliseconds - (statistics->TotalMilliseconds / statistics->Successful);
			currentJitter = abs(currentJitter);
			if (currentJitter > statistics->MaxJitter)
				statistics->MaxJitter = currentJitter;
			else if (currentJitter < statistics->MinJitter || statistics->MinJitter == 0)
				statistics->MinJitter = currentJitter;

			statistics->TotalJitter += currentJitter;

			outputText = WWuString::Format(L"%ws jitter=%.2fms", outputText.Raw(), currentJitter);
		}

		statistics->Successful++;
		statistics->TotalMilliseconds += currentMilliseconds;

		if (workForm->OutputToFile) {
			// Instead of printing the same output as the one in the file
			// We show a nice progress bar with condensed information.
			int percentage = 0;
			WWuString action = WWuString::Format(L"Probing %ws", displayName.Raw());
			WWuString status = WWuString::Format(L"TCP:%d - Port is open - time %.2fms", workForm->Port, currentMilliseconds);

			if (workForm->IncludeJitter && statistics->Successful > 1)
				status += WWuString::Format(L" jitter=%.2fms", currentJitter);

			MAPPED_PROGRESS_DATA progressRecord;
			if (workForm->IsContinuous) {
				status += L" - press Ctrl + C to stop.";
				progressRecord = MAPPED_PROGRESS_DATA(
					action.Raw(),
					0,
					(LPWSTR)NULL,
					-1,
					0,
					ProgressRecordType::Processing,
					-1,
					status.Raw()
				);

				QUEUED_DATA queueData(WriteDataType::Progress, &progressRecord, NULL);
				infoQueue->push(queueData);
			}
			else {
				percentage = std::lround((static_cast<float>(statistics->Sent) / workForm->Count) * 100);
				progressRecord = MAPPED_PROGRESS_DATA(
					action.Raw(),
					0,
					(LPWSTR)NULL,
					-1,
					percentage,
					ProgressRecordType::Processing,
					-1,
					status.Raw()
				);

				QUEUED_DATA queueData(WriteDataType::Progress, &progressRecord, NULL);
				infoQueue->push(queueData);
			}

			IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", outputText.Raw()));
		}
		else {
#if defined(_TCPING_TEST)
			wprintf(L"%ws\n", outputText.Raw());
#else
			GetSystemTimeAsFileTime(&timestamp);
			TCPING_OUTPUT tcpingOut(
				timestamp,
				(LPWSTR)workForm->Destination.Raw(),
				(LPWSTR)displayName.Raw(),
				workForm->Port,
				PortProbeStatus::Open,
				currentMilliseconds,
				currentJitter
			);

			auto objType = WriteOutputType::TcpingOutput;
			QUEUED_DATA queueData(WriteDataType::Object, &tcpingOut, &objType);
			infoQueue->push(queueData);

			/*LPWSTR tags[1] = { L"PSHOST" };
			Notification::MAPPED_INFORMATION_DATA report(
				(LPWSTR)NULL, GetCurrentThreadId(), outputText.Raw(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
			);

			context->NativeWriteInformation(&report);*/
#endif
		}

		result = finalResult;
	}

	void Network::ProcessStatistics(TcpingForm* workForm, WuNativeContext* context)
	{
		workForm->Statistics.FailedPercent = (static_cast<double>(workForm->Statistics.Failed) / workForm->Statistics.Sent) * 100.00;

		if (workForm->Statistics.TotalMilliseconds == 0)
			workForm->Statistics.AvgRtt = 0.00;
		else
			workForm->Statistics.AvgRtt = workForm->Statistics.TotalMilliseconds / workForm->Statistics.Successful;

		// Yes, this can be done better, but my ADHD doesn't wanna think right now.
		WWuString output = WWuString::Format(
			L"\nPinging statistics for %ws:\n\tPackets: Sent = %d, Successful = %d, Failed = %d (%.2f%%),\n",
			workForm->DisplayName.Raw(),
			workForm->Statistics.Sent,
			workForm->Statistics.Successful,
			workForm->Statistics.Failed,
			workForm->Statistics.FailedPercent
		);
		output += WWuString::Format(
			L"Approximate round trip times in milli-seconds:\n\tMinimum = %.2fms, Maximum = %.2fms, Average = %.2fms",
			workForm->Statistics.MinRtt,
			workForm->Statistics.MaxRtt,
			workForm->Statistics.AvgRtt
		);
		if (workForm->IncludeJitter) {
			if (workForm->Statistics.TotalJitter == 0)
				workForm->Statistics.AvgJitter = 0.00;
			else
				workForm->Statistics.AvgJitter = workForm->Statistics.TotalJitter / (workForm->Statistics.Successful - 1);

			output += WWuString::Format(
				L",\nApproximate jitter in milli-seconds:\n\tMinimum = %.2fms, Maximum = %.2fms, Average = %.2fms",
				workForm->Statistics.MinJitter,
				workForm->Statistics.MaxJitter,
				workForm->Statistics.AvgJitter
			);
		}

		if (workForm->OutputToFile)
			IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", output.Raw()));
		else {
#if defined(_TCPING_TEST)
			wprintf(L"%ws\n", output.Raw());
#else
			context->NativeWriteObject(&workForm->Statistics, WriteOutputType::TcpingStatistics);

			/*LPWSTR tags[1] = { L"PSHOST" };
			Notification::MAPPED_INFORMATION_DATA report(
				(LPWSTR)NULL, GetCurrentThreadId(), output.Raw(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
			);

			context->NativeWriteInformation(&report);*/
		}
#endif
	}

	void Network::PrintHeader(TcpingForm* workForm, const WWuString& displayText)
	{
		WWuString header;

		header = L"~ Start-Tcping\n";
		header += WWuString::Format(L"\nDestination: %ws : %s\n", workForm->Destination.Raw(), workForm->PortAsString);

		if (workForm->IsContinuous) {
			header += WWuString::Format(
				L"Count: continuous\nFail threshold: continuous\nInterval: %ds\nTimeout: %ds\nForce: %s\nPrint FQDN: %s\n",
				workForm->SecondsInterval,
				workForm->Timeout,
				workForm->IsForce ? "true" : "false",
				workForm->PrintFqdn ? "true" : "false"
			);
		}
		else {
			header += WWuString::Format(
				L"Count: %d\nFail threshold: %d\nInterval: %ds\nTimeout: %ds\nForce: %s\nPrint FQDN: %s\n",
				workForm->Count,
				workForm->FailedCountThreshold,
				workForm->SecondsInterval,
				workForm->Timeout,
				workForm->IsForce ? "true" : "false",
				workForm->PrintFqdn ? "true" : "false"
			);
		}

		IO::AppendTextToFile(workForm->File, WWuString::Format(L"\n%ws\n", header.Raw()));
	}

	void Network::FormatIp(ADDRINFOW* address, WWuString& ipString)
	{
		wchar_t buffer[46];
		DWORD ret;

		switch (address->ai_family) {
			case AF_INET:
			{
				struct sockaddr_in* sockaddr_ipv4;
				sockaddr_ipv4 = (struct sockaddr_in*)address->ai_addr;
				InetNtopW(sockaddr_ipv4->sin_family, &sockaddr_ipv4->sin_addr, buffer, 46);
			} break;
			case AF_INET6:
				ret = GetNameInfoW(address->ai_addr, (int)address->ai_addrlen, buffer, sizeof(buffer), NULL, 0, NI_NUMERICHOST);
				break;
		}

		ipString = WWuString(buffer);
	}

	// Returns a reversed ip for DNS reverse lookup.
	// https://learn.microsoft.com/en-us/troubleshoot/windows/win32/use-dnsquery-resolve-host-names
	void Network::ReverseIp(WWuString& ip)
	{
		wchar_t seps[] = L".";
		wchar_t* token;
		wchar_t* context = NULL;
		int i = 0;
		wchar_t buffer[4][4] = { 0 };
		token = wcstok_s((wchar_t*)ip.Raw(), seps, &context);
		while (token != NULL) {
			/* While there are "." characters in "string"*/
			swprintf_s(buffer[i], L"%ws", token);
			/* Get next "." character: */
			token = wcstok_s(NULL, seps, &context);
			i++;
		}

		ip = WWuString::Format(L"%ws.%ws.%ws.%ws.%ws", buffer[3], buffer[2], buffer[1], buffer[0], L"IN-ADDR.ARPA");

		// Faaaar to slow \/

		/*wuvector<WWuString> ipSplit = ip.Split('.');
		size_t splitSize = ipSplit.size();
		WWuString output;
		for (size_t i = 0; i < splitSize; i++) {
			size_t revIndex = splitSize - i - 1;
			output = WWuString::Format(L"%ws%ws.", output.Raw(), ipSplit[revIndex].Raw());
		}

		ip = WWuString::Format(L"%ws%ws", output.Raw(), L"IN-ADDR.ARPA");*/
	}

	void Network::ReverseIPv6(WWuString& ip)
	{
		in6_addr addr;
		InetPtonW(AF_INET6, ip.Raw(), &addr);
		wchar_t buffer[MAX_PATH] = { 0 };

		WORD buffIndex = 0;
		for (WORD i = 0; i < 16; i++) {
			WORD reverseIndex = 15 - i;

			wchar_t tempBuffer[3];
			wsprintf(tempBuffer, L"%02X", addr.s6_addr[reverseIndex]);

			buffer[buffIndex++] = tempBuffer[1];
			buffer[buffIndex++] = '.';
			buffer[buffIndex++] = tempBuffer[0];
			buffer[buffIndex++] = '.';
		}

		wsprintf(buffer, L"%ws%ws", buffer, L"IP6.ARPA");

		ip = buffer;
	}

	// Resolves an IP address to a host domain name.
	void Network::ResolveIpToDomainName(const WWuString& ip, WWuString& domainName)
	{
		PDNS_RECORD pDnsRecord;
		WORD wType = DNS_TYPE_PTR;
		PIP4_ARRAY pSrvList = NULL;
		DNS_FREE_TYPE freetype = DnsFreeRecordListDeep;

		WWuString reverse(ip.Raw());
		ReverseIp(reverse);

		DNS_STATUS status = DnsQuery(reverse.Raw(),	//Pointer to OwnerName. 
			wType,											//Type of the record to be queried.
			DNS_QUERY_BYPASS_CACHE,							// Bypasses the resolver cache on the lookup. 
			pSrvList,										//Contains DNS server IP address.
			&pDnsRecord,									//Resource record that contains the response.
			NULL											// Reserved for future use.
		);
		if (status != ERROR_SUCCESS)
			return;

		if (pDnsRecord != NULL)
			domainName = pDnsRecord->Data.PTR.pNameHost;
	}

	void Network::ResolveIpv6ToDomainName(WWuString& ip, WWuString& reverse)
	{
		PDNS_ADDR_ARRAY srvList = NULL;
		DNS_QUERY_RESULT queryResult = { DNS_QUERY_REQUEST_VERSION1 };

		ReverseIPv6(ip);

		DNS_QUERY_REQUEST request = {
			DNS_QUERY_REQUEST_VERSION1,
			ip.Raw(),
			DNS_TYPE_PTR,
			NULL,
			srvList,
			0,
			NULL,
			NULL
		};

		DNS_STATUS status = DnsQueryEx(&request, &queryResult, NULL);
		if (status != ERROR_SUCCESS)
			return;

		if (queryResult.pQueryRecords != NULL)
			reverse = queryResult.pQueryRecords->Data.PTR.pNameHost;
	}
}