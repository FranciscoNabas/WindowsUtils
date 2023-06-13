using System.Runtime.InteropServices;

namespace WindowsUtils.Interop
{
    internal partial class NativeFunctions
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern SafeSystemHandle GetCurrentProcess();

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern SafeSystemHandle GetCurrentThread();
    }
}

namespace WindowsUtils.Engine
{
    public class Process
    {

    }
}
