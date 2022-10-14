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
	BOOL EndsWith(PWSTR& inputstr, LPCWSTR& comparestr)
	{
		LPCWSTR pdest = wcsstr(inputstr, comparestr);
		if (NULL == pdest)
			return FALSE;
		else
			return TRUE;
	}

	DWORD CloseExtProcessHandle(HANDLE& hsourceproc, LPCWSTR& objname)
	{
		DWORD result = ERROR_SUCCESS;
		NTSTATUS ntcall = STATUS_SUCCESS;
		HANDLE htarget = NULL;
		HANDLE hdupproc = NULL;
		
		LPCWSTR pupath = PathSkipRootW(objname);
		if (NULL == pupath)
			return GetLastError();

		HANDLE hproccurrent = GetCurrentProcess();
		if (NULL == hproccurrent)
		{
			result = GetLastError();
			goto CLEANUP;

		}

		HMODULE hmodule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || NULL == hmodule)
			return GetLastError();

		_NtQueryObject NtQueryObject = (_NtQueryObject)GetProcAddress(hmodule, "NtQueryObject");
		_NtQueryInformationProcess NtQueryInformationProcess = (_NtQueryInformationProcess)GetProcAddress(hmodule, "NtQueryInformationProcess");
		_NtDuplicateObject NtDuplicateObject = (_NtDuplicateObject)GetProcAddress(hmodule, "NtDuplicateObject");

		POBJECT_NAME_INFORMATION nameinfo = (POBJECT_NAME_INFORMATION)LocalAlloc(LMEM_ZEROINIT, sizeof(OBJECT_NAME_INFORMATION));
		if(NULL == nameinfo)
		{
			result = GetLastError();
			goto CLEANUP;
		}

		PPROCESS_HANDLE_SNAPSHOT_INFORMATION pprochandleinfo = (PPROCESS_HANDLE_SNAPSHOT_INFORMATION)LocalAlloc(LMEM_ZEROINIT, sizeof(PROCESS_HANDLE_SNAPSHOT_INFORMATION));
		if (NULL == pprochandleinfo)
		{
			result = GetLastError();
			goto CLEANUP;
		}

		ULONG infosize = (ULONG)sizeof(PROCESS_HANDLE_SNAPSHOT_INFORMATION);
		ULONG infoneeded = 0;

		do
		{
			ntcall = NtQueryInformationProcess(hsourceproc, ProcessHandleInformation, pprochandleinfo, infosize, &infoneeded);
			if (STATUS_SUCCESS != ntcall && STATUS_INFO_LENGTH_MISMATCH != ntcall)
				return ntcall;

			if (STATUS_SUCCESS == ntcall)
				break;

			infosize = infoneeded;
			LocalFree(pprochandleinfo);

			pprochandleinfo = (PPROCESS_HANDLE_SNAPSHOT_INFORMATION)LocalAlloc(LMEM_ZEROINIT, infosize);
			if (NULL == pprochandleinfo)
			{
				result = GetLastError();
				goto CLEANUP;
			}

		} while (ntcall == STATUS_INFO_LENGTH_MISMATCH);

		for (ULONG i = 0; i < pprochandleinfo->NumberOfHandles; i++)
		{
			HANDLE hcurrent = pprochandleinfo->Handles[i].HandleValue;
			HANDLE hclose = NULL;

			if (FALSE == DuplicateHandle(hsourceproc, hcurrent, hproccurrent, &htarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
				continue;

			POBJECT_NAME_INFORMATION objnameinfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OBJECT_NAME_INFORMATION));
			if (NULL == objnameinfo)
				return ERROR_NOT_ENOUGH_MEMORY;

			ntcall = NtQueryObjectWithTimeout(htarget, ObjectNameInformation, objnameinfo, 200);
			CloseHandle(htarget);

			if (objnameinfo->Name.Buffer && TRUE == EndsWith(objnameinfo->Name.Buffer, pupath))
			{
				if (FALSE == DuplicateHandle(hsourceproc, hcurrent, hproccurrent, &hclose, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
				{
					result = GetLastError();
					goto CLEANUP;
				}
				else
				{
					CloseHandle(hclose);
					break;
				}
			}
			
			HeapFree(GetProcessHeap(), NULL, objnameinfo);
		}

	CLEANUP:

		LOCFREEWCHECK(pprochandleinfo);

		return result;
	}

	DWORD GetProcessImageVersionInfo(LPWSTR& imagepath, const LPCWSTR& propname, LPWSTR& value)
	{
		DWORD result = S_OK;
		DWORD verinfosize = 0;
		LPVOID databuffer;
		UINT transdatasize = 0;
		LPWSTR subblockfuffer = L"";

		struct LANGUAGE_AND_CODEPAGE {
			WORD	Language;
			WORD	CodePage;
		} *PLANGUAGE_AND_CODEPAGE;

		verinfosize = GetFileVersionInfoSizeW(imagepath, NULL);
		if (verinfosize == 0)
			return GetLastError();

		databuffer = (LPVOID)LocalAlloc(LMEM_ZEROINIT, verinfosize);
		ALLCHECK(databuffer);

		if (FALSE == (GetFileVersionInfoW(imagepath, NULL, verinfosize, databuffer)))
			return GetLastError();

		if (FALSE == (VerQueryValueW(databuffer, L"\\VarFileInfo\\Translation", (LPVOID*)&PLANGUAGE_AND_CODEPAGE, &transdatasize)))
			return GetLastError();

		for (UINT i = 0; i < (transdatasize / sizeof(struct LANGUAGE_AND_CODEPAGE)); i++)
		{
			subblockfuffer = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, sizeof(WCHAR) * MAX_PATH);
			ALLCHECK(subblockfuffer);

			LPVOID propbuffer;
			UINT currdatasize = 0;

			StringCchPrintfW(subblockfuffer, MAX_PATH, L"\\StringFileInfo\\%04x%04x\\", PLANGUAGE_AND_CODEPAGE[i].Language, PLANGUAGE_AND_CODEPAGE[i].CodePage);

			size_t tsubbstrsize = wcslen(subblockfuffer) + wcslen(propname) + 2;
			wcscat_s(subblockfuffer, tsubbstrsize, propname);

			VerQueryValueW(databuffer, subblockfuffer, &propbuffer, &currdatasize);
			
			LPWSTR strbuff = static_cast<LPWSTR>(propbuffer);
			size_t bufflen = wcslen(strbuff) + 1;

			value = new WCHAR[bufflen];
			wcscpy_s(value, bufflen, strbuff);

			LOCFREEWCHECK(subblockfuffer);
		}

		return result;
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

	DWORD Unmanaged::GetProcessObjectHandle
	(
		std::vector<Unmanaged::ObjectHandle>& ppvecfho,
		std::vector<LPCWSTR>& reslist,
		BOOL closeHandle = FALSE,
		BOOL terminateProcess = FALSE
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
		HANDLE hProcess = NULL;

		Unmanaged::ObjectHandle* single;

		for (size_t i = 0; i < reslist.size(); i++)
		{
			HRESULT hresult = S_OK;

			PFILE_PROCESS_IDS_USING_FILE_INFORMATION ntprocinfo = (PFILE_PROCESS_IDS_USING_FILE_INFORMATION)LocalAlloc(LMEM_ZEROINIT, sizeof(FILE_PROCESS_IDS_USING_FILE_INFORMATION));
			ALLCHECK(ntprocinfo);

			size_t pathsz = wcslen(reslist.at(i)) + 1;
			LPWSTR inputobj = new WCHAR[pathsz];

			wcscpy_s(inputobj, pathsz, reslist.at(i));
			PathStripPathW(inputobj);

			// TODO: Error handling
			ntcall = GetNtProcessUsingFileList(reslist.at(i), ntprocinfo);
			
			for (ULONG j = 0; j < ntprocinfo->NumberOfProcessIdsInList; j++)
			{
				single = (Unmanaged::ObjectHandle*)LocalAlloc(LMEM_ZEROINIT, sizeof(Unmanaged::ObjectHandle));
				ALLCHECK(single);

				single->InputObject = inputobj;
				single->ProcessId = (DWORD)ntprocinfo->ProcessIdList[j];

				hProcess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
					FALSE,
					(DWORD)ntprocinfo->ProcessIdList[j]
				);
				
				if (INVALID_HANDLE_VALUE != hProcess && NULL != hProcess)
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

					res = GetProcessImageVersionInfo(single->ImagePath, L"FileDescription", single->Application);
					if (res != 0)
						single->Application = L"";
					
					res = GetProcessImageVersionInfo(single->ImagePath, L"FileVersion", single->FileVersion);
					if (res != 0)
						single->FileVersion = L"";

					res = GetProcessImageVersionInfo(single->ImagePath, L"CompanyName", single->CompanyName);
					if (res != 0)
						single->CompanyName = L"";

					res = GetProcessImageVersionInfo(single->ImagePath, L"ProductName", single->ProductName);
					if (res != 0)
						single->ProductName = L"";

					if (TRUE == closeHandle)
					{
						res = CloseExtProcessHandle(hProcess, reslist.at(i));
						if (ERROR_SUCCESS != res)
							return res;
					}
					
					CloseHandle(hProcess);
				}
				
				ppvecfho.push_back(*single);
			
				if (nullptr != single)
					LocalFree(single);
			}

			if (nullptr != ntprocinfo)
				LocalFree(ntprocinfo);
		}
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
	LPWSTR Unmanaged::GetFormattedError(DWORD errorCode)
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
		Unmanaged::ResourceMessageTable* psingle;
		
		if (nullptr == psingle)
		{
			err = GetLastError();
			goto CLEANUP;
		}

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
							psingle = (Unmanaged::ResourceMessageTable*)LocalAlloc(LMEM_ZEROINIT, sizeof(Unmanaged::ResourceMessageTable));
							if (nullptr == psingle)
							{
								err = GetLastError();
								goto CLEANUP;
							}

							PMESSAGE_RESOURCE_ENTRY messageEntry =
								(PMESSAGE_RESOURCE_ENTRY)((PBYTE)messageTable +
									(DWORD)messageBlock[block].OffsetToEntries + offset);

							psingle->Id = id;
							psingle->Message = new WCHAR[150];
							StringCchPrintfW(psingle->Message, 150, L"%s", messageEntry->Text);

							ppvecmdo.push_back(*psingle);
							if (NULL == LocalFree(psingle))
								psingle = NULL;

							offset += messageEntry->Length;
						}
					}
				}
			}
			
		}
		else
			err = GetLastError();

	CLEANUP:

		if (NULL != psingle)
			LocalFree(psingle);

		if (NULL != hDll)
			FreeLibrary(hDll);

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

	DWORD Unmanaged::SendClick()
	{
		DWORD result = ERROR_SUCCESS;
		UINT sin = 0;
		UINT tries = 0;

		PPOINT pposition = (PPOINT)LocalAlloc(LMEM_ZEROINIT, sizeof(POINT));
		ALLCHECK(pposition);

		LPINPUT pinput = (LPINPUT)LocalAlloc(LMEM_ZEROINIT, sizeof(INPUT) * 2);
		ALLCHECK(pinput);

		if (FALSE == (GetCursorPos(pposition)))
		{
			result = GetLastError();
			goto CLEANUP;
		}

		pinput[0].type = INPUT_MOUSE;
		pinput[0].mi.dx = pposition->x;
		pinput[0].mi.dy = pposition->y;
		pinput[0].mi.mouseData = 0;
		pinput[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

		pinput[1].type = INPUT_MOUSE;
		pinput[1].mi.dx = pposition->x;
		pinput[1].mi.dy = pposition->y;
		pinput[1].mi.mouseData = 0;
		pinput[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		do
		{
			sin = SendInput(2, pinput, sizeof(INPUT));
			if (sin != 0)
				break;

			Sleep(20);

		} while (sin == 0 && tries++ <3 );

		if (sin == 0)
			result = GetLastError();

	CLEANUP:
		
		if (nullptr != pposition)
			LocalFree(pposition);
		
		if (nullptr != pinput)
			LocalFree(pinput);

		return result;
	}
}