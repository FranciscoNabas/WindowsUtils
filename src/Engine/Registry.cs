using System.Management.Automation;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using Microsoft.Win32.SafeHandles;
using WindowsUtils.Interop;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Interop
{
    internal partial class NativeFunctions
    {
        [DllImport("Advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "RegConnectRegistryW")]
        internal static extern int RegConnectRegistry(
            string lpMachineName,
            RegistryHive hKey,
            out SafeRegistryHandle phkResult
        );

        [DllImport("Advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "RegConnectRegistryW")]
        internal static extern int RegConnectRegistry(
            IntPtr lpMachineName,
            RegistryHive hKey,
            out SafeRegistryHandle phkResult
        );
    }
}

namespace WindowsUtils.Registry
{
    public class RegistryManager : IDisposable
    {
        private readonly string? _computerName;
        private readonly RegistryHive _hive;
        private readonly SafeRegistryHandle _hRegistry;

        private readonly RegistryWrapper _unwrapper = new();

        public string? ComputerName { get { return _computerName; } }
        public RegistryHive Hive { get { return _hive; } }

        public RegistryManager(string computerName, PSCredential credential, RegistryHive hive)
        {
            UtilitiesWrapper.LogonAndImpersonateUser(credential.UserName, credential.Password);
            int connectResult = NativeFunctions.RegConnectRegistry(computerName, hive, out _hRegistry);
            if (connectResult != 0)
            {
                NativeFunctions.RevertToSelf();
                throw new NativeException(connectResult);
            }

            NativeFunctions.RevertToSelf();

            _hive = hive;
            _computerName = computerName;
        }

        public RegistryManager(PSCredential credential, RegistryHive hive)
        {
            UtilitiesWrapper.LogonAndImpersonateUser(credential.UserName, credential.Password);
            int connectResult = NativeFunctions.RegConnectRegistry(IntPtr.Zero, hive, out _hRegistry);
            if (connectResult != 0)
            {
                NativeFunctions.RevertToSelf();
                throw new NativeException(connectResult);
            }

            NativeFunctions.RevertToSelf();

            _hive = hive;
        }

        public RegistryManager(string computerName, RegistryHive hive)
        {
            int connectResult = NativeFunctions.RegConnectRegistry(computerName, hive, out _hRegistry);
            if (connectResult != 0)
                throw new NativeException(connectResult);

            _hive = hive;
            _computerName = computerName;
        }

        public RegistryManager(RegistryHive hive)
        {
            int connectResult = NativeFunctions.RegConnectRegistry(IntPtr.Zero, hive, out _hRegistry);
            if (connectResult != 0)
                throw new NativeException(connectResult);

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
            => _unwrapper.GetRegistryValue(_hRegistry.DangerousGetHandle(), subKey, valueName);

        public object[] GetRegistryValueList(string subKey, string[] valueNames)
            => _unwrapper.GetRegistryValueList(_hRegistry.DangerousGetHandle(), subKey, valueNames);

        public string[] GetRegistrySubKeyNames(string rootKey)
            => _unwrapper.GetRegistrySubKeyNames(_hRegistry.DangerousGetHandle(), rootKey);
    }
}