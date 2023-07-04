#include "pch.h"
#include "Utilities.h"
#include <psapi.h>
#include <new>
#include <system_error>

#pragma comment(lib, "Psapi")

namespace WindowsUtils::Core
{
	/*========================================
	==		 Main function definition		==
	==========================================*/

	// Get-ObjectHandle
	DWORD Utilities::GetProcessObjectHandle(
		std::vector<Utilities::WU_OBJECT_HANDLE>& rvecobjhandle	// A vector of object handle output objects.
		, std::vector<LPCWSTR>& rvecinputpath					// A vector of input objects.
		, BOOL closehandle = FALSE								// TRUE for closing the handles found.
	)
	{
		DWORD result = ERROR_SUCCESS;

		for (size_t i = 0; i < rvecinputpath.size(); i++)
		{
			/*
			* This strips the path out of the input object so we can add it to the 'InputObject' property.
			* This will need to be changed in the future to support multiple object types.
			*/
			WCHAR lpinputobj[MAX_PATH] = { 0 };
			wcscpy_s(lpinputobj, MAX_PATH, rvecinputpath.at(i));
			PathStripPathW(lpinputobj);

			PFILE_PROCESS_IDS_USING_FILE_INFORMATION pprocidufile = NULL;
			result = GetNtProcessUsingFile(rvecinputpath.at(i), pprocidufile);
			if (result != ERROR_SUCCESS)
				return result;

			for (ULONG j = 0; j < pprocidufile->NumberOfProcessIdsInList; j++)
			{
				PWU_OBJECT_HANDLE uphobj = new WU_OBJECT_HANDLE;
				HANDLE hprocess = NULL;

				size_t szinputobj = wcslen(lpinputobj) + 1;
				uphobj->InputObject = new WCHAR[szinputobj];
				wcscpy_s(uphobj->InputObject, szinputobj, lpinputobj);

				uphobj->ProcessId = (DWORD)pprocidufile->ProcessIdList[j];

				if (uphobj->ProcessId == 10220)
					result = 0;

				hprocess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE
					, FALSE
					, (DWORD)pprocidufile->ProcessIdList[j]
				);
				if (NULL != hprocess)
				{
					DWORD szmaxpath = MAX_PATH;
					uphobj->ImagePath = new WCHAR[MAX_PATH]{ 0 };
					uphobj->Name = new WCHAR[MAX_PATH]{ 0 };
					::QueryFullProcessImageNameW(hprocess, 0, uphobj->ImagePath, &szmaxpath);

					if (wcslen(uphobj->ImagePath) == 0)
					{
						result = GetProcessImageName(uphobj->ProcessId, uphobj->Name);
						if (wcslen(uphobj->Name) > 0)
						{
							wcscpy_s(uphobj->ImagePath, MAX_PATH, uphobj->Name);
							::PathStripPathW(uphobj->Name);
						}

					}
					else
					{
						wcscpy_s(uphobj->Name, MAX_PATH, uphobj->ImagePath);
						::PathStripPathW(uphobj->Name);
					}

					for (VERSION_INFO_PROPERTY verinfoprop : { FileDescription, ProductName, FileVersion, CompanyName })
					{
						LPWSTR lpinter = { 0 };
						GetProccesVersionInfo(uphobj->ImagePath, verinfoprop, lpinter);
						uphobj->VersionInfo.emplace(std::make_pair(verinfoprop, LPCWSTR(lpinter)));
					}

					if (closehandle)
					{
						result = CloseExtProcessHandle(hprocess, rvecinputpath.at(i));
						if (ERROR_SUCCESS != result)
							return result;
					}
					::CloseHandle(hprocess);
				}
				else
				{
					if (ERROR_ACCESS_DENIED == ::GetLastError())
					{
						hprocess = OpenProcess(
							PROCESS_QUERY_LIMITED_INFORMATION
							, FALSE
							, (DWORD)pprocidufile->ProcessIdList[j]
						);

						DWORD szmaxpath = MAX_PATH;
						uphobj->ImagePath = new WCHAR[MAX_PATH]{ 0 };
						uphobj->Name = new WCHAR[MAX_PATH]{ 0 };
						if (::QueryFullProcessImageNameW(hprocess, 0, uphobj->ImagePath, &szmaxpath))
						{
							wcscpy_s(uphobj->Name, MAX_PATH, uphobj->ImagePath);
							::PathStripPathW(uphobj->Name);

							for (VERSION_INFO_PROPERTY verinfoprop : { FileDescription, ProductName, FileVersion, CompanyName })
							{
								LPWSTR lpinter;
								GetProccesVersionInfo(uphobj->ImagePath, verinfoprop, lpinter);
								uphobj->VersionInfo.emplace(std::make_pair(verinfoprop, LPCWSTR(lpinter)));
							}
						}
						else
						{
							if (ERROR_SUCCESS == GetProcessImageName(uphobj->ProcessId, uphobj->Name))
							{
								wcscpy_s(uphobj->ImagePath, MAX_PATH, uphobj->Name);
								::PathStripPathW(uphobj->Name);
							}

							if (
								wcscmp(uphobj->Name, L"System") == 0
								|| wcscmp(uphobj->Name, L"Secure System") == 0
								|| wcscmp(uphobj->Name, L"Registry") == 0)
							{
								LPCWSTR windir = L"windir";
								LPWSTR lpenvvalue;
								result = GetEnvVariableW(windir, lpenvvalue);
								DWERRORCHECKV(result);

								std::wstring wstrwindir(lpenvvalue);
								std::wstring imagepath = L"\\System32\\ntoskrnl.exe";
								std::wstring fullimagepath = wstrwindir + imagepath;

								wcscpy_s(uphobj->ImagePath, MAX_PATH, fullimagepath.c_str());

								for (VERSION_INFO_PROPERTY verinfoprop : { FileDescription, ProductName, FileVersion, CompanyName })
								{
									LPWSTR lpinter;
									GetProccesVersionInfo(uphobj->ImagePath, verinfoprop, lpinter);
									uphobj->VersionInfo.emplace(std::make_pair(verinfoprop, LPCWSTR(lpinter)));
								}
							}
						}
					}
				}

				rvecobjhandle.push_back(*uphobj);

				delete uphobj;
			}
		}

		return result;
	}

	// Get-FormattedMessage
	DWORD Utilities::GetFormattedError(
		DWORD dwerrorcode				// The Win32 error code.
		, LPWSTR& rlperrormess			// The output message string.
	)
	{
		if (!::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER
			, NULL
			, dwerrorcode
			, 0
			, (LPWSTR)&rlperrormess
			, 0
			, NULL
		))
			return GetLastError();

		return ERROR_SUCCESS;
	}

	// Get-LastWin32Error
	DWORD Utilities::GetFormattedWin32Error(
		LPWSTR& rlperrormess				// The output message string.
	)
	{
		if (!::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER
			, NULL
			, GetLastError()
			, 0
			, (LPWSTR)&rlperrormess
			, 0
			, NULL
		))
			return GetLastError();

		return ERROR_SUCCESS;
	}

	// Send-Click
	DWORD Utilities::SendClick()
	{
		DWORD result = ERROR_SUCCESS;
		UINT usendin = 0;
		UINT utries = 0;
		POINT pointpos = { 0 };
		LPINPUT pinput = new INPUT[2]{ 0 };

		if (!::GetCursorPos(&pointpos))
			return ::GetLastError();

		pinput[0].type, pinput[1].type = INPUT_MOUSE;
		pinput[0].mi.dx, pinput[1].mi.dx = pointpos.x;
		pinput[0].mi.dy, pinput[1].mi.dy = pointpos.y;
		pinput[0].mi.mouseData, pinput[1].mi.mouseData = 0;

		pinput[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		pinput[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		do
		{
			usendin = ::SendInput(2, pinput, sizeof(INPUT));
			if (usendin != 0)
				break;

			::Sleep(20);

		} while (usendin == 0 && utries++ < 3);

		if (usendin == 0)
			result = ::GetLastError();

		delete[] pinput;

		return result;
	}

	// Get-ResourceMessageTable
	DWORD Utilities::GetResourceMessageTable(
		std::vector<Utilities::WU_RESOURCE_MESSAGE_TABLE>& rvecresmestb	// A vector of resource message table objects.
		, LPWSTR& lplibname												// The resource path.
	)
	{
		DWORD result = ERROR_SUCCESS;

		HMODULE hmodule = ::LoadLibraryExW(lplibname, NULL, LOAD_LIBRARY_AS_DATAFILE);
		if (NULL == hmodule)
			return ::GetLastError();

		HRSRC hresource = ::FindResourceW(hmodule, MAKEINTRESOURCE(1), RT_MESSAGETABLE);
		if (NULL == hresource)
			return ::GetLastError();

		HGLOBAL hload = LoadResource(hmodule, hresource);
		if (NULL == hload)
			return ::GetLastError();

		PVOID pmessagetable = LockResource(hload);
		DWORD dwblocknumber = ((PMESSAGE_RESOURCE_DATA)pmessagetable)->NumberOfBlocks;
		PMESSAGE_RESOURCE_BLOCK pmessblock = ((PMESSAGE_RESOURCE_DATA)pmessagetable)->Blocks;

		for (DWORD block = 0; block < dwblocknumber; block++)
		{
			DWORD dwlowid = pmessblock[block].LowId;
			DWORD dwhighid = pmessblock[block].HighId;
			DWORD dwoffset = 0;

			for (DWORD id = dwlowid; id <= dwhighid; id++)
			{
				WU_RESOURCE_MESSAGE_TABLE rmessingle;

				PMESSAGE_RESOURCE_ENTRY pmessentry =
					(PMESSAGE_RESOURCE_ENTRY)((PBYTE)pmessagetable +
						(DWORD)pmessblock[block].OffsetToEntries + dwoffset);

				rmessingle.Id = id;
				PrintBufferW(rmessingle.Message, L"%S", pmessentry->Text);

				rvecresmestb.push_back(rmessingle);
				dwoffset += pmessentry->Length;
			}
		}

		if (NULL != hmodule)
			FreeLibrary(hmodule);

		return result;
	}

	// Get-MsiProperties
	DWORD Utilities::GetMsiProperties(
		std::map<LPWSTR, LPWSTR>& rmapprop	// A map with the properties and values from the MSI database.
		, LPWSTR& lpfilename				// The MSI file path.
	)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD dwszbuffer = 0;
		PMSIHANDLE pdatabase;
		PMSIHANDLE pview;
		PMSIHANDLE precord;

		result = MsiOpenDatabaseW(lpfilename, L"MSIDBOPEN_READONLY", &pdatabase);
		DWERRORCHECKV(result);

		result = MsiDatabaseOpenViewW(pdatabase, L"Select Property, Value From Property", &pview);
		DWERRORCHECKV(result);

		result = MsiViewExecute(pview, NULL);
		DWERRORCHECKV(result);

		do
		{
			LPWSTR lproperty = { 0 };
			LPWSTR lpvalue = { 0 };

			result = MsiViewFetch(pview, &precord);
			DWERRORCHECKV(result);

			/*
			* First column, property name.
			* Calculating buffer size.
			*/
			result = MsiRecordGetStringW(precord, 1, NULL, &dwszbuffer);
			DWERRORCHECKV(result);

			// \0
			dwszbuffer++;
			lproperty = new WCHAR[dwszbuffer];
			result = MsiRecordGetStringW(precord, 1, lproperty, &dwszbuffer);
			DWERRORCHECKV(result);

			// Second column, value.
			dwszbuffer = 0;
			result = MsiRecordGetStringW(precord, 2, NULL, &dwszbuffer);
			DWERRORCHECKV(result);

			dwszbuffer++;
			lpvalue = new WCHAR[dwszbuffer];
			result = MsiRecordGetStringW(precord, 2, lpvalue, &dwszbuffer);
			DWERRORCHECKV(result);

			rmapprop[lproperty] = lpvalue;

		} while (precord != 0);

		return result;
	}

	DWORD Utilities::ExpandArchiveFile(const LPSTR& lpszFileName, const LPSTR& lpszFilePath, const LPSTR& lpszDestination, ARCHIVE_FILE_TYPE fileType)
	{
		switch (fileType)
		{
		case ARCHIVE_FILE_TYPE::Cabinet:
			return ExpandCabinetFile(lpszFileName, lpszFilePath, lpszDestination);
			break;

		default:
			break;
		}
	}

	
	/*========================================
	==		Utility function definition		==
	==========================================*/

	// Gets process image version information.
	DWORD GetProccesVersionInfo(
		LPWSTR& imagepath							// The process image file path.
		,Utilities::VERSION_INFO_PROPERTY& propname	// The property name, or 'key' value.
		,LPWSTR& value)								// The return value.
	{
		DWORD result = ERROR_SUCCESS;
		WORD* langncodepage;
		UINT len;
		WCHAR* desc;
		WCHAR text[256];

		LPWSTR lppropname = { 0 };
		switch (propname)
		{
		case Utilities::FileDescription:
			lppropname = new WCHAR[16];
			wcscpy_s(lppropname, 16, L"FileDescription");
			break;
		case Utilities::ProductName:
			lppropname = new WCHAR[12];
			wcscpy_s(lppropname, 12, L"ProductName");
			break;
		case Utilities::FileVersion:
			lppropname = new WCHAR[12];
			wcscpy_s(lppropname, 12, L"FileVersion");
			break;
		case Utilities::CompanyName:
			lppropname = new WCHAR[12];
			wcscpy_s(lppropname, 12, L"CompanyName");
			break;
		default:
			break;
		}

		if (0 == lppropname)
			return ::GetLastError();

		size_t complen = 256 + wcslen(lppropname) + 2;

		DWORD verinfosize = ::GetFileVersionInfoSizeW(imagepath, NULL);
		LPVOID buffer = (LPVOID)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, verinfosize);
		if (NULL == buffer)
			return ERROR_NOT_ENOUGH_MEMORY;

		if (!::GetFileVersionInfoW(imagepath, NULL, verinfosize, buffer))
			return GetLastError();

		if (!::VerQueryValueW(buffer, L"\\VarFileInfo\\Translation", (LPVOID*)&langncodepage, &len))
			return GetLastError();

		::StringCchPrintfW(text, complen, TEXT("\\StringFileInfo\\%04x%04x\\"), langncodepage[0], langncodepage[1]);

		std::wstring subbuffer(text);
		std::wstring propertyname(lppropname);
		std::wstring resulttest = subbuffer + propertyname;

		if (::VerQueryValueW(buffer, resulttest.c_str(), (LPVOID*)&desc, &len))
		{
			
			size_t descsz = wcslen(desc) + 1;
			if (descsz > 1 && !IsNullOrWhiteSpace(desc))
			{
				value = new WCHAR[descsz];
				wcscpy_s(value, descsz, desc);
			}
			else
				value = new WCHAR[1]{ 0 };
		}
		else
			value = new WCHAR[1]{ 0 };

		if (NULL != buffer)
			::HeapFree(::GetProcessHeap(), NULL, buffer);

		delete[] lppropname;

		return result;
	}

	/*
	* Used by the parameter -CloseHandle, from Get-ObjectHandle.
	*/
	DWORD CloseExtProcessHandle(
		HANDLE& rhextprocess		// A valid handle to the external process.
		, LPCWSTR& rlpcobjectname	// The object name, used on GetProcessObjectHandle.
	)
	{
		DWORD result = ERROR_SUCCESS;
		NTSTATUS ntcall = STATUS_SUCCESS;
		HANDLE htarget = NULL;

		LPCWSTR lpcpathnoroot = ::PathSkipRootW(rlpcobjectname);
		if (NULL == lpcpathnoroot)
			return ::GetLastError();

		HMODULE hmodule = ::GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || NULL == hmodule)
			return ::GetLastError();

		_NtQueryInformationProcess NtQueryInformationProcess = (_NtQueryInformationProcess)::GetProcAddress(hmodule, "NtQueryInformationProcess");

		std::unique_ptr<BYTE[]> buffer;
		ULONG szbuffer = (ULONG)sizeof(PROCESS_HANDLE_SNAPSHOT_INFORMATION);
		ULONG szbuffneed = 0;
		do
		{
			buffer = std::make_unique<BYTE[]>(szbuffer);
			ntcall = NtQueryInformationProcess(rhextprocess, ProcessHandleInformation, buffer.get(), szbuffer, &szbuffneed);

			if (STATUS_SUCCESS != ntcall && STATUS_INFO_LENGTH_MISMATCH != ntcall)
				return ntcall;

			if (STATUS_SUCCESS == ntcall)
				break;

			szbuffer = szbuffneed;

		} while (ntcall == STATUS_INFO_LENGTH_MISMATCH);

		PPROCESS_HANDLE_SNAPSHOT_INFORMATION phsnapinfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());

		for (ULONG i = 0; i < phsnapinfo->NumberOfHandles; i++)
		{
			POBJECT_NAME_INFORMATION pobjnameinfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OBJECT_NAME_INFORMATION));
			if (NULL == pobjnameinfo)
				return ERROR_NOT_ENOUGH_MEMORY;

			if (!::DuplicateHandle(rhextprocess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &htarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
				continue;

			ntcall = NtQueryObjectWithTimeout(htarget, ObjectNameInformation, pobjnameinfo, 200);
			if (STATUS_SUCCESS != ntcall)
				return ntcall;

			::CloseHandle(htarget);

			if (pobjnameinfo->Name.Buffer && EndsWith(pobjnameinfo->Name.Buffer, lpcpathnoroot))
			{
				if (!::DuplicateHandle(rhextprocess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &htarget, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
					result = GetLastError();

				else
				{
					::CloseHandle(htarget);
					break;
				}
			}
		}

		return result;
	}

	BOOL EndsWith(PWSTR& inputstr, LPCWSTR& comparestr)
	{
		LPCWSTR pdest = wcsstr(inputstr, comparestr);
		if (NULL == pdest)
			return FALSE;
		else
			return TRUE;
	}

	// Write formatted output in a buffer. The memory allocation is what makes this function needed.
	VOID PrintBufferW(LPWSTR& lpbuffer, WCHAR const* const format, ...)
	{
		va_list args;
		int ilength;

		va_start(args, format);
		ilength = _vscwprintf(format, args) + 1;

		lpbuffer = new WCHAR[ilength];
		vswprintf_s(lpbuffer, ilength, format, args);
	}

	// Gets extented error information for Get-MsiProperties
	DWORD Utilities::GetMsiExtendedError(LPWSTR& lperrormessage)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD dwszerrbuffer = 0;

		PMSIHANDLE hlasterror = MsiGetLastErrorRecord();
		if (NULL == hlasterror)
			return GetLastError();

		result = MsiFormatRecordW(NULL, hlasterror, NULL, &dwszerrbuffer);
		if (ERROR_SUCCESS != result)
			return result;

		dwszerrbuffer++;
		lperrormessage = new WCHAR[dwszerrbuffer];

		result = MsiFormatRecordW(NULL, hlasterror, lperrormessage, &dwszerrbuffer);

		return result;
	}

	// Checks if a C-style string is null or white space.
	BOOL IsNullOrWhiteSpace(LPWSTR& lpinputstr)
	{
		size_t strlen = wcslen(lpinputstr);
		if (strlen > 0)
		{
			for (size_t i = 0; i < strlen; i++)
			{
				if (lpinputstr[i] != ' ')
					return FALSE;
			}
		}
		
		return TRUE;
	}

	// Helper function to retrieve environment variables safely.
	DWORD GetEnvVariableW(LPCWSTR& rlpcvarname, LPWSTR& rlpvalue)
	{
		DWORD result = ERROR_SUCCESS;
		size_t szrequiredstr = 0;

		_wgetenv_s(&szrequiredstr, NULL, 0, rlpcvarname);
		if (szrequiredstr == 0)
			return ERROR_FILE_NOT_FOUND;

		rlpvalue = new WCHAR[szrequiredstr]{ 0 };
		
		_wgetenv_s(&szrequiredstr, rlpvalue, szrequiredstr, rlpcvarname);

		return result;
	}

	DWORD ExpandCabinetFile(const LPSTR& lpszFileName, const LPSTR& lpszFilePath, const LPSTR& lpszDestination)
	{
		DWORD result = ERROR_SUCCESS;
		ERF erfError;
		HFDI hContext;

		hContext = FDICreate(
			FdiFnMemAloc,
			FdiFnMemFree,
			FdiFnFileOpen,
			FdiFnFileRead,
			FdiFnFileWrite,
			FdiFnFileClose,
			FdiFnFileSeek,
			cpuUNKNOWN,
			&erfError
		);

		if (hContext == NULL)
			return (DWORD)erfError.erfOper;

		if (!FDICopy(hContext, lpszFileName, lpszFilePath, 0, FdiFnNotifyCallback, NULL, lpszDestination))
		{
			if (hContext != NULL)
				FDIDestroy(hContext);

			return (DWORD)erfError.erfOper;
		}

		if (hContext != NULL)
			FDIDestroy(hContext);

		return result;
	}

	// Memory management helper functions
	WuMemoryManagement& WuMemoryManagement::GetManager()
	{
		static WuMemoryManagement instance;

		return instance;
	}

	PVOID WuMemoryManagement::Allocate(size_t size)
	{
		PVOID block = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
		if (NULL == block)
			throw std::system_error(std::error_code(static_cast<int>(ERROR_NOT_ENOUGH_MEMORY), std::generic_category()));

		MemoryList.push_back(block);

		return block;
	}

	VOID WuMemoryManagement::Free(PVOID block)
	{
		if (IsRegistered(block))
		{
			std::vector<PVOID>::iterator it;
			for (it = MemoryList.begin(); it != MemoryList.end(); it++)
			{
				if (*it == block)
				{
					HeapFree(GetProcessHeap(), NULL, block);
					MemoryList.erase(it);
					break;
				}
			}
		}
	}

	BOOL WuMemoryManagement::IsRegistered(PVOID block)
	{
		if (NULL == block)
			return FALSE;

		for (PVOID regblock : MemoryList)
			if (regblock == block)
				return TRUE;

		return FALSE;
	}

	WuMemoryManagement::~WuMemoryManagement()
	{
		for (PVOID block : MemoryList)
			HeapFree(GetProcessHeap(), NULL, block);
	}

	// FDI macro functions.

	FNALLOC(FdiFnMemAloc)
	{
		return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
	}
	FNFREE(FdiFnMemFree)
	{
		HeapFree(GetProcessHeap(), NULL, pv);
	}
	FNOPEN(FdiFnFileOpen)
	{
		HANDLE hFile = NULL;
		DWORD dwDesiredAccess = 0;
		DWORD dwCreationDisposition = 0;

		UNREFERENCED_PARAMETER(pmode);

		switch (oflag)
		{
		case _O_RDWR:
			dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			break;

		case _O_WRONLY:
			dwDesiredAccess = GENERIC_WRITE;
			break;

		default:
			dwDesiredAccess = GENERIC_READ;
			break;
		}

		if (oflag & _O_CREAT)
		{
			dwCreationDisposition = CREATE_ALWAYS;
		}
		else
		{
			dwCreationDisposition = OPEN_EXISTING;
		}

		hFile = CreateFileA(pszFile,
			dwDesiredAccess,
			FILE_SHARE_READ,
			NULL,
			dwCreationDisposition,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		return (INT_PTR)hFile;
	}
	FNREAD(FdiFnFileRead)
	{
		DWORD dwBytesRead = 0;

		if (ReadFile((HANDLE)hf, pv, cb, &dwBytesRead, NULL) == FALSE)
			dwBytesRead = (DWORD)-1L;
		
		return dwBytesRead;
	}
	FNWRITE(FdiFnFileWrite)
	{
		DWORD dwBytesWritten = 0;

		if (WriteFile((HANDLE)hf, pv, cb, &dwBytesWritten, NULL) == FALSE)
			dwBytesWritten = (DWORD)-1;

		return dwBytesWritten;
	}
	FNCLOSE(FdiFnFileClose)
	{
		return (CloseHandle((HANDLE)hf) == TRUE) ? 0 : -1;
	}
	FNSEEK(FdiFnFileSeek)
	{
		return SetFilePointer((HANDLE)hf, dist, NULL, seektype);
	}
	
	FNFDINOTIFY(FdiFnNotifyCallback)
	{
		switch (fdint)
		{
		case fdintCABINET_INFO:
			break;

		default:
			break;
		}
		return 0;
	}
}