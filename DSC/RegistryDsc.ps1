Configuration RegistryConfiguration
{
    Registry MainHive
    {
        Ensure = 'Present'
        Key = 'HKEY_LOCAL_MACHINE\SOFTWARE'
        ValueName = 'WindowsUtils'
    }

    Registry ProcessManager
    {
        Ensure = 'Present'
        Key = 'HKEY_LOCAL_MACHINE\SOFTWARE\WindowsUtils'
        ValueName = 'ProcessManager'
    }
}