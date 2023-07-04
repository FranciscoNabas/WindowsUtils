#pragma once
#pragma unmanaged

#include <winreg.h>

#include "Utilities.h"

#define MAX_KEY_LENGTH 255

namespace WindowsUtils::Core
{
    extern "C" public class __declspec(dllexport) Registry
    {
    public:
        LSTATUS GetRegistryKeyValue(const LPWSTR& lpszComputerName, const HKEY& hRootKey, const LPWSTR& lpszSubKey, const LPWSTR& lpszValueName, DWORD& dwType, PVOID& pvData, DWORD& dwBytesReturned);
        LSTATUS GetRegistryKeyValue(HKEY& hRegistry, const LPWSTR& lpszSubKey, const LPWSTR& lpszValueName, DWORD& dwType, PVOID& pvData, DWORD& dwBytesReturned);

        LSTATUS GetRegistryKeyValueList(const LPWSTR& lpszComputerName, const HKEY& hRootKey, const LPWSTR& lpszSubKey, PVALENT pValArray, DWORD dwValCount, LPWSTR& lpDataBuffer);
        LSTATUS GetRegistryKeyValueList(HKEY& hRegistry, const LPWSTR& lpszSubKey, PVALENT pValArray, DWORD dwValCount, LPWSTR& lpDataBuffer);
        
        LSTATUS GetRegistrySubkeyNames(const LPWSTR& lpszComputerName, const HKEY& hRootKey, const LPWSTR& lpszSubKey, DWORD dwOptions, std::vector<LPWSTR>& vecSubkeyNames);
        LSTATUS GetRegistrySubkeyNames(HKEY& hRegistry, const LPWSTR& lpszSubKey, DWORD dwOptions, std::vector<LPWSTR>& vecSubkeyNames);
    };
}