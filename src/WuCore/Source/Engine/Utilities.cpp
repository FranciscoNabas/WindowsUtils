#include "../../pch.h"

#include "../../Headers/Engine/Utilities.h"

namespace WindowsUtils::Core
{
	/*
	*	~ WU_RESOURCE_MESSAGE_TABLE ~
	*/

	_WU_RESOURCE_MESSAGE_TABLE::_WU_RESOURCE_MESSAGE_TABLE()
		: Id(0) { }

	_WU_RESOURCE_MESSAGE_TABLE::_WU_RESOURCE_MESSAGE_TABLE(DWORD id, LPWSTR message)
		: Id(id), Message(message) { }


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
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetCursorPos", WriteErrorCategory::InvalidResult);

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
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"SendInput", WriteErrorCategory::InvalidResult);
		}

		delete[] pinput;
	}

	/*
	*	~ Get-ResourceMessageTable
	*/

	void Utilities::GetResourceMessageTable(
		std::vector<WU_RESOURCE_MESSAGE_TABLE>& messageTableOut,	// A vector of resource message table objects.
		const WWuString& libName									// The resource path.
	)
	{
		HMODULE hModule = ::LoadLibraryExW(libName.Raw(), NULL, LOAD_LIBRARY_AS_DATAFILE);
		if (NULL == hModule)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"LoadLibraryEx", WriteErrorCategory::InvalidResult);

		HRSRC hResource = ::FindResourceW(hModule, MAKEINTRESOURCE(1), RT_MESSAGETABLE);
		if (NULL == hResource)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"FindResource", WriteErrorCategory::InvalidResult);

		HGLOBAL hLoad = LoadResource(hModule, hResource);
		if (NULL == hLoad)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"LoadResource", WriteErrorCategory::InvalidResult);

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

	void Utilities::GetEnvVariable(const WWuString& variableName, WWuString& value)
	{
		size_t bufferSize = 0;

		_wgetenv_s(&bufferSize, NULL, 0, variableName.Raw());
		if (bufferSize == 0)
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_FILE_NOT_FOUND, L"_wgetenv_s", WriteErrorCategory::InvalidResult);

		std::unique_ptr<WCHAR[]> buffer = std::make_unique<WCHAR[]>(bufferSize);
		_wgetenv_s(&bufferSize, buffer.get(), bufferSize, variableName.Raw());

		value = buffer.get();
	}
}