#include "pch.h"
#include "Registry.h"

namespace WindowsUtils::Core
{
    LSTATUS Registry::GetRegistryKeyValue(
        const LPWSTR& lpszComputerName,   // The computer name to get the value from.
        const HKEY& hRootKey,             // The hive.
        const LPWSTR& lpszSubKey,         // The subkey path.
        const LPWSTR& lpszValueName,      // The key value name, or property name.
        DWORD& dwType,                    // The return data type.
        PVOID& pvData,                    // The pointer that receives the data.
        DWORD& dwBytesReturned            // The number of bytes returned.
    ) {
        LSTATUS result = ERROR_SUCCESS;
        HKEY hRegistry;

        WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

        result = RegConnectRegistry(lpszComputerName, hRootKey, &hRegistry);
        DWERRORCHECKV(result);

        result = RegGetValue(hRegistry, lpszSubKey, lpszValueName, RRF_RT_ANY, &dwType, NULL, &dwBytesReturned);
        if (result != ERROR_SUCCESS)
        {
            if (result != ERROR_MORE_DATA)
            {
                RegCloseKey(hRegistry);
                return result;
            }
        }

        pvData = (PVOID)MemoryManager.Allocate(dwBytesReturned);
        result = RegGetValue(hRegistry, lpszSubKey, lpszValueName, RRF_RT_ANY, &dwType, pvData, &dwBytesReturned);

        RegCloseKey(hRegistry);
        return result;
    }

    LSTATUS Registry::GetRegistrySubkeyNames(
        const LPWSTR& lpszComputerName,         // The computer name to get the value from.
        const HKEY& hRootKey,                   // The hive.
        const LPWSTR& lpszSubKey,               // The subkey path.
        DWORD dwOptions,                        // 0 for normal keys, REG_OPTION_OPEN_LINK for sym link keys.
        std::vector<LPWSTR>& vecSubkeyNames     // The vector that receives all subkey names. This operation is not recursive.
    ) {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwSubkeyNameSz = MAX_KEY_LENGTH;
        WCHAR szSubkeyName[MAX_KEY_LENGTH];
        HKEY hRegistry;
        HKEY hSubKey;

        WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

        result = RegConnectRegistry(lpszComputerName, hRootKey, &hRegistry);
        DWERRORCHECKV(result);

        result = RegOpenKeyEx(hRegistry, lpszSubKey, dwOptions, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS)
        {
            RegCloseKey(hRegistry);
            return result;
        }

        DWORD currentIndex = 0;
        do
        {
            dwSubkeyNameSz = MAX_KEY_LENGTH;
            result = RegEnumKeyEx(hSubKey, currentIndex, szSubkeyName, &dwSubkeyNameSz, NULL, NULL, NULL, NULL);
            if (result != ERROR_SUCCESS && result != ERROR_NO_MORE_ITEMS)
            {
                RegCloseKey(hSubKey);
                RegCloseKey(hRegistry);
                return result;
            }
            if (result == ERROR_NO_MORE_ITEMS)
                break;

            DWORD realNameSz = dwSubkeyNameSz + 1;
            LPWSTR persName = (LPWSTR)MemoryManager.Allocate(realNameSz);
            wcscpy_s(persName, realNameSz, szSubkeyName);
            vecSubkeyNames.push_back(persName);
            currentIndex++;

        } while (result != ERROR_NO_MORE_ITEMS);

        if (result == ERROR_NO_MORE_ITEMS) { result = ERROR_SUCCESS; }
        RegCloseKey(hSubKey);
        RegCloseKey(hRegistry);

        return result;
    }
}