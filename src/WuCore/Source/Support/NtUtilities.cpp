#include "../../pch.h"

#include "../../Headers/Support/NtUtilities.h"

#include <TlHelp32.h>

namespace WindowsUtils::Core
{
	void NtUtilities::GetProcessUsingFile(const WWuString& fileName, wuvector<DWORD>& processIdList)
	{
		IO_STATUS_BLOCK ioStatusBlock { };
		NTSTATUS statusResult = STATUS_SUCCESS;
		
		WWuString finalFileName;
		if (fileName.Length() > MAX_PATH)
			finalFileName = L"\\\\?\\" + fileName;
		else
			finalFileName = fileName;

		HANDLE hFile = CreateFileW(
			finalFileName.GetBuffer(),
			FILE_READ_ATTRIBUTES,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS,
			NULL
		);
		if (INVALID_HANDLE_VALUE == hFile)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		ULONG bufferSize = 1 << 10;
		std::unique_ptr<BYTE[]> buffer;
		do {
			buffer = std::make_unique<BYTE[]>(bufferSize);
			statusResult = NtFunctions::GetInstance()->NtQueryInformationFile(hFile, &ioStatusBlock, buffer.get(), bufferSize, FILE_INFORMATION_CLASS::FileProcessIdsUsingFileInformation);
			if (statusResult == STATUS_SUCCESS)
				break;

			if (statusResult != STATUS_INFO_LENGTH_MISMATCH)
				throw WuStdException(statusResult, __FILEW__, __LINE__, CoreErrorType::NtError);

			bufferSize = static_cast<ULONG>(ioStatusBlock.Information);

		} while (statusResult == STATUS_INFO_LENGTH_MISMATCH);

		auto info = reinterpret_cast<PFILE_PROCESS_IDS_USING_FILE_INFORMATION>(buffer.get());
		for (ULONG i = 0; i < info->NumberOfProcessIdsInList; i++)
			processIdList.push_back(static_cast<DWORD>(info->ProcessIdList[i]));

		CloseHandle(hFile);
	}

	void NtUtilities::GetProcessUsingKey(const WWuString& ntKeyName, wuvector<DWORD>& resultPidList, std::unordered_map<DWORD, WWuString>& allProcessMap, bool closeHandle)
	{
		NTSTATUS status;
		NtFunctions* ntdll = NtFunctions::GetInstance();

		for (const auto& [pid, imagePath] : allProcessMap) {
			ProcessHandle hProcess;
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, FALSE, pid);
			if (hProcess.get() == NULL)
				continue;

			// Querying process handle information.
			ULONG bytesNeeded = 9216;
			wuunique_ptr<BYTE[]> buffer;
			do {
				bytesNeeded += 1024;
				buffer = make_wuunique<BYTE[]>(bytesNeeded);
				status = ntdll->NtQueryInformationProcess(hProcess, PROCESSINFOCLASS::ProcessHandleInformation, buffer.get(), bytesNeeded, &bytesNeeded);
				if (status == STATUS_SUCCESS)
					break;

				if (status != STATUS_INFO_LENGTH_MISMATCH)
					throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };

			} while (status == STATUS_INFO_LENGTH_MISMATCH);

			// Going through each handle entry.
			HANDLE hCurrentProcess = GetCurrentProcess();
			auto procHandleInfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());
			for (ULONG_PTR i = 0; i < procHandleInfo->NumberOfHandles; i++) {

				// Duplicating the handle.
				ObjectHandle hDup;
				if (!NT_SUCCESS(ntdll->NtDuplicateObject(hProcess, procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, NULL, 0, DUPLICATE_SAME_ACCESS)))
					continue;

				// Querying the handle object type name.
				bytesNeeded = 1024;
				wuunique_ptr<BYTE[]> infoBuffer = make_wuunique<BYTE[]>(bytesNeeded);
				if (!NT_SUCCESS(status = ntdll->NtQueryObject(hDup, OBJECT_INFORMATION_CLASS::ObjectTypeInformation, infoBuffer.get(), bytesNeeded, &bytesNeeded)))
					throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };

				WWuString objectTypeName = reinterpret_cast<POBJECT_TYPE_INFORMATION>(infoBuffer.get())->TypeName.Buffer;
				if (objectTypeName == L"Key") {

					// Checking the key name.
					bytesNeeded = 1024;
					if (!NT_SUCCESS(status = ntdll->NtQueryObject(hDup, OBJECT_INFORMATION_CLASS::ObjectNameInformation, infoBuffer.get(), bytesNeeded, &bytesNeeded)))
						continue;

					// Checking if it's our key.
					auto nameInfo = reinterpret_cast<POBJECT_NAME_INFORMATION>(infoBuffer.get());
					if (nameInfo->Name.Buffer != NULL)
						if (_wcsnicmp(ntKeyName.GetBuffer(), nameInfo->Name.Buffer, ntKeyName.Length()) == 0) {
							
							// Attempting to close the original handle if requested.
							if (closeHandle) {
								if (!NT_SUCCESS(status = ntdll->NtDuplicateObject(hProcess, procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, NULL, 0, DUPLICATE_CLOSE_SOURCE)))
									throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };
							}

							resultPidList.push_back(pid);
						}
				}
			}
		}
	}

	void NtUtilities::CloseExternalHandlesToFile(DWORD processId, const WWuString& fileName)
	{
		ProcessHandle hProcess;
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, FALSE, processId);
		if (hProcess.get() == NULL)
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };

		NTSTATUS status;
		NtFunctions* ntdll = NtFunctions::GetInstance();

		// Querying process handle information.
		ULONG bytesNeeded = 9216;
		wuunique_ptr<BYTE[]> buffer;
		do {
			bytesNeeded += 1024;
			buffer = make_wuunique<BYTE[]>(bytesNeeded);
			status = ntdll->NtQueryInformationProcess(hProcess, PROCESSINFOCLASS::ProcessHandleInformation, buffer.get(), bytesNeeded, &bytesNeeded);
			if (status == STATUS_SUCCESS)
				break;

			if (status != STATUS_INFO_LENGTH_MISMATCH)
				throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };

		} while (status == STATUS_INFO_LENGTH_MISMATCH);

		// Going through each handle entry.
		HANDLE hCurrentProcess = GetCurrentProcess();
		auto procHandleInfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());
		for (ULONG_PTR i = 0; i < procHandleInfo->NumberOfHandles; i++) {

			// Duplicating the handle.
			ObjectHandle hDup;
			if (!NT_SUCCESS(ntdll->NtDuplicateObject(hProcess, procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, NULL, 0, DUPLICATE_SAME_ACCESS)))
				continue;

			// Querying the handle object type name.
			bytesNeeded = 1024;
			wuunique_ptr<BYTE[]> infoBuffer = make_wuunique<BYTE[]>(bytesNeeded);
			if (!NT_SUCCESS(status = ntdll->NtQueryObject(hDup, OBJECT_INFORMATION_CLASS::ObjectTypeInformation, infoBuffer.get(), bytesNeeded, &bytesNeeded)))
				throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };

			WWuString objectTypeName = reinterpret_cast<POBJECT_TYPE_INFORMATION>(infoBuffer.get())->TypeName.Buffer;
			if (objectTypeName == L"File") {

				// Querying the object name.
				try {
					WU_QUERY_OBJECT_DATA threadData {
						hDup,
						OBJECT_INFORMATION_CLASS::ObjectNameInformation
					};

					QueryObjectWithTimeout(threadData, 100);
					if (threadData.Status != STATUS_SUCCESS)
						continue;
					
					// Checking if it's our file.
					auto nameInfo = reinterpret_cast<POBJECT_NAME_INFORMATION>(threadData.ObjectData);
					if (nameInfo->Name.Buffer != NULL) {
						WWuString nameStr { nameInfo->Name.Buffer };
						if (nameStr == fileName) {
							
							// Attempting to close the original handle.
							if (!NT_SUCCESS(status = ntdll->NtDuplicateObject(hProcess, procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, NULL, 0, DUPLICATE_CLOSE_SOURCE)))
								throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };
						}
					}
				}
				catch (const WuStdException& ex) {
					throw ex;
				}
			}
		}
	}

	void NtUtilities::GetRunnningProcessIdList(wuvector<DWORD>& procIdList)
	{
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		PROCESSENTRY32 procEntry { };
		procEntry.dwSize = sizeof(procEntry);

		// Skip the idle process.
		Process32First(hSnapshot, &procEntry);
		while (Process32Next(hSnapshot, &procEntry))
			procIdList.push_back(procEntry.th32ProcessID);

		CloseHandle(hSnapshot);
	}

	/*
	* Alternative to QueryFullProcessImageNameW.
	* This function returns names from processes like, System, Registry or Secure System.
	*/
	void WINAPI NtUtilities::GetProcessImageName(DWORD processId, WWuString& imageName)
	{
		ULONG bytesNeeded { };
		ULONG bufferSize { 1 << 12 };
		NTSTATUS statusResult { STATUS_SUCCESS };

		std::unique_ptr<BYTE[]> buffer;
		do {
			buffer = std::make_unique<BYTE[]>(bufferSize);
			statusResult = NtFunctions::GetInstance()->NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS::SystemFullProcessInformation, buffer.get(), bufferSize, &bytesNeeded);
			if (STATUS_SUCCESS != statusResult
				&& STATUS_INFO_LENGTH_MISMATCH != statusResult
				&& statusResult != STATUS_BUFFER_OVERFLOW
				&& statusResult != STATUS_BUFFER_TOO_SMALL)
				throw WuStdException(GetLastError(), __FILEW__, __LINE__);

			if (STATUS_SUCCESS == statusResult)
				break;

			bufferSize = bytesNeeded;

		} while (statusResult == STATUS_BUFFER_TOO_SMALL || statusResult == STATUS_BUFFER_OVERFLOW || statusResult == STATUS_INFO_LENGTH_MISMATCH);

		PSYSTEM_PROCESS_INFORMATION systemProcInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(buffer.get());
		do {
			if (systemProcInfo->UniqueProcessId == reinterpret_cast<HANDLE>((UINT_PTR)processId)) {
				size_t imgNameSize = wcslen(systemProcInfo->ImageName.Buffer) + 1;
				if (imgNameSize > 1)
					imageName = systemProcInfo->ImageName.Buffer;

				break;
			}
			systemProcInfo = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)systemProcInfo + systemProcInfo->NextEntryOffset);

		} while (systemProcInfo->NextEntryOffset != 0);
	}

	void NtUtilities::ListRunningProcessesWithImageName(std::unordered_map<DWORD, WWuString>& processMap)
	{
		NTSTATUS status;
		ULONG bytesNeeded = 10240;

		wuunique_ptr<BYTE[]> buffer;
		do {
			buffer = make_wuunique<BYTE[]>(bytesNeeded);
			if (!NT_SUCCESS(status = NtFunctions::GetInstance()->NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS::SystemFullProcessInformation, buffer.get(), bytesNeeded, &bytesNeeded))) {
				if (status != STATUS_INFO_LENGTH_MISMATCH && status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
					throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };
			}
			else
				break;

			// More processes might have been created until the next call.
			bytesNeeded += 1024;

		} while (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH);

		auto systemProcInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(buffer.get());
		do {
			processMap.emplace(static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(systemProcInfo->UniqueProcessId)), systemProcInfo->ImageName.Buffer);

			// Advancing to the next entry.
			systemProcInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(reinterpret_cast<PBYTE>(systemProcInfo) + systemProcInfo->NextEntryOffset);

		} while (systemProcInfo->NextEntryOffset != 0);
	}

	void NtUtilities::GetProcessCommandLine(HANDLE hProcess, WWuString& commandLine)
	{
		ULONG bytesNeeded;
		PROCESS_BASIC_INFORMATION basicInfo { };
		NTSTATUS status = NtFunctions::GetInstance()->NtQueryInformationProcess(hProcess, PROCESSINFOCLASS::ProcessBasicInformation, &basicInfo, sizeof(basicInfo), &bytesNeeded);

		wuunique_ptr<BYTE[]> pebBuffer = make_wuunique<BYTE[]>(sizeof(PEB));
		if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(basicInfo.PebBaseAddress), pebBuffer.get(), sizeof(PEB), (SIZE_T*)(&bytesNeeded)))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		PPEB peb = reinterpret_cast<PPEB>(pebBuffer.get());
		wuunique_ptr<BYTE[]> procParamsBuffer = make_wuunique<BYTE[]>(sizeof(RTL_USER_PROCESS_PARAMETERS));
		if (!ReadProcessMemory(hProcess, peb->ProcessParameters, procParamsBuffer.get(), sizeof(RTL_USER_PROCESS_PARAMETERS), (SIZE_T*)(&bytesNeeded)))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		PRTL_USER_PROCESS_PARAMETERS procParams = reinterpret_cast<PRTL_USER_PROCESS_PARAMETERS>(procParamsBuffer.get());
		size_t cmdLineSize = procParams->CommandLine.Length + 1;
		LPWSTR cmdLineBuffer = new WCHAR[cmdLineSize];
		if (!ReadProcessMemory(hProcess, procParams->CommandLine.Buffer, cmdLineBuffer, cmdLineSize * 2, (SIZE_T*)(&bytesNeeded)))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		commandLine = WWuString(reinterpret_cast<LPWSTR>(cmdLineBuffer));

		delete[] cmdLineBuffer;
	}

	void NtUtilities::ListProcessHandleInformation(const ProcessHandle& hProcess, wuvector<WU_OBJECT_HANDLE_INFO>& handleInfo, const bool all, const WuNativeContext* context)
	{
		NTSTATUS status;
		NtFunctions* ntdll = NtFunctions::GetInstance();

		// Querying process handle information.
		ULONG bytesNeeded = 9216;
		wuunique_ptr<BYTE[]> buffer;
		do {
			bytesNeeded += 1024;
			buffer = make_wuunique<BYTE[]>(bytesNeeded);
			status = ntdll->NtQueryInformationProcess(hProcess, PROCESSINFOCLASS::ProcessHandleInformation, buffer.get(), bytesNeeded, &bytesNeeded);
			if (status == STATUS_SUCCESS)
				break;

			if (status != STATUS_INFO_LENGTH_MISMATCH) {
				auto ex = WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };
				ex.Cry(L"ErrorQueryInformationProcess", WriteErrorCategory::InvalidResult, L"", context);

				throw ex;
			}

		} while (status == STATUS_INFO_LENGTH_MISMATCH);

		// Querying information for each handle.
		std::unordered_map<HANDLE, WWuString> processInfoMap;
		HANDLE hCurrentProcess = GetCurrentProcess();
		auto procHandleInfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());
		for (ULONG_PTR i = 0; i < procHandleInfo->NumberOfHandles; i++) {

			// Duplicating the handle.
			ObjectHandle hDup;
			if (!NT_SUCCESS(ntdll->NtDuplicateObject(hProcess, procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, NULL, 0, DUPLICATE_SAME_ACCESS))) {
				WU_OBJECT_HANDLE_INFO currentInfo = {
					procHandleInfo->Handles[i].HandleValue,
					L"UnknownType"
				};

				if (all)
					handleInfo.push_back(currentInfo);

				continue;
			}

			// Querying the handle object type name.
			bytesNeeded = 1024;
			wuunique_ptr<BYTE[]> infoBuffer = make_wuunique<BYTE[]>(bytesNeeded);
			if (!NT_SUCCESS(status = ntdll->NtQueryObject(hDup, OBJECT_INFORMATION_CLASS::ObjectTypeInformation, infoBuffer.get(), bytesNeeded, &bytesNeeded))) {
				auto ex = WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };
				ex.Cry(L"ErrorNtQueryObject", WriteErrorCategory::InvalidResult, L"", context);

				throw ex;
			}

			WWuString objTypeName { reinterpret_cast<POBJECT_TYPE_INFORMATION>(infoBuffer.get())->TypeName.Buffer };
			if (!all && objTypeName != L"File" && objTypeName != L"Key")
				continue;

			WU_OBJECT_HANDLE_INFO currentInfo = {
				procHandleInfo->Handles[i].HandleValue,
				objTypeName
			};

			// Querying the object name.
			// We need to take special precaution with files because 'NtQueryObject' can hang.
			if (currentInfo.Type == L"File") {
				try {
					WU_QUERY_OBJECT_DATA threadData {
						hDup,
						OBJECT_INFORMATION_CLASS::ObjectNameInformation
					};

					QueryObjectWithTimeout(threadData, 100);
					if (threadData.Status != STATUS_SUCCESS)
						continue;

					currentInfo.Name = reinterpret_cast<POBJECT_NAME_INFORMATION>(threadData.ObjectData)->Name.Buffer;
				}
				catch (const WuStdException& ex) {
					ex.Cry(L"ErrorQueryObjectNameWithTimeout", WriteErrorCategory::InvalidResult, L"", context);
					throw ex;
				}
			}
			else if (currentInfo.Type == L"Process") {
				// TODO: Try to do this without duplicating the handle again.
				if (NT_SUCCESS(ntdll->NtDuplicateObject(hProcess, procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, PROCESS_QUERY_LIMITED_INFORMATION, 0, NULL))) {
					// Getting process ID.
					LPCWSTR imgName { };
					HANDLE processId { };
					bytesNeeded = sizeof(PROCESS_BASIC_INFORMATION);
					if (NT_SUCCESS(ntdll->NtQueryInformationProcess(hDup, PROCESSINFOCLASS::ProcessBasicInformation, infoBuffer.get(), bytesNeeded, &bytesNeeded))) {
						processId = reinterpret_cast<PPROCESS_BASIC_INFORMATION>(infoBuffer.get())->UniqueProcessId;

						// Getting the image name.
						bytesNeeded = sizeof(UNICODE_STRING) + 1024;
						if (NT_SUCCESS(ntdll->NtQueryInformationProcess(hDup, PROCESSINFOCLASS::ProcessImageFileName, infoBuffer.get(), bytesNeeded, &bytesNeeded))) {
							LPCWSTR imageNtPath = reinterpret_cast<PUNICODE_STRING>(infoBuffer.get())->Buffer;
							imgName = wcsrchr(imageNtPath, L'\\');
							if (imgName) {
								// Advancing the last '\'
								imgName++;
							}
							else
								imgName = L"<Process already closed>";
						}
						else
							imgName = L"<Process already closed>";
					}
					else
						imgName = L"<Process already closed>";

					processInfoMap.emplace(processId, imgName);
					currentInfo.Name = WWuString::Format(L"%ws(%d)", imgName, processId);
				}
			}
			else if (currentInfo.Type == L"Thread") {
				// TODO: Try to do this without duplicating the handle again.
				if (NT_SUCCESS(ntdll->NtDuplicateObject(hProcess, procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, THREAD_QUERY_LIMITED_INFORMATION, 0, NULL))) {
					bytesNeeded = sizeof(THREAD_BASIC_INFORMATION);
					if (NT_SUCCESS(ntdll->NtQueryInformationThread(hDup, THREADINFOCLASS::ThreadBasicInformation, infoBuffer.get(), bytesNeeded, &bytesNeeded))) {
						auto threadInfo = reinterpret_cast<PTHREAD_BASIC_INFORMATION>(infoBuffer.get());
						HANDLE threadId = threadInfo->ClientId.UniqueThread;
						HANDLE processId = threadInfo->ClientId.UniqueProcess;
						
						// Trying to find the process ID in our map.
						if (auto iterator = processInfoMap.find(processId); iterator != processInfoMap.end()) {
							currentInfo.Name = WWuString::Format(L"%ws(%d): %d", iterator->second.GetBuffer(), processId, threadId);
						}
						else {
							// Opening a handle to the process.
							LPCWSTR imgName { };
							if (HANDLE hThreadProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(processId)))) {
								
								// Getting the image name.
								bytesNeeded = sizeof(UNICODE_STRING) + 1024;
								if (NT_SUCCESS(ntdll->NtQueryInformationProcess(hThreadProcess, PROCESSINFOCLASS::ProcessImageFileName, infoBuffer.get(), bytesNeeded, &bytesNeeded))) {
									LPCWSTR imageNtPath = reinterpret_cast<PUNICODE_STRING>(infoBuffer.get())->Buffer;
									imgName = wcsrchr(imageNtPath, L'\\');
									if (imgName) {
										// Advancing the last '\'
										imgName++;
									}
									else
										imgName = L"<Process already closed>";
								}
								else
									imgName = L"<Process already closed>";
							}
							else
								imgName = L"<Process already closed>";

							processInfoMap.emplace(processId, imgName);
							currentInfo.Name = WWuString::Format(L"%ws(%d): %d", imgName, processId, threadId);
						}
					}
				}
			}
			else {
				bytesNeeded = 1024;
				if (!NT_SUCCESS(status = ntdll->NtQueryObject(hDup, OBJECT_INFORMATION_CLASS::ObjectNameInformation, infoBuffer.get(), bytesNeeded, &bytesNeeded))) {
					auto ex = WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };
					ex.Cry(L"ErrorNtQueryObject", WriteErrorCategory::InvalidResult, L"", context);

					throw ex;
				}

				currentInfo.Name = reinterpret_cast<POBJECT_NAME_INFORMATION>(infoBuffer.get())->Name.Buffer;
			}
			
			handleInfo.push_back(currentInfo);
		}
	}

	void NtUtilities::QueryObjectWithTimeout(WU_QUERY_OBJECT_DATA& objectData, DWORD timeout)
	{
		NTSTATUS status;
		ObjectHandle hThread;
		CLIENT_ID clientId { };

		// Creating thread.
		if (!NT_SUCCESS(status = NtFunctions::GetInstance()->RtlCreateUserThread(
			GetCurrentProcess(),
			NULL,
			FALSE,
			0,
			0,
			0,
			QueryObjectNameThread,
			&objectData,
			&hThread,
			&clientId
		))) {
			throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };
		}

		// Waiting the thread signal or timeout.
		DWORD waitResult = WaitForSingleObject(hThread, timeout);
		if (waitResult == 0xFFFFFFFF) {
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };
		}

		// Timeout or error. We terminate the thread.
		if (waitResult != WAIT_OBJECT_0) {
			TerminateThread(hThread, STATUS_UNSUCCESSFUL);
			objectData.Status = STATUS_UNSUCCESSFUL;
		}
	}

	void NtUtilities::GetThreadProcessInformation(const HANDLE hThread, WU_THREAD_PROCESS_INFORMATION& threadProcInfo)
	{
		NTSTATUS status;
		NtFunctions* ntdll = NtFunctions::GetInstance();
		ULONG bufferSize = sizeof(THREAD_BASIC_INFORMATION);
		wuunique_ptr<THREAD_BASIC_INFORMATION> threadBuffer = make_wuunique<THREAD_BASIC_INFORMATION>();

		// Querying thread basic information.
		if (!NT_SUCCESS(status = ntdll->NtQueryInformationThread(
			hThread,
			THREADINFOCLASS::ThreadBasicInformation,
			reinterpret_cast<PVOID>(threadBuffer.get()),
			bufferSize,
			&bufferSize
		))) {
			throw WuStdException { status, __FILEW__, __LINE__, CoreErrorType::NtError };
		}

		threadProcInfo.ThreadId = static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(threadBuffer->ClientId.UniqueThread));
		threadProcInfo.ProcessId = static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(threadBuffer->ClientId.UniqueProcess));

		// Getting the process image name.
		GetProcessImageName(threadProcInfo.ProcessId, threadProcInfo.ModuleName);
	}

	NTSTATUS WINAPI QueryObjectNameThread(PVOID lpThreadParameter)
	{
		ULONG bytesNeeded = 1024;
		PWU_QUERY_OBJECT_DATA objectData = reinterpret_cast<PWU_QUERY_OBJECT_DATA>(lpThreadParameter);

		objectData->Status = NtFunctions::GetInstance()->NtQueryObject(
			objectData->DuplicateHandle,
			objectData->DataType,
			objectData->ObjectData,
			bytesNeeded,
			&bytesNeeded
		);

		return STATUS_SUCCESS;
	}

	/*
	*	NtFunctions
	* 
	*	This singleton was designed so we can access ntdll functions
	*	minimizing the amount of time we call 'GetProcAddress'.
	*/

	NtFunctions* NtFunctions::m_instance { nullptr };
	ModuleHandle NtFunctions::m_ntdll { ModuleHandle(L"ntdll.dll") };

	NtFunctions::NtFunctions()
		: m_ntQueryKey(nullptr), m_ntQueryObject(nullptr), m_ntResumeProcess(nullptr), m_ntSuspendProcess(nullptr), m_ntDuplicateObject(nullptr),
		m_ntQuerySystemTime(nullptr), m_rtlCreateUserThread(nullptr), m_ntQueryInformationFile(nullptr), m_ntQuerySystemInformation(nullptr), m_ntQueryInformationProcess(nullptr),
		m_ntQueryInformationThread(nullptr)
	{ }

	NtFunctions::~NtFunctions() { }

	NtFunctions* NtFunctions::GetInstance()
	{
		if (m_instance == nullptr)
			m_instance = new NtFunctions();

		return m_instance;
	}

	NTSTATUS NTAPI NtFunctions::NtResumeProcess(_In_ HANDLE ProcessHandle)
	{
		if (m_ntResumeProcess == nullptr)
			m_ntResumeProcess = reinterpret_cast<_NtResumeProcess>(GetProcAddress(m_ntdll, "NtResumeProcess"));

		return m_ntResumeProcess(ProcessHandle);
	}

	NTSTATUS NTAPI NtFunctions::NtSuspendProcess(_In_ HANDLE ProcessHandle)
	{
		if (m_ntSuspendProcess == nullptr)
			m_ntSuspendProcess = reinterpret_cast<_NtSuspendProcess>(GetProcAddress(m_ntdll, "NtSuspendProcess"));

		return m_ntSuspendProcess(ProcessHandle);
	}

	NTSTATUS NTAPI NtFunctions::NtQuerySystemTime(_Out_ PLARGE_INTEGER SystemTime)
	{
		if (m_ntQuerySystemTime == nullptr)
			m_ntQuerySystemTime = reinterpret_cast<_NtQuerySystemTime>(GetProcAddress(m_ntdll, "NtQuerySystemTime"));

		return m_ntQuerySystemTime(SystemTime);
	}

	NTSTATUS NTAPI NtFunctions::NtQueryKey(
		_In_ HANDLE KeyHandle,
		_In_ KEY_INFORMATION_CLASS KeyInformationClass,
		_Out_opt_ PVOID KeyInformation,
		_In_ ULONG Length,
		_Out_ PULONG ResultLength
	)
	{
		if (m_ntQueryKey == nullptr)
			m_ntQueryKey = reinterpret_cast<_NtQueryKey>(GetProcAddress(m_ntdll, "NtQueryKey"));

		return m_ntQueryKey(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
	}

	NTSTATUS NTAPI NtFunctions::NtQueryObject(
		_In_opt_ HANDLE Handle,
		_In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
		_Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
		_In_ ULONG ObjectInformationLength,
		_Out_opt_ PULONG ReturnLength
	)
	{
		if (m_ntQueryObject == nullptr)
			m_ntQueryObject = reinterpret_cast<_NtQueryObject>(GetProcAddress(m_ntdll, "NtQueryObject"));

		return m_ntQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
	}

	NTSTATUS NTAPI NtFunctions::NtDuplicateObject(
		_In_ HANDLE SourceProcessHandle,
		_In_ HANDLE SourceHandle,
		_In_opt_ HANDLE TargetProcessHandle,
		_Out_opt_ PHANDLE TargetHandle,
		_In_ ACCESS_MASK DesiredAccess,
		_In_ ULONG HandleAttributes,
		_In_ ULONG Options
	)
	{
		if (m_ntDuplicateObject == nullptr)
			m_ntDuplicateObject = reinterpret_cast<_NtDuplicateObject>(GetProcAddress(m_ntdll, "NtDuplicateObject"));

		return m_ntDuplicateObject(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
	}

	NTSTATUS NTAPI NtFunctions::RtlCreateUserThread(
		_In_ HANDLE Process,
		_In_opt_ PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
		_In_ BOOLEAN CreateSuspended,
		_In_opt_ ULONG ZeroBits,
		_In_opt_ SIZE_T MaximumStackSize,
		_In_opt_ SIZE_T CommittedStackSize,
		_In_ PUSER_THREAD_START_ROUTINE StartAddress,
		_In_opt_ PVOID Parameter,
		_Out_opt_ PHANDLE Thread,
		_Out_opt_ PCLIENT_ID ClientId
	)
	{
		if (m_rtlCreateUserThread == nullptr)
			m_rtlCreateUserThread = reinterpret_cast<_RtlCreateUserThread>(GetProcAddress(m_ntdll, "RtlCreateUserThread"));

		return m_rtlCreateUserThread(Process, ThreadSecurityDescriptor, CreateSuspended, ZeroBits, MaximumStackSize, CommittedStackSize, StartAddress, Parameter, Thread, ClientId);
	}

	NTSTATUS NTAPI NtFunctions::NtQueryInformationFile(
		_In_ HANDLE FileHandle,
		_Out_ PIO_STATUS_BLOCK IoStatusBlock,
		_Out_writes_bytes_(Length) PVOID FileInformation,
		_In_ ULONG Length,
		_In_ FILE_INFORMATION_CLASS FileInformationClass
	)
	{
		if (m_ntQueryInformationFile == nullptr)
			m_ntQueryInformationFile = reinterpret_cast<_NtQueryInformationFile>(GetProcAddress(m_ntdll, "NtQueryInformationFile"));

		return m_ntQueryInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
	}

	NTSTATUS NTAPI NtFunctions::NtQuerySystemInformation(
		_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
		_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
		_In_ ULONG SystemInformationLength,
		_Out_opt_ PULONG ReturnLength
	)
	{
		if (m_ntQuerySystemInformation == nullptr)
			m_ntQuerySystemInformation = reinterpret_cast<_NtQuerySystemInformation>(GetProcAddress(m_ntdll, "NtQuerySystemInformation"));

		return m_ntQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
	}

	NTSTATUS NTAPI NtFunctions::NtQueryInformationProcess(
		_In_ HANDLE ProcessHandle,
		_In_ PROCESSINFOCLASS ProcessInformationClass,
		_Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
		_In_ ULONG ProcessInformationLength,
		_Out_opt_ PULONG ReturnLength
	)
	{
		if (m_ntQueryInformationProcess == nullptr)
			m_ntQueryInformationProcess = reinterpret_cast<_NtQueryInformationProcess>(GetProcAddress(m_ntdll, "NtQueryInformationProcess"));

		return m_ntQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
	}

	NTSTATUS NTAPI NtFunctions::NtQueryInformationThread(
		_In_ HANDLE ThreadHandle,
		_In_ THREADINFOCLASS ThreadInformationClass,
		_Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
		_In_ ULONG ThreadInformationLength,
		_Out_opt_ PULONG ReturnLength
	)
	{
		if (m_ntQueryInformationThread == nullptr)
			m_ntQueryInformationThread = reinterpret_cast<_NtQueryInformationThread>(GetProcAddress(m_ntdll, "NtQueryInformationThread"));

		return m_ntQueryInformationThread(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
	}
}