#pragma once
#pragma unmanaged

#include <winreg.h>

#include "String.h"
#include "Expressions.h"
#include "MemoryManagement.h"

#define MAX_KEY_LENGTH 255

namespace WindowsUtils::Core
{
    extern "C" public class __declspec(dllexport) Registry
    {
    public:
        LSTATUS GetRegistryKeyValue(const WuString& computerName, const HKEY hRootKey, const WuString& subKey, const WuString& valueName, DWORD type, wuunique_ha_ptr<void>& data, DWORD bytesReturned);
        LSTATUS GetRegistryKeyValue(HKEY hRegistry, const WuString& subKey, const WuString& valueName, DWORD type, wuunique_ha_ptr<void>& data, DWORD bytesReturned);

        LSTATUS GetRegistryKeyValueList(const WuString& computerName, const HKEY hRootKey, const WuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer);
        LSTATUS GetRegistryKeyValueList(HKEY hRegistry, const WuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer);
        
        LSTATUS GetRegistrySubkeyNames(const WuString& computerName, const HKEY hRootKey, const WuString& subKey, DWORD options, wuvector<WuString>* subkeyNames);
        LSTATUS GetRegistrySubkeyNames(HKEY hRegistry, const WuString& subKey, DWORD options, wuvector<WuString>* subkeyNames);
    };
}