#include "pch.h"
#include "Unmanaged.h"
#include "Utilities.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

#define CHECKDWRESULT(result, err) if (result != ERROR_SUCCESS) { err = GetLastError(); goto CLEANUP; }
#define IFFAILRETURNDW(result) if (ERROR_SUCCESS != result) { return result; }

using namespace std;

namespace Unmanaged
{
	void PrintBuffer(LPWSTR& pwref, wchar_t const* const format, ...)
	{
		va_list args;
		int len;
		va_start(args, format);
		
		len = _vscwprintf(format, args) + 1;
		pwref = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * len));
		vswprintf_s(pwref, len, format, args);
		
		va_end(args);
	}

	DWORD Utilities::GetMsiExtendedErrorMessage(LPWSTR& pErrorMessage)
	{
		UINT uiStatus = 0;
		DWORD extErrBuffSize = 0;

		// The MSI engine disposes PMSIHANDLE objects as they go out of scope.
		// With MSIHANDLE, you have to call MsiCloseHandle.
		PMSIHANDLE hLastErr = MsiGetLastErrorRecord();
		
		if (hLastErr)
		{
			uiStatus = MsiFormatRecord(NULL, hLastErr, L"", &extErrBuffSize);
			IFFAILRETURNDW(uiStatus);

			extErrBuffSize++;
			pErrorMessage = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * extErrBuffSize));
			if (nullptr == pErrorMessage)
				return ERROR_NOT_ENOUGH_MEMORY;

			uiStatus = MsiFormatRecord(NULL, hLastErr, pErrorMessage, &extErrBuffSize);

			return uiStatus;
		}
	}

	DWORD Utilities::GetMsiProperties(map<LPWSTR, LPWSTR>& ppmapout, LPWSTR fileName)
	{
		MSIHANDLE pDatabase;
		MSIHANDLE pView;
		MSIHANDLE pRecord;
		UINT uiReturn = 0;
		DWORD dwRecValBuffer = 0;
		LPWSTR pRecProperty;
		LPWSTR pRecValue;
		
		uiReturn = MsiOpenDatabase(fileName, L"MSIDBOPEN_READONLY", &pDatabase);
		if (ERROR_SUCCESS == uiReturn)
		{
			uiReturn = MsiDatabaseOpenView(pDatabase, L"Select Property, Value From Property", &pView);
			if (ERROR_SUCCESS == uiReturn)
			{
				uiReturn = MsiViewExecute(pView, NULL);
				if (ERROR_SUCCESS == uiReturn)
				{
					do
					{
						uiReturn = MsiViewFetch(pView, &pRecord);
						IFFAILRETURNDW(uiReturn);

						// First 'column'. Property name.
						//Calculating buffer size.
						uiReturn = MsiRecordGetString(pRecord, 1, 0, &dwRecValBuffer);
						IFFAILRETURNDW(uiReturn);

						// Getting property name.
						dwRecValBuffer++;
						pRecProperty = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * dwRecValBuffer));
						uiReturn = MsiRecordGetString(pRecord, 1, pRecProperty, &dwRecValBuffer);
						IFFAILRETURNDW(uiReturn);

						// Second 'column'. Value.
						dwRecValBuffer = 0;
						uiReturn = MsiRecordGetString(pRecord, 2, 0, &dwRecValBuffer);
						IFFAILRETURNDW(uiReturn);

						dwRecValBuffer++;
						pRecValue = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * dwRecValBuffer));
						uiReturn = MsiRecordGetString(pRecord, 2, pRecValue, &dwRecValBuffer);
						IFFAILRETURNDW(uiReturn);

						ppmapout.emplace(pRecProperty, pRecValue);

					} while (pRecord != 0);
				}
			}
		}

		return uiReturn;
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
		size_t appNameSize;
		Utilities::FileHandleOutput single;
		
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

		RM_PROCESS_INFO* pprocInfo = (RM_PROCESS_INFO*)LocalAlloc(LPTR, (sizeof(RM_PROCESS_INFO) * nProcInfoNeeded));
		nProcInfo = nProcInfoNeeded;

		// Call to get the RM_PROCESS_INFO objects
		res = RmGetList(session, &nProcInfoNeeded, &nProcInfo, pprocInfo, &rebReason);
		if (res != ERROR_SUCCESS)
		{
			err = GetLastError();
			return err;
		}
		
		for (UINT i = 0; i < nProcInfo; i++)
		{
			appNameSize = wcslen(pprocInfo[i].strAppName) + 1;
			
			single.AppType = pprocInfo[i].ApplicationType;
			single.ProcessId = pprocInfo[i].Process.dwProcessId;
			single.AppName = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * appNameSize));
			single.ImagePath = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * MAX_PATH));

			wcscpy_s(single.AppName, appNameSize, pprocInfo[i].strAppName);

			HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pprocInfo[i].Process.dwProcessId);
			if (INVALID_HANDLE_VALUE != hProcess)
			{
				FILETIME ftCreate, ftExit, ftKernel, ftUser;
				if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser) && CompareFileTime(&pprocInfo[i].Process.ProcessStartTime, &ftCreate) == 0)
				{
					DWORD cch = MAX_PATH;
					if (!QueryFullProcessImageNameW(hProcess, 0, single.ImagePath, &cch))
						single.ImagePath = L"";
				}
				CloseHandle(hProcess);
			}

			ppvecfho.push_back(single);
		}
		
		if (nullptr != pprocInfo) { LocalFree(pprocInfo); }
	
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

	DWORD Utilities::GetResourceMessageTable(vector<Utilities::MessageDumpOutput>& ppvecmdo, LPTSTR libName)
	{
		DWORD err = 0;
		Utilities::MessageDumpOutput inner;

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
							PMESSAGE_RESOURCE_ENTRY messageEntry =
								(PMESSAGE_RESOURCE_ENTRY)((PBYTE)messageTable +
									(DWORD)messageBlock[block].OffsetToEntries + offset);

							inner.Id = id;
							PrintBuffer(inner.Message, L"%s", messageEntry->Text);

							ppvecmdo.push_back(inner);
							offset += messageEntry->Length;
						}
					}
				}
			}
			FreeLibrary(hDll);
		}
		else { err = GetLastError(); }

		return err;
	}

	DWORD Utilities::MapRpcEndpoints(vector<Utilities::RpcMapperOutput> &ppOutVec)
	{
		RPC_EP_INQ_HANDLE inqContext;
		RPC_STATUS result = 0;
		RPC_BINDING_HANDLE bindingHandle;
		RPC_WSTR annotation;
		RPC_WSTR stringBinding;
		size_t sbLen;
		size_t annLen;

		result = RpcMgmtEpEltInqBegin(NULL, RPC_C_EP_ALL_ELTS, 0, 0, 0, &inqContext);
		if (result != RPC_S_OK) { return result; }

		do
		{
			Utilities::RpcMapperOutput inner;
			result = RpcMgmtEpEltInqNextW(inqContext, NULL, &bindingHandle, NULL, &annotation);
			if (result != 0 || result == RPC_X_NO_MORE_ENTRIES)
				break;

			result = RpcBindingToStringBindingW(bindingHandle, &stringBinding);
			if (result != 0)
				break;

			sbLen = wcslen((const wchar_t*)stringBinding) + 1;
			annLen = wcslen((const wchar_t*)annotation) + 1;

			inner.BindingString = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * sbLen));
			inner.Annotation = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * annLen));

			wcscpy_s(inner.BindingString, sbLen, (const wchar_t*)stringBinding);
			wcscpy_s(inner.Annotation, annLen, (const wchar_t*)annotation);

			ppOutVec.push_back(inner);

			RpcStringFree(&annotation);
			RpcStringFree(&stringBinding);
			RpcBindingFree(&bindingHandle);

		} while (result != RPC_X_NO_MORE_ENTRIES);

		result = RpcMgmtEpEltInqDone(&inqContext);
		return result;
	}
}