using System.ServiceProcess;
using System.Management.Automation;
using WindowsUtils.Core;
using WindowsUtils.Wrappers;
using WindowsUtils.ArgumentCompletion;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Deletes a service from the local or remote computer.</para>
    /// <para type="description">This cmdlet removes (deletes) a specified service from the local or remote computer.</para>
    /// <para type="description">If the service is running, and you don't use 'Stop', it will be marked to deletion.</para>
    /// <para type="description">If the service have dependents, and 'Stop' is not used, it will be marked for deletion.</para>
    /// <para type="description">If the service have dependents, and you use 'Stop', it will stop all dependent services.</para>
    /// <para type="description"></para>
    /// <para type="description">Attention!</para>
    /// <para type="description">The act of deleting a service is ultimately marking it for deletion.</para>
    /// <para type="description">A service is only permanently deleted when it's stopped and there are no more open handles to it.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Remove-Service -Name 'MyCoolService'</code>
    ///     <para>Removes the service 'MyCoolService'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Remove-Service -Name 'MyCoolService' -Stop -Force</code>
    ///     <para>Stops the service, and its dependents, and remove it. 'Force' skips confirmation.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Remove, "Service",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.Medium,
        DefaultParameterSetName = "WithServiceName"
    )]
    public class RemoveServiceCommand : CoreCommandBase
    {
        private readonly ServicesWrapper _unwrapper = new();

        /// <summary>
        /// <para type="description">The service controller input object.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = true,
            ParameterSetName = "WithServiceController",
            HelpMessage = "The service controller input object."
        )]
        public ServiceController InputObject { get; set; }

        /// <summary>
        /// <para type="description">The service name.</para>
        /// </summary>
        [Parameter(
            Position = 0,
            Mandatory = true,
            ValueFromPipeline = false,
            ParameterSetName = "WithServiceName",
            HelpMessage = "The service name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceNameCompleter))]
        public string Name { get; set; }

        /// <summary>
        /// <para type="description">The computer name to remove the service from.</para>
        /// </summary>
        [Parameter(HelpMessage = "The computer name to remove the service from.")]
        [ValidateNotNullOrEmpty]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">Stops the service and any dependents.</para>
        /// </summary>
        [Parameter(HelpMessage = "Stops the service and any dependents.")]
        public SwitchParameter Stop { get; set; }

        /// <summary>
        /// <para type="description">Does not wait for the service(s) to stop.</para>
        /// </summary>
        [Parameter(HelpMessage = "Does not wait for the service(s) to stop.")]
        public SwitchParameter NoWait { get; set; }

        /// <summary>
        /// <para type="description">Does not prompt for confirmation.</para>
        /// </summary>
        [Parameter(HelpMessage = "Does not prompt for confirmation.")]
        public SwitchParameter Force { get; set; }

        protected override void ProcessRecord()
        {
            if (NoWait.IsPresent && !Stop.IsPresent)
                WriteWarning("'-NoWait' was used without '-Stop'. NoWait will have no effect.");

            if (ParameterSetName == "WithServiceController")
            {
                if (Force)
                    _unwrapper.RemoveService(InputObject.ServiceName, InputObject.MachineName, Stop, CmdletContext, NoWait);
                else
                {
                    if (InputObject.MachineName == ".")
                    {
                        if (ShouldProcess(
                           $"Removing service {InputObject.ServiceName} from the local computer.",
                           $"Are you sure you want to remove service {InputObject.ServiceName}?",
                           "Removing Service"))
                            _unwrapper.RemoveService(InputObject.ServiceName, InputObject.MachineName, Stop, CmdletContext, NoWait);
                    }
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {InputObject.ServiceName} on computer {InputObject.MachineName}",
                           $"Are you sure you want to remove service {InputObject.ServiceName} on {InputObject.MachineName}?",
                           "Removing Service"))
                            _unwrapper.RemoveService(InputObject.ServiceName, InputObject.MachineName, Stop, CmdletContext, NoWait);
                    }
                }

                InputObject.Dispose();
            }
            else
            {
                if (string.IsNullOrEmpty(ComputerName))
                {
                    if (Force)
                        _unwrapper.RemoveService(Name, Stop, CmdletContext, NoWait);
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {Name} from the local computer.",
                           $"Are you sure you want to remove service {Name}?",
                           "Removing Service"))
                            _unwrapper.RemoveService(Name, Stop, CmdletContext, NoWait);
                    }
                }
                else
                {
                    if (Force)
                        _unwrapper.RemoveService(Name, ComputerName, Stop, CmdletContext, NoWait);
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {Name} on computer {ComputerName}",
                           $"Are you sure you want to remove service {Name} on {ComputerName}?",
                           "Removing Service"))
                            _unwrapper.RemoveService(Name, ComputerName, Stop, CmdletContext, NoWait);
                    }
                }
            }
        }
    }
}