using System.Text;
using System.Security;
using System.Management.Automation;
using WindowsUtils.Engine.Interop;

namespace WindowsUtils.Engine;

public static class Utils
{
    internal static bool IsAdministrator()
    {
        using ScopedBuffer sid = NativeAccessControl.GetAccountSid(null, "Administrators");
        return NativeAccessControl.CheckCurrentTokenMembership(sid);
    }

    internal static SecureString ReadPassword()
    {
        var pwd = new SecureString();
        while (true) {
            ConsoleKeyInfo i = Console.ReadKey(true);
            if (i.Key == ConsoleKey.Enter) {
                break;
            }
            else if (i.Key == ConsoleKey.Backspace) {
                if (pwd.Length > 0) {
                    pwd.RemoveAt(pwd.Length - 1);
                    Console.Write("\b \b");
                }
            }
            // KeyChar == '\u0000' if the key pressed does not correspond to a printable character, e.g. F1, Pause-Break, etc
            else if (i.KeyChar != '\u0000') {
                pwd.AppendChar(i.KeyChar);
                Console.Write("*");
            }
        }
        return pwd;
    }

    internal static PSObject PSObjectFactory(Dictionary<string, object> properties)
    {
        PSObject output = new();

        foreach (KeyValuePair<string, object> property in properties) {
            if (string.IsNullOrEmpty(property.Key))
                throw new ArgumentException("Property name cannot be null");

            output.Members.Add(new PSNoteProperty(property.Key, property.Value));
        }

        return output;
    }

    internal static PSObject PSObjectFactory(IOrderedEnumerable<KeyValuePair<string, object>> properties)
    {
        PSObject output = new();

        foreach (KeyValuePair<string, object> property in properties) {
            if (string.IsNullOrEmpty(property.Key))
                throw new ArgumentException("Property name cannot be null");

            output.Members.Add(new PSNoteProperty(property.Key, property.Value));
        }

        return output;
    }
}
