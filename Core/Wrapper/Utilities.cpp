#include "pch.h"
#include "Unmanaged.h"
#include "Utilities.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

#define CHECKDWRESULT(result) if (ERROR_SUCCESS != result) { goto CLEANUP; }
#define IFFAILRETURNDW(result) if (ERROR_SUCCESS != result) { return result; }

namespace WindowsUtils::Core
{
	// Wrapper for SHCreateItemFromParsingName(), IShellItem2::GetString()
	// Throws std::system_error in case of any error.
	HRESULT GetShellPropStringFromPath(LPCWSTR pPath, std::vector<FileProperty>& propertyinfo)
	{
		HRESULT hresult = S_OK;
		hresult = CoInitialize(nullptr);
		if (hresult != S_OK && hresult != S_FALSE && hresult != RPC_E_CHANGED_MODE) // Success ,already initialized and thread mode already set
			return hresult;

		// Use CComPtr to automatically release the IShellItem2 interface when the function returns
		// or an exception is thrown.
		CComPtr<IShellItem2> pItem;
		hresult = SHCreateItemFromParsingName(pPath, nullptr, IID_PPV_ARGS(&pItem));
		if (FAILED(hresult))
			return hresult;

		// Use CComHeapPtr to automatically release the string allocated by the shell when the function returns
		// or an exception is thrown (calls CoTaskMemFree).

		for (size_t i = 0; i < propertyinfo.size(); i++)
		{
			hresult = pItem->GetString(propertyinfo.at(i).Property, &propertyinfo.at(i).Value);
			if (FAILED(hresult))
				return hresult;
		}

		CoUninitialize();
		
		return hresult;;
	}

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

	DWORD Unmanaged::GetMsiExtendedErrorMessage(LPWSTR& pErrorMessage)
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
		}
	
		return uiStatus;
	}

	DWORD Unmanaged::GetMsiProperties(std::map<std::wstring, std::wstring>& ppmapout, LPWSTR fileName)
	{
		PMSIHANDLE pDatabase;
		PMSIHANDLE pView;
		PMSIHANDLE pRecord;
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

						if (nullptr != pRecProperty && nullptr != pRecValue)
						{
							std::wstring recproperty(pRecProperty);
							std::wstring recvalue(pRecValue);
							ppmapout[recproperty] = recvalue;
						}
						else
							return ERROR_NOT_ENOUGH_MEMORY;

					} while (pRecord != 0);
				}
			}
		}

		return uiReturn;
	}

	DWORD Unmanaged::GetProcessFileHandle
	(
		std::vector<Unmanaged::FileHandle>& ppvecfho,
		std::vector<LPCWSTR> reslist
	)
	{
		// Invalid handle value for DWORD
		DWORD sessionHandle = 0xFFFFFFFF;
		
		UINT nProcInfo = 0;
		UINT nProcInfoNeeded = 0;
		RM_REBOOT_REASON rebReason = RmRebootReasonNone;
		PRM_PROCESS_INFO pprocInfo = NULL;
		
		DWORD res = 0;
		NTSTATUS ntcall;
		UINT retry = 0;
		Unmanaged::FileHandle* single = new Unmanaged::FileHandle;

		for (size_t i = 0; i < reslist.size(); i++)
		{
			HRESULT hresult = S_OK;
			PFILE_PROCESS_IDS_USING_FILE_INFORMATION ntprocinfo = (PFILE_PROCESS_IDS_USING_FILE_INFORMATION)LocalAlloc(LMEM_ZEROINIT, sizeof(FILE_PROCESS_IDS_USING_FILE_INFORMATION));
			if (nullptr == ntprocinfo)
				return ERROR_NOT_ENOUGH_MEMORY;

			single->FileName = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (sizeof(WCHAR) * MAX_PATH));
			if (nullptr == single->FileName)
				return ERROR_NOT_ENOUGH_MEMORY;
			
			wcscpy_s(single->FileName, MAX_PATH, reslist.at(i));
			PathStripPathW(single->FileName);

			// TODO: Error handling
			ntcall = GetNtProcessUsingFileList(reslist.at(i), ntprocinfo);
			
			for (ULONG j = 0; j < ntprocinfo->NumberOfProcessIdsInList; j++)
			{
				single->ProcessId = (DWORD)ntprocinfo->ProcessIdList[j];
				std::vector<FileProperty>* imageprop = (std::vector<FileProperty>*)LocalAlloc(LMEM_ZEROINIT, sizeof(std::vector<FileProperty>));
				if (nullptr == imageprop)
					return ERROR_NOT_ENOUGH_MEMORY;

				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)ntprocinfo->ProcessIdList[j]);
				if (INVALID_HANDLE_VALUE != hProcess)
				{
					single->ImagePath = (LPWSTR)LocalAlloc(LPTR, (sizeof(wchar_t) * MAX_PATH));
					if (nullptr != single->ImagePath)
					{
						DWORD cch = MAX_PATH;
						if (!QueryFullProcessImageNameW(hProcess, 0, single->ImagePath, &cch))
							single->ImagePath = L"";
					}
					else
						return ERROR_NOT_ENOUGH_MEMORY;


					imageprop->push_back(FileProperty(PKEY_FileDescription, LPWSTR(L"")));
					imageprop->push_back(FileProperty(PKEY_Software_ProductName, LPWSTR(L"")));
					imageprop->push_back(FileProperty(PKEY_FileVersion, LPWSTR(L"")));

					hresult = GetShellPropStringFromPath(single->ImagePath, *imageprop);

					for (size_t k = 0; k < imageprop->size(); k++)
					{
						FileProperty innerprop = imageprop->at(k);
						if (innerprop.Property == PKEY_FileDescription)
							single->Application = innerprop.Value;

						if (innerprop.Property == PKEY_FileVersion)
							single->FileVersion = innerprop.Value;

						if (innerprop.Property == PKEY_Software_ProductName)
							single->ProductName = innerprop.Value;
					}
					
					CloseHandle(hProcess);
				}

				ppvecfho.push_back(*single);

				if (nullptr != imageprop)
					LocalFree(imageprop);
			}

			if (nullptr != ntprocinfo)
				LocalFree(ntprocinfo);
		}

		if (nullptr != single)
			delete single;

		return res;
	}

	LPWSTR Unmanaged::GetFormatedWSError()
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
	LPWSTR Unmanaged::GetFormatedWin32Error()
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
	LPWSTR Unmanaged::GetFormatedError(DWORD errorCode)
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

	DWORD Unmanaged::GetResourceMessageTable(std::vector<Unmanaged::ResourceMessageTable>& ppvecmdo, LPTSTR libName)
	{
		DWORD err = 0;
		Unmanaged::ResourceMessageTable inner;

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

	DWORD Unmanaged::MapRpcEndpoints(std::vector<Unmanaged::RpcEndpoint>& ppOutVec)
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
			Unmanaged::RpcEndpoint inner;
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