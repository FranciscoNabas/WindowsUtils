#include "../../pch.h"

#include "../../Headers/Engine/Utilities.h"
#include "../../Headers/Support/WuStdException.h"

#include <Msi.h>
#include <MsiQuery.h>

namespace WindowsUtils::Core
{
	/*
	*	~ Get-LastWin32Error
	*/

	void Utilities::GetFormattedWin32Error(
		WWuString& errorMessage		// The output message string.
	)
	{
		LPWSTR buffer { };
		if (!::FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			GetLastError(),
			0,
			(LPWSTR)&buffer,
			0,
			NULL
		))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		errorMessage = buffer;
		LocalFree(buffer);
	}

	/*
	*	~ Send-Click
	*/

	void Utilities::SendClick()
	{
		UINT usendin { };
		UINT utries { };
		POINT pointpos { };
		LPINPUT pinput = new INPUT[2] { };

		if (!GetCursorPos(&pointpos))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		pinput[0].type, pinput[1].type = INPUT_MOUSE;
		pinput[0].mi.dx, pinput[1].mi.dx = pointpos.x;
		pinput[0].mi.dy, pinput[1].mi.dy = pointpos.y;
		pinput[0].mi.mouseData, pinput[1].mi.mouseData = 0;

		pinput[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		pinput[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		do {
			usendin = ::SendInput(2, pinput, sizeof(INPUT));
			if (usendin != 0)
				break;

			Sleep(20);

		} while (usendin == 0 && utries++ < 3);

		if (usendin == 0) {
			delete[] pinput;
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		delete[] pinput;
	}

	/*
	*	~ Get-ResourceMessageTable
	*/

	void Utilities::GetResourceMessageTable(
		wuvector<WU_RESOURCE_MESSAGE_TABLE>& messageTableOut,	// A vector of resource message table objects.
		const WWuString& libName								// The resource path.
	)
	{
		HMODULE hModule = ::LoadLibraryExW(libName.GetBuffer(), NULL, LOAD_LIBRARY_AS_DATAFILE);
		if (NULL == hModule)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		HRSRC hResource = ::FindResourceW(hModule, MAKEINTRESOURCE(1), RT_MESSAGETABLE);
		if (NULL == hResource)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		HGLOBAL hLoad = LoadResource(hModule, hResource);
		if (NULL == hLoad)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		PVOID messageTable = LockResource(hLoad);
		DWORD blockNumber = ((PMESSAGE_RESOURCE_DATA)messageTable)->NumberOfBlocks;
		PMESSAGE_RESOURCE_BLOCK messageBlock = ((PMESSAGE_RESOURCE_DATA)messageTable)->Blocks;

		for (DWORD block = 0; block < blockNumber; block++) {
			DWORD offset = 0;

			for (DWORD id = messageBlock[block].LowId; id <= messageBlock[block].HighId; id++) {
				WU_RESOURCE_MESSAGE_TABLE tableEntryOut;

				PMESSAGE_RESOURCE_ENTRY tableEntry =
					(PMESSAGE_RESOURCE_ENTRY)((PBYTE)messageTable +
						(DWORD)messageBlock[block].OffsetToEntries + offset);

				tableEntryOut.Id = id;
				tableEntryOut.Message = WWuString::Format(L"%ws", tableEntry->Text);

				messageTableOut.push_back(tableEntryOut);
				offset += tableEntry->Length;
			}
		}

		if (NULL != hModule)
			FreeLibrary(hModule);
	}

	/*
	*	~ Get-MsiProperties
	*/

	void Utilities::GetMsiProperties(
		wumap<WWuString, WWuString>& propertyMap,	// A map with the properties and values from the MSI database.
		const WWuString& fileName					// The MSI file path.
	)
	{
		DWORD dwResult;
		DWORD bufferSize = 0;
		PMSIHANDLE hDatabase;
		PMSIHANDLE hView;
		PMSIHANDLE hRecord;

		dwResult = MsiOpenDatabaseW(fileName.GetBuffer(), L"MSIDBOPEN_READONLY", &hDatabase);
		if (dwResult != ERROR_SUCCESS)
			throw WuStdException(dwResult, __FILEW__, __LINE__);

		dwResult = MsiDatabaseOpenViewW(hDatabase, L"Select Property, Value From Property", &hView);
		if (dwResult != ERROR_SUCCESS)
			throw WuStdException(dwResult, __FILEW__, __LINE__);

		dwResult = MsiViewExecute(hView, NULL);
		if (dwResult != ERROR_SUCCESS)
			throw WuStdException(dwResult, __FILEW__, __LINE__);

		do {
			dwResult = MsiViewFetch(hView, &hRecord);
			if (dwResult != ERROR_SUCCESS)
				throw WuStdException(dwResult, __FILEW__, __LINE__);

			/*
			* First column, property name.
			* Calculating buffer size.
			*/
			dwResult = MsiRecordGetStringW(hRecord, 1, NULL, &bufferSize);
			if (dwResult != ERROR_SUCCESS)
				throw WuStdException(dwResult, __FILEW__, __LINE__);

			// \0
			bufferSize++;
			DWORD bytesNeeded = bufferSize * 2;
			wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);

			dwResult = MsiRecordGetStringW(hRecord, 1, buffer.get(), &bufferSize);
			if (dwResult != ERROR_SUCCESS)
				throw WuStdException(dwResult, __FILEW__, __LINE__);

			WWuString property(buffer.get());

			// Second column, value.
			bufferSize = 0;
			dwResult = MsiRecordGetStringW(hRecord, 2, NULL, &bufferSize);
			if (dwResult != ERROR_SUCCESS)
				throw WuStdException(dwResult, __FILEW__, __LINE__);

			bufferSize++;
			bytesNeeded = bufferSize * 2;
			buffer = make_wuunique_ha<WCHAR>(bytesNeeded);

			dwResult = MsiRecordGetStringW(hRecord, 2, buffer.get(), &bufferSize);
			if (dwResult != ERROR_SUCCESS)
				throw WuStdException(dwResult, __FILEW__, __LINE__);

			WWuString value(buffer.get());

			// We are using 'emplace' here because both strings created in this iteration will
			// be passed as parameter for the constructors from the strings inside the map.
			// The constructor will take care of the copy, and the destructor will deallocate
			// both strings created in this iteration.
			propertyMap.emplace(property, value);

		} while (hRecord != 0);
	}

	/*
	*	~ Start-ProcessAsUser
	* 
	*	For this function I tried reverse engineering
	*	'runas.exe' to the best of my ability (which is
	*	not much).
	*/

	void Utilities::RunAs(const WWuString& userName, const WWuString& domain, WWuString& password, WWuString& commandLine, WWuString& titleBar)
	{
		WORD processorArch;
		size_t bytesNeeded;
		SYSTEM_INFO sysInfo;

		LPPROC_THREAD_ATTRIBUTE_LIST attrList;

		wuunique_ptr<STARTUPINFOEX>startupInfo = make_wuunique<STARTUPINFOEX>();
		wuunique_ptr<PROCESS_INFORMATION>procInfo = make_wuunique<PROCESS_INFORMATION>();

		GetNativeSystemInfo(&sysInfo);
		switch (sysInfo.wProcessorArchitecture) {
			case 9:
				processorArch = -31132;
				break;
			case 5:
				processorArch = 452;
				break;
			case 12:
				processorArch = -21916;
				break;
			case 0:
				processorArch = 332;
				break;
			default:
				processorArch = 0xFFFF;
				break;
		}

		if (!InitializeProcThreadAttributeList(NULL, 1, 0, &bytesNeeded)) {
			DWORD lastError = GetLastError();
			if (lastError != ERROR_INSUFFICIENT_BUFFER) {
				password.SecureErase();
				throw WuStdException(lastError, __FILEW__, __LINE__);
			}
		}

		// This is an opaque structure, working with smart pointers would be a hassle.
		attrList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytesNeeded);
		if (attrList == NULL) {
			password.SecureErase();
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		if (!InitializeProcThreadAttributeList(attrList, 1, 0, &bytesNeeded)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}


		if (!UpdateProcThreadAttribute(attrList, 0, 0x00020019, &processorArch, 2, NULL, NULL)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		WCHAR currentPath[MAX_PATH] = { 0 };
		GetCurrentDirectoryW(MAX_PATH, currentPath);

		startupInfo->lpAttributeList = attrList;
		startupInfo->StartupInfo.cb = 112;
		startupInfo->StartupInfo.lpTitle = titleBar.GetBuffer();

		if (!CreateProcessWithLogonW(
			userName.GetBuffer(),
			domain.GetBuffer(),
			password.GetBuffer(),
			LOGON_WITH_PROFILE,
			NULL,
			commandLine.GetBuffer(),
			CREATE_UNICODE_ENVIRONMENT,
			NULL,
			currentPath,
			&startupInfo->StartupInfo,
			procInfo.get()
		)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		DeleteProcThreadAttributeList(attrList);
		password.SecureErase();
	}

	/*
	*	~ Utility functions
	*/

	void GetMsiExtendedError(WWuString& errorMessage)
	{
		DWORD dwResult = ERROR_SUCCESS;
		DWORD bufferSize = 0;

		MSIHANDLE hLastError = MsiGetLastErrorRecord();
		if (hLastError == NULL)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		dwResult = MsiFormatRecordW(NULL, hLastError, NULL, &bufferSize);
		if (dwResult != ERROR_SUCCESS)
			throw WuStdException(dwResult, __FILEW__, __LINE__);

		bufferSize++;
		DWORD bytesNeeded = bufferSize * 2;
		wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);

		dwResult = MsiFormatRecordW(NULL, hLastError, buffer.get(), &bufferSize);

		errorMessage = buffer.get();
	}

	void GetEnvVariable(const WWuString& variableName, WWuString& value)
	{
		size_t bufferSize = 0;

		_wgetenv_s(&bufferSize, NULL, 0, variableName.GetBuffer());
		if (bufferSize == 0)
			throw WuStdException(ERROR_FILE_NOT_FOUND, __FILEW__, __LINE__);

		size_t bytesNeeded = bufferSize * 2;
		wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);

		_wgetenv_s(&bufferSize, buffer.get(), bufferSize, variableName.GetBuffer());

		value = buffer.get();
	}
}