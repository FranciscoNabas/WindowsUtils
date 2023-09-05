using System.Text;
using System.Security;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using WindowsUtils.Core;
using WindowsUtils.Interop;

namespace WindowsUtils
{
    public static class Utils
    {
        internal static bool IsAdministrator()
        {
            string account = "Administrators";
            byte[] Sid = new byte[0];
            uint cbSid = 0;
            StringBuilder referencedDomainName = new();
            uint cchReferencedDomainName = (uint)referencedDomainName.Capacity;

            int err;
            if (!NativeFunctions.LookupAccountName(string.Empty, account, Sid, ref cbSid, referencedDomainName, ref cchReferencedDomainName, out _))
            {
                err = Marshal.GetLastWin32Error();
                if (err == NativeConstants.ERROR_INSUFFICIENT_BUFFER || err == NativeConstants.ERROR_INVALID_FLAGS)
                {
                    Sid = new byte[cbSid];
                    referencedDomainName.EnsureCapacity((int)cchReferencedDomainName);
                    err = NativeConstants.ERROR_SUCCESS;
                    if (!NativeFunctions.LookupAccountName(string.Empty, account, Sid, ref cbSid, referencedDomainName, ref cchReferencedDomainName, out _))
                        err = Marshal.GetLastWin32Error();
                }
            }
            else
                return false;

            if (err == NativeConstants.ERROR_SUCCESS)
            {
                if (!NativeFunctions.CheckTokenMembership(IntPtr.Zero, Sid, out bool isAdmin))
                    throw new NativeException(Marshal.GetLastWin32Error());

                return isAdmin;
            }
            else
                throw new NativeException(err);
        }

        internal static SecureString ReadPassword()
        {
            var pwd = new SecureString();
            while (true)
            {
                ConsoleKeyInfo i = Console.ReadKey(true);
                if (i.Key == ConsoleKey.Enter)
                {
                    break;
                }
                else if (i.Key == ConsoleKey.Backspace)
                {
                    if (pwd.Length > 0)
                    {
                        pwd.RemoveAt(pwd.Length - 1);
                        Console.Write("\b \b");
                    }
                }
                // KeyChar == '\u0000' if the key pressed does not correspond to a printable character, e.g. F1, Pause-Break, etc
                else if (i.KeyChar != '\u0000')
                {
                    pwd.AppendChar(i.KeyChar);
                    Console.Write("*");
                }
            }
            return pwd;
        }
    }
}