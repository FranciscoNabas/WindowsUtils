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
        LPWSTR persName = NULL;
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

        std::unique_ptr<BYTE[]> buffer;
        DWORD currentIndex = 0;
        do
        {
            dwSubkeyNameSz = MAX_KEY_LENGTH;
            buffer = std::make_unique<BYTE[]>(MAX_KEY_LENGTH);

            result = RegEnumKeyEx(hSubKey, currentIndex, (LPWSTR)buffer.get(), &dwSubkeyNameSz, NULL, NULL, NULL, NULL);
            if (result != ERROR_SUCCESS && result != ERROR_NO_MORE_ITEMS)
            {
                RegCloseKey(hSubKey);
                RegCloseKey(hRegistry);
                return result;
            }

            if (result == ERROR_NO_MORE_ITEMS)
                break;

            DWORD szChars = dwSubkeyNameSz + 1;
            persName = (LPWSTR)MemoryManager.Allocate(szChars * 2);
            wcscpy_s(persName, szChars, (LPWSTR)buffer.get());
            vecSubkeyNames.push_back(persName);
            currentIndex++;

        } while (result != ERROR_NO_MORE_ITEMS);

        if (result == ERROR_NO_MORE_ITEMS) { result = ERROR_SUCCESS; }
        RegCloseKey(hSubKey);
        RegCloseKey(hRegistry);

        return result;
    }

    LSTATUS Registry::GetRegistryKeyValueList(
        const LPWSTR& lpszComputerName,    // The computer name.
        const HKEY& hRootKey,              // The hive.
        const LPWSTR& lpszSubKey,          // The subkey path.
        PVALENT pValArray,                 // An array of VALENT objects. The 've_valuename' of each object must contain the value name to retrieve the value.
        DWORD dwValCount,                  // The number of VALENT objects in 'pValArray'.
        LPWSTR& lpDataBuffer               // The buffer that receives the data.
    ) {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwBuffSize = 0;
        HKEY hRegistry;
        HKEY hSubKey;

        WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

        result = RegConnectRegistry(lpszComputerName, hRootKey, &hRegistry);
        DWERRORCHECKV(result);

        result = RegOpenKeyEx(hRegistry, lpszSubKey, KEY_QUERY_VALUE, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS)
        {
            RegCloseKey(hRegistry);
            return result;
        }

        result = RegQueryMultipleValuesW(hSubKey, pValArray, dwValCount, NULL, &dwBuffSize);
        if (result != ERROR_SUCCESS)
        {
            if (result != ERROR_MORE_DATA)
            {
                RegCloseKey(hRegistry);
                return result;
            }
        }

        lpDataBuffer = (LPWSTR)MemoryManager.Allocate(dwBuffSize);
        result = RegQueryMultipleValues(hSubKey, pValArray, dwValCount, lpDataBuffer, &dwBuffSize);
        
        // We don't want to free the buffer because in every VALENT structure in the list,
        // the 've_valueptr' points to the data in that buffer. The registry API is weird man.
        // MemoryManager.Free(lpszBuffer);
        
        RegCloseKey(hRegistry);

        return result;
    }
}