#pragma once
#pragma unmanaged

#include "String.h"
#include "MemoryManagement.h"

#define SE_PRIVILEGE_DISABLED 0x00000000L
#define SE_PRIVILEGE_NONE 0x00000000L

namespace WindowsUtils::Core
{
    extern "C" public class __declspec(dllexport) AccessControl
    {
    public:

        DWORD static GetCurrentTokenPrivileges(std::shared_ptr<TOKEN_PRIVILEGES> tokenPrivileges);
        DWORD static AdjustCurrentTokenPrivilege(SharedVecPtr(WuString)& spvlpPrivilegeNameList, const DWORD dwAttributes);
    };
}