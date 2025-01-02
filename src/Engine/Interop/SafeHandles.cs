using Microsoft.Win32.SafeHandles;

namespace WindowsUtils.Engine.Interop;

public sealed class SafeSystemHandle : SafeHandleZeroOrMinusOneIsInvalid
{
    public SafeSystemHandle()
        : base(false) { }

    public SafeSystemHandle(IntPtr handle)
        : base(true) => SetHandle(handle);

    override protected bool ReleaseHandle()
    {
        if (!IsInvalid && !IsClosed) {
            return Common.CloseHandle(handle);
        }

        return true;
    }
}

public sealed class SafeServiceHandle : SafeHandleZeroOrMinusOneIsInvalid
{
    public SafeServiceHandle()
        : base(false) { }

    public SafeServiceHandle(IntPtr handle)
        : base(true) => SetHandle(handle);

    override protected bool ReleaseHandle()
        => Services.CloseServiceHandle(handle);
}

public sealed class SafeWtsServerHandle : SafeHandleZeroOrMinusOneIsInvalid
{
    public SafeWtsServerHandle()
        : base(false) { }

    public SafeWtsServerHandle(IntPtr handle)
        : base(true) => SetHandle(handle);

    protected override bool ReleaseHandle()
    {
        NativeWts.WTSCloseServer(handle);
        return true;
    }
}