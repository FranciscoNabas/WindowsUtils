#pragma once
#pragma unmanaged

#include <winreg.h>

#include "Common.h"
#include "String.h"
#include "Expressions.h"
#include "MemoryManagement.h"

#define MAX_KEY_LENGTH 255

namespace WindowsUtils::Core
{
    extern "C" public class __declspec(dllexport) Registry
    {
    public:
        WuResult GetRegistryKeyValue(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, const WWuString& valueName, DWORD type, wuunique_ha_ptr<void>& data, DWORD bytesReturned);
        WuResult GetRegistryKeyValue(HKEY hRegistry, const WWuString& subKey, const WWuString& valueName, DWORD type, wuunique_ha_ptr<void>& data, DWORD bytesReturned);

        WuResult GetRegistryKeyValueList(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer);
        WuResult GetRegistryKeyValueList(HKEY hRegistry, const WWuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer);
        
        WuResult GetRegistrySubkeyNames(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, DWORD options, wuvector<WWuString>* subkeyNames);
        WuResult GetRegistrySubkeyNames(HKEY hRegistry, const WWuString& subKey, DWORD options, wuvector<WWuString>* subkeyNames);
    };
}