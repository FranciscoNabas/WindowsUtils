using System.Runtime.InteropServices;

namespace WindowsUtils.Engine.Interop;

#region Structures

[StructLayout(LayoutKind.Explicit)]
internal struct LARGE_INTEGER
{
    [FieldOffset(0)] internal uint LowPart;
    [FieldOffset(4)] internal int HighPart;
    [FieldOffset(0)] internal long QuadPart;
}

[StructLayout(LayoutKind.Explicit)]
internal struct LARGE_UINTEGER
{
    [FieldOffset(0)] internal uint LowPart;
    [FieldOffset(4)] internal uint HighPart;
    [FieldOffset(0)] internal ulong QuadPart;
}

#endregion

internal static partial class Constants
{
    internal static readonly IntPtr INVALID_HANDLE_VALUE = (IntPtr)(-1);
}

internal static partial class ErrorCodes
{
    internal const int ERROR_SUCCESS              = 0;
    internal const int ERROR_INSUFFICIENT_BUFFER  = 122;
    internal const int ERROR_INVALID_FLAGS        = 1004;
}

internal static class Common
{
    [DllImport("kernel32.dll", SetLastError = true)]
    internal static extern bool CloseHandle(IntPtr hObject);
}