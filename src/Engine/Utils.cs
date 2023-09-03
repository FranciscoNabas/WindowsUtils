using System.Security;
using System.Security.Principal;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using System.Runtime.CompilerServices;
using WindowsUtils.Core;

namespace WindowsUtils
{
    public static class Utils
    {
        public static bool TryGetWindowsCurrentIdentity(out WindowsIdentity? currentIdentity)
        {
            try
            {
                currentIdentity = WindowsIdentity.GetCurrent();
            }
            catch (SecurityException)
            {

                currentIdentity = null;
            }

            return currentIdentity != null;
        }

        public static bool IsAdministrator()
        {
            if (TryGetWindowsCurrentIdentity(out WindowsIdentity? currentIdentity))
            {
                if (currentIdentity is null)
                    return false;

                WindowsPrincipal principal = new(currentIdentity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }

            return false;
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

        internal static string? GetRegistryNtPath(string keyPath)
        {
            string[] splitPath = keyPath.Split('\\');
            string subkeyPath = string.Join("\\", new ArraySegment<string>(splitPath, 1, splitPath.Length - 1));

            int bufferSize = 1 << 10;
            IntPtr buffer = Marshal.AllocHGlobal(bufferSize);
            RegistryKey hiveKey = splitPath[0] switch {
                "HKEY_CLASSES_ROOT" => Microsoft.Win32.Registry.ClassesRoot,
                "HKEY_CURRENT_USER" => Microsoft.Win32.Registry.CurrentUser,
                "HKEY_LOCAL_MACHINE" => Microsoft.Win32.Registry.LocalMachine,
                "HKEY_USERS" => Microsoft.Win32.Registry.Users,
                "HKEY_CURRENT_CONFIG" => Microsoft.Win32.Registry.CurrentConfig,
                _ => throw new ArgumentException($"Invalid hive '{splitPath[0]}'."),
            };

            using (RegistryKey? keyHandle = hiveKey.OpenSubKey(subkeyPath))
            {
                if (keyHandle is null)
                    return null;

                int result = Interop.NativeFunctions.NtQueryKey(keyHandle.Handle.DangerousGetHandle(), Interop.KEY_INFORMATION_CLASS.KeyNameInformation, buffer, bufferSize, out bufferSize);
                if (result != 0)
                    throw new NativeException(result);
            }

            char[] bytes = new char[2 + bufferSize + 2];
            Marshal.Copy(buffer, bytes, 0, bufferSize);
            // startIndex == 2  skips the NameLength field of the structure (2 chars == 4 bytes)
            // needed/2         reduces value from bytes to chars
            //  needed/2 - 2    reduces length to not include the NameLength
            return new(bytes, 2, (bufferSize / 2) - 2);
        }
    }
}