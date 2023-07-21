#pragma once
#pragma unmanaged

#include "MemoryManagement.h"

#define SE_PRIVILEGE_DISABLED 0x00000000L
#define SE_PRIVILEGE_NONE 0x00000000L

namespace WindowsUtils::Core
{
    extern "C" public class __declspec(dllexport) AccessControl
    {
    public:

        DWORD static GetCurrentTokenPrivileges(PTOKEN_PRIVILEGES lpTokenPrivileges);
        DWORD static AdjustCurrentTokenPrivilege(SharedVecPtr(LPCWSTR)& spvlpPrivilegeNameList, const DWORD dwAttributes);
    };
}