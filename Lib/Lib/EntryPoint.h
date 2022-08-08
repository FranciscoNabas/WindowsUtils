#pragma once

#include <vector>

using namespace std;

//Terminal services exports
extern "C" __declspec(dllexport) typedef struct SessionEnumOutput;
extern __declspec(dllexport) SessionEnumOutput* GetEnumeratedSession(LPWSTR computerName, BOOL onlyActive, BOOL excludeSystemSessions);