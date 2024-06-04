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
	*	~ Utility functions
	*/

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