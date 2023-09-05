using System.Runtime.InteropServices;
using WindowsUtils.Core;

namespace WindowsUtils.Interop
{
    internal partial class NativeFunctions
    {
        [DllImport("Wtsapi32.dll", CharSet = CharSet.Unicode, SetLastError = true, EntryPoint = "WTSOpenServerW")]
        internal extern static SafeSystemHandle WTSOpenServer(string pServerName);
    }
}

namespace WindowsUtils.TerminalServices
{
    internal sealed class WtsSession : IDisposable
    {
        internal string ComputerName { get; set; }
        internal Interop.SafeSystemHandle SessionHandle { get; set; }

        internal WtsSession(string computerName)
        {
            SessionHandle = Interop.NativeFunctions.WTSOpenServer(computerName);
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
}