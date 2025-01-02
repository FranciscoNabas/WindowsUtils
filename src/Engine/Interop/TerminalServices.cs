using System.Runtime.InteropServices;

namespace WindowsUtils.Engine.Interop;

internal static class NativeWts
{
    [DllImport("wtsapi32.dll", CharSet = CharSet.Unicode, SetLastError = true, EntryPoint = "WTSOpenServerW")]
    internal extern static SafeWtsServerHandle WTSOpenServer(string pServerName);

    [DllImport("wtsapi32.dll", SetLastError = true)]
    internal static extern void WTSCloseServer(IntPtr hServer);
}