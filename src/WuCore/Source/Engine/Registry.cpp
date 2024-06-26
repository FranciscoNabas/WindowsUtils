#include "../../pch.h"

#include "../../Headers/Engine/Registry.h"
#include "../../Headers/Support/WuStdException.h"

namespace WindowsUtils::Core
{
    void Registry::GetRegistryKeyValue(
        const WWuString& computerName,  // The computer name to get the value from.
        const HKEY hRootKey,            // The hive.
        const WWuString& subKey,        // The subkey path.
        const WWuString& valueName,     // The key value name, or property name.
        DWORD type,                     // The return data type.
        wuunique_ha_ptr<void>& data,    // The pointer that receives the data.
        DWORD bytesReturned             // The number of bytes returned.
    )
    {
        LSTATUS result = ERROR_SUCCESS;
        HKEY hRegistry;

        result = RegConnectRegistry(computerName.GetBuffer(), hRootKey, &hRegistry);
        if (result != ERROR_SUCCESS)
            throw WuStdException(result, __FILEW__, __LINE__);

        result = RegGetValue(hRegistry, subKey.GetBuffer(), valueName.GetBuffer(), RRF_RT_ANY, &type, NULL, &bytesReturned);
        if (result != ERROR_SUCCESS) {
            if (result != ERROR_MORE_DATA) {
                RegCloseKey(hRegistry);
                throw WuStdException(result, __FILEW__, __LINE__);
            }
        }

        data = make_wuunique_ha<void>(bytesReturned);
        result = RegGetValue(hRegistry, subKey.GetBuffer(), valueName.GetBuffer(), RRF_RT_ANY, &type, data.get(), &bytesReturned);

        RegCloseKey(hRegistry);
    }

    void Registry::GetRegistryKeyValue(HKEY hRegistry, const WWuString& subKey, const WWuString& valueName, DWORD dwType, wuunique_ha_ptr<void>& data, DWORD bytesReturned)
    {
        LSTATUS result = ERROR_SUCCESS;

        result = RegGetValue(hRegistry, subKey.GetBuffer(), valueName.GetBuffer(), RRF_RT_ANY, &dwType, NULL, &bytesReturned);
        if (result != ERROR_SUCCESS)
            throw WuStdException(result, __FILEW__, __LINE__);

        data = make_wuunique_ha<void>(bytesReturned);
        result = RegGetValue(hRegistry, subKey.GetBuffer(), valueName.GetBuffer(), RRF_RT_ANY, &dwType, data.get(), &bytesReturned);
        if (result != ERROR_SUCCESS)
            throw WuStdException(result, __FILEW__, __LINE__);

        RegCloseKey(hRegistry);
    }

    void Registry::GetRegistrySubkeyNames(
        const WWuString& computerName,         // The computer name to get the value from.
        const HKEY hRootKey,                  // The hive.
        const WWuString& subKey,               // The subkey path.
        DWORD options,                        // 0 for normal keys, REG_OPTION_OPEN_LINK for sym link keys.
        wuvector<WWuString>* subkeyNames       // The vector that receives all subkey names. This operation is not recursive.
    )
    {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwSubkeyNameSz = MAX_KEY_LENGTH;
        WWuString persName;
        HKEY hRegistry;
        HKEY hSubKey;

        result = RegConnectRegistry(computerName.GetBuffer(), hRootKey, &hRegistry);
        if (result != ERROR_SUCCESS)
            throw WuStdException(result, __FILEW__, __LINE__);

        result = RegOpenKeyEx(hRegistry, subKey.GetBuffer(), options, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS) {
            RegCloseKey(hRegistry);
            throw WuStdException(result, __FILEW__, __LINE__);
        }

        std::unique_ptr<BYTE[]> buffer;
        DWORD currentIndex = 0;
        do {
            dwSubkeyNameSz = MAX_KEY_LENGTH;
            buffer = std::make_unique<BYTE[]>(MAX_KEY_LENGTH);

            result = RegEnumKeyEx(hSubKey, currentIndex, (LPWSTR)buffer.get(), &dwSubkeyNameSz, NULL, NULL, NULL, NULL);
            if (result != ERROR_SUCCESS && result != ERROR_NO_MORE_ITEMS) {
                RegCloseKey(hSubKey);
                RegCloseKey(hRegistry);
                throw WuStdException(result, __FILEW__, __LINE__);
            }

            if (result == ERROR_NO_MORE_ITEMS)
                break;

            persName = reinterpret_cast<LPWSTR>(buffer.get());
            subkeyNames->push_back(persName);
            currentIndex++;

        } while (result != ERROR_NO_MORE_ITEMS);

        if (result == ERROR_NO_MORE_ITEMS) { result = ERROR_SUCCESS; }
        RegCloseKey(hSubKey);
        RegCloseKey(hRegistry);
    }

    void Registry::GetRegistrySubkeyNames(HKEY hRegistry, const WWuString& subKey, DWORD options, wuvector<WWuString>* subkeyNames)
    {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwSubkeyNameSz = MAX_KEY_LENGTH;
        WWuString persName;
        HKEY hSubKey;

        result = RegOpenKeyEx(hRegistry, subKey.GetBuffer(), options, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS)
            throw WuStdException(result, __FILEW__, __LINE__);

        std::unique_ptr<BYTE[]> buffer;
        DWORD currentIndex = 0;
        do {
            dwSubkeyNameSz = MAX_KEY_LENGTH;
            buffer = std::make_unique<BYTE[]>(MAX_KEY_LENGTH);

            result = RegEnumKeyEx(hSubKey, currentIndex, (LPWSTR)buffer.get(), &dwSubkeyNameSz, NULL, NULL, NULL, NULL);
            if (result != ERROR_SUCCESS && result != ERROR_NO_MORE_ITEMS) {
                RegCloseKey(hSubKey);
                throw WuStdException(result, __FILEW__, __LINE__);
            }

            if (result == ERROR_NO_MORE_ITEMS)
                break;

            persName = reinterpret_cast<LPWSTR>(buffer.get());
            subkeyNames->push_back(persName);
            currentIndex++;

        } while (result != ERROR_NO_MORE_ITEMS);

        if (result == ERROR_NO_MORE_ITEMS) { result = ERROR_SUCCESS; }
        RegCloseKey(hSubKey);
    }

    void Registry::GetRegistryKeyValueList(
        const WWuString& computerName,     // The computer name.
        const HKEY hRootKey,               // The hive.
        const WWuString& subKey,           // The subkey path.
        PVALENT valArray,                  // An array of VALENT objects. The 've_valuename' of each object must contain the value name to retrieve the value.
        DWORD valCount,                    // The number of VALENT objects in 'pValArray'.
        wuunique_ha_ptr<void>& dataBuffer  // The buffer that receives the data.
    )
    {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwBuffSize = 0;
        HKEY hRegistry;
        HKEY hSubKey;

        result = RegConnectRegistry(computerName.GetBuffer(), hRootKey, &hRegistry);
        if (result != ERROR_SUCCESS)
            throw WuStdException(result, __FILEW__, __LINE__);

        result = RegOpenKeyEx(hRegistry, subKey.GetBuffer(), KEY_QUERY_VALUE, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS) {
            RegCloseKey(hSubKey);
            RegCloseKey(hRegistry);
            throw WuStdException(result, __FILEW__, __LINE__);
        }

        result = RegQueryMultipleValues(hSubKey, valArray, valCount, NULL, &dwBuffSize);
        if (result != ERROR_SUCCESS) {
            if (result != ERROR_MORE_DATA) {
                RegCloseKey(hSubKey);
                RegCloseKey(hRegistry);
                throw WuStdException(result, __FILEW__, __LINE__);
            }
        }

        dataBuffer = make_wuunique_ha<void>(dwBuffSize);
        result = RegQueryMultipleValues(hSubKey, valArray, valCount, (LPWSTR)dataBuffer.get(), &dwBuffSize);

        RegCloseKey(hSubKey);
        RegCloseKey(hRegistry);
    }

    void Registry::GetRegistryKeyValueList(HKEY hRegistry, const WWuString& subKey, PVALENT valArray, DWORD valCount, wuunique_ha_ptr<void>& dataBuffer)
    {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwBuffSize = 0;
        HKEY hSubKey;

        result = RegOpenKeyEx(hRegistry, subKey.GetBuffer(), KEY_QUERY_VALUE, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS)
            throw WuStdException(result, __FILEW__, __LINE__);

        result = RegQueryMultipleValues(hSubKey, valArray, valCount, NULL, &dwBuffSize);
        if (result != ERROR_SUCCESS) {
            if (result != ERROR_MORE_DATA) {
                RegCloseKey(hSubKey);
                throw WuStdException(result, __FILEW__, __LINE__);
            }
        }

        dataBuffer = make_wuunique_ha<void>(dwBuffSize);
        result = RegQueryMultipleValues(hSubKey, valArray, valCount, (LPWSTR)dataBuffer.get(), &dwBuffSize);

        RegCloseKey(hSubKey);
    }

    void Registry::GetRegistryPathFromNtPath(WWuString& ntPath, WWuString& path)
    {
        if (ntPath.StartsWith(L"\\REGISTRY\\MACHINE")) {
            path = ntPath.Replace(L"\\REGISTRY\\MACHINE", L"HKLM:");
            if (path == L"HKLM:")
                path = L"HKLM:\\";
        }
        else if (ntPath.StartsWith(L"\\REGISTRY\\USER")) {
            WWuString currentUserRoot { L"\\REGISTRY\\USER\\" + AccessControl::GetCurrentTokenUserSid() };
            if (ntPath.StartsWith(currentUserRoot))
                path = ntPath.Replace(currentUserRoot, L"HKCU:");
            else
                path = ntPath.Replace(L"\\REGISTRY\\USER", L"HKU:");

            if (path == L"HKCU:" || path == L"HKU:")
                path += L"\\";
        }
        else
            path = ntPath;
    }
}