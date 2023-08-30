#include "..\pch.h"

#include "..\Headers\Utilities.h"


namespace WindowsUtils::Core
{
	/*========================================
	==		 Main function definition		==
	==========================================*/

	// Get-FormattedMessage
	WuResult Utilities::GetFormattedError(
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
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		errorMessage = buffer;
		LocalFree(buffer);

		return WuResult();
	}

	// Get-LastWin32Error
	WuResult Utilities::GetFormattedWin32Error(
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
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		errorMessage = buffer;
		LocalFree(buffer);

		return WuResult();
	}

	// Send-Click
	WuResult Utilities::SendClick()
	{
		WuResult result;
		UINT usendin = 0;
		UINT utries = 0;
		POINT pointpos = { 0 };
		LPINPUT pinput = new INPUT[2]{ 0 };

		if (!GetCursorPos(&pointpos))
			return WuResult(GetLastError(), __FILEW__, __LINE__);

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

			Sleep(20);

		} while (usendin == 0 && utries++ < 3);

		if (usendin == 0)
			result = WuResult(GetLastError(), __FILEW__, __LINE__);

		delete[] pinput;

		return result;
	}

	// Get-ResourceMessageTable
	WuResult Utilities::GetResourceMessageTable(
		wuvector<Utilities::WU_RESOURCE_MESSAGE_TABLE>* messageTableOut,	// A vector of resource message table objects.
		const WWuString& libName												// The resource path.
	) {
		HMODULE hModule = ::LoadLibraryExW(libName.GetBuffer(), NULL, LOAD_LIBRARY_AS_DATAFILE);
		if (NULL == hModule)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		HRSRC hResource = ::FindResourceW(hModule, MAKEINTRESOURCE(1), RT_MESSAGETABLE);
		if (NULL == hResource)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		HGLOBAL hLoad = LoadResource(hModule, hResource);
		if (NULL == hLoad)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

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

		return WuResult();
	}

	// Get-MsiProperties
	WuResult Utilities::GetMsiProperties(
		wumap<WWuString, WWuString>* propertyMap,		// A map with the properties and values from the MSI database.
		const WWuString& fileName					// The MSI file path.
	)
	{
		DWORD dwResult;
		DWORD bufferSize = 0;
		MSIHANDLE hDatabase;
		MSIHANDLE hView;
		MSIHANDLE hRecord;

		dwResult = MsiOpenDatabaseW(fileName.GetBuffer(), L"MSIDBOPEN_READONLY", &hDatabase);
		DWERRORCHECKV(dwResult);

		dwResult = MsiDatabaseOpenViewW(hDatabase, L"Select Property, Value From Property", &hView);
		DWERRORCHECKV(dwResult);

		dwResult = MsiViewExecute(hView, NULL);
		DWERRORCHECKV(dwResult);

		do
		{
			dwResult = MsiViewFetch(hView, &hRecord);
			DWERRORCHECKV(dwResult);

			/*
			* First column, property name.
			* Calculating buffer size.
			*/
			dwResult = MsiRecordGetStringW(hRecord, 1, NULL, &bufferSize);
			DWERRORCHECKV(dwResult);

			// \0
			bufferSize++;
			DWORD bytesNeeded = bufferSize * 2;
			wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
			
			dwResult = MsiRecordGetStringW(hRecord, 1, buffer.get(), &bufferSize);
			DWERRORCHECKV(dwResult);

			WWuString property(buffer.get());

			// Second column, value.
			bufferSize = 0;
			dwResult = MsiRecordGetStringW(hRecord, 2, NULL, &bufferSize);
			DWERRORCHECKV(dwResult);

			bufferSize++;
			bytesNeeded = bufferSize * 2;
			buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
			
			dwResult = MsiRecordGetStringW(hRecord, 2, buffer.get(), &bufferSize);
			DWERRORCHECKV(dwResult);
			
			WWuString value(buffer.get());

			// We are using 'emplace' here because both strings created in this iteration will
			// be passed as parameter for the constructors from the strings inside the map.
			// The constructor will take care of the copy, and the destructor will deallocate
			// both strings created in this iteration.
			propertyMap->emplace(property, value);

		} while (hRecord != 0);

		return WuResult();
	}
	
	/*========================================
	==		Utility function definition		==
	==========================================*/

	// Gets extended error information for Get-MsiProperties
	WuResult Utilities::GetMsiExtendedError(WWuString& errorMessage)
	{
		DWORD dwResult = ERROR_SUCCESS;
		DWORD bufferSize = 0;

		MSIHANDLE hLastError = MsiGetLastErrorRecord();
		if (hLastError == NULL)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		dwResult = MsiFormatRecordW(NULL, hLastError, NULL, &bufferSize);
		DWERRORCHECKV(dwResult);

		bufferSize++;
		DWORD bytesNeeded = bufferSize * 2;
		wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);

		dwResult = MsiFormatRecordW(NULL, hLastError, buffer.get(), &bufferSize);

		errorMessage = buffer.get();

		return WuResult();
	}

	// Helper function to retrieve environment variables safely.
	WuResult GetEnvVariable(const WWuString& variableName, WWuString& value)
	{
		size_t bufferSize = 0;

		_wgetenv_s(&bufferSize, NULL, 0, variableName.GetBuffer());
		if (bufferSize == 0)
			return WuResult(ERROR_FILE_NOT_FOUND, __FILEW__, __LINE__);

		size_t bytesNeeded = bufferSize * 2;
		wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
		
		_wgetenv_s(&bufferSize, buffer.get(), bufferSize, variableName.GetBuffer());

		value = buffer.get();

		return WuResult();
	}
}