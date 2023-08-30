#include "..\pch.h"

#include "..\Headers\Network.h"

namespace WindowsUtils::Core
{
	/*
	*	~ Ephemeral socket ~
	*/

	Network::EphemeralSocket::EphemeralSocket(ADDRINFO* addressInfo)
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
		PREFERRED_IP_PROTOCOL ipProt,
		DWORD failedThreshold,
		bool continuous,
		bool includeJitter,
		bool includeDateTime,
		bool printFqdn,
		bool force,
		bool outputFile,
		const WWuString& filePath,
		bool append
	) :Destination(destination.GetBuffer()), Port(port), Count(count), Timeout(timeout), SecondsInterval(secondsInterval), PreferredIpProtocol(ipProt),
			FailedCountThreshold(failedThreshold), IsContinuous(continuous), IncludeJitter(includeJitter), IncludeDateTime(includeDateTime), PrintFqdn(printFqdn),
				IsForce(force), OutputToFile(outputFile), Append(append)
	{
		WSADATA wsaData;
		WORD reqVersion = MAKEWORD(2, 2);
		int result;

		if ((result = WSAStartup(reqVersion, &wsaData)) != 0)
			throw result;

		_ui64toa_s(Port, PortAsString, 6, 10);

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

	inline const bool Network::TcpingForm::IsCtrlCHit() {
		Network::TcpingForm* instance = GetForm();
		if (instance == nullptr)
			return false;

		return instance->_ctrlCHit;
	}

	/*
	*	~ Function definition ~
	*/

	void Network::StartTcpPing(TcpingForm& workForm, WuNativeContext* context)
	{
		// Defining environment.
		int intResult;
		ADDRINFO hints = { 0 }, * addressInfo, * singleInfo;

		double milliseconds = 0.0;
		double totalMilliseconds = 0.0;
		double jitter = 0.0;
		DWORD successCount = 0;
		int failedCount = 0;

		WWuString header;
		WWuString displayName;
		WWuString destText;

		// Using a variable here, cause if the user goes Ctrl + C we can set
		// up the error to 'ERROR_CANCELLED'.
		DWORD finalResult;

		// Statistics showed at the end.
		TCPING_STATISTICS statistics;

		auto prefIpProto = workForm.PreferredIpProtocol;

		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_UNSPEC;

		// Attempting to get destination address information.
		WuString narrowDest = WWuStringToNarrow(workForm.Destination);
		intResult = getaddrinfo(narrowDest.GetBuffer(), workForm.PortAsString, &hints, &addressInfo);
		if (intResult != 0)
			throw WuStdException(intResult, __FILEW__, __LINE__);

		bool found = false;
		WuResult lastResult;
		for (singleInfo = addressInfo; singleInfo != NULL; singleInfo = singleInfo->ai_next) {
			if (workForm.IsCtrlCHit()) {
				finalResult = ERROR_CANCELLED;
				goto END;
			}
			
			if ((singleInfo->ai_family == AF_UNSPEC && prefIpProto == None) ||
				(singleInfo->ai_family == AF_INET && prefIpProto != IPv6) ||
				(singleInfo->ai_family == AF_INET6 && prefIpProto != IPv4)
				) {
				found = true;
				break;
			}
		}

		if (!found)
			throw WuStdException(WSAHOST_NOT_FOUND, __FILEW__, __LINE__);

		FormatIp(addressInfo, destText);
		if (workForm.PrintFqdn) {
			if (singleInfo->ai_family == AF_INET6) {
				WWuString ipStr = WWuString(destText);
				ResolveIpv6ToDomainName(ipStr, displayName);
				if (displayName.Length() == 0)
					displayName = destText;
			}
			else {
				ResolveIpToDomainName(destText, displayName);
				if (displayName.Length() == 0)
					displayName = destText;
			}
		}
		else
			displayName = destText;

		// Header
		PrintHeader(&workForm, displayName, context);

		// Main loop. Here the 'ping' will happen for 'count' times.
		if (workForm.IsContinuous) {
			while (!Network::TcpingForm::IsCtrlCHit()) {
				DWORD testResult = ERROR_SUCCESS;
				PerformSingleTestProbe(singleInfo, &workForm, displayName, &statistics, context, testResult);
				if (testResult != ERROR_SUCCESS && testResult != WSAETIMEDOUT) {
					if (testResult == ERROR_CANCELLED)
						goto END;

					throw WuStdException(testResult, __FILEW__, __LINE__);
				}
				// We don't wanna sleep on the last one.
				else if (testResult == ERROR_SUCCESS && (statistics.Sent < workForm.Count || workForm.IsContinuous))
					Sleep(workForm.SecondsInterval * 1000);
			}
		}
		else {
			for (DWORD i = 0; i < workForm.Count; i++) {
				if (static_cast<int>(statistics.Failed) >= workForm.FailedCountThreshold)
					goto END;

				DWORD testResult = ERROR_SUCCESS;
				PerformSingleTestProbe(singleInfo, &workForm, displayName, &statistics, context, testResult);
				if (testResult != ERROR_SUCCESS && testResult != WSAETIMEDOUT) {
					if (testResult == ERROR_CANCELLED)
						goto END;

					throw WuStdException(testResult, __FILEW__, __LINE__);
				}
				else if (testResult == ERROR_SUCCESS && (statistics.Sent < workForm.Count || workForm.IsContinuous))
					Sleep(workForm.SecondsInterval * 1000);
			}
		}

	END:
		ProcessStatistics(&statistics, displayName, &workForm, context);
	};

	//////////////////////////////////////////////////////////////////////
	//
	// ~ Important note:
	//
	// Since this is running in native, and managed code performance
	// is very important. At first everything was nice and structured
	// with return types, and separate functions, but that was 
	// affecting the results.
	//
	//////////////////////////////////////////////////////////////////////

	void PerformSingleTestProbe(ADDRINFO* singleInfo, Network::TcpingForm* workForm, const WWuString& displayName, Network::PTCPING_STATISTICS statistics, WuNativeContext* context, DWORD& result)
	{
		DWORD finalResult = ERROR_SUCCESS;
		DWORD sendResult = ERROR_SUCCESS;
		WWuString outputText;
		WuStopWatch timeoutSpw;


		wuunique_ptr<Network::EphemeralSocket> ephSocket;
		
		CHECKIFCTRLC;

		if (workForm->IncludeDateTime) {
			SYSTEMTIME dateTime;
			GetLocalTime(&dateTime);
			
			outputText = WWuString::Format(
				L"%d-%d-%d %d:%d:%d.%d - ",
				dateTime.wYear,
				dateTime.wMonth,
				dateTime.wDay,
				dateTime.wHour,
				dateTime.wMinute,
				dateTime.wSecond,
				dateTime.wMilliseconds);
		}

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

		statistics->Sent++;

		if (timedOut) {
			CHECKIFCTRLC;
			outputText += WWuString::Format(L"%ws%ws - TCP:%d - No response - time=%dms", outputText.GetBuffer(), displayName.GetBuffer(), workForm->Port, (workForm->Timeout * 1000));

			if (workForm->OutputToFile) {
				// Instead of printing the same output as the one in the file
				// We show a nice progress bar with condensed information.
				int percentage = 0;
				WWuString action = WWuString::Format(L"Probing %ws", displayName.GetBuffer());
				WWuString status;

				wuunique_ptr<Notification::MAPPED_PROGRESS_DATA> progressRecord;
				if (workForm->IsContinuous) {
					status = WWuString::Format(L"TCP:%d - No response - time %dms - press Ctrl + C to stop.", workForm->Port, (workForm->Timeout * 1000));
					progressRecord = make_wuunique<Notification::MAPPED_PROGRESS_DATA>(
						action.GetBuffer(),
						0,
						(LPWSTR)NULL,
						-1,
						0,
						Notification::Processing,
						-1,
						status.GetBuffer()
					);

					context->NativeWriteProgress(progressRecord.get());
				}
				else {
					status = WWuString::Format(L"TCP:%d - No response - time %dms", workForm->Port, (workForm->Timeout * 1000));
					percentage = std::lround((static_cast<float>(statistics->Sent) / workForm->Count) * 100);
					progressRecord = make_wuunique<Notification::MAPPED_PROGRESS_DATA>(
						action.GetBuffer(),
						0,
						(LPWSTR)NULL,
						-1,
						percentage,
						Notification::Processing,
						-1,
						status.GetBuffer()
					);

					context->NativeWriteProgress(progressRecord.get());
				}

				IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", outputText.GetBuffer()));
			}
			else {
#if defined(_TCPING_TEST)
				wprintf(L"%ws\n", outputText.GetBuffer());
#else
				LPWSTR tags[1] = { L"PSHOST" };
				Notification::MAPPED_INFORMATION_DATA report(
					(LPWSTR)NULL, GetCurrentThreadId(), outputText.GetBuffer(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
				);

				context->NativeWriteInformation(&report);
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

		double currentJitter = 0.00;
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

			wuunique_ptr<Notification::MAPPED_PROGRESS_DATA> progressRecord;
			if (workForm->IsContinuous) {
				status += L" - press Ctrl + C to stop.";
				progressRecord = make_wuunique<Notification::MAPPED_PROGRESS_DATA>(
					action.GetBuffer(),
					0,
					(LPWSTR)NULL,
					-1,
					0,
					Notification::Processing,
					-1,
					status.GetBuffer()
				);

				context->NativeWriteProgress(progressRecord.get());
			}
			else {
				percentage = std::lround((static_cast<float>(statistics->Sent) / workForm->Count) * 100);
				progressRecord = make_wuunique<Notification::MAPPED_PROGRESS_DATA>(
					action.GetBuffer(),
					0,
					(LPWSTR)NULL,
					-1,
					percentage,
					Notification::Processing,
					-1,
					status.GetBuffer()
				);

				context->NativeWriteProgress(progressRecord.get());
			}

			IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", outputText.GetBuffer()));
		}
		else {
#if defined(_TCPING_TEST)
			wprintf(L"%ws\n", outputText.GetBuffer());
#else
			LPWSTR tags[1] = { L"PSHOST" };
			Notification::MAPPED_INFORMATION_DATA report(
				(LPWSTR)NULL, GetCurrentThreadId(), outputText.GetBuffer(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
			);

			context->NativeWriteInformation(&report);
#endif
		}

	END:
	
		result = finalResult;
	}

	void ProcessStatistics(const Network::PTCPING_STATISTICS statistics, const WWuString& displayName, Network::TcpingForm* workForm, WuNativeContext* context)
	{
		statistics->FailedPercent = std::lround((static_cast<double>(statistics->Failed) / statistics->Sent) * 100);
		
		if (statistics->TotalMilliseconds == 0)
			statistics->AvgRtt = 0.00;
		else
			statistics->AvgRtt = statistics->TotalMilliseconds / statistics->Successful;

		// Yes, this can be done better, but my ADHD doesn't wanna think right now.
		WWuString output = WWuString::Format(
			L"\nPinging statistics for %ws:\n\tPackets: Sent = %d, Successful = %d, Failed = %d (%d%%),\n",
			displayName.GetBuffer(),
			statistics->Sent,
			statistics->Successful,
			statistics->Failed,
			statistics->FailedPercent
		);
		output += WWuString::Format(
			L"Approximate round trip times in milli-seconds:\n\tMinimum = %.2fms, Maximum = %.2fms, Average = %.2fms",
			statistics->MinRtt,
			statistics->MaxRtt,
			statistics->AvgRtt
		);
		if (workForm->IncludeJitter) {
			if (statistics->TotalJitter == 0)
				statistics->AvgJitter = 0.00;
			else
				statistics->AvgJitter = statistics->TotalJitter / (statistics->Successful - 1);
			
			output += WWuString::Format(
				L",\nApproximate jitter in milli-seconds:\n\tMinimum = %.2fms, Maximum = %.2fms, Average = %.2fms",
				statistics->MinJitter,
				statistics->MaxJitter,
				statistics->AvgJitter
			);
		}

#if defined(_TCPING_TEST)
		if (workForm->OutputToFile)
			IO::AppendTextToFile(workForm->File, WWuString::Format(L"%ws\n", output.GetBuffer()));
		else
			// Testing before using 'NativeWriteInformation'.
			wprintf(L"%ws\n", output.GetBuffer());
#else
		LPWSTR tags[1] = { L"PSHOST" };
		Notification::MAPPED_INFORMATION_DATA report(
			(LPWSTR)NULL, GetCurrentThreadId(), output.GetBuffer(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
		);

		context->NativeWriteInformation(&report);
#endif
	}

	void PrintHeader(Network::TcpingForm* workForm, const WWuString& displayText, WuNativeContext* context)
	{
		WWuString header;

		if (workForm->OutputToFile) {
			WWuString protoStr;
			switch (workForm->PreferredIpProtocol) {
				case Network::IPv4:
					protoStr = L"IPv4";
					break;

				case Network::IPv6:
					protoStr = L"IPv4";
					break;

				case Network::None:
					protoStr = L"Unspecified";
					break;
			}
			
			header = L"~ Start-Tcping\n";
			header += WWuString::Format(L"\nDestination: %ws : %s\n", workForm->Destination.GetBuffer(), workForm->PortAsString);
			
			if (workForm->IsContinuous) {
				header += WWuString::Format(
					L"Count: continuous\nFail threshold: continuous\nInterval: %ds\nTimeout: %ds\nForce: %s\nPrint FQDN: %s\nPreferred protocol: %ws\n",
					workForm->SecondsInterval,
					workForm->Timeout,
					workForm->IsForce ? "true" : "false",
					workForm->PrintFqdn ? "true" : "false",
					protoStr.GetBuffer()
				);
			}
			else {
				header += WWuString::Format(
					L"Count: %d\nFail threshold: %d\nInterval: %ds\nTimeout: %ds\nForce: %s\nPrint FQDN: %s\nPreferred protocol: %ws\n",
					workForm->Count,
					workForm->FailedCountThreshold,
					workForm->SecondsInterval,
					workForm->Timeout,
					workForm->IsForce ? "true" : "false",
					workForm->PrintFqdn ? "true" : "false",
					protoStr.GetBuffer()
				);
			}

			IO::AppendTextToFile(workForm->File, WWuString::Format(L"\n%ws\n", header.GetBuffer()));
		}
		else {
			if (workForm->IsContinuous)
				header = WWuString::Format(L"Probing %ws [%ws] continuously on port %s (press Ctrl + C to stop):", workForm->Destination.GetBuffer(), displayText.GetBuffer(), workForm->PortAsString);
			else
				header = WWuString::Format(L"Probing %ws [%ws] on port %s:", workForm->Destination.GetBuffer(), displayText.GetBuffer(), workForm->PortAsString);

#if defined(_TCPING_TEST)
			wprintf(L"\n%ws\n", header.GetBuffer());
#else
			LPWSTR tags[1] = { L"PSHOST" };
			Notification::MAPPED_INFORMATION_DATA report(
				(LPWSTR)NULL, GetCurrentThreadId(), WWuString::Format(L"\n%ws", header.GetBuffer()).GetBuffer(), L"Start-Tcping", tags, 1, 0, (LPWSTR)NULL
			);

			context->NativeWriteInformation(&report);
#endif
		}
	}

	void FormatIp(ADDRINFO* address, WWuString& ipString)
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