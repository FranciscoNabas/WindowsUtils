#pragma once
#pragma unmanaged

#include "AccessControl.h"

#include "../Support/WuString.h"
#include "../Support/Expressions.h"
#include "../Support/SafeHandle.h"
#include "../Support/WuException.h"

#include <winreg.h>
#include <memory>
#include <vector>

constexpr WORD MAX_KEY_LENGTH = 255;

namespace WindowsUtils::Core
{
    class Registry
    {
    public:
        static ScopedBuffer GetRegistryKeyValue(const WWuString* computerName, const RegistryHandle& hRootKey, const WWuString& subKey, const WWuString& valueName, PDWORD type);
        static ScopedBuffer GetRegistryKeyValueList(const WWuString* computerName, const RegistryHandle& hRootKey, const WWuString& subKey, PVALENT valArray, DWORD valCount);
        static void GetRegistrySubkeyNames(const WWuString* computerName, const RegistryHandle& hRootKey, const WWuString& subKey, DWORD options, WuList<WWuString>& subkeyNames);

        static void GetRegistryPathFromNtPath(WWuString& ntPath, WWuString& path);
        static void GetRegistryKeyValueNames(const RegistryHandle& root, LPCWSTR subKey, WuList<WWuString>& valueList);
    };
}