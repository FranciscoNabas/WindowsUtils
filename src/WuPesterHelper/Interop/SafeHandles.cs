using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace WuPesterHelper.Interop;

public sealed class SafeSystemHandle : SafeHandleZeroOrMinusOneIsInvalid
{
    private bool m_ownsHandle;

    public SafeSystemHandle()
        : base(false) { }

    public SafeSystemHandle(nint inputHandle, bool ownsHandle)
        : base(ownsHandle) => SetHandleAndOwnership(inputHandle, ownsHandle);

    protected override bool ReleaseHandle()
        => Common.CloseHandle(handle);

    public static explicit operator SafeProcessHandle(SafeSystemHandle other)
        => new(other.handle, other.m_ownsHandle);

    public static explicit operator SafeSystemHandle(SafeProcessHandle other)
        => new(other.DangerousGetHandle(), false);

    private void SetHandleAndOwnership(nint inputHandle, bool ownsHandle)
    {
        m_ownsHandle = ownsHandle;
        SetHandle(inputHandle);
    }
}

public sealed class SafeModuleHandle : SafeHandleZeroOrMinusOneIsInvalid
{
    public SafeModuleHandle()
        : base(false) { }


    public SafeModuleHandle(string module)
        : this(GetModuleHandle(module)) { }

    private SafeModuleHandle((nint, bool) handle)
        : base(handle.Item2) => SetHandle(handle.Item1);

    protected override bool ReleaseHandle()
        => Common.FreeLibrary(handle);

    private static (nint, bool) GetModuleHandle(string module)
    {
        bool ownsHandle = false;
        nint hModule = Common.GetModuleHandle(module);
        if (hModule == 0) {
            hModule = Common.LoadLibrary(module);
            if (hModule == 0)
                throw new SystemException($"Load library failed with: {Marshal.GetLastWin32Error()}.");

            ownsHandle = true;
        }

        return (hModule, ownsHandle);
    }
}