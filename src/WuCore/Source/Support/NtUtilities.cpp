#include "../../pch.h"

#include "../../Headers/Support/Nt/NtUtilities.h"

namespace WindowsUtils::Core
{
#pragma region WU_OBJECT_HANDLE_INFO

	_WU_OBJECT_HANDLE_INFO::_WU_OBJECT_HANDLE_INFO(const _WU_OBJECT_HANDLE_INFO& other)
		: HandleValue{ other.HandleValue }, Type{ other.Type }, Name{ other.Name } { }
	
	_WU_OBJECT_HANDLE_INFO::_WU_OBJECT_HANDLE_INFO(const HANDLE handle, const WWuString& type, const WWuString& name)
		: HandleValue{ handle }, Type{ type }, Name{ name } { }

	_WU_OBJECT_HANDLE_INFO::~_WU_OBJECT_HANDLE_INFO() { }

#pragma endregion

#pragma region WU_QUERY_OBJECT_DATA

	_WU_QUERY_OBJECT_DATA::_WU_QUERY_OBJECT_DATA(HANDLE hDup, OBJECT_INFORMATION_CLASS dataType)
	{
		DataType = dataType;
		DuplicateHandle = hDup;
		Status = STATUS_SUCCESS;

		ObjectData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024);
		if (!ObjectData)
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_NOT_ENOUGH_MEMORY, L"HeapAlloc", WriteErrorCategory::QuotaExceeded);
	}

	_WU_QUERY_OBJECT_DATA::~_WU_QUERY_OBJECT_DATA()
	{
		if (ObjectData)
			HeapFree(GetProcessHeap(), 0, ObjectData);
	}

#pragma endregion

#pragma region NtUtilities

	HANDLE NtUtilities::s_currentProcess = GetCurrentProcess();
	WWuString NtUtilities::s_objTypeRegistryKey = L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows";
	WWuString NtUtilities::s_objTypeFile = NtUtilities::GetEnvVariable(L"SystemDrive") + L"\\Windows\\System32\\kernel32.dll";

	std::unordered_map<HANDLE, DWORD> NtUtilities::GetProcessUsingObject(const WWuString& objectName, const SupportedHandleType type, const bool closeHandle)
	{
		NTSTATUS status;
		UCHAR typeIndex = GetObjectTypeIndex(type);

		// Enough for a little more than 200k handles.
		ULONG handleInfoBufferSize = 1 << 23;
		ULONG objectInfoBufferSize = 1 << 10;
		ScopedBuffer handleInfoBuffer{ handleInfoBufferSize };
		ScopedBuffer objectInfoBuffer{ objectInfoBufferSize };
		do {
			if ((status = NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS::SystemExtendedHandleInformation, handleInfoBuffer.Get(), handleInfoBufferSize, &handleInfoBufferSize)) < 0 &&
				status != STATUS_INFO_LENGTH_MISMATCH)
				_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QuerySystemInformation", WriteErrorCategory::InvalidResult);

			if (!status)
				break;

			handleInfoBufferSize += 1 << 10;
			handleInfoBuffer.Resize(handleInfoBufferSize);

		} while (status == STATUS_INFO_LENGTH_MISMATCH);

		std::unordered_map<HANDLE, DWORD> output(10);
		auto handleInfo = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>(handleInfoBuffer.Get());
		ULONG_PTR numberOfHandles = handleInfo->NumberOfHandles;
		PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handles = handleInfo->Handles;
		do {
			if (handles->ObjectTypeIndex == typeIndex) {
				DWORD currentProcessId = static_cast<DWORD>(handles->UniqueProcessId);
				ProcessHandle process{ currentProcessId, PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, false };
				if (!process.Get())
					continue;

				SafeObjectHandle duplicateHandle;
				if (!NT_SUCCESS(NtDuplicateObject(process.Get(), (HANDLE)handles->HandleValue, s_currentProcess, &duplicateHandle, NULL, 0, DUPLICATE_SAME_ACCESS)))
					continue;

				// Checking if the file object is an on disk file.
				// NtQueryObject hangs indefinitely with some asynchronous file handles like pipes.
				if (type == SupportedHandleType::FileSystem && GetFileType(duplicateHandle.Get()) != FILE_TYPE_DISK)
					continue;

				objectInfoBufferSize = 1 << 10;
				if (!NT_SUCCESS(NtQueryObject(duplicateHandle.Get(), OBJECT_INFORMATION_CLASS::ObjectNameInformation, objectInfoBuffer.Get(), objectInfoBufferSize, &objectInfoBufferSize)))
					continue;

				auto nameInfo = reinterpret_cast<POBJECT_NAME_INFORMATION>(objectInfoBuffer.Get());
				if (nameInfo->Name.Buffer && _wcsnicmp(objectName.Raw(), nameInfo->Name.Buffer, objectName.Length()) == 0) {
					if (closeHandle) {
						if (!NT_SUCCESS(status = NtDuplicateObject(process.Get(), (HANDLE)handles->HandleValue, s_currentProcess, &duplicateHandle, NULL, 0, DUPLICATE_CLOSE_SOURCE)))
							_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"DuplicateObject", WriteErrorCategory::InvalidResult);
					}

					output.emplace(reinterpret_cast<HANDLE>(handles->HandleValue), currentProcessId);
				}
			}

		} while (handles++, numberOfHandles--);

		return output;
	}

	WuList<DWORD> NtUtilities::ListRunningProcesses()
	{
		NTSTATUS status;
		ULONG bytesNeeded = 1 << 14;
		ScopedBuffer buffer{ bytesNeeded };
		do {
			if (!NT_SUCCESS(status = NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS::SystemProcessInformation, buffer.Get(), bytesNeeded, &bytesNeeded))) {
				if (status != STATUS_INFO_LENGTH_MISMATCH && status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
					_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QuerySystemInformation", WriteErrorCategory::InvalidResult);
			}
			else
				break;

			// More processes might have been created until the next call.
			bytesNeeded += 1 << 10;
			buffer.Resize(bytesNeeded);

		} while (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH);

		WuList<DWORD> output(200);
		auto systemProcInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(buffer.Get());
		do {
			output.Add(static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(systemProcInfo->UniqueProcessId)));

			// Advancing to the next entry.
			systemProcInfo = GetNextProcess(systemProcInfo);

		} while (systemProcInfo);

		return output;
	}

	// Requires administrator! (SystemFullProcessInformation).
	std::unordered_map<DWORD, WWuString> NtUtilities::ListRunningProcessIdAndNames()
	{
		NTSTATUS status;
		ULONG bytesNeeded = 1 << 14;
		ScopedBuffer buffer{ bytesNeeded };
		std::unordered_map<DWORD, WWuString> output;
		do {
			if (!NT_SUCCESS(status = NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS::SystemFullProcessInformation, buffer.Get(), bytesNeeded, &bytesNeeded))) {
				if (status != STATUS_INFO_LENGTH_MISMATCH && status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
					_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QuerySystemInformation", WriteErrorCategory::InvalidResult);
			}
			else
				break;

			// More processes might have been created until the next call.
			bytesNeeded += 1 << 10;
			buffer.Resize(bytesNeeded);

		} while (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH);

		auto systemProcInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(buffer.Get());
		do {
			output.emplace(static_cast<DWORD>((ULONG_PTR)systemProcInfo->UniqueProcessId), systemProcInfo->ImageName.Buffer);

			// Advancing to the next entry.
			systemProcInfo = GetNextProcess(systemProcInfo);

		} while (systemProcInfo);

		return output;
	}

	void NtUtilities::ListRunningProcesses(std::unordered_map<DWORD, WWuString>& processMap)
	{
		NTSTATUS status;
		ULONG bytesNeeded = 1 << 14;
		ScopedBuffer buffer{ bytesNeeded };
		do {
			if (!NT_SUCCESS(status = NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS::SystemFullProcessInformation, buffer.Get(), bytesNeeded, &bytesNeeded))) {
				if (status != STATUS_INFO_LENGTH_MISMATCH && status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
					_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QuerySystemInformation", WriteErrorCategory::InvalidResult);
			}
			else
				break;

			// More processes might have been created until the next call.
			bytesNeeded += 1 << 10;
			buffer.Resize(bytesNeeded);

		} while (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH);

		auto systemProcInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(buffer.Get());
		do {
			processMap.emplace(static_cast<DWORD>((ULONG_PTR)systemProcInfo->UniqueProcessId), systemProcInfo->ImageName.Buffer);

			// Advancing to the next entry.
			systemProcInfo = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)systemProcInfo + systemProcInfo->NextEntryOffset);

		} while (systemProcInfo->NextEntryOffset != 0);
	}

	void NtUtilities::GetProcessCommandLine(const ProcessHandle& hProcess, WWuString& commandLine)
	{
		ULONG bytesNeeded;
		PROCESS_BASIC_INFORMATION basicInfo { };
		NTSTATUS status = NtQueryInformationProcess(hProcess.Get(), PROCESSINFOCLASS::ProcessBasicInformation, &basicInfo, sizeof(basicInfo), &bytesNeeded);

		PEB peb{ };
		if (!ReadProcessMemory(hProcess.Get(), reinterpret_cast<LPCVOID>(basicInfo.PebBaseAddress), &peb, sizeof(PEB), (SIZE_T*)(&bytesNeeded)))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"ReadProcessMemory", WriteErrorCategory::ReadError);

		RTL_USER_PROCESS_PARAMETERS processParameters{ };
		if (!ReadProcessMemory(hProcess.Get(), peb.ProcessParameters, &processParameters, sizeof(RTL_USER_PROCESS_PARAMETERS), (SIZE_T*)(&bytesNeeded)))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"ReadProcessMemory", WriteErrorCategory::ReadError);

		USHORT cmdLineChars = processParameters.CommandLine.Length + 1;
		LPWSTR cmdLineBuffer = new WCHAR[cmdLineChars];
		USHORT cmdLineBytes = cmdLineChars * 2;
		if (!ReadProcessMemory(hProcess.Get(), processParameters.CommandLine.Buffer, cmdLineBuffer, cmdLineBytes, (SIZE_T*)(&bytesNeeded)))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"ReadProcessMemory", WriteErrorCategory::ReadError);

		commandLine = WWuString(reinterpret_cast<LPWSTR>(cmdLineBuffer));

		delete[] cmdLineBuffer;
	}

	void NtUtilities::ListProcessHandleInformation(const ProcessHandle& hProcess, const bool all, WuList<WU_OBJECT_HANDLE_INFO>& output, const WuNativeContext* context)
	{
		NTSTATUS status;

		// Querying process handle information.
		ULONG procInfoBufferSize = 1 << 6;
		ULONG handleInfoBufferSize = 1 << 14;
		ULONG objectInfoBufferSize = 1 << 10;
		ScopedBuffer procInfoBuffer{ procInfoBufferSize };
		ScopedBuffer handleInfoBuffer{ handleInfoBufferSize };
		ScopedBuffer objectInfoBuffer{ objectInfoBufferSize };
		do {
			handleInfoBufferSize += 1024;
			status = NtQueryInformationProcess(hProcess.Get(), PROCESSINFOCLASS::ProcessHandleInformation, handleInfoBuffer.Get(), handleInfoBufferSize, &handleInfoBufferSize);
			if (status == STATUS_SUCCESS)
				break;

			if (status != STATUS_INFO_LENGTH_MISMATCH) {
				_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QueryInformationProcess", WriteErrorCategory::InvalidResult);
			}

			handleInfoBuffer.Resize(handleInfoBufferSize);

		} while (status == STATUS_INFO_LENGTH_MISMATCH);

		// Querying information for each handle.
		HANDLE hCurrentProcess = GetCurrentProcess();
		std::unordered_map<HANDLE, WWuString> processInfoMap;
		auto procHandleInfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(handleInfoBuffer.Get());
		for (ULONG_PTR i = 0; i < procHandleInfo->NumberOfHandles; i++) {

			// Duplicating the handle.
			SafeObjectHandle hDup;
			if (!NT_SUCCESS(NtDuplicateObject(hProcess.Get(), procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, NULL, 0, DUPLICATE_SAME_ACCESS))) {
				if (all)
					output.Add(procHandleInfo->Handles[i].HandleValue, L"UnknownType", L"");

				continue;
			}

			// Querying the handle object type name.
			objectInfoBufferSize = 1 << 10;
			if (!NT_SUCCESS(status = NtQueryObject(hDup.Get(), OBJECT_INFORMATION_CLASS::ObjectTypeInformation, objectInfoBuffer.Get(), objectInfoBufferSize, &objectInfoBufferSize))) {
				_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QueryObject", WriteErrorCategory::InvalidResult);
			}

			WWuString objTypeName { reinterpret_cast<POBJECT_TYPE_INFORMATION>(objectInfoBuffer.Get())->TypeName.Buffer };
			if (!all && objTypeName != L"File" && objTypeName != L"Key")
				continue;

			WU_OBJECT_HANDLE_INFO currentInfo{
				procHandleInfo->Handles[i].HandleValue,
				objTypeName,
				L""
			};

			// Querying the object name.
			// We need to take special precaution with files because 'NtQueryObject' can hang.
			if (currentInfo.Type == L"File") {
				WU_QUERY_OBJECT_DATA threadData{
						hDup.Get(),
						OBJECT_INFORMATION_CLASS::ObjectNameInformation
				};

				QueryObjectWithTimeout(threadData, 100);
				if (threadData.Status != STATUS_SUCCESS)
					continue;

				currentInfo.Name = reinterpret_cast<POBJECT_NAME_INFORMATION>(threadData.ObjectData)->Name.Buffer;
			}
			else if (currentInfo.Type == L"Process") {
				// TODO: Try to do this without duplicating the handle again.
				if (NT_SUCCESS(NtDuplicateObject(hProcess.Get(), procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, PROCESS_QUERY_LIMITED_INFORMATION, 0, NULL))) {
					// Getting process ID.
					LPCWSTR imgName { };
					HANDLE processId { };
					procInfoBufferSize = 1 << 6;
					if (NT_SUCCESS(NtQueryInformationProcess(hDup.Get(), PROCESSINFOCLASS::ProcessBasicInformation, procInfoBuffer.Get(), procInfoBufferSize, &procInfoBufferSize))) {
						processId = reinterpret_cast<PPROCESS_BASIC_INFORMATION>(procInfoBuffer.Get())->UniqueProcessId;

						// Getting the image name.
						procInfoBufferSize = 1 << 6;
						if (NT_SUCCESS(NtQueryInformationProcess(hDup.Get(), PROCESSINFOCLASS::ProcessImageFileName, procInfoBuffer.Get(), procInfoBufferSize, &procInfoBufferSize))) {
							LPCWSTR imageNtPath = reinterpret_cast<PUNICODE_STRING>(procInfoBuffer.Get())->Buffer;
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
				if (NT_SUCCESS(NtDuplicateObject(hProcess.Get(), procHandleInfo->Handles[i].HandleValue, hCurrentProcess, &hDup, THREAD_QUERY_LIMITED_INFORMATION, 0, NULL))) {
					procInfoBufferSize = 1 << 6;
					if (NT_SUCCESS(NtQueryInformationThread(hDup.Get(), THREADINFOCLASS::ThreadBasicInformation, procInfoBuffer.Get(), procInfoBufferSize, &procInfoBufferSize))) {
						auto threadInfo = reinterpret_cast<PTHREAD_BASIC_INFORMATION>(procInfoBuffer.Get());
						HANDLE threadId = threadInfo->ClientId.UniqueThread;
						HANDLE processId = threadInfo->ClientId.UniqueProcess;
						
						// Trying to find the process ID in our map.
						if (auto iterator = processInfoMap.find(processId); iterator != processInfoMap.end()) {
							currentInfo.Name = WWuString::Format(L"%ws(%d): %d", iterator->second.Raw(), processId, threadId);
						}
						else {
							// Opening a handle to the process.
							LPCWSTR imgName { };
							ProcessHandle threadProcess{ static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(processId)), PROCESS_QUERY_LIMITED_INFORMATION, false };
							if (threadProcess.Get()) {
								
								// Getting the image name.
								procInfoBufferSize = 1 << 6;
								if (NT_SUCCESS(NtQueryInformationProcess(threadProcess.Get(), PROCESSINFOCLASS::ProcessImageFileName, procInfoBuffer.Get(), procInfoBufferSize, &procInfoBufferSize))) {
									LPCWSTR imageNtPath = reinterpret_cast<PUNICODE_STRING>(procInfoBuffer.Get())->Buffer;
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
				procInfoBufferSize = 1 << 6;
				if (!NT_SUCCESS(status = NtQueryObject(hDup.Get(), OBJECT_INFORMATION_CLASS::ObjectNameInformation, procInfoBuffer.Get(), procInfoBufferSize, &procInfoBufferSize))) {
					_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QueryObject", WriteErrorCategory::InvalidResult);
				}

				currentInfo.Name = reinterpret_cast<POBJECT_NAME_INFORMATION>(procInfoBuffer.Get())->Name.Buffer;
			}
			
			output.Add(currentInfo);
		}
	}

	void NtUtilities::QueryObjectWithTimeout(WU_QUERY_OBJECT_DATA& objectData, DWORD timeout)
	{
		NTSTATUS status;
		SafeObjectHandle hThread;
		CLIENT_ID clientId { };

		// Creating thread.
		if (!NT_SUCCESS(status = RtlCreateUserThread(
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
			_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"CreateUserThread", WriteErrorCategory::InvalidResult);
		}

		// Waiting the thread signal or timeout.
		DWORD waitResult = WaitForSingleObject(hThread.Get(), timeout);
		if (waitResult == 0xFFFFFFFF) {
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"WaitForSingleObject", WriteErrorCategory::InvalidResult);
		}

		// Timeout or error. We terminate the thread.
		if (waitResult != WAIT_OBJECT_0) {
			TerminateThread(hThread.Get(), STATUS_UNSUCCESSFUL);
			objectData.Status = STATUS_UNSUCCESSFUL;
		}
	}

	UCHAR NtUtilities::GetObjectTypeIndex(const SupportedHandleType type)
	{
		UCHAR typeIndex;

		switch (type) {
		case SupportedHandleType::FileSystem:
		{
			FileHandle hFile(s_objTypeFile.Raw(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
			
			ULONG bufferSize = 1 << 10;
			ScopedBuffer buffer(bufferSize);
			NTSTATUS status = NtQueryObject(hFile.Get(), OBJECT_INFORMATION_CLASS::ObjectTypeInformation, buffer.Get(), bufferSize, &bufferSize);
			if (status)
				_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QueryObject", WriteErrorCategory::InvalidResult);

			typeIndex = reinterpret_cast<POBJECT_TYPE_INFORMATION>(buffer.Get())->TypeIndex;

		} break;

		case SupportedHandleType::Registry:
		{
			UNICODE_STRING keyName{
				s_objectTypeRegistryKeyLength,
				s_objectTypeRegistryKeyMaxLength,
				s_objTypeRegistryKey.Raw()
			};

			SafeObjectHandle keyHandle;
			OBJECT_ATTRIBUTES objAttributes{ };
			objAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
			objAttributes.ObjectName = &keyName;
			NTSTATUS status = NtOpenKey(&keyHandle, KEY_QUERY_VALUE, &objAttributes);
			if (status)
				_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"OpenKey", WriteErrorCategory::OpenError);

			ULONG bufferSize = 1 << 10;
			ScopedBuffer buffer{ bufferSize };
			status = NtQueryObject(keyHandle.Get(), OBJECT_INFORMATION_CLASS::ObjectTypeInformation, buffer.Get(), bufferSize, &bufferSize);
			if (status)
				_WU_RAISE_NATIVE_NT_EXCEPTION(status, L"QueryObject", WriteErrorCategory::InvalidResult);

			typeIndex = reinterpret_cast<POBJECT_TYPE_INFORMATION>(buffer.Get())->TypeIndex;

		} break;

		default:
			_WU_RAISE_COR_EXCEPTION(COR_E_ARGUMENT, L"GetObjectTypeIndex", WriteErrorCategory::InvalidArgument);
		}

		return typeIndex;
	}

	WWuString NtUtilities::GetEnvVariable(const WWuString& name)
	{
		size_t bufferSize = 0;

		_wgetenv_s(&bufferSize, NULL, 0, name.Raw());
		if (bufferSize == 0)
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_FILE_NOT_FOUND, L"_wgetenv_s", WriteErrorCategory::InvalidResult);

		std::unique_ptr<WCHAR[]> buffer = std::make_unique<WCHAR[]>(bufferSize);
		_wgetenv_s(&bufferSize, buffer.get(), bufferSize, name.Raw());

		return WWuString(buffer.get());
	}

	NTSTATUS WINAPI QueryObjectNameThread(PVOID lpThreadParameter)
	{
		ULONG bytesNeeded = 1024;
		PWU_QUERY_OBJECT_DATA objectData = reinterpret_cast<PWU_QUERY_OBJECT_DATA>(lpThreadParameter);

		objectData->Status = NtQueryObject(
			objectData->DuplicateHandle,
			objectData->DataType,
			objectData->ObjectData,
			bytesNeeded,
			&bytesNeeded
		);

		return STATUS_SUCCESS;
	}

#pragma endregion
}