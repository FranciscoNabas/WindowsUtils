using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Closes a network file.</para>
    /// <para type="description">This Cmdlet closes a file opened remotely on the current or remote computer.</para>
    /// <para type="description">It was inspired by 'psfile.exe' from Sysinternals.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Close-NetworkFile -ComputerName 'SUPERSERVER' -FileId 222</code>
    ///     <para>Closes file ID 222 on computer 'SUPERSERVER'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>closenetfile 'SUPERSERVER' 100, 102, 111 -Force</code>
    ///     <para>Closes file IDs 100, 102 and 111 on computer 'SUPERSERVER' without prompting for confirmation.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-NetworkFile 'SUPERSERVER' -BasePath "$env:windir\MySuperPath" | Close-NetworkFile</code>
    ///     <para>Closes all files that starts with "$env:windir\MySuperPath" on computer 'SUPERSERVER'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Close, "NetworkFile",
        DefaultParameterSetName = "withFileId",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High
    )]
    [Alias("closenetfile")]
    public class CloseNetworkFileCommand : PSCmdlet
    {
        public readonly Wrapper _unwrapper = new();

        /// <summary>
        /// <para type="description">The computer name. If not present, the current computer is used.</para>
        /// </summary>
        [Parameter(
            Position = 0,
            ParameterSetName = "withFileId",
            HelpMessage = "The computer name. If not present, the current computer is used."
        )]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">One or more file IDs.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 1,
            ParameterSetName = "withFileId",
            HelpMessage = "One or more file IDs."
        )]
        [ValidateNotNullOrEmpty]
        public int[] FileId { get; set; }

        /// <summary>
        /// <para type="description">The result from 'Get-NetworkFile'.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "withInputObject",
            ValueFromPipeline = true,
            HelpMessage = "The result from 'Get-NetworkFile'."
        )]
        public NetworkFileInfo[] InputObject { get; set; }

        /// <summary>
        /// <para type="description">Used to skip confirmation.</para>
        /// </summary>
        [Parameter(HelpMessage = "Used to skip confirmation.")]
        public SwitchParameter Force { get; set; }

        protected override void ProcessRecord()
        {
            if (ParameterSetName == "withFileId")
            {
                if (Force.IsPresent)
                {
                    foreach (int id in FileId)
                        if (string.IsNullOrEmpty(ComputerName))
                            _unwrapper.CloseNetworkFile(string.Empty, id);
                        else
                            _unwrapper.CloseNetworkFile(ComputerName, id);
                }
                else
                {
                    foreach (int id in FileId)
                    {
                        if (string.IsNullOrEmpty(ComputerName))
                        {
                            if (ShouldProcess(
                                $"Closing file id {id} on the current computer.",
                                $"Are you sure you want to close the remote file id {id}?",
                                "Closing remote file"))
                                _unwrapper.CloseNetworkFile(string.Empty, id);
                        }
                        else
                        {
                            if (ShouldProcess(
                                $"Closing file id {id} on computer '{ComputerName}'.",
                                $"Are you sure you want to close the remote file id {id}?",
                                "Closing remote file"))
                                _unwrapper.CloseNetworkFile(ComputerName, id);
                        }
                    }
                }
            }
            else
            {
                if (Force.IsPresent)
                {
                    foreach (NetworkFileInfo info in InputObject)
                        if (string.IsNullOrEmpty(info.ComputerName))
                            _unwrapper.CloseNetworkFile(string.Empty, info.Id);
                        else
                            _unwrapper.CloseNetworkFile(info.ComputerName, info.Id);
                }
                else
                {
                    foreach (NetworkFileInfo info in InputObject)
                    {
                        if (string.IsNullOrEmpty(info.ComputerName))
                        {
                            if (ShouldProcess(
                                $"Closing file id {info.Id} on the current computer.",
                                $"Are you sure you want to close the remote file id {info.Id}?",
                                "Closing remote file"))
                                _unwrapper.CloseNetworkFile(string.Empty, info.Id);
                        }
                        else
                        {
                            if (ShouldProcess(
                                $"Closing file id {info.Id} on computer '{info.ComputerName}'.",
                                $"Are you sure you want to close the remote file id {info.Id}?",
                                "Closing remote file"))
                                _unwrapper.CloseNetworkFile(info.ComputerName, info.Id);
                        }
                    }
                }
            }
        }
    }
}