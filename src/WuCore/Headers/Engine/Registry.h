#pragma once
#pragma unmanaged

#include "AccessControl.h"
#include "../Support/String.h"
#include "../Support/Expressions.h"

#include <winreg.h>

constexpr WORD MAX_KEY_LENGTH = 255;

namespace WindowsUtils::Core
{
    extern "C" public class __declspec(dllexport) Registry
    {
    public:
        void GetRegistryKeyValue(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, const WWuString& valueName, DWORD type, wuunique_ha_ptr<void>& data, DWORD bytesReturned);
        void GetRegistryKeyValue(HKEY hRegistry, const WWuString& subKey, const WWuString& valueName, DWORD type, wuunique_ha_ptr<void>& data, DWORD bytesReturned);

        void GetRegistryKeyValueList(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer);
        void GetRegistryKeyValueList(HKEY hRegistry, const WWuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer);

        void GetRegistrySubkeyNames(const WWuString& computerName, const HKEY hRootKey, const WWuString& subKey, DWORD options, wuvector<WWuString>* subkeyNames);
        void GetRegistrySubkeyNames(HKEY hRegistry, const WWuString& subKey, DWORD options, wuvector<WWuString>* subkeyNames);

        static void GetRegistryPathFromNtPath(WWuString& ntPath, WWuString& path);
    };
}