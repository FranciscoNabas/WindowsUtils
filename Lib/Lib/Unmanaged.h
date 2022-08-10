#pragma once

#include "pch.h"

class Unmanaged
{
public:
	typedef struct SessionEnumOutput;
	SessionEnumOutput* GetEnumeratedSession(OUT DWORD* rCount, LPWSTR computerName, BOOL onlyActive, BOOL excludeSystemSessions);
};