#include "pch.h"
#include "Registry.h"

namespace WindowsUtils::Core
{
    LSTATUS Registry::GetRegistryKeyValue(
        const WuString& computerName,  // The computer name to get the value from.
        const HKEY hRootKey,           // The hive.
        const WuString& subKey,        // The subkey path.
        const WuString& valueName,     // The key value name, or property name.
        DWORD type,                    // The return data type.
        std::shared_ptr<BYTE[]> data,  // The pointer that receives the data.
        DWORD bytesReturned            // The number of bytes returned.
    ) {
        LSTATUS result = ERROR_SUCCESS;
        HKEY hRegistry;

        result = RegConnectRegistry(computerName.GetWideBuffer(), hRootKey, &hRegistry);
        DWERRORCHECKV(result);

        result = RegGetValue(hRegistry, subKey.GetWideBuffer(), valueName.GetWideBuffer(), RRF_RT_ANY, &type, NULL, &bytesReturned);
        if (result != ERROR_SUCCESS)
        {
            if (result != ERROR_MORE_DATA)
            {
                RegCloseKey(hRegistry);
                return result;
            }
        }

        data = std::make_shared<BYTE[]>(bytesReturned);
        result = RegGetValue(hRegistry, subKey.GetWideBuffer(), valueName.GetWideBuffer(), RRF_RT_ANY, &type, data.get(), &bytesReturned);

        RegCloseKey(hRegistry);
        return result;
    }

    LSTATUS Registry::GetRegistryKeyValue(HKEY hRegistry, const WuString& subKey, const WuString& valueName, DWORD dwType, std::shared_ptr<BYTE[]> data, DWORD bytesReturned)
    {
        LSTATUS result = ERROR_SUCCESS;

        result = RegGetValue(hRegistry, subKey.GetWideBuffer(), valueName.GetWideBuffer(), RRF_RT_ANY, &dwType, NULL, &bytesReturned);
        if (result != ERROR_SUCCESS)
            return result;

        data = std::make_shared<BYTE[]>(bytesReturned);
        result = RegGetValue(hRegistry, subKey.GetWideBuffer(), valueName.GetWideBuffer(), RRF_RT_ANY, &dwType, data.get(), &bytesReturned);

        RegCloseKey(hRegistry);
        return result;
    }

    LSTATUS Registry::GetRegistrySubkeyNames(
        const WuString& computerName,         // The computer name to get the value from.
        const HKEY hRootKey,                  // The hive.
        const WuString& subKey,               // The subkey path.
        DWORD options,                        // 0 for normal keys, REG_OPTION_OPEN_LINK for sym link keys.
        std::vector<WuString>& subkeyNames    // The vector that receives all subkey names. This operation is not recursive.
    ) {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwSubkeyNameSz = MAX_KEY_LENGTH;
        WuString persName;
        HKEY hRegistry;
        HKEY hSubKey;

        result = RegConnectRegistry(computerName.GetWideBuffer(), hRootKey, &hRegistry);
        DWERRORCHECKV(result);

        result = RegOpenKeyEx(hRegistry, subKey.GetWideBuffer(), options, KEY_READ, &hSubKey);
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

            persName = reinterpret_cast<LPWSTR>(buffer.get());
            subkeyNames.push_back(persName);
            currentIndex++;

        } while (result != ERROR_NO_MORE_ITEMS);

        if (result == ERROR_NO_MORE_ITEMS) { result = ERROR_SUCCESS; }
        RegCloseKey(hSubKey);
        RegCloseKey(hRegistry);

        return result;
    }

    LSTATUS Registry::GetRegistrySubkeyNames(HKEY hRegistry, const WuString& subKey, DWORD options, std::vector<WuString>& subkeyNames)
    {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwSubkeyNameSz = MAX_KEY_LENGTH;
        WuString persName;
        HKEY hSubKey;

        result = RegOpenKeyEx(hRegistry, subKey.GetWideBuffer(), options, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS)
            return result;

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
                return result;
            }

            if (result == ERROR_NO_MORE_ITEMS)
                break;

            persName = reinterpret_cast<LPWSTR>(buffer.get());
            subkeyNames.push_back(persName);
            currentIndex++;

        } while (result != ERROR_NO_MORE_ITEMS);

        if (result == ERROR_NO_MORE_ITEMS) { result = ERROR_SUCCESS; }
        RegCloseKey(hSubKey);

        return result;
    }

    LSTATUS Registry::GetRegistryKeyValueList(
        const WuString& computerName,      // The computer name.
        const HKEY hRootKey,               // The hive.
        const WuString& subKey,            // The subkey path.
        PVALENT valArray,                  // An array of VALENT objects. The 've_valuename' of each object must contain the value name to retrieve the value.
        DWORD valCount,                    // The number of VALENT objects in 'pValArray'.
        WuString& dataBuffer               // The buffer that receives the data.
    ) {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwBuffSize = 0;
        HKEY hRegistry;
        HKEY hSubKey;

        result = RegConnectRegistry(computerName.GetWideBuffer(), hRootKey, &hRegistry);
        DWERRORCHECKV(result);

        result = RegOpenKeyEx(hRegistry, subKey.GetWideBuffer(), KEY_QUERY_VALUE, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS)
        {
            RegCloseKey(hSubKey);
            RegCloseKey(hRegistry);
            return result;
        }

        result = RegQueryMultipleValues(hSubKey, valArray, valCount, NULL, &dwBuffSize);
        if (result != ERROR_SUCCESS)
        {
            if (result != ERROR_MORE_DATA)
            {
                RegCloseKey(hSubKey);
                RegCloseKey(hRegistry);
                return result;
            }
        }

        dataBuffer.Initialize(dwBuffSize);
        result = RegQueryMultipleValues(hSubKey, valArray, valCount, dataBuffer.GetWideBuffer(), &dwBuffSize);
        
        RegCloseKey(hSubKey);
        RegCloseKey(hRegistry);

        return result;
    }

    LSTATUS Registry::GetRegistryKeyValueList(HKEY hRegistry, const WuString& subKey, PVALENT valArray, DWORD valCount, WuString& dataBuffer)
    {
        LSTATUS result = ERROR_SUCCESS;
        DWORD dwBuffSize = 0;
        HKEY hSubKey;

        WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

        result = RegOpenKeyEx(hRegistry, subKey.GetWideBuffer(), KEY_QUERY_VALUE, KEY_READ, &hSubKey);
        if (result != ERROR_SUCCESS)
            return result;

        result = RegQueryMultipleValues(hSubKey, valArray, valCount, NULL, &dwBuffSize);
        if (result != ERROR_SUCCESS)
        {
            if (result != ERROR_MORE_DATA)
            {
                RegCloseKey(hSubKey);
                return result;
            }
        }

        dataBuffer.Initialize(dwBuffSize);
        result = RegQueryMultipleValues(hSubKey, valArray, valCount, dataBuffer.GetWideBuffer(), &dwBuffSize);

        RegCloseKey(hSubKey);

        return result;
    }
}