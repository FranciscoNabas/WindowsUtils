#pragma once
#pragma unmanaged

#include "../Support/WuString.h"
#include "../Support/SafeHandle.h"
#include "../Support/WuException.h"
#include "../Support/ScopedBuffer.h"

#include <sddl.h>
#include <map>
#include <memory>

constexpr DWORD SE_PRIVILEGE_NONE      = 0x00000000;
constexpr DWORD SE_PRIVILEGE_DISABLED  = 0x00000000;

namespace WindowsUtils::Core
{
    class PrivilegeCookie
    {
    public:
        PrivilegeCookie()
            : m_bufferSize{ }, m_token{ }, m_enabledPrivileges{ nullptr } { }

        ~PrivilegeCookie()
        {
            if (m_enabledPrivileges) {
                for (DWORD i = 0; i < m_enabledPrivileges->PrivilegeCount; i++)
                    m_enabledPrivileges->Privileges[i].Attributes = SE_PRIVILEGE_DISABLED;

                AdjustTokenPrivileges(m_token.Get(), FALSE, m_enabledPrivileges, m_bufferSize, nullptr, nullptr);

                HeapFree(GetProcessHeap(), 0, m_enabledPrivileges);
                m_enabledPrivileges = nullptr;
            }
        }

        template<std::same_as<const wchar_t*>... Args>
        static PrivilegeCookie Ensure(Args... privileges)
        {
            constexpr size_t privCount = sizeof...(Args);

            PrivilegeCookie output;
            output.m_bufferSize = sizeof(TOKEN_PRIVILEGES) + (sizeof(LUID_AND_ATTRIBUTES) * (privCount - 1));
            output.m_enabledPrivileges = reinterpret_cast<PTOKEN_PRIVILEGES>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, output.m_bufferSize));

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &output.m_token))
                _WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"OpenProcessToken", WriteErrorCategory::OpenError);

            DWORD bytesNeeded;
            if (!GetTokenInformation(output.m_token.Get(), TOKEN_INFORMATION_CLASS::TokenPrivileges, nullptr, 0, &bytesNeeded)) {
                DWORD lastError = GetLastError();
                if (lastError != ERROR_INSUFFICIENT_BUFFER)
                    _WU_RAISE_NATIVE_EXCEPTION(lastError, L"GetTokenInformation", WriteErrorCategory::InvalidResult);
            }

            ScopedBuffer buffer{ bytesNeeded };
            if (!GetTokenInformation(output.m_token.Get(), TOKEN_INFORMATION_CLASS::TokenPrivileges, buffer.Get(), bytesNeeded, &bytesNeeded))
                _WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetTokenInformation", WriteErrorCategory::InvalidResult);

            int index = 0;
            PTOKEN_PRIVILEGES tokenPrivileges = reinterpret_cast<PTOKEN_PRIVILEGES>(buffer.Get());
            ([&] {
                if (privileges) {
                    bool found = false;
                    for (DWORD i = 0; i < tokenPrivileges->PrivilegeCount; i++) {
                        ULONG privNameBufferLen = 260;
                        WCHAR privString[260]{ };
                        if (!LookupPrivilegeName(nullptr, &tokenPrivileges->Privileges[i].Luid, privString, &privNameBufferLen))
                            _WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"LookupPrivilegeName", WriteErrorCategory::InvalidResult);

                        if (wcscmp(privString, privileges) == 0 &&
                            (tokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) != SE_PRIVILEGE_ENABLED
                            && (tokenPrivileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) != SE_PRIVILEGE_ENABLED_BY_DEFAULT) {
                            output.m_enabledPrivileges->Privileges[index] = tokenPrivileges->Privileges[i];
                            output.m_enabledPrivileges->Privileges[index].Attributes = SE_PRIVILEGE_ENABLED;
                            found = true;
                            index++;

                            break;
                        }
                    }

                    if (!found)
                        _WU_RAISE_NATIVE_EXCEPTION(ERROR_PRIVILEGE_NOT_HELD, L"GetTokenInformation", WriteErrorCategory::InvalidResult);
                }
            }(), ...);

            output.m_enabledPrivileges->PrivilegeCount = privCount;
            if (!AdjustTokenPrivileges(output.m_token.Get(), FALSE, output.m_enabledPrivileges, output.m_bufferSize, nullptr, nullptr))
                _WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"AdjustTokenPrivileges", WriteErrorCategory::InvalidResult);

            return output;
        }

    private:
        DWORD m_bufferSize;
        SafeObjectHandle m_token;
        PTOKEN_PRIVILEGES m_enabledPrivileges;
    };

    class AccessControl
    {
    public:
        /// <summary>
        /// Gets the current logged on user SID in the string format.
        /// </summary>
        /// <returns>The current user SID as a string.</returns>
        static WWuString GetCurrentTokenUserSid();
    };
}