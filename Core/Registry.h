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
        LSTATUS GetRegistrySubkeyNames(const LPWSTR& lpszComputerName, const HKEY& hRootKey, const LPWSTR& lpszSubKey, DWORD dwOptions, std::vector<LPWSTR>& vecSubkeyNames);
    };
}