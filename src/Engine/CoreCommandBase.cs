using System.Management.Automation;
using WindowsUtils.Core;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Engine;

public class CoreCommandBase : PSCmdlet
{
    private CmdletContextProxy?       m_context;

    private ContainersWrapper?        m_containers;
    private InstallerWrapper?         m_installer;
    private NetworkWrapper?           m_network;
    private ProcessAndThreadWrapper?  m_processAndThread;
    private ServicesWrapper?          m_services;
    private TerminalServicesWrapper?  m_terminalServices;
    private UtilitiesWrapper?         m_utils;

    protected ContainersWrapper Containers {
        get {
            m_containers ??= new(EnsureProxyAllocated());

            return m_containers;
        }
    }

    protected InstallerWrapper Installer {
        get {
            m_installer ??= new(EnsureProxyAllocated());

            return m_installer;
        }
    }

    protected NetworkWrapper Network {
        get {
            m_network ??= new(EnsureProxyAllocated());

            return m_network;
        }
    }

    protected ProcessAndThreadWrapper ProcessAndThread {
        get {
            m_processAndThread ??= new(EnsureProxyAllocated());

            return m_processAndThread;
        }
    }

    protected ServicesWrapper Services {
        get {
            m_services ??= new(EnsureProxyAllocated());

            return m_services;
        }
    }

    protected TerminalServicesWrapper TermServices {
        get {
            m_terminalServices ??= new(EnsureProxyAllocated());

            return m_terminalServices;
        }
    }

    protected UtilitiesWrapper Utilities {
        get {
            m_utils ??= new(EnsureProxyAllocated());

            return m_utils;
        }
    }

    private CmdletContextProxy EnsureProxyAllocated()
    {
        m_context ??= new(
            WriteProgress,
            WriteWarning,
            WriteInformation,
            WriteObject,
            WriteError
        );

        return m_context;
    }
}