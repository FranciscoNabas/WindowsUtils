using System.Management.Automation;
using Microsoft.Win32;
using Microsoft.Win32.SafeHandles;
using WindowsUtils.Wrappers;
using WindowsUtils.Engine.Interop;

namespace WindowsUtils.Registry;

public class RegistryManager : IDisposable
{
    private readonly string? _computerName;
    private readonly RegistryHive _hive;
    private readonly SafeRegistryHandle _hRegistry;

    public string? ComputerName { get { return _computerName; } }
    public RegistryHive Hive { get { return _hive; } }

    public RegistryManager(string computerName, PSCredential credential, RegistryHive hive)
    {
        UtilitiesWrapper.LogonAndImpersonateUser(credential.UserName, credential.Password);
        try {
            _hRegistry = NativeRegistry.ConnectRegistry(computerName, hive);
            _hive = hive;
            _computerName = computerName;
        }
        catch { throw; }
        finally {
            NativeAccessControl.RevertToSelf();
        }
    }

    public RegistryManager(PSCredential credential, RegistryHive hive)
    {
        UtilitiesWrapper.LogonAndImpersonateUser(credential.UserName, credential.Password);
        try {
            _hRegistry = NativeRegistry.ConnectRegistry(null, hive);
            _hive = hive;
        }
        catch { throw; }
        finally {
            NativeAccessControl.RevertToSelf();
        }

        _hive = hive;
    }

    public RegistryManager(string computerName, RegistryHive hive)
    {
        _hRegistry = NativeRegistry.ConnectRegistry(computerName, hive);
        _hive = hive;
        _computerName = computerName;
    }

    public RegistryManager(RegistryHive hive)
    {
        _hRegistry = NativeRegistry.ConnectRegistry(null, hive);
        _hive = hive;
    }

    public void Dispose()
    {
        Dispose(true);
        GC.SuppressFinalize(this);
    }
    private void Dispose(bool disposing)
    {
        if (disposing)
            _hRegistry.Dispose();
    }

    public object GetRegistryValue(string subKey, string valueName)
        => RegistryWrapper.GetRegistryValue(_hRegistry.DangerousGetHandle(), subKey, valueName);

    public object[] GetRegistryValueList(string subKey, string[] valueNames)
        => RegistryWrapper.GetRegistryValueList(_hRegistry.DangerousGetHandle(), subKey, valueNames);

    public string[] GetRegistrySubKeyNames(string rootKey)
        => RegistryWrapper.GetRegistrySubKeyNames(_hRegistry.DangerousGetHandle(), rootKey);
}