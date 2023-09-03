#include "..\pch.h"
#include "..\Headers\NtUtilities.h"

namespace WindowsUtils::Core
{
	void GetProcessUsingFile(const WWuString& fileName, wuvector<DWORD>& processIdList)
	{
		NTSTATUS statusResult = STATUS_SUCCESS;
		IO_STATUS_BLOCK ioStatusBlock = { 0 };

		HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hModule || 0 == hModule)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		_NtQueryInformationFile NtQueryInformationFile = (_NtQueryInformationFile)GetProcAddress(hModule, "NtQueryInformationFile");
		if (NULL == NtQueryInformationFile)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		HANDLE hFile = CreateFileW(
			fileName.GetBuffer(),
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
		do
		{
			buffer = std::make_unique<BYTE[]>(bufferSize);
			statusResult = NtQueryInformationFile(hFile, &ioStatusBlock, buffer.get(), bufferSize, FileProcessIdsUsingFileInformation);
			if (statusResult == STATUS_SUCCESS)
				break;

			if (statusResult != STATUS_INFO_LENGTH_MISMATCH)
				throw WuStdException(statusResult, __FILEW__, __LINE__, NtError);

			bufferSize = (ULONG)ioStatusBlock.Information;

		} while (statusResult == STATUS_INFO_LENGTH_MISMATCH);

		auto info = reinterpret_cast<PFILE_PROCESS_IDS_USING_FILE_INFORMATION>(buffer.get());
		for (ULONG i = 0; i < info->NumberOfProcessIdsInList; i++)
			processIdList.push_back(static_cast<DWORD>(info->ProcessIdList[i]));
		
		CloseHandle(hFile);
	}

	void GetProcessUsingObject(
		const WWuString& objectName,		// Object name here needs to be the same as the object name from 'NtQueryObject'.
		wuvector<DWORD>& processIdList,		// The list of process IDs that have a handle opened to the object.
		bool closeHandle
	)
	{
		NTSTATUS status = STATUS_SUCCESS;
		ULONG bytesNeeded;

		wuvector<DWORD> procIdList;
		GetRunnningProcessIdList(procIdList);

		HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
		if (hNtdll == NULL) {
			hNtdll = LoadLibrary(L"ntdll.dll");
			if (hNtdll == NULL)
				throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		_NtQueryObject NtQueryObject = (_NtQueryObject)GetProcAddress(hNtdll, "NtQueryObject");
		_NtQueryInformationProcess NtQueryInformationProcess = (_NtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");

		for (DWORD pid : procIdList) {
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, FALSE, pid);
			if (hProcess == NULL)
				continue;

			// Getting the initial size needed.
			NtQueryInformationProcess(hProcess, ProcessHandleInformation, NULL, 0, &bytesNeeded);
			
			wuunique_ptr<BYTE[]> buffer;
			bool succeeded = true;
			do {
				// By the second call time more handles might be opened.
				bytesNeeded += 64;
				buffer = make_wuunique<BYTE[]>(bytesNeeded);

				status = NtQueryInformationProcess(hProcess, ProcessHandleInformation, buffer.get(), bytesNeeded, &bytesNeeded);
				if (status != STATUS_SUCCESS && status != STATUS_INFO_LENGTH_MISMATCH) {
					succeeded = false;
					break;
				}

				if (status == STATUS_SUCCESS)
					break;

			} while (status == STATUS_INFO_LENGTH_MISMATCH);

			if (!succeeded)
				continue;

			auto procSnapshotInfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());

			// Iterating through each handle.
			for (size_t i = 0; i < procSnapshotInfo->NumberOfHandles; i++) {
				HANDLE hTarget;
				if (!DuplicateHandle(hProcess, procSnapshotInfo->Handles[i].HandleValue, GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
					continue;

				wuunique_ptr<BYTE[]> nameBuffer;
				NtQueryObjectWithTimeout(hTarget, ObjectNameInformation, nameBuffer, 200);
				CloseHandle(hTarget);

				auto nameInfo = reinterpret_cast<POBJECT_NAME_INFORMATION>(nameBuffer.get());
				WWuString test(nameInfo->Name.Buffer);

				if (nameInfo->Name.Buffer != NULL)
					if (_wcsnicmp(objectName.GetBuffer(), nameInfo->Name.Buffer, objectName.Length())) {
						if (closeHandle) {
							if (!DuplicateHandle(hProcess, procSnapshotInfo->Handles[i].HandleValue, GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
								throw WuStdException(GetLastError(), __FILEW__, __LINE__);

							CloseHandle(hTarget);
						}
						
						processIdList.push_back(pid);
						break;
					}
			}
		}
	}

	void GetRunnningProcessIdList(wuvector<DWORD>& procIdList)
	{
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		PROCESSENTRY32 procEntry = { 0 };
		procEntry.dwSize = sizeof(procEntry);

		// Skip the idle process.
		Process32First(hSnapshot, &procEntry);
		while (Process32Next(hSnapshot, &procEntry))
			procIdList.push_back(procEntry.th32ProcessID);

		CloseHandle(hSnapshot);
	}

	DWORD WINAPI NtQueryObjectRaw(LPVOID parameters)
	{
		NTSTATUS result = STATUS_SUCCESS;
		PTHREAD_FUNC_ARGUMENTS threadArgs = reinterpret_cast<PTHREAD_FUNC_ARGUMENTS>(parameters);

		HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
		if (hNtdll == NULL) {
			hNtdll = LoadLibrary(L"ntdll.dll");
			if (hNtdll == NULL)
				throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		_NtQueryObject NtQueryObject = (_NtQueryObject)GetProcAddress(hNtdll, "NtQueryObject");

		if (threadArgs->ObjectInformationClass == ObjectNameInformation)
		{
			ULONG bufferSize = static_cast<ULONG>(sizeof(OBJECT_NAME_INFORMATION));
			do
			{
				threadArgs->ObjectInfo = make_wuunique<BYTE[]>(bufferSize);

				result = NtQueryObject(threadArgs->ObjectHandle, ObjectNameInformation, threadArgs->ObjectInfo.get(), bufferSize, &bufferSize);
				if (STATUS_SUCCESS != result
					&& STATUS_INFO_LENGTH_MISMATCH != result
					&& result != STATUS_BUFFER_OVERFLOW
					&& result != STATUS_BUFFER_TOO_SMALL)
					return result;

				if (STATUS_SUCCESS == result)
					break;

			} while (result == STATUS_BUFFER_TOO_SMALL || result == STATUS_BUFFER_OVERFLOW || result == STATUS_INFO_LENGTH_MISMATCH);
		}
		else
		{
			ULONG bufferSize = static_cast<ULONG>(sizeof(OBJECT_TYPE_INFORMATION));
			do
			{
				threadArgs->ObjectInfo = make_wuunique<BYTE[]>(bufferSize);

				result = NtQueryObject(threadArgs->ObjectHandle, ObjectTypeInformation, threadArgs->ObjectInfo.get(), bufferSize, &bufferSize);
				if (STATUS_SUCCESS != result
					&& STATUS_INFO_LENGTH_MISMATCH != result
					&& result != STATUS_BUFFER_OVERFLOW
					&& result != STATUS_BUFFER_TOO_SMALL)
					return result;

				if (STATUS_SUCCESS == result)
					break;

			} while (result == STATUS_BUFFER_TOO_SMALL || result == STATUS_BUFFER_OVERFLOW || result == STATUS_INFO_LENGTH_MISMATCH);
		}

		return result;
	}

	/*
	* When calling NtQueryObject, the procedure hangs for certain granted accesses and object types.
	* There isn't a pattern, nor documentation on this behavior.
	* We use a separate thread that gets terminated after a timeout.
	*/
	void WINAPI NtQueryObjectWithTimeout(
		HANDLE hObject,								// A valid handle to the object.
		OBJECT_INFORMATION_CLASS objInfoClass,		// One of the OBJECT_INFORMATION_CLASS enumerations.
		wuunique_ptr<BYTE[]>& objInfo,				// Object containing the queried information. The type of object depends on the object information class.
		ULONG timeout								// Maximum timeout in milliseconds.
	) {
		DWORD threadId = 0;
		DWORD threadExitCode = STATUS_SUCCESS;
		HANDLE hThread = NULL;

		THREAD_FUNC_ARGUMENTS threadCallArgs = {
			hObject,
			objInfoClass,
			objInfo
		};

		hThread = CreateThread(NULL, 0, NtQueryObjectRaw, &threadCallArgs, 0, &threadId);
		if (NULL == hThread)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		DWORD wait = WaitForSingleObject(hThread, timeout);
		if (wait != WAIT_OBJECT_0)
			TerminateThread(hThread, 0);
		else
		{
			GetExitCodeThread(hThread, &threadExitCode);
			CloseHandle(hThread);
		}
	}

	/*
	* Alternative to QueryFullProcessImageNameW.
	* This function returns names from processes like, System, Registry or Secure System.
	*/
	void WINAPI GetProcessImageName(DWORD processId, WWuString& imageName)
	{
		NTSTATUS statusResult = STATUS_SUCCESS;
		ULONG bufferSize = 1 << 12;
		ULONG bytesNeeded = 0;

		HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hModule || NULL == hModule)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		_NtQuerySystemInformation NtQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(hModule, "NtQuerySystemInformation");

		std::unique_ptr<BYTE[]> buffer;
		do
		{
			buffer = std::make_unique<BYTE[]>(bufferSize);
			statusResult = NtQuerySystemInformation(SystemFullProcessInformation, buffer.get(), bufferSize, &bytesNeeded);
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
		do
		{
			if (systemProcInfo->UniqueProcessId == reinterpret_cast<HANDLE>((UINT_PTR)processId))
			{
				size_t imgNameSize = wcslen(systemProcInfo->ImageName.Buffer) + 1;
				if (imgNameSize > 1)
					imageName = systemProcInfo->ImageName.Buffer;
				
				break;
			}
			systemProcInfo = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)systemProcInfo + systemProcInfo->NextEntryOffset);

		} while (systemProcInfo->NextEntryOffset != 0);
	}
}