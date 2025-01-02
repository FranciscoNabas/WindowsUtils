#include "../../pch.h"

#include "../../Headers/Engine/AccessControl.h"

namespace WindowsUtils::Core
{
    WWuString AccessControl::GetCurrentTokenUserSid()
    {
        ObjectHandle hToken;
        DWORD bytesNeeded;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_QUERY_SOURCE, &hToken))
            _WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"OpenProcessToken", WriteErrorCategory::OpenError);

        // First call to determine the needed buffer size.
        // First call to determine the needed buffer size.
        if (!GetTokenInformation(hToken.Get(), ::TokenPrivileges, nullptr, 0, &bytesNeeded)) {
            DWORD result = GetLastError();
            if (result != ERROR_INSUFFICIENT_BUFFER)
                _WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetTokenInformation", WriteErrorCategory::InvalidResult);

            result = ERROR_SUCCESS;
        }

        ScopedBuffer buffer{ bytesNeeded };
        if (!GetTokenInformation(hToken.Get(), TOKEN_INFORMATION_CLASS::TokenUser, buffer.Get(), bytesNeeded, &bytesNeeded))
            _WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetTokenInformation", WriteErrorCategory::InvalidResult);

        LPWSTR sidString;
        auto tokenUserInfo = reinterpret_cast<PTOKEN_USER>(buffer.Get());
        if (!ConvertSidToStringSid(tokenUserInfo->User.Sid, &sidString))
            _WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"ConvertSidToStringSid", WriteErrorCategory::InvalidResult);

        WWuString output { sidString };
        LocalFree(sidString);

        return output;
    }
}