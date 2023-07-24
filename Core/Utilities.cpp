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
		WuString& errorMessage			// The output message string.
	) {
		LPWSTR buffer = NULL;
		if (!::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			errorCode,
			0,
			buffer,
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
		WuString& errorMessage		// The output message string.
	)
	{
		LPWSTR buffer = NULL;
		if (!::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			GetLastError(),
			0,
			buffer,
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
		const WuString& libName												// The resource path.
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
				tableEntryOut.Message.Format(L"%ws", tableEntry->Text);

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
		wumap<WuString, WuString>* propertyMap,		// A map with the properties and values from the MSI database.
		const WuString& fileName					// The MSI file path.
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
			WuString property;
			property.Initialize(bufferSize);
			result = MsiRecordGetStringW(hRecord, 1, property.GetBuffer(), &bufferSize);
			DWERRORCHECKV(result);

			// Second column, value.
			bufferSize = 0;
			result = MsiRecordGetStringW(hRecord, 2, NULL, &bufferSize);
			DWERRORCHECKV(result);

			bufferSize++;
			WuString value;
			value.Initialize(bufferSize);
			result = MsiRecordGetStringW(hRecord, 2, value.GetBuffer(), &bufferSize);
			DWERRORCHECKV(result);

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
	DWORD Utilities::GetMsiExtendedError(WuString& errorMessage)
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
		errorMessage.Initialize(bufferSize);

		result = MsiFormatRecordW(NULL, hLastError, errorMessage.GetBuffer(), &bufferSize);

		return result;
	}

	// Helper function to retrieve environment variables safely.
	DWORD GetEnvVariable(const WuString& variableName, WuString& value)
	{
		DWORD result = ERROR_SUCCESS;
		size_t bufferSize = 0;

		_wgetenv_s(&bufferSize, NULL, 0, variableName.GetBuffer());
		if (bufferSize == 0)
			return ERROR_FILE_NOT_FOUND;

		value.Initialize(bufferSize);
		
		_wgetenv_s(&bufferSize, value.GetBuffer(), bufferSize, variableName.GetBuffer());

		return result;
	}
}