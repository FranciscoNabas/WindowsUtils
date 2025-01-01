using System.Runtime.InteropServices;
using WindowsUtils.Engine.Interop;

namespace WindowsUtils.TerminalServices;

internal sealed class WtsSession : IDisposable
{
    internal string ComputerName { get; }
    internal SafeWtsServerHandle SessionHandle { get; }

    internal WtsSession(string computerName)
    {
        SessionHandle = NativeWts.WTSOpenServer(computerName);
        if (SessionHandle is null || SessionHandle.IsInvalid || SessionHandle.IsClosed)
            throw new NativeException(Marshal.GetLastWin32Error());

        ComputerName = computerName;
    }

    public void Dispose()
    {
        SessionHandle.Dispose();
        GC.SuppressFinalize(this);
    }
}