#include "pch.h"

#include "AccessControl.h"

namespace WindowsUtils::Core
{
    DWORD AccessControl::GetCurrentTokenPrivileges(PTOKEN_PRIVILEGES lpTokenPrivileges)
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

        lpTokenPrivileges = (PTOKEN_PRIVILEGES)MemoryManager.Allocate(dwBytesNeeded);
        if (!GetTokenInformation(hToken, TokenPrivileges, (LPVOID)lpTokenPrivileges, dwBytesNeeded, &dwBytesNeeded))
        {
            CloseHandle(hToken);
            MemoryManager.Free(lpTokenPrivileges);
            return GetLastError();
        }

        return result;
    }

    DWORD AccessControl::AdjustCurrentTokenPrivilege(SharedVecPtr(LPCWSTR)& spvlpPrivilegeNameList, const DWORD dwAttributes)
    {
        DWORD result = ERROR_SUCCESS;
        HANDLE hToken;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
            return GetLastError();

        DWORD privilegeCount = static_cast<DWORD>(spvlpPrivilegeNameList->size());
        WuAllocator<TOKEN_PRIVILEGES> privAll(sizeof(TOKEN_PRIVILEGES) + (sizeof(LUID_AND_ATTRIBUTES) * privilegeCount));
        
        privAll.Get()->PrivilegeCount = privilegeCount;
        for (size_t i = 0; i < privilegeCount; i++)
        {
            if (!LookupPrivilegeValueW(NULL, spvlpPrivilegeNameList->at(i), &privAll.Get()->Privileges[i].Luid))
                return GetLastError();
            
            privAll.Get()->Privileges[i].Attributes = dwAttributes;
        }

        if (!AdjustTokenPrivileges(hToken, FALSE, privAll.Get(), 0, NULL, NULL))
            result = GetLastError();

        CloseHandle(hToken);

        return result;
    }
}