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
	
	/*========================================
	==		Utility function definition		==
	==========================================*/

	BOOL EndsWith(const LPWSTR& inputstr, const LPWSTR& comparestr)
	{
		LPCWSTR pdest = wcsstr(inputstr, comparestr);
		if (NULL == pdest)
			return FALSE;
		else
			return TRUE;
	}

	// Write formatted output in a buffer. The memory allocation is what makes this function needed.
	VOID PrintBufferW(LPWSTR& lpbuffer,  const WCHAR* const format, ...)
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
	DWORD GetEnvVariable(LPCWSTR& rlpcvarname, LPWSTR& rlpvalue)
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
}