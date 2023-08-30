#pragma once
#pragma unmanaged

#include "Common.h"
#include "String.h"
#include "Expressions.h"
#include "MemoryManagement.h"

#define SE_PRIVILEGE_DISABLED 0x00000000L
#define SE_PRIVILEGE_NONE 0x00000000L

namespace WindowsUtils::Core
{
    extern "C" public class __declspec(dllexport) AccessControl
    {
    public:

        WuResult static GetCurrentTokenPrivileges(wuunique_ha_ptr<TOKEN_PRIVILEGES>& tokenPrivileges);
        WuResult static AdjustCurrentTokenPrivilege(wuvector<WWuString>* spvlpPrivilegeNameList, const DWORD dwAttributes);
    };
}