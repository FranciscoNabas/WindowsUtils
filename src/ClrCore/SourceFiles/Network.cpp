#include "..\pch.h"

#include "..\Headers\Network.h"

namespace WindowsUtils::Core
{
	/*
	*	~ Ephemeral socket ~
	*/

	Network::EphemeralSocket::EphemeralSocket(ADDRINFOW* addressInfo)
	{
		UnderlyingSocket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
		if (UnderlyingSocket == INVALID_SOCKET)
			throw WuStdException(WSAGetLastError(), __FILEW__, __LINE__);
		else {
			// Setting the IO mode to non-blocking.
			u_long mode = 1;
			ioctlsocket(UnderlyingSocket, FIONBIO, &mode);
		}
	}

	Network::EphemeralSocket::~EphemeralSocket()
	{
		if (UnderlyingSocket != INVALID_SOCKET) {
			shutdown(UnderlyingSocket, SD_SEND);
			closesocket(UnderlyingSocket);
		}
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
		DWORD failedThreshold,
		bool continuous,
		bool includeJitter,
		bool printFqdn,
		bool force,
		bool single,
		bool outputFile,
		const WWuString& filePath,
		bool append
	) :Destination(destination.GetBuffer()), Port(port), Count(count), Timeout(timeout), SecondsInterval(secondsInterval),
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
				filePath.GetBuffer(),
				access,
				FILE_SHARE_READ,
				NULL,
				disposition,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);
			if (File == INVALID_HANDLE_VALUE)
				throw WuResult(GetLastError(), __FILEW__, __LINE__);
		}
		else
			File = INVALID_HANDLE_VALUE;

		_instance = this;
		_ctrlCHit = false;
		_ctrlCHitCount = 0;
		SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);
	}

	Network::TcpingForm::~TcpingForm()
	{
		WSACleanup();
		if (File != NULL && File != INVALID_HANDLE_VALUE)
			CloseHandle(File);

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

	const bool Network::TcpingForm::IsCtrlCHit() {
		Network::TcpingForm* instance = GetForm();
		if (instance == nullptr)
			return false;

		return instance->_ctrlCHit;
	}

	/*
	*	~ Queued data ~
	*/

	Network::_QUEUED_DATA::_QUEUED_DATA(const _QUEUED_DATA& other)
	{
		Type = other.Type;
		switch (other.Type) {
			case Notification::InformationData:
				InformationData = new Notification::MAPPED_INFORMATION_DATA(
					other.InformationData->Computer,
					other.InformationData->NativeThreadId,
					other.InformationData->Text,
					other.InformationData->Source,
					other.InformationData->Tags,
					other.InformationData->TagCount,
					other.InformationData->TimeGenerated,
					other.InformationData->User
				);
				break;

			case Notification::ObjectData:
			{
				ObjectData.ObjectType = other.ObjectData.ObjectType;
				switch (other.ObjectData.ObjectType) {

					case Notification::TCPING_OUTPUT:
					{
						auto objData = reinterpret_cast<PTCPING_OUTPUT>(other.ObjectData.Object);
						ObjectData.Object = new Network::TCPING_OUTPUT(
							objData->Timestamp,
							objData->Destination,
							objData->DestAddress,
							objData->Port,
							objData->Status,
							objData->RoundTripTime,
							objData->Jitter
						);
					} break;

					case Notification::TCPING_STATISTICS:
					{
						auto objData = reinterpret_cast<PTCPING_STATISTICS>(other.ObjectData.Object);
						ObjectData.Object = new Network::TCPING_STATISTICS(
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

			case Notification::ProgressData:
				ProgressData = new Notification::MAPPED_PROGRESS_DATA(*other.ProgressData);
				break;

			case Notification::WarningData:
				WarningData = WWuString(other.WarningData);
				break;
		}
	}

	Network::_QUEUED_DATA::_QUEUED_DATA(
		__in Notification::WRITE_DATA_TYPE type,
		__in PVOID data,
		__in_opt Notification::WRITE_OUTPUT_TYPE* objectData
	)
	{
		Type = type;
		switch (type) {
			case Notification::InformationData:
			{
				auto infoData = reinterpret_cast<Notification::PMAPPED_INFORMATION_DATA>(data);
				InformationData = new Notification::MAPPED_INFORMATION_DATA(*infoData);
			} break;

			case Notification::ObjectData:
			{
				ObjectData.ObjectType = *objectData;
				switch (*objectData) {

					case Notification::TCPING_OUTPUT:
					{
						auto objData = reinterpret_cast<Network::PTCPING_OUTPUT>(data);
						ObjectData.Object = new Network::TCPING_OUTPUT(*objData);
					} break;

					case Notification::TCPING_STATISTICS:
					{
						auto objData = reinterpret_cast<Network::PTCPING_STATISTICS>(data);
						ObjectData.Object = new Network::TCPING_STATISTICS(*objData);
					} break;
				}
			} break;

			case Notification::ProgressData:
			{
				auto progData = reinterpret_cast<Notification::PMAPPED_PROGRESS_DATA>(data);
				ProgressData = new Notification::MAPPED_PROGRESS_DATA(*progData);
			} break;

			case Notification::WarningData:
			{
				auto warnData = reinterpret_cast<WWuString*>(data);
				WarningData = WWuString(*warnData);
			} break;
		}
	}

	Network::_QUEUED_DATA::~_QUEUED_DATA()
	{
		switch (Type) {
			case Notification::InformationData:
				if (InformationData != NULL)
					delete InformationData;
				break;

			case Notification::ObjectData:
				if (ObjectData.Object != NULL)
					delete ObjectData.Object;
				break;

			case Notification::ProgressData:
				if (ProgressData != NULL)
					delete ProgressData;
				break;
		}
	}

	/*
	*	~ Start-Tcping
	*/

	void Network::StartTcpPing(TcpingForm& workForm, WuNativeContext* context)
	{
		// Creating notification queue and worker parameters.
		wuqueue<QUEUED_DATA> queue;
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
			throw WuStdException(workerExitCode, __FILEW__, __LINE__);

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

	Network::_NETWORK_FILE_INFO::_NETWORK_FILE_INFO(DWORD id, DWORD perms, DWORD locks, const WWuString& path, const WWuString& userName)
		: Id(id), Permissions(perms), LockCount(locks), Path(path), UserName(userName) { }

	Network::_NETWORK_FILE_INFO::~_NETWORK_FILE_INFO() { }

	Network::_NETWORK_SESSION_INFO::_NETWORK_SESSION_INFO(const WWuString& sessName, const WWuString& userName, DWORD ioCount)
		: ComputerSessionName(sessName), UserName(userName), OpenIoCount(ioCount) { }

	Network::_NETWORK_SESSION_INFO::~_NETWORK_SESSION_INFO() { }

	void Network::ListNetworkFiles(const WWuString& computerName, const WWuString& basePath, const WWuString& userName, wuvector<NETWORK_FILE_INFO>& result)
	{
		LPBYTE buffer;
		DWORD entryCount;
		DWORD totalEntryCount;

		NET_API_STATUS status = NetFileEnum(
			(LPWSTR)computerName.GetBuffer(),	// The computer name. NULL for the current computer.
			(LPWSTR)basePath.GetBuffer(),		// A path prefix. If used, only paths that starts with this prefix are returned.
			(LPWSTR)userName.GetBuffer(),		// Qualifier for user name or connection name. Results are limited by matches to this qualifier.
			3,									// Level of information data. 3 = FILE_INFO_3.
			&buffer,							// The buffer that receives the list.
			MAX_PREFERRED_LENGTH,				// Maximum preferred buffer length. MAX_PREFERRED_LENGTH = no limit.
			&entryCount,						// The number of entries returned in the buffer.
			&totalEntryCount,					// A hint to total number of entries if the operation is resumed.
			NULL								// Resume handle used in possible subsequent calls.
		);

		if (status != NERR_Success)
			throw WuStdException(status, __FILEW__, __LINE__);

		size_t dataSize = sizeof(FILE_INFO_3);
		LPBYTE offset = buffer;
		for (DWORD i = 0; i < entryCount; i++) {
			auto currentInfo = reinterpret_cast<PFILE_INFO_3>(offset);
			result.push_back(
				NETWORK_FILE_INFO(
					currentInfo->fi3_id,
					currentInfo->fi3_permissions,
					currentInfo->fi3_num_locks,
					currentInfo->fi3_pathname,
					currentInfo->fi3_username
				)
			);

			offset += dataSize;
		}

		NetApiBufferFree(buffer);
	}

	void Network::ListNetworkFiles(const WWuString& computerName, const WWuString& basePath, const WWuString& userName, wuvector<NETWORK_FILE_INFO>& fileInfo, wuvector<NETWORK_SESSION_INFO>& sessionInfo)
	{
		LPBYTE buffer;
		DWORD entryCount;
		DWORD totalEntryCount;

		NET_API_STATUS status = NetFileEnum(
			(LPWSTR)computerName.GetBuffer(),	// The computer name. NULL for the current computer.
			(LPWSTR)basePath.GetBuffer(),		// A path prefix. If used, only paths that starts with this prefix are returned.
			(LPWSTR)userName.GetBuffer(),		// Qualifier for user name or connection name. Results are limited by matches to this qualifier.
			3,									// Level of information data. 3 = FILE_INFO_3.
			&buffer,							// The buffer that receives the list.
			MAX_PREFERRED_LENGTH,				// Maximum preferred buffer length. MAX_PREFERRED_LENGTH = no limit.
			&entryCount,						// The number of entries returned in the buffer.
			&totalEntryCount,					// A hint to total number of entries if the operation is resumed.
			NULL								// Resume handle used in possible subsequent calls.
		);

		if (status != NERR_Success)
			throw WuStdException(status, __FILEW__, __LINE__);

		size_t dataSize = sizeof(FILE_INFO_3);
		LPBYTE offset = buffer;
		for (DWORD i = 0; i < entryCount; i++) {
			auto currentInfo = reinterpret_cast<PFILE_INFO_3>(offset);
			fileInfo.push_back(
				NETWORK_FILE_INFO(
					currentInfo->fi3_id,
					currentInfo->fi3_permissions,
					currentInfo->fi3_num_locks,
					currentInfo->fi3_pathname,
					currentInfo->fi3_username
				)
			);

			offset += dataSize;
		}

		NetApiBufferFree(buffer);

		status = NetSessionEnum(
			(LPWSTR)computerName.GetBuffer(),	// The computer name. NULL for the current computer.
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
			throw WuStdException(status, __FILEW__, __LINE__);

		dataSize = sizeof(SESSION_INFO_1);
		offset = buffer;
		for (DWORD i = 0; i < entryCount; i++) {
			auto currentInfo = reinterpret_cast<PSESSION_INFO_1>(offset);
			sessionInfo.push_back(
				NETWORK_SESSION_INFO(
					currentInfo->sesi1_cname,
					currentInfo->sesi1_username,
					currentInfo->sesi1_num_opens
				)
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
		NET_API_STATUS status = NetFileClose((LPWSTR)computerName.GetBuffer(), fileId);
		if (status != NERR_Success)
			throw WuStdException(status, __FILEW__, __LINE__);
	}

	/*
	*	~ Utility functions
	*/

	void PrintQueueData(const Network::QUEUED_DATA& data, WuNativeContext* context)
	{
		switch (data.Type) {
			case Notification::InformationData:
				context->NativeWriteInformation(data.InformationData);
				break;

			case Notification::ObjectData:
			{
				switch (data.ObjectData.ObjectType) {

					case Notification::TCPING_OUTPUT:
					{
						auto objData = reinterpret_cast<Network::PTCPING_OUTPUT>(data.ObjectData.Object);
						context->NativeWriteObject<Network::TCPING_OUTPUT>(objData, data.ObjectData.ObjectType);
					} break;

					case Notification::TCPING_STATISTICS:
					{
						auto objData = reinterpret_cast<Network::PTCPING_STATISTICS>(data.ObjectData.Object);
						context->NativeWriteObject<Network::TCPING_STATISTICS>(objData, data.ObjectData.ObjectType);
					} break;
				}
			} break;

			case Notification::ProgressData:
				context->NativeWriteProgress(data.ProgressData);
				break;

			case Notification::WarningData:
				context->NativeWriteWarning(data.WarningData);
				break;
		}
	}

	DWORD WINAPI StartTcpingWorker(PVOID params)
	{
		// Defining environment.
		auto threadArgs = reinterpret_cast<Network::PTCPING_WORKER_DATA>(params);
		Network::TcpingForm* workForm = threadArgs->WorkForm;
		wuqueue<Network::QUEUED_DATA>* infoQueue = threadArgs->Queue;

		int intResult;
		ADDRINFOW hints = { 0 }, *addressInfo, *singleInfo;

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
		WuString narrowDest = WWuStringToNarrow(workForm->Destination);
		intResult = GetAddrInfoW(workForm->Destination.GetBuffer(), workForm->PortAsString, &hints, &addressInfo);
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
			while (!Network::TcpingForm::IsCtrlCHit()) {
				DWORD testResult = ERROR_SUCCESS;
				try {
					PerformSingleTestProbe(singleInfo, workForm, workForm->DisplayName, &workForm->Statistics, infoQueue, testResult);
				}
				catch (const WuStdException& ex) {
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
				catch (const WuStdException& ex) {
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

	void PerformSingleTestProbe(ADDRINFOW* singleInfo, Network::TcpingForm* workForm, const WWuString& displayName,
		Network::PTCPING_STATISTICS statistics, wuqueue<Network::QUEUED_DATA>* infoQueue, DWORD& result)
	{
		DWORD finalResult = ERROR_SUCCESS;
		DWORD sendResult = ERROR_SUCCESS;
		WWuString outputText;
		WuStopWatch timeoutSpw;
		FILETIME timestamp;

		wuunique_ptr<Network::EphemeralSocket> ephSocket;
		
		workForm->StopWatch.Restart();
		bool timedOut = false;

		// Creating an ephemeral socket.
		try {
			ephSocket = make_wuunique<Network::EphemeralSocket>(singleInfo);
		}
		catch (const WuStdException& ex) {
			sendResult = ex.ErrorCode();
			return;
		}

		// Connecting.
		int connResult = connect(ephSocket->UnderlyingSocket, singleInfo->ai_addr, (int)singleInfo->ai_addrlen);
		if (connResult == SOCKET_ERROR) {
			connResult = WSAGetLastError();
			if (connResult != ERROR_SUCCESS && connResult != WSAEWOULDBLOCK)
				throw WuStdException(WSAGetLastError(), __FILEW__, __LINE__);
		}

		// Trying to send until we succeed or timeout.
		timeoutSpw.Start();
		while (!Network::TcpingForm::IsCtrlCHit()) {
			try {
				if (workForm->IsForce) {
					connResult = send(ephSocket->UnderlyingSocket, "tits", 4, 0);
					if (connResult == SOCKET_ERROR)
						throw WuStdException(WSAGetLastError(), __FILEW__, __LINE__);
				}
				else {
					connResult = send(ephSocket->UnderlyingSocket, NULL, 0, 0);
					if (connResult == SOCKET_ERROR)
						throw WuStdException(WSAGetLastError(), __FILEW__, __LINE__);
				}
				break;
			}
			catch (const WuStdException&) { }

			if ((timeoutSpw.ElapsedMilliseconds() / 1000) >= workForm->Timeout) {
				timedOut = true;
				break;
			}

			Sleep(1);
		}
		workForm->StopWatch.Stop();

		if (Network::TcpingForm::IsCtrlCHit()) {
			result = ERROR_CANCELLED;
			return;
		}

		statistics->Sent++;

		if (timedOut) {
			outputText += WWuString::Format(L"%ws%ws - TCP:%d - No response - time=%dms", outputText.GetBuffer(), displayName.GetBuffer(), workForm->Port, (workForm->Timeout * 1000));

			if (workForm->OutputToFile) {
				// Instead of printing the same output as the one in the file
				// We show a nice progress bar with condensed information.
				int percentage = 0;
				WWuString action = WWuString::Format(L"Probing %ws", displayName.GetBuffer());
				WWuString status;

				Notification::MAPPED_PROGRESS_DATA progressRecord;
				if (workForm->IsContinuous) {
					status = WWuString::Format(L"TCP:%d - No response - time %dms - press Ctrl + C to stop.", workForm->Port, (workForm->Timeout * 1000));
					progressRecord = Notification::MAPPED_PROGRESS_DATA(
						action.GetBuffer(),
						0,
						(LPWSTR)NULL,
						-1,
						0,
						Notification::Processing,
						-1,
						status.GetBuffer()
					);

					Network::QUEUED_DATA queueData(Notification::ProgressData, &progressRecord, NULL);
					infoQueue->push(queueData);
				}
				else {
					status = WWuString::Format(L"TCP:%d - No response - time %dms", workForm->Port, (workForm->Timeout * 1000));
					percentage = std::lround((static_cast<float>(statistics->Sent) / workForm->Count) * 100);
					progressRecord = Notification::MAPPED_PROGRESS_DATA(
						action.GetBuffer(),
						0,
						(LPWSTR)NULL,
						-1,
						percentage,
						Notification::Processing,
						-1,
						status.GetBuffer()
					);

					Network::QUEUED_DATA queueData(Notification::ProgressData, &progressRecord, NULL);
					infoQueue->push(queueData);
				}

				IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", outputText.GetBuffer()));
			}
			else {
#if defined(_TCPING_TEST)
				wprintf(L"%ws\n", outputText.GetBuffer());
#else
				GetSystemTimeAsFileTime(&timestamp);
				Network::TCPING_OUTPUT tcpingOut(
					timestamp,
					(LPWSTR)workForm->Destination.GetBuffer(),
					(LPWSTR)displayName.GetBuffer(),
					workForm->Port,
					Network::TCPING_STATUS::Timeout,
					static_cast<double>(workForm->Timeout * 1000),
					-1.00
				);

				auto objType = Notification::TCPING_OUTPUT;
				Network::QUEUED_DATA queueData(Notification::ObjectData, &tcpingOut, &objType);
				infoQueue->push(queueData);

				/*LPWSTR tags[1] = { L"PSHOST" };
				Notification::MAPPED_INFORMATION_DATA report(
					(LPWSTR)NULL, GetCurrentThreadId(), outputText.GetBuffer(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
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
		outputText = WWuString::Format(L"%ws%ws - TCP:%d - Port is open - time=%.2fms", outputText.GetBuffer(), displayName.GetBuffer(), workForm->Port, currentMilliseconds);

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

			outputText = WWuString::Format(L"%ws jitter=%.2fms", outputText.GetBuffer(), currentJitter);
		}

		statistics->Successful++;
		statistics->TotalMilliseconds += currentMilliseconds;

		if (workForm->OutputToFile) {
			// Instead of printing the same output as the one in the file
			// We show a nice progress bar with condensed information.
			int percentage = 0;
			WWuString action = WWuString::Format(L"Probing %ws", displayName.GetBuffer());
			WWuString status = WWuString::Format(L"TCP:%d - Port is open - time %.2fms", workForm->Port, currentMilliseconds);

			if (workForm->IncludeJitter && statistics->Successful > 1)
				status += WWuString::Format(L" jitter=%.2fms", currentJitter);

			Notification::MAPPED_PROGRESS_DATA progressRecord;
			if (workForm->IsContinuous) {
				status += L" - press Ctrl + C to stop.";
				progressRecord = Notification::MAPPED_PROGRESS_DATA(
					action.GetBuffer(),
					0,
					(LPWSTR)NULL,
					-1,
					0,
					Notification::Processing,
					-1,
					status.GetBuffer()
				);

				Network::QUEUED_DATA queueData(Notification::ProgressData, &progressRecord, NULL);
				infoQueue->push(queueData);
			}
			else {
				percentage = std::lround((static_cast<float>(statistics->Sent) / workForm->Count) * 100);
				progressRecord = Notification::MAPPED_PROGRESS_DATA(
					action.GetBuffer(),
					0,
					(LPWSTR)NULL,
					-1,
					percentage,
					Notification::Processing,
					-1,
					status.GetBuffer()
				);

				Network::QUEUED_DATA queueData(Notification::ProgressData, &progressRecord, NULL);
				infoQueue->push(queueData);
			}

			IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", outputText.GetBuffer()));
		}
		else {
#if defined(_TCPING_TEST)
			wprintf(L"%ws\n", outputText.GetBuffer());
#else
			GetSystemTimeAsFileTime(&timestamp);
			Network::TCPING_OUTPUT tcpingOut(
				timestamp,
				(LPWSTR)workForm->Destination.GetBuffer(),
				(LPWSTR)displayName.GetBuffer(),
				workForm->Port,
				Network::TCPING_STATUS::Open,
				currentMilliseconds,
				currentJitter
			);

			auto objType = Notification::TCPING_OUTPUT;
			Network::QUEUED_DATA queueData(Notification::ObjectData, &tcpingOut, &objType);
			infoQueue->push(queueData);

			/*LPWSTR tags[1] = { L"PSHOST" };
			Notification::MAPPED_INFORMATION_DATA report(
				(LPWSTR)NULL, GetCurrentThreadId(), outputText.GetBuffer(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
			);

			context->NativeWriteInformation(&report);*/
#endif
		}

		result = finalResult;
	}

	void ProcessStatistics(Network::TcpingForm* workForm, WuNativeContext* context)
	{
		workForm->Statistics.FailedPercent = (static_cast<double>(workForm->Statistics.Failed) / workForm->Statistics.Sent) * 100.00;
		
		if (workForm->Statistics.TotalMilliseconds == 0)
			workForm->Statistics.AvgRtt = 0.00;
		else
			workForm->Statistics.AvgRtt = workForm->Statistics.TotalMilliseconds / workForm->Statistics.Successful;

		// Yes, this can be done better, but my ADHD doesn't wanna think right now.
		WWuString output = WWuString::Format(
			L"\nPinging statistics for %ws:\n\tPackets: Sent = %d, Successful = %d, Failed = %d (%.2f%%),\n",
			workForm->DisplayName.GetBuffer(),
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
			IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", output.GetBuffer()));
		else {
#if defined(_TCPING_TEST)
			wprintf(L"%ws\n", output.GetBuffer());
#else
			context->NativeWriteObject<Network::TCPING_STATISTICS>(&workForm->Statistics, Notification::TCPING_STATISTICS);

			/*LPWSTR tags[1] = { L"PSHOST" };
			Notification::MAPPED_INFORMATION_DATA report(
				(LPWSTR)NULL, GetCurrentThreadId(), output.GetBuffer(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
			);

			context->NativeWriteInformation(&report);*/
		}
#endif
	}

	void PrintHeader(Network::TcpingForm* workForm, const WWuString& displayText)
	{
		WWuString header;

		header = L"~ Start-Tcping\n";
		header += WWuString::Format(L"\nDestination: %ws : %s\n", workForm->Destination.GetBuffer(), workForm->PortAsString);

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

		IO::AppendTextToFile(workForm->File, WWuString::Format(L"\n%ws\n", header.GetBuffer()));
	}

	void FormatIp(ADDRINFOW* address, WWuString& ipString)
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
	void ReverseIp(WWuString& ip)
	{
		wchar_t seps[] = L".";
		wchar_t* token;
		wchar_t* context = NULL;
		int i = 0;
		wchar_t buffer[4][4] = { 0 };
		token = wcstok_s((wchar_t*)ip.GetBuffer(), seps, &context);
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
			output = WWuString::Format(L"%ws%ws.", output.GetBuffer(), ipSplit[revIndex].GetBuffer());
		}

		ip = WWuString::Format(L"%ws%ws", output.GetBuffer(), L"IN-ADDR.ARPA");*/
	}

	void ReverseIPv6(WWuString& ip)
	{
		in6_addr addr;
		InetPtonW(AF_INET6, ip.GetBuffer(), &addr);
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
	void ResolveIpToDomainName(const WWuString& ip, WWuString& domainName)
	{
		PDNS_RECORD pDnsRecord;
		WORD wType = DNS_TYPE_PTR;
		PIP4_ARRAY pSrvList = NULL;
		DNS_FREE_TYPE freetype = DnsFreeRecordListDeep;

		WWuString reverse(ip.GetBuffer());
		ReverseIp(reverse);

		DNS_STATUS status = DnsQuery(reverse.GetBuffer(),	//Pointer to OwnerName. 
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

	void ResolveIpv6ToDomainName(WWuString& ip, WWuString& reverse)
	{
		PDNS_ADDR_ARRAY srvList = NULL;
		DNS_QUERY_RESULT queryResult = { DNS_QUERY_REQUEST_VERSION1 };

		ReverseIPv6(ip);

		DNS_QUERY_REQUEST request = {
			DNS_QUERY_REQUEST_VERSION1,
			ip.GetBuffer(),
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