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
}
