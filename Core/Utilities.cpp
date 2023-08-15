#include "pch.h"

#include "Utilities.h"

#pragma comment(lib, "Psapi")

namespace WindowsUtils::Core
{
	/*========================================
	==		 Main function definition		==
	==========================================*/

	// Get-FormattedMessage
	DWORD Utilities::GetFormattedError(
		DWORD errorCode,				// The Win32 error code.
		WWuString& errorMessage			// The output message string.
	) {
		LPWSTR buffer = NULL;
		if (!::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			errorCode,
			0,
			(LPWSTR)&buffer,
			0,
			NULL
		))
			return GetLastError();

		errorMessage = buffer;
		LocalFree(buffer);

		return ERROR_SUCCESS;
	}

	// Get-LastWin32Error
	DWORD Utilities::GetFormattedWin32Error(
		WWuString& errorMessage		// The output message string.
	)
	{
		LPWSTR buffer = NULL;
		if (!::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			GetLastError(),
			0,
			(LPWSTR)&buffer,
			0,
			NULL
		))
			return GetLastError();

		errorMessage = buffer;
		LocalFree(buffer);

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
		wuvector<Utilities::WU_RESOURCE_MESSAGE_TABLE>* messageTableOut,	// A vector of resource message table objects.
		const WWuString& libName												// The resource path.
	) {
		DWORD result = ERROR_SUCCESS;

		HMODULE hModule = ::LoadLibraryExW(libName.GetBuffer(), NULL, LOAD_LIBRARY_AS_DATAFILE);
		if (NULL == hModule)
			return ::GetLastError();

		HRSRC hResource = ::FindResourceW(hModule, MAKEINTRESOURCE(1), RT_MESSAGETABLE);
		if (NULL == hResource)
			return ::GetLastError();

		HGLOBAL hLoad = LoadResource(hModule, hResource);
		if (NULL == hLoad)
			return ::GetLastError();

		PVOID messageTable = LockResource(hLoad);
		DWORD blockNumber = ((PMESSAGE_RESOURCE_DATA)messageTable)->NumberOfBlocks;
		PMESSAGE_RESOURCE_BLOCK messageBlock = ((PMESSAGE_RESOURCE_DATA)messageTable)->Blocks;

		for (DWORD block = 0; block < blockNumber; block++)
		{
			DWORD offset = 0;

			for (DWORD id = messageBlock[block].LowId; id <= messageBlock[block].HighId; id++)
			{
				WU_RESOURCE_MESSAGE_TABLE tableEntryOut;

				PMESSAGE_RESOURCE_ENTRY tableEntry =
					(PMESSAGE_RESOURCE_ENTRY)((PBYTE)messageTable +
						(DWORD)messageBlock[block].OffsetToEntries + offset);

				tableEntryOut.Id = id;
				tableEntryOut.Message = WWuString::Format(L"%ws", tableEntry->Text);

				messageTableOut->push_back(tableEntryOut);
				offset += tableEntry->Length;
			}
		}

		if (NULL != hModule)
			FreeLibrary(hModule);

		return result;
	}

	// Get-MsiProperties
	DWORD Utilities::GetMsiProperties(
		wumap<WWuString, WWuString>* propertyMap,		// A map with the properties and values from the MSI database.
		const WWuString& fileName					// The MSI file path.
	)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD bufferSize = 0;
		MSIHANDLE hDatabase;
		MSIHANDLE hView;
		MSIHANDLE hRecord;

		result = MsiOpenDatabaseW(fileName.GetBuffer(), L"MSIDBOPEN_READONLY", &hDatabase);
		DWERRORCHECKV(result);

		result = MsiDatabaseOpenViewW(hDatabase, L"Select Property, Value From Property", &hView);
		DWERRORCHECKV(result);

		result = MsiViewExecute(hView, NULL);
		DWERRORCHECKV(result);

		do
		{
			result = MsiViewFetch(hView, &hRecord);
			DWERRORCHECKV(result);

			/*
			* First column, property name.
			* Calculating buffer size.
			*/
			result = MsiRecordGetStringW(hRecord, 1, NULL, &bufferSize);
			DWERRORCHECKV(result);

			// \0
			bufferSize++;
			DWORD bytesNeeded = bufferSize * 2;
			wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
			
			result = MsiRecordGetStringW(hRecord, 1, buffer.get(), &bufferSize);
			DWERRORCHECKV(result);

			WWuString property(buffer.get());

			// Second column, value.
			bufferSize = 0;
			result = MsiRecordGetStringW(hRecord, 2, NULL, &bufferSize);
			DWERRORCHECKV(result);

			bufferSize++;
			bytesNeeded = bufferSize * 2;
			buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
			
			result = MsiRecordGetStringW(hRecord, 2, buffer.get(), &bufferSize);
			DWERRORCHECKV(result);
			
			WWuString value(buffer.get());

			// We are using 'emplace' here because both strings created in this iteration will
			// be passed as parameter for the constructors from the strings inside the map.
			// The constructor will take care of the copy, and the destructor will deallocate
			// both strings created in this iteration.
			propertyMap->emplace(property, value);

		} while (hRecord != 0);

		return result;
	}
	
	/*========================================
	==		Utility function definition		==
	==========================================*/

	// Gets extended error information for Get-MsiProperties
	DWORD Utilities::GetMsiExtendedError(WWuString& errorMessage)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD bufferSize = 0;

		MSIHANDLE hLastError = MsiGetLastErrorRecord();
		if (hLastError == NULL)
			return GetLastError();

		result = MsiFormatRecordW(NULL, hLastError, NULL, &bufferSize);
		if (result != ERROR_SUCCESS)
			return result;

		bufferSize++;
		DWORD bytesNeeded = bufferSize * 2;
		wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);

		result = MsiFormatRecordW(NULL, hLastError, buffer.get(), &bufferSize);

		errorMessage = buffer.get();

		return result;
	}

	// Helper function to retrieve environment variables safely.
	DWORD GetEnvVariable(const WWuString& variableName, WWuString& value)
	{
		DWORD result = ERROR_SUCCESS;
		size_t bufferSize = 0;

		_wgetenv_s(&bufferSize, NULL, 0, variableName.GetBuffer());
		if (bufferSize == 0)
			return ERROR_FILE_NOT_FOUND;

		size_t bytesNeeded = bufferSize * 2;
		wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
		
		_wgetenv_s(&bufferSize, buffer.get(), bufferSize, variableName.GetBuffer());

		value = buffer.get();

		return result;
	}
}