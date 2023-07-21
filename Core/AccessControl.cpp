#include "pch.h"

#include "AccessControl.h"

namespace WindowsUtils::Core
{
    DWORD AccessControl::GetCurrentTokenPrivileges(std::shared_ptr<TOKEN_PRIVILEGES> tokenPrivileges)
    {
        DWORD result = ERROR_SUCCESS;
        HANDLE hToken;
        DWORD dwBytesNeeded;

        WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE, &hToken))
            return GetLastError();

        if (!GetTokenInformation(hToken, TokenPrivileges, NULL, NULL, &dwBytesNeeded))
        {
            result = GetLastError();
            if (result != ERROR_INSUFFICIENT_BUFFER)
            {
                CloseHandle(hToken);
                return result;
            }
            
            result = ERROR_SUCCESS;
        }

        tokenPrivileges = std::make_shared<TOKEN_PRIVILEGES>(dwBytesNeeded);
        if (!GetTokenInformation(hToken, TokenPrivileges, (LPVOID)tokenPrivileges.get(), dwBytesNeeded, &dwBytesNeeded))
        {
            CloseHandle(hToken);
            return GetLastError();
        }

        return result;
    }

    DWORD AccessControl::AdjustCurrentTokenPrivilege(SharedVecPtr(WuString)& spvlpPrivilegeNameList, const DWORD dwAttributes)
    {
        DWORD result = ERROR_SUCCESS;
        HANDLE hToken;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
            return GetLastError();

        DWORD privilegeCount = static_cast<DWORD>(spvlpPrivilegeNameList->size());
        WuAllocator<TOKEN_PRIVILEGES> privAll(sizeof(TOKEN_PRIVILEGES) + (sizeof(LUID_AND_ATTRIBUTES) * privilegeCount));
        
        privAll->PrivilegeCount = privilegeCount;
        for (size_t i = 0; i < privilegeCount; i++)
        {
            if (!LookupPrivilegeValueW(NULL, spvlpPrivilegeNameList->at(i).GetWideBuffer(), &privAll->Privileges[i].Luid))
                return GetLastError();
            
            privAll->Privileges[i].Attributes = dwAttributes;
        }

        if (!AdjustTokenPrivileges(hToken, FALSE, privAll.get(), 0, NULL, NULL))
            result = GetLastError();

        CloseHandle(hToken);

        return result;
    }
}