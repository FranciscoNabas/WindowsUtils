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

	DWORD Utilities::MapRpcEndpoints(
		vector<Utilities::RpcMapperOutput> &ppOutVec
	)
	{
		RPC_EP_INQ_HANDLE inqContext;
		RPC_STATUS result = 0;
		RPC_BINDING_HANDLE bindingHandle;
		RPC_WSTR annotation;
		RPC_WSTR stringBinding;

		result = RpcMgmtEpEltInqBegin(NULL, RPC_C_EP_ALL_ELTS, 0, 0, 0, &inqContext);
		if (result != RPC_S_OK) { return result; }

		do
		{
			result = RpcMgmtEpEltInqNextW(inqContext, NULL, &bindingHandle, NULL, &annotation);
			if (result != 0 || result == RPC_X_NO_MORE_ENTRIES) { break; }

			result = RpcBindingToStringBindingW(bindingHandle, &stringBinding);
			if (result != 0) { break; }

			ppOutVec.push_back(Utilities::RpcMapperOutput((LPWSTR)stringBinding, (LPWSTR)annotation));

			RpcStringFree(&annotation);
			RpcStringFree(&stringBinding);
			RpcBindingFree(&bindingHandle);

		} while (result != RPC_X_NO_MORE_ENTRIES);

		result = RpcMgmtEpEltInqDone(&inqContext);
		return result;
	}
}