using System.Runtime.InteropServices;

namespace WindowsUtils.Interop
{
    internal partial class NativeFunctions
    {
        [DllImport("Wtsapi32.dll", CharSet = CharSet.Unicode, SetLastError = true, EntryPoint = "WTSOpenServerW")]
        internal extern static SafeSystemHandle WTSOpenServer(string pServerName);
    }
}

namespace WindowsUtils.Engine
{
    public class TerminalServices
    {

    }
}