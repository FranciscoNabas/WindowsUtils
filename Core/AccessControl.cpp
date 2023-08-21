#include "pch.h"

#include "AccessControl.h"

namespace WindowsUtils::Core
{
    // Caller needs to call 'LocalFree'.
    WuResult AccessControl::GetCurrentTokenPrivileges(PTOKEN_PRIVILEGES tokenPrivileges)
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

        tokenPrivileges = (PTOKEN_PRIVILEGES)LocalAlloc(LMEM_ZEROINIT, dwBytesNeeded);
        if (!GetTokenInformation(hToken, TokenPrivileges, (LPVOID)tokenPrivileges, dwBytesNeeded, &dwBytesNeeded))
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
        LUID_AND_ATTRIBUTES* luidAndAttr = new LUID_AND_ATTRIBUTES[privilegeCount];
        TOKEN_PRIVILEGES privileges = {
            privilegeCount,
            luidAndAttr[0]
        };
        
        for (size_t i = 0; i < privilegeCount; i++)
        {
            if (!LookupPrivilegeValueW(NULL, spvlpPrivilegeNameList->at(i).GetBuffer(), &privileges.Privileges[i].Luid))
                return WuResult(GetLastError(), __FILEW__, __LINE__);
            
            privileges.Privileges[i].Attributes = dwAttributes;
        }

        if (!AdjustTokenPrivileges(hToken, FALSE, &privileges, 0, NULL, NULL)) {
            delete[] luidAndAttr;
            CloseHandle(hToken);
            
            return WuResult(GetLastError(), __FILEW__, __LINE__);
        }

        delete[] luidAndAttr;
        CloseHandle(hToken);

        return WuResult();
    }
}