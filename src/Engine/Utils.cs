using System.Security;
using System.Security.Principal;

namespace WindowsUtils;

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
            else if (i.KeyChar != '\u0000') // KeyChar == '\u0000' if the key pressed does not correspond to a printable character, e.g. F1, Pause-Break, etc
            {
                pwd.AppendChar(i.KeyChar);
                Console.Write("*");
            }
        }
        return pwd;
    }
}
