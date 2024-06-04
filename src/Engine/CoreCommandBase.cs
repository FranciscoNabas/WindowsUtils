using System.Management.Automation;
using System.Management.Automation.Provider;
using WindowsUtils.Core;

namespace WindowsUtils.Commands;

public abstract class CoreCommandBase : PSCmdlet
{
    private bool _suppressWildcardExpansion;
    private CmdletProvider? _providerInstance;

    private CmdletContextProxy? _nativeContext;

    internal virtual CmdletContextProxy CmdletContext
    {
        get
        {
            _nativeContext ??= new(
                WriteProgress,
                WriteWarning,
                WriteInformation,
                WriteObject,
                WriteError
            );

            return _nativeContext;
        }
    }

    internal virtual SwitchParameter SuppressWildcardExpansion
    {
        get { return _suppressWildcardExpansion; }
        set { _suppressWildcardExpansion = value; }
    }

    internal CmdletProvider? ProviderInstance {
        get { return _providerInstance; }
        set { _providerInstance = value; }
    }
}