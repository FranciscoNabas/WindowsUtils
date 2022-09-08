using System;
using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using Microsoft.Win32.SafeHandles;

namespace UtilitiesLibrary
{
    internal class SystemSafeHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        override public bool IsInvalid { get; }
        private SystemSafeHandle() : base(true) { }

        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        override protected bool ReleaseHandle()
        {
            if (!IsInvalid && !IsClosed)
            {
                return Interop.CloseHandle(handle);
            }
            return true;
        }

        internal IntPtr ToIntPtr() { return handle; }
    }

    internal class Interop
    {
        [DllImport("kernel32", SetLastError = true)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        internal extern static bool CloseHandle(IntPtr handle);

        [DllImport("Wtsapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal extern static SystemSafeHandle WTSOpenServerW(string pServerName);

        internal enum MouseEvent : uint
        {
            MOUSEEVENTF_LEFTDOWN = 2,
            MOUSEEVENTF_LEFTUP = 4,
            MOUSEEVENTF_RIGHTDOWN = 8,
            MOUSEEVENTF_RIGHTUP = 16
        }

        [DllImport("user32.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        internal static extern void mouse_event(
            uint dwFlags,
            uint dx,
            uint dy,
            uint cButtons,
            uint dwExtraInfo
        );
    }
}
