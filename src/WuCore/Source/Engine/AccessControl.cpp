#include "../../pch.h"

#include "../../Headers/Engine/AccessControl.h"
#include "../../Headers/Support/WuStdException.h"

namespace WindowsUtils::Core
{
    void AccessControl::GetCurrentTokenPrivileges(wuunique_ha_ptr<TOKEN_PRIVILEGES>& tokenPrivileges)
    {
        DWORD result { };
        HANDLE hToken { };
        DWORD dwBytesNeeded { };

        // Opening the current process token.
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE, &hToken))
            throw WuStdException(GetLastError(), __FILEW__, __LINE__);

        // First call to determine the needed buffer size.
        if (!GetTokenInformation(hToken, TokenPrivileges, NULL, NULL, &dwBytesNeeded)) {
            result = GetLastError();
            if (result != ERROR_INSUFFICIENT_BUFFER) {
                CloseHandle(hToken);
                throw WuStdException(result, __FILEW__, __LINE__);
            }

            result = ERROR_SUCCESS;
        }

        // Attempting to get token privilege information.
        tokenPrivileges = make_wuunique_ha<TOKEN_PRIVILEGES>(dwBytesNeeded);
        if (!GetTokenInformation(hToken, TokenPrivileges, (LPVOID)tokenPrivileges.get(), dwBytesNeeded, &dwBytesNeeded)) {
            CloseHandle(hToken);
            throw WuStdException(GetLastError(), __FILEW__, __LINE__);
        }
    }

    void AccessControl::AdjustCurrentTokenPrivilege(wuvector<WWuString>& privilegeNameList, const DWORD dwAttributes)
    {
        HANDLE hToken { };

        // Opening the current process token.
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
            throw WuStdException(GetLastError(), __FILEW__, __LINE__);

        // Going through each privilege in the list.
        DWORD privilegeCount = static_cast<DWORD>(privilegeNameList.size());
        for (size_t i = 0; i < privilegeCount; i++) {
            TOKEN_PRIVILEGES tokenPriv { };
            tokenPriv.PrivilegeCount = 1;

            // Looking up the current privilege value.
            if (!LookupPrivilegeValueW(NULL, privilegeNameList.at(i).GetBuffer(), &tokenPriv.Privileges[0].Luid))
                throw WuStdException(GetLastError(), __FILEW__, __LINE__);

            tokenPriv.Privileges[0].Attributes = dwAttributes;

            // Attempting to adjust the privilege.
            if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, 0, NULL, NULL)) {
                CloseHandle(hToken);

                throw WuStdException(GetLastError(), __FILEW__, __LINE__);
            }
        }

        CloseHandle(hToken);

        // For testing. Checking if the privilege adjustment worked.
        // wuunique_ha_ptr<TOKEN_PRIVILEGES> testPriv;
        // AccessControl::GetCurrentTokenPrivileges(testPriv);
        // for (size_t i = 0; i < testPriv->PrivilegeCount; i++)
        // {
        //     DWORD size = 49;
        //     LPWSTR privName = new WCHAR[50] { 0 };
        //     LookupPrivilegeName(NULL, &testPriv->Privileges[i].Luid, privName, &size);
        //     wprintf(L"%ws\n", privName);
        // }
    }
}