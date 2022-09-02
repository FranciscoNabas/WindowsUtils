#include "pch.h"
#include "Unmanaged.h"
#include "Utilities.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

#define CHECKDWRESULT(result, err) if (result != ERROR_SUCCESS) { err = GetLastError(); goto CLEANUP; }

using namespace std;

namespace Unmanaged
{
	wchar_t* PrintBuffer(wchar_t const* const format, ...)
	{
		va_list args;
		int len;
		wchar_t* _buff;

		va_start(args, format);
		len = _vscwprintf(format, args) + 1;
		_buff = (wchar_t*)malloc(len * sizeof(wchar_t));
		if (NULL != _buff)
		{
			vswprintf_s(_buff, len, format, args);
		}
		va_end(args);
		return _buff;
	}

	DWORD Utilities::GetProcessFileHandle(vector<Utilities::FileHandleOutput>& ppvecfho, PCWSTR fileName)
	{
		WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1] = { 0 };
		DWORD session;
		UINT nProcInfo = 0;
		UINT nProcInfoNeeded;
		DWORD rebReason;
		DWORD err = 0;
		DWORD res = 0;
		
		
		if (!SUCCEEDED(RmStartSession(&session, 0, szSessionKey)))
		{
			err = GetLastError();
			return err;
		}
		if (!SUCCEEDED(RmRegisterResources(session, 1, &fileName, 0, NULL, 0, NULL)))
		{
			err = GetLastError();
			return err;
		}
		
		/* First call passing '0' to retrieve the number of processes, to size our array.
		If this call returns '0', the file is not in use by any process.*/
		res = RmGetList(session, &nProcInfoNeeded, &nProcInfo, NULL, &rebReason);
		if (res == ERROR_SUCCESS)
			return err;

		if (res != ERROR_MORE_DATA) 
		{
			err = GetLastError();
			return err;
		}

		RM_PROCESS_INFO* pprocInfo = new RM_PROCESS_INFO[nProcInfoNeeded];
		nProcInfo = nProcInfoNeeded;

		// Call to get the RM_PROCESS_INFO objects
		res = RmGetList(session, &nProcInfoNeeded, &nProcInfo, pprocInfo, &rebReason);
		if (res != ERROR_SUCCESS)
		{
			err = GetLastError();
			return err;
		}
		SIZE_T ppInfoSize = sizeof(*pprocInfo) / sizeof(RM_PROCESS_INFO);

		for (SIZE_T i = 0; i < ppInfoSize; i++)
		{
			Utilities::PFileHandleOutput pSingle = new Utilities::FileHandleOutput(pprocInfo[i].ApplicationType, pprocInfo[i].Process.dwProcessId, pprocInfo[i].strAppName);

			HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pprocInfo[i].Process.dwProcessId);
			if (INVALID_HANDLE_VALUE != hProcess)
			{
				FILETIME ftCreate, ftExit, ftKernel, ftUser;
				if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser) && CompareFileTime(&pprocInfo[i].Process.ProcessStartTime, &ftCreate) == 0)
				{
					DWORD cch = MAX_PATH;
					if (!QueryFullProcessImageNameW(hProcess, 0, pSingle->ImagePath, &cch))
						pSingle->ImagePath = L"";
				}
				CloseHandle(hProcess);
			}

			ppvecfho.push_back(*pSingle);

			delete pSingle;
		}
		
		if (nullptr != pprocInfo) { delete[] pprocInfo; }
	
		return err;
	}

	LPWSTR Utilities::GetFormatedWSError()
	{
		LPWSTR inter = NULL;
		FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			WSAGetLastError(),
			0,
			(LPWSTR)&inter,
			0,
			NULL
		);
		return inter;
	}
	LPWSTR Utilities::GetFormatedWin32Error()
	{
		LPWSTR inter = NULL;
		FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			GetLastError(),
			0,
			(LPWSTR)&inter,
			0,
			NULL
		);
		return inter;
	}
	LPWSTR Utilities::GetFormatedError(DWORD errorCode)
	{
		LPWSTR inter = NULL;
		FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			errorCode,
			0,
			(LPWSTR)&inter,
			0,
			NULL
		);
		return inter;
	}

	vector<Utilities::MessageDumpOutput> Utilities::GetResourceMessageTable(LPTSTR libName)
	{
		DWORD err;
		vector<Utilities::MessageDumpOutput> output;
		HMODULE hDll = LoadLibraryEx(libName, NULL, LOAD_LIBRARY_AS_DATAFILE);
		if (hDll != NULL)
		{
			HRSRC resource = FindResource(hDll, MAKEINTRESOURCE(1), RT_MESSAGETABLE);
			if (resource != NULL)
			{
				HGLOBAL load = LoadResource(hDll, resource);
				if (load != NULL)
				{
					PVOID messageTable = LockResource(load);
					DWORD blockNumber = ((PMESSAGE_RESOURCE_DATA)messageTable)->NumberOfBlocks;
					PMESSAGE_RESOURCE_BLOCK messageBlock = ((PMESSAGE_RESOURCE_DATA)messageTable)->Blocks;

					for (DWORD block = 0; block < blockNumber; block++)
					{
						DWORD lowId = messageBlock[block].LowId;
						DWORD highId = messageBlock[block].HighId;
						DWORD offset = 0;

						for (DWORD id = lowId; id <= highId; id++)
						{
							Utilities::PMessageDumpOutput inner;
							inner = (Utilities::PMessageDumpOutput)LocalAlloc(LMEM_ZEROINIT, sizeof(Utilities::MessageDumpOutput));
							PMESSAGE_RESOURCE_ENTRY messageEntry =
								(PMESSAGE_RESOURCE_ENTRY)((PBYTE)messageTable +
									(DWORD)messageBlock[block].OffsetToEntries + offset);

							if (inner)
							{
								wchar_t* buffer = PrintBuffer(L"%08X", id);
								inner->Id = wstring(buffer);
								free(buffer);

								buffer = PrintBuffer(L"%s", messageEntry->Text);
								inner->Message = wstring(buffer);
								free(buffer);

								output.push_back(*inner);
								LocalFree(inner);
								offset += messageEntry->Length;
							}
						}
					}
				}
			}
			FreeLibrary(hDll);
		}
		else { err = GetLastError(); }

		return output;
	}

	DWORD Utilities::MapRpcEndpoints(
		vector<Utilities::RpcMapperOutput> &ppOutVec
	)
	{
		RPC_EP_INQ_HANDLE inqContext;
		RPC_STATUS result = 0;
		RPC_BINDING_HANDLE bindingHandle;
		RPC_WSTR annotation;
		RPC_WSTR stringBinding;

		result = RpcMgmtEpEltInqBegin(NULL, RPC_C_EP_ALL_ELTS, 0, 0, 0, &inqContext);
		if (result != RPC_S_OK) { return result; }

		do
		{
			result = RpcMgmtEpEltInqNextW(inqContext, NULL, &bindingHandle, NULL, &annotation);
			if (result != 0 || result == RPC_X_NO_MORE_ENTRIES) { break; }

			result = RpcBindingToStringBindingW(bindingHandle, &stringBinding);
			if (result != 0) { break; }

			ppOutVec.push_back(Utilities::RpcMapperOutput((LPWSTR)stringBinding, (LPWSTR)annotation));

			RpcStringFree(&annotation);
			RpcStringFree(&stringBinding);
			RpcBindingFree(&bindingHandle);

		} while (result != RPC_X_NO_MORE_ENTRIES);

		result = RpcMgmtEpEltInqDone(&inqContext);
		return result;
	}
}