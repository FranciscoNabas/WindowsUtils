#include "../../pch.h"

#include "../../Headers/Engine/Registry.h"

namespace WindowsUtils::Core
{
    ScopedBuffer Registry::GetRegistryKeyValue(
        const WWuString* computerName,  // The computer name to get the value from.
        const RegistryHandle& rootKey,  // The hive.
        const WWuString& subKey,        // The subkey path.
        const WWuString& valueName,     // The key value name, or property name.
        PDWORD type
    )
    {
        DWORD bytesNeeded;
        LSTATUS result{ };

        RegistryHandle handle;
        if (computerName) {
            HKEY registry;
            result = RegConnectRegistry(computerName->Raw(), rootKey.Get(), &registry);
            if (result != ERROR_SUCCESS)
                _WU_RAISE_NATIVE_EXCEPTION(result, L"RegConnectRegistry", WriteErrorCategory::ConnectionError);

            handle = RegistryHandle{ registry, true };
        }
        else {
            handle = rootKey;
        }

        result = RegGetValue(handle.Get(), subKey.Raw(), valueName.Raw(), RRF_RT_ANY, type, nullptr, &bytesNeeded);
        if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA)
            _WU_RAISE_NATIVE_EXCEPTION(result, L"RegGetValue", WriteErrorCategory::ReadError);

        ScopedBuffer buffer{ bytesNeeded };
        result = RegGetValue(handle.Get(), subKey.Raw(), valueName.Raw(), RRF_RT_ANY, type, buffer.Get(), &bytesNeeded);

        return buffer;
    }

    void Registry::GetRegistrySubkeyNames(
        const WWuString* computerName,        // The computer name to get the value from.
        const RegistryHandle& rootKey,        // The hive.
        const WWuString& subKeyName,          // The subkey path.
        DWORD options,                        // 0 for normal keys, REG_OPTION_OPEN_LINK for sym link keys.
        WuList<WWuString>& subkeyNames        // The vector that receives all subkey names. This operation is not recursive.
    )
    {
        LSTATUS result{ };
        WWuString persName;

        RegistryHandle handle;
        if (computerName) {
            HKEY registry;
            result = RegConnectRegistry(computerName->Raw(), rootKey.Get(), &registry);
            if (result != ERROR_SUCCESS)
                _WU_RAISE_NATIVE_EXCEPTION(result, L"RegConnectRegistry", WriteErrorCategory::ConnectionError);

            handle = RegistryHandle{ registry, true };
        }
        else {
            handle = rootKey;
        }

        RegistryHandle subKey;
        result = RegOpenKeyEx(handle.Get(), subKeyName.Raw(), options, KEY_READ, &subKey);
        if (result != ERROR_SUCCESS)
            _WU_RAISE_NATIVE_EXCEPTION(result, L"RegOpenKey", WriteErrorCategory::OpenError);

        DWORD currentIndex = 0;
        do {
            DWORD subkeyNameLength = MAX_KEY_LENGTH;
            ScopedBuffer buffer{ subkeyNameLength };
            result = RegEnumKeyEx(subKey.Get(), currentIndex, (LPWSTR)buffer.Get(), &subkeyNameLength, nullptr, nullptr, nullptr, nullptr);
            if (result != ERROR_SUCCESS && result != ERROR_NO_MORE_ITEMS)
                _WU_RAISE_NATIVE_EXCEPTION(result, L"RegEnumKeyEx", WriteErrorCategory::ReadError);

            if (result == ERROR_NO_MORE_ITEMS)
                break;

            persName = reinterpret_cast<LPWSTR>(buffer.Get());
            subkeyNames.Add(persName);
            currentIndex++;

        } while (result != ERROR_NO_MORE_ITEMS);

        if (result == ERROR_NO_MORE_ITEMS) { result = ERROR_SUCCESS; }
    }

    ScopedBuffer Registry::GetRegistryKeyValueList(
        const WWuString* computerName,       // The computer name.
        const RegistryHandle& rootKey,       // The hive.
        const WWuString& subKeyName,         // The subkey path.
        PVALENT valArray,                    // An array of VALENT objects. The 've_valuename' of each object must contain the value name to retrieve the value.
        DWORD valCount
    )
    {
        LSTATUS result{ };

        RegistryHandle handle;
        if (computerName) {
            HKEY registry;
            result = RegConnectRegistry(computerName->Raw(), rootKey.Get(), &registry);
            if (result != ERROR_SUCCESS)
                _WU_RAISE_NATIVE_EXCEPTION(result, L"RegConnectRegistry", WriteErrorCategory::ConnectionError);

            handle = RegistryHandle{ registry, true };
        }
        else {
            handle = rootKey;
        }

        RegistryHandle subKey;
        result = RegOpenKeyEx(handle.Get(), subKeyName.Raw(), KEY_QUERY_VALUE, KEY_READ, &subKey);
        if (result != ERROR_SUCCESS)
            _WU_RAISE_NATIVE_EXCEPTION(result, L"RegOpenKeyEx", WriteErrorCategory::OpenError);

        DWORD bytesNeeded;
        result = RegQueryMultipleValues(subKey.Get(), valArray, valCount, nullptr, &bytesNeeded);
        if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA)
            _WU_RAISE_NATIVE_EXCEPTION(result, L"RegQueryMultipleValues", WriteErrorCategory::ReadError);

        ScopedBuffer buffer{ bytesNeeded };
        result = RegQueryMultipleValues(subKey.Get(), valArray, valCount, (LPWSTR)buffer.Get(), &bytesNeeded);

        return buffer;
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

    void Registry::GetRegistryKeyValueNames(const RegistryHandle& root, LPCWSTR subKey, WuList<WWuString>& valueList)
    {
        LSTATUS status;
        RegistryHandle hKey;

        if (status = RegOpenKeyEx(root.Get(), subKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
            _WU_RAISE_NATIVE_EXCEPTION(status, L"RegOpenKeyEx", WriteErrorCategory::OpenError);

        DWORD index = 0;
        do {
            DWORD charsNeeded = 16383;
            WCHAR currentValueName[16383] { };
            status = RegEnumValue(hKey.Get(), index, currentValueName, &charsNeeded, nullptr, nullptr, nullptr, nullptr);
            if (status != ERROR_SUCCESS && status != ERROR_NO_MORE_ITEMS)
                _WU_RAISE_NATIVE_EXCEPTION(status, L"RegEnumValue", WriteErrorCategory::ReadError);

            if (status == ERROR_NO_MORE_ITEMS)
                break;

            valueList.Add(currentValueName);
            index++;

        } while (status != ERROR_NO_MORE_ITEMS);
    }
}