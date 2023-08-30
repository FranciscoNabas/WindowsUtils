#include "..\pch.h"

#include "..\Headers\AccessControl.h"

namespace WindowsUtils::Core
{
    // Caller needs to call 'LocalFree'.
    WuResult AccessControl::GetCurrentTokenPrivileges(wuunique_ha_ptr<TOKEN_PRIVILEGES>& tokenPrivileges)
    {
        DWORD result;
        HANDLE hToken;
        DWORD dwBytesNeeded;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE, &hToken))
            return WuResult(GetLastError(), __FILEW__, __LINE__);

        if (!GetTokenInformation(hToken, TokenPrivileges, NULL, NULL, &dwBytesNeeded))
        {
            result = GetLastError();
            if (result != ERROR_INSUFFICIENT_BUFFER)
            {
                CloseHandle(hToken);
                return WuResult(result, __FILEW__, __LINE__);
            }
            
            result = ERROR_SUCCESS;
        }

        tokenPrivileges = make_wuunique_ha<TOKEN_PRIVILEGES>(dwBytesNeeded);
        if (!GetTokenInformation(hToken, TokenPrivileges, (LPVOID)tokenPrivileges.get(), dwBytesNeeded, &dwBytesNeeded))
        {
            CloseHandle(hToken);
            return WuResult(GetLastError(), __FILEW__, __LINE__);
        }

        return WuResult();
    }

    WuResult AccessControl::AdjustCurrentTokenPrivilege(wuvector<WWuString>* spvlpPrivilegeNameList, const DWORD dwAttributes)
    {
        HANDLE hToken;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
            return WuResult(GetLastError(), __FILEW__, __LINE__);

        DWORD privilegeCount = static_cast<DWORD>(spvlpPrivilegeNameList->size());
        for (size_t i = 0; i < privilegeCount; i++)
        {
            TOKEN_PRIVILEGES tokenPriv = { 0 };
            tokenPriv.PrivilegeCount = 1;
            if (!LookupPrivilegeValueW(NULL, spvlpPrivilegeNameList->at(i).GetBuffer(), &tokenPriv.Privileges[0].Luid))
                return WuResult(GetLastError(), __FILEW__, __LINE__);
            
            tokenPriv.Privileges[0].Attributes = dwAttributes;

            if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, 0, NULL, NULL)) {
                CloseHandle(hToken);

                return WuResult(GetLastError(), __FILEW__, __LINE__);
            }
        }

        CloseHandle(hToken);

        // For testing. Checking if the privilege adjustment worked.
        /*wuunique_ha_ptr<TOKEN_PRIVILEGES> testPriv;
        AccessControl::GetCurrentTokenPrivileges(testPriv);
        for (size_t i = 0; i < testPriv->PrivilegeCount; i++)
        {
            DWORD size = 49;
            LPWSTR privName = new WCHAR[50] { 0 };
            LookupPrivilegeName(NULL, &testPriv->Privileges[i].Luid, privName, &size);
            wprintf(L"%ws\n", privName);
        }*/

        return WuResult();
    }
}