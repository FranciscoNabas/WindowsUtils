#pragma once
#pragma unmanaged

#include <sddl.h>

#include "../Support/Expressions.h"
#include "../Support/String.h"
#include "../Support/SafeHandle.h"

constexpr DWORD SE_PRIVILEGE_DISABLED = 0x00000000L;
constexpr DWORD SE_PRIVILEGE_NONE = 0x00000000;

namespace WindowsUtils::Core
{
    extern "C" public class __declspec(dllexport) AccessControl
    {
    public:

        /// <summary>
        /// Gets the list of token privileges for the current process token.
        /// </summary>
        /// <param name="tokenPrivileges">The token list.</param>
        static void GetCurrentTokenPrivileges(wuunique_ha_ptr<TOKEN_PRIVILEGES>& tokenPrivileges);
        
        /// <summary>
        /// This function attempts to adjust the current token privileges.
        /// </summary>
        /// <param name="privilegeNameList">The list of token privileges to adjust.</param>
        /// <param name="dwAttributes">The attributes</param>
        static void AdjustCurrentTokenPrivilege(wuvector<WWuString>& spvlpPrivilegeNameList, const DWORD dwAttributes);

        static WWuString GetCurrentTokenUserSid();
    };
}