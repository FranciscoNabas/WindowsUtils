#include "pch.h"
#include "Unmanaged.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

using namespace std;

namespace Unmanaged
{
	wchar_t* PrintBuffer(wchar_t const* const format, ...)
	{
		va_list args;
		int len;
		wchar_t* _buff;

		va_start(args, format);
		len = _vscwprintf(format, args) + 1;
		_buff = (wchar_t*)malloc(len * sizeof(wchar_t));
		if (NULL != _buff)
		{
			vswprintf_s(_buff, len, format, args);
		}
		va_end(args);
		return _buff;
	}

	vector<Utilities::MessageDumpOutput> Utilities::GetResourceMessageTable(LPTSTR libName)
	{
		DWORD err;
		vector<Utilities::MessageDumpOutput> output;
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
							Utilities::PMessageDumpOutput inner;
							inner = (Utilities::PMessageDumpOutput)LocalAlloc(LMEM_ZEROINIT, sizeof(Utilities::MessageDumpOutput));
							PMESSAGE_RESOURCE_ENTRY messageEntry =
								(PMESSAGE_RESOURCE_ENTRY)((PBYTE)messageTable +
									(DWORD)messageBlock[block].OffsetToEntries + offset);

							if (inner)
							{
								wchar_t* buffer = PrintBuffer(L"%08X", id);
								inner->Id = wstring(buffer);
								free(buffer);

								buffer = PrintBuffer(L"%s", messageEntry->Text);
								inner->Message = wstring(buffer);
								free(buffer);

								output.push_back(*inner);
								LocalFree(inner);
								offset += messageEntry->Length;
							}
						}
					}
				}
			}
			FreeLibrary(hDll);
		}
		else { err = GetLastError(); }

		return output;
	}
}