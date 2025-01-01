using System;
using System.Runtime.InteropServices;

namespace WuPesterHelper.Interop;

public static partial class Constants
{
    internal const int FORMAT_MESSAGE_FROM_SYSTEM      = 0x00001000;
    internal const int FORMAT_MESSAGE_FROM_HMODULE     = 0x00000800;
    internal const int FORMAT_MESSAGE_IGNORE_INSERTS   = 0x00000200;
    internal const int FORMAT_MESSAGE_ALLOCATE_BUFFER  = 0x00000100;
}

/// <summary>
/// An exception from a native function call.
/// </summary>
/// <param name="errorCode">The native error code.</param>
/// <param name="isNt">True if it's a NTSTATUS.</param>
public sealed partial class NativeException(int errorCode, bool isNt = false) : SystemException(GetMessageFromNativeError(errorCode, isNt))
{
    public int ErrorCode { get; } = errorCode;

    /// <seealso href="https://learn.microsoft.com/windows/win32/api/winbase/nf-winbase-formatmessage">FormatMessage function (winbase.h)</seealso>
    [LibraryImport("kernel32.dll", EntryPoint = "FormatMessageW", SetLastError = true)]
    private static partial int FormatMessage(
        int dwFlags,
        nint lpSource,
        int dwMessageId,
        int dwLanguage,
        out nint lpBuffer,
        int nSize,
        nint Arguments
    );

    /// <seealso href="https://learn.microsoft.com/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlew">GetModuleHandleW function (libloaderapi.h)</seealso>
    [LibraryImport("kernel32.dll", EntryPoint = "GetModuleHandleW", SetLastError = true, StringMarshalling = StringMarshalling.Utf16)]
    private static partial nint GetModuleHandle([MarshalAs(UnmanagedType.LPWStr)] string lpModuleName);

    /// <seealso href="https://learn.microsoft.com/windows/win32/api/winbase/nf-winbase-localfree">LocalFree function (winbase.h)</seealso>
    [LibraryImport("kernel32.dll", SetLastError = true)]
    private static partial nint LocalFree(nint hMem);

    /// <summary>
    /// Gets the message text from a native error code.
    /// </summary>
    /// <param name="errorCode">The error code.</param>
    /// <param name="isNt">True if it's a NTSTATUS.</param>
    /// <returns></returns>
    private static string GetMessageFromNativeError(int errorCode, bool isNt)
    {
        string output = string.Empty;
        if (isNt) {
            nint hNtdll = GetModuleHandle("ntdll.dll");
            int res = FormatMessage(
                Constants.FORMAT_MESSAGE_ALLOCATE_BUFFER |
                Constants.FORMAT_MESSAGE_FROM_HMODULE |
                Constants.FORMAT_MESSAGE_IGNORE_INSERTS,
                hNtdll,
                errorCode,
                0,
                out nint buffer,
                0,
                nint.Zero
            );

            if (res > 0) {
                string? message = Marshal.PtrToStringUni(buffer);
                output = message is null ? string.Empty : message;
                LocalFree(buffer);
            }
        }
        else {
            int res = FormatMessage(
                Constants.FORMAT_MESSAGE_ALLOCATE_BUFFER |
                Constants.FORMAT_MESSAGE_FROM_SYSTEM |
                Constants.FORMAT_MESSAGE_IGNORE_INSERTS,
                nint.Zero,
                errorCode,
                0,
                out nint buffer,
                0,
                nint.Zero
            );

            if (res > 0) {
                string? message = Marshal.PtrToStringUni(buffer);
                output = message is null ? string.Empty : message;
                LocalFree(buffer);
            }
        }

        return output;
    }
}