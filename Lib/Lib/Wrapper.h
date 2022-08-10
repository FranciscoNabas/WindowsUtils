#pragma once

#include <Windows.h>
#include "Unmanaged.h"

class __declspec(dllexport) Wrapper
{
public:
	Unmanaged* ptr;
	
	Wrapper() : ptr(new Unmanaged()) {};
	
	DWORD* rCount = (DWORD*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD));
	LPWSTR computerName = NULL;
	BOOL onlyActive = 0;
	BOOL excludeSystemSessions = 0;

	Unmanaged::SessionEnumOutput* GetEnumeratedSession() {
		return ptr->GetEnumeratedSession(OUT rCount, computerName, onlyActive, excludeSystemSessions);
	};
};