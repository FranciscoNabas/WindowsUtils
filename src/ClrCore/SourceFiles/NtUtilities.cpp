#include "..\pch.h"
#include "..\Headers\NtUtilities.h"

namespace WindowsUtils::Core
{
	WuResult GetNtProcessUsingFile(
		const WWuString& fileName,													// File full name.
		wuunique_ha_ptr<FILE_PROCESS_IDS_USING_FILE_INFORMATION>& procUsingFileInfo	// Output with a list of process IDs with handles to the file.
	) {
		NTSTATUS statusResult = STATUS_SUCCESS;
		IO_STATUS_BLOCK ioStatusBlock = { 0 };

		HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hModule || 0 == hModule)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		_NtQueryInformationFile NtQueryInformationFile = (_NtQueryInformationFile)GetProcAddress(hModule, "NtQueryInformationFile");
		if (NULL == NtQueryInformationFile)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

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
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		ULONG bufferSize = 1 << 10;
		std::unique_ptr<BYTE[]> buffer;
		do
		{
			buffer = std::make_unique<BYTE[]>(bufferSize);
			statusResult = NtQueryInformationFile(hFile, &ioStatusBlock, buffer.get(), bufferSize, FileProcessIdsUsingFileInformation);
			if (statusResult == STATUS_SUCCESS)
				break;

			if (statusResult != STATUS_INFO_LENGTH_MISMATCH)
				return WuResult(statusResult, __FILEW__, __LINE__, true);

			bufferSize = (ULONG)ioStatusBlock.Information;

		} while (statusResult == STATUS_INFO_LENGTH_MISMATCH);

		procUsingFileInfo = make_wuunique_ha<FILE_PROCESS_IDS_USING_FILE_INFORMATION>(ioStatusBlock.Information);
		RtlCopyMemory(procUsingFileInfo.get(), reinterpret_cast<PFILE_PROCESS_IDS_USING_FILE_INFORMATION>(buffer.get()), (ULONG)ioStatusBlock.Information);

		CloseHandle(hFile);

		return WuResult();
	}

	DWORD WINAPI NtQueryObjectRaw(LPVOID lpparam)
	{
		NTSTATUS result = STATUS_SUCCESS;
		PTHREAD_FUNC_ARGUMENTS tfuncargs = reinterpret_cast<PTHREAD_FUNC_ARGUMENTS>(lpparam);

		HMODULE hmodule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || 0 == hmodule)
			return result;

		_NtQueryObject NtQueryObject = (_NtQueryObject)GetProcAddress(hmodule, "NtQueryObject");
		if (NULL == NtQueryObject)
			return GetLastError();

		if (tfuncargs->ObjectInformationClass == ObjectNameInformation)
		{
			ULONG szbuffneeded = 0;
			ULONG szbuffer = (ULONG)sizeof(OBJECT_NAME_INFORMATION);

			tfuncargs->ObjectInfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OBJECT_NAME_INFORMATION));
			if (NULL == tfuncargs->ObjectInfo)
				return ERROR_NOT_ENOUGH_MEMORY;

			do
			{
				result = NtQueryObject(tfuncargs->ObjectHandle, ObjectNameInformation, tfuncargs->ObjectInfo, szbuffer, &szbuffneeded);
				if (STATUS_SUCCESS != result
					&& STATUS_INFO_LENGTH_MISMATCH != result
					&& result != STATUS_BUFFER_OVERFLOW
					&& result != STATUS_BUFFER_TOO_SMALL)
					return result;

				if (STATUS_SUCCESS == result)
					break;

				szbuffer = szbuffneeded;
				HeapFree(GetProcessHeap(), NULL, tfuncargs->ObjectInfo);

				tfuncargs->ObjectInfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, szbuffer);
				if (NULL == tfuncargs->ObjectInfo)
					return ERROR_NOT_ENOUGH_MEMORY;

			} while (result == STATUS_BUFFER_TOO_SMALL || result == STATUS_BUFFER_OVERFLOW || result == STATUS_INFO_LENGTH_MISMATCH);
		}
		else
		{
			ULONG szbuffneeded = 0;
			ULONG szbuffer = (ULONG)sizeof(POBJECT_TYPE_INFORMATION);

			tfuncargs->ObjectInfo = (POBJECT_TYPE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POBJECT_TYPE_INFORMATION));
			if (NULL == tfuncargs->ObjectInfo)
				return ERROR_NOT_ENOUGH_MEMORY;

			do
			{
				result = NtQueryObject(tfuncargs->ObjectHandle, ObjectNameInformation, tfuncargs->ObjectInfo, szbuffer, &szbuffneeded);
				if (STATUS_SUCCESS != result
					&& STATUS_INFO_LENGTH_MISMATCH != result
					&& result != STATUS_BUFFER_OVERFLOW
					&& result != STATUS_BUFFER_TOO_SMALL)
					return result;

				if (STATUS_SUCCESS == result)
					break;

				szbuffer = szbuffneeded;
				HeapFree(GetProcessHeap(), NULL, tfuncargs->ObjectInfo);

				tfuncargs->ObjectInfo = (POBJECT_TYPE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, szbuffer);
				if (NULL == tfuncargs->ObjectInfo)
					return ERROR_NOT_ENOUGH_MEMORY;

			} while (result == STATUS_BUFFER_TOO_SMALL || result == STATUS_BUFFER_OVERFLOW || result == STATUS_INFO_LENGTH_MISMATCH);
		}

		return result;
	}

	/*
	* When calling NtQueryObject, the procedure hangs for certain granted accesses and object types.
	* There isn't a pattern, nor documentation on this behavior.
	* We use a separate thread that gets terminated after a timeout.
	*/
	WuResult WINAPI NtQueryObjectWithTimeout(
		HANDLE hobject,								// A valid handle to the object.
		OBJECT_INFORMATION_CLASS objinfoclass,		// One of the OBJECT_INFORMATION_CLASS enumerations.
		PVOID pobjinfo,								// Object containing the queried information. The type of object depends on the object information class.
		ULONG mstimeout								// Maximum timeout in milliseconds.
	) {
		PTHREAD_FUNC_ARGUMENTS threadcallargs = new THREAD_FUNC_ARGUMENTS;
		DWORD dwthreadid = 0;
		DWORD dwthexitcode = STATUS_SUCCESS;
		HANDLE hthread = NULL;

		threadcallargs->ObjectHandle = hobject;
		threadcallargs->ObjectInformationClass = objinfoclass;

		hthread = CreateThread(NULL, 0, NtQueryObjectRaw, threadcallargs, 0, &dwthreadid);
		if (NULL == hthread)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		DWORD wait = WaitForSingleObject(hthread, mstimeout);
		if (wait != WAIT_OBJECT_0)
			TerminateThread(hthread, 0);
		else
		{
			GetExitCodeThread(hthread, &dwthexitcode);
			CloseHandle(hthread);
		}

		size_t szbuffer = 0;
		size_t szpobjinfo = 0;
		if (objinfoclass == ObjectNameInformation)
		{
			szbuffer = sizeof(*reinterpret_cast<POBJECT_NAME_INFORMATION>(threadcallargs->ObjectInfo));
			szpobjinfo = sizeof(*reinterpret_cast<POBJECT_NAME_INFORMATION>(threadcallargs->ObjectInfo));

			if (szbuffer > szpobjinfo)
			{
				HeapFree(GetProcessHeap(), NULL, pobjinfo);
				pobjinfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, szbuffer);
				if (NULL == pobjinfo)
					return WuResult(ERROR_NOT_ENOUGH_MEMORY, __FILEW__, __LINE__);
			}

			CopyMemory(pobjinfo, threadcallargs->ObjectInfo, szbuffer);
		}
		else
		{
			szbuffer = sizeof(*reinterpret_cast<POBJECT_TYPE_INFORMATION>(threadcallargs->ObjectInfo));
			szpobjinfo = sizeof(*reinterpret_cast<POBJECT_TYPE_INFORMATION>(threadcallargs->ObjectInfo));

			if (szbuffer > szpobjinfo)
			{
				HeapFree(GetProcessHeap(), NULL, pobjinfo);
				pobjinfo = (POBJECT_TYPE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, szbuffer);
				if (NULL == pobjinfo)
					return WuResult(ERROR_NOT_ENOUGH_MEMORY, __FILEW__, __LINE__);
			}

			CopyMemory(pobjinfo, threadcallargs->ObjectInfo, szbuffer);
		}

		return WuResult();
	}

	/*
	* Alternative to QueryFullProcessImageNameW.
	* This function returns names from processes like, System, Registry or Secure System.
	*/
	WuResult WINAPI GetProcessImageName(DWORD processId, WWuString& imageName)
	{
		NTSTATUS statusResult = STATUS_SUCCESS;
		ULONG bufferSize = 1 << 12;
		ULONG bytesNeeded = 0;

		HMODULE hModule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hModule || NULL == hModule)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

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
				return WuResult(statusResult, __FILEW__, __LINE__, true);

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

		return WuResult();
	}
}