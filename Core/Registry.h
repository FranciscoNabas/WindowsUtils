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
        LSTATUS GetRegistryKeyValue(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, const WWuString& valueName, DWORD type, wuunique_ha_ptr<void>& data, DWORD bytesReturned);
        LSTATUS GetRegistryKeyValue(HKEY hRegistry, const WWuString& subKey, const WWuString& valueName, DWORD type, wuunique_ha_ptr<void>& data, DWORD bytesReturned);

        LSTATUS GetRegistryKeyValueList(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer);
        LSTATUS GetRegistryKeyValueList(HKEY hRegistry, const WWuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer);
        
        LSTATUS GetRegistrySubkeyNames(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, DWORD options, wuvector<WWuString>* subkeyNames);
        LSTATUS GetRegistrySubkeyNames(HKEY hRegistry, const WWuString& subKey, DWORD options, wuvector<WWuString>* subkeyNames);
    };
}