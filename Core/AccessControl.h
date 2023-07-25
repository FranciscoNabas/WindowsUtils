#pragma once
#pragma unmanaged

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

        DWORD static GetCurrentTokenPrivileges(PTOKEN_PRIVILEGES tokenPrivileges);
        DWORD static AdjustCurrentTokenPrivilege(wuvector<WWuString>* spvlpPrivilegeNameList, const DWORD dwAttributes);
    };
}