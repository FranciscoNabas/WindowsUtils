#include "pch.h"
#include "Unmanaged.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

using namespace std;

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

vector<Unmanaged::MessageDumpOutput> Unmanaged::GetResourceMessageTable(LPTSTR libName)
{
	DWORD err;
	vector<Unmanaged::MessageDumpOutput> output;
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
						Unmanaged::PMessageDumpOutput inner;
						inner = (Unmanaged::PMessageDumpOutput)LocalAlloc(LMEM_ZEROINIT, sizeof(Unmanaged::MessageDumpOutput));
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

vector<DWORD> Unmanaged::InvokeMessage(
	LPWSTR pTitle,
	LPWSTR pMessage,
	DWORD style,
	DWORD timeout,
	BOOL bWait,
	vector<DWORD> sessionId,
	HANDLE session = WTS_CURRENT_SERVER_HANDLE
)
{
	vector<DWORD> output;
	if (sessionId.empty())
	{
		DWORD pCount;
		BOOL enumResult;
		BOOL mesResult;
		shared_ptr<PWTS_SESSION_INFO> sessInfo = make_shared<PWTS_SESSION_INFO>();
		enumResult = WTSEnumerateSessions(session, 0, 1, &*sessInfo, &pCount);

		for (DWORD i = 0; i < pCount; i++)
		{
			DWORD response;
			mesResult = WTSSendMessageW(
				session,
				(*sessInfo)[i].SessionId,
				pTitle,
				(DWORD)wcslen(pTitle) * 2,
				pMessage,
				(DWORD)wcslen(pMessage) * 2,
				style,
				timeout,
				&response,
				bWait
			);

			output.push_back(response);
		}
	}
	else
	{
		BOOL mesResult;

		for (size_t i = 0; i < sessionId.size(); i++)
		{
			DWORD response;
			mesResult = WTSSendMessageW(
				session,
				sessionId.at(i),
				pTitle,
				(DWORD)wcslen(pTitle) * 2,
				pMessage,
				(DWORD)wcslen(pMessage) * 2,
				style,
				timeout,
				&response,
				bWait
			);

			output.push_back(response);
		}
	}

	return output;
}

Unmanaged::SessionEnumOutput GetSessionOutput(HANDLE session, WTS_SESSION_INFO info)
{
	DWORD pBytesReturned = 0;
	LPWSTR ppBuffer = L"";
	WTSQuerySessionInformation(session, info.SessionId, WTSUserName, &ppBuffer, &pBytesReturned);
	
	if (wcslen(ppBuffer) > 0) {
		Unmanaged::SessionEnumOutput result = Unmanaged::SessionEnumOutput(ppBuffer, info.pWinStationName);
		result.SessionId = info.SessionId;
		result.SessionState = (Unmanaged::WtsSessionState)info.State;
		return result;
	}
	else {
		Unmanaged::SessionEnumOutput result = Unmanaged::SessionEnumOutput(L"System", info.pWinStationName);
		result.SessionId = info.SessionId;
		result.SessionState = (Unmanaged::WtsSessionState)info.State;
		return result;
	}
}

void Unmanaged::GetEnumeratedSession(
	vector<Unmanaged::SessionEnumOutput> &ppOutVec,
	HANDLE session = WTS_CURRENT_SERVER_HANDLE,
	BOOL onlyActive = 0,
	BOOL excludeSystemSessions = 0
)
{
	BOOL enumResult = 0;
	DWORD pCount = 0;
	DWORD pLevel = 1;

	PWTS_SESSION_INFO sessionInfo = (PWTS_SESSION_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WTS_SESSION_INFO));

	enumResult = WTSEnumerateSessions(session, 0, 1, &sessionInfo, &pCount);
	if (enumResult == 0) { goto END; }

	switch (onlyActive)
	{
	case 1:
		for (DWORD i = 0; i < pCount; i++)
		{
			WTS_SESSION_INFO innerSes = sessionInfo[i];
			if (innerSes.State == WTSActive)
			{
				Unmanaged::SessionEnumOutput result = GetSessionOutput(session, innerSes);
				ppOutVec.push_back(result);
			}
		}
		break;

	default:
		if (excludeSystemSessions == 0)
		{
			for (DWORD i = 0; i < pCount; i++)
			{
				WTS_SESSION_INFO innerSes = sessionInfo[i];
				Unmanaged::SessionEnumOutput result = GetSessionOutput(session, innerSes);
				ppOutVec.push_back(result);
			}
		}
		else
		{
			for (DWORD i = 0; i < pCount; i++)
			{
				WTS_SESSION_INFO innerSes = sessionInfo[i];
				Unmanaged::SessionEnumOutput result = GetSessionOutput(session, innerSes);
				if (result.UserName != L"System") { ppOutVec.push_back(result); }
			}
		}
		break;
	}



END:
	if (pCount > 0) { WTSFreeMemory(sessionInfo); }
}