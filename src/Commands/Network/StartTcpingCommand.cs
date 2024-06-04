using System.Management.Automation;
using WindowsUtils.Core;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">"Pings" a destination using TCP.</para>
    /// <para type="description">This Cmdlet measures network statistics when trying to connect with a server with TCP.</para>
    /// <para type="description">It was designed to be familiar with tools like 'ping.exe' and 'tcping.exe'.</para>
    /// <para type="description">The parameters have aliases that mimics those programs.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Start-Tcping -Destination 'learn.microsoft.com' -Port 443</code>
    ///     <para>Measure statistics with 'learn.microsoft.com' as destination on port 443.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Start-Tcping 'learn.microsoft.com' -Continuous -IncludeJitter</code>
    ///     <para>Measure RTT and jitter with 'learn.microsoft.com' as destination on port 80.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Start-Tcping 'learn.microsoft.com' -n 10 -j -o 'C:\SuperTcpTest.txt'</code>
    ///     <para>Measure statistics to 'learn.microsoft.com' on port 80, with jitter and saving the output in a file.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Start-Tcping 'learn.microsoft.com', 'google.com' -p 80, 443 -s</code>
    ///     <para>Measure statistics to 'learn.microsoft.com' and 'google.com' on port 80 and 443, with a single probe.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsLifecycle.Start, "Tcping")]
    [OutputType(typeof(TcpingProbeInfo), typeof(TcpingStatistics))]
    [Alias("tcping")]
    public class StartTcpingCommand : CoreCommandBase
    {
        private readonly NetworkWrapper _unwrapper = new();
        private int? _count;
        private int _failedThreshold = -1;

        private string _outputFile;
        private bool _append = false;
        private bool _continuous;
        private bool _single = false;

        private bool _ignoreSingle = false;

        string[] _destination;
        int[] _port = new int[] { 80 };

        /// <summary>
        /// <para type="description">The destination.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0
        )]
        [ValidateNotNullOrEmpty]
        public string[] Destination {
            get { return _destination; }
            set { _destination = value; }
        }

        /// <summary>
        /// <para type="description">The port.</para>
        /// </summary>
        [Parameter(Position = 1)]
        [Alias("p")]
        [ValidateRange(1, 65535)]
        public int[] Port {
            get { return _port; }
            set { _port = value; }
        }

        /// <summary>
        /// <para type="description">The number of tests to perform.</para>
        /// </summary>
        [Parameter()]
        [Alias("n")]
        [ValidateRange(1, int.MaxValue)]
        public int Count {
            get {
                if (_count is null)
                    return 4;

                return _count.Value;
            }
            set {
                if (this.Continuous.IsPresent)
                {
                    if (this.Single.IsPresent)
                    {
                        WriteWarning("'-Continuous' was used with '-Count' and '-Single'. Count and Single will be ignored.");
                        _ignoreSingle = true;
                    }
                    else
                        WriteWarning("'-Count' was used with '-Continuous'. Count will be ignored.");
                }
                else
                    if (this.Single.IsPresent)
                {
                    WriteWarning("'-Count' was used with '-Single'. Count will be ignored.");
                    _ignoreSingle = true;
                }

                _count = value;
            }
        }

        /// <summary>
        /// <para type="description">The timeout while trying to connect, in seconds.</para>
        /// </summary>
        [Parameter()]
        [Alias("w")]
        [ValidateRange(1, int.MaxValue)]
        public int Timeout { get; set; } = 2;

        /// <summary>
        /// <para type="description">The interval between each probe, in seconds.</para>
        /// </summary>
        [Parameter()]
        [Alias("i")]
        [ValidateRange(1, int.MaxValue)]
        public int Interval { get; set; } = 1;

        /// <summary>
        /// <para type="description">The number of failed attempts before aborting.</para>
        /// </summary>
        [Parameter()]
        [Alias("g")]
        public int FailedThreshold {
            get {
                if (_failedThreshold == -1)
                {
                    if (_count is null)
                        return 4;

                    return _count.Value;
                }

                return _failedThreshold;
            }
            set {
                if (value <= 0 || value > _count)
                    throw new ArgumentOutOfRangeException("'FailedThreshold' cannot be smaller or equal to zero, or bigger than 'Count'.");

                _failedThreshold = value;
            }
        }

        /// <summary>
        /// <para type="description">Probe continuously.</para>
        /// </summary>
        [Parameter()]
        [Alias("t")]
        public SwitchParameter Continuous {
            get { return _continuous; }
            set {
                if (this.Single.IsPresent)
                {
                    WriteWarning("'-Continuous' was used with '-Single'. Single will be ignored");
                    _ignoreSingle = true;
                }

                _continuous = value;
            }
        }

        /// <summary>
        /// <para type="description">Include jitter.</para>
        /// </summary>
        [Parameter()]
        [Alias("j")]
        public SwitchParameter IncludeJitter { get; set; }

        /// <summary>
        /// <para type="description">Attempts to resolve the ip to a FQDN and print in the output.</para>
        /// </summary>
        [Parameter()]
        [Alias("fqdn")]
        public SwitchParameter PrintFqdn { get; set; }

        /// <summary>
        /// <para type="description">Sends only a single probe. Does not print statistics.</para>
        /// </summary>
        [Parameter()]
        [Alias("s")]
        public SwitchParameter Single {
            get { return _single; }
            set { _single = value; }
        }

        /// <summary>
        /// <para type="description">Forces sending 4 bytes instead of an empty request.</para>
        /// </summary>
        [Parameter()]
        public SwitchParameter Force { get; set; }

        // File parameters

        /// <summary>
        /// <para type="description">The path to a file to store the output.</para>
        /// </summary>
        [Parameter()]
        [Alias("o")]
        [ValidateNotNullOrEmpty]
        public string OutputFile {
            get { return _outputFile; }
            set {
                if (!Directory.Exists(Path.GetDirectoryName(value)))
                    throw new FileNotFoundException($"Folder '{value}' not found.");

                _outputFile = value;
            }
        }

        /// <summary>
        /// <para type="description">Append to the file instead of overwriting it.</para>
        /// </summary>
        [Parameter()]
        public SwitchParameter Append {
            get { return _append; }
            set { _append = value; }
        }

        protected override void ProcessRecord()
        {
            if ((Destination.Length > 1 || Port.Length > 1) && _continuous)
                WriteWarning("'-Continuous' was used with multiple destination or ports. Only the first will be processed.");

            bool isCancel;
            bool isFirst = true;
            foreach (string server in Destination)
            {
                foreach (int singlePort in Port)
                {
                    if ((Destination.Length > 0 || Port.Length > 0) && !isFirst)
                        _append = true;

                    if (_ignoreSingle)
                        _unwrapper.StartTcpPing(server, singlePort, Count, Timeout, Interval, FailedThreshold, Continuous,
                            IncludeJitter, PrintFqdn, Force, false, OutputFile, Append, CmdletContext, out isCancel);
                    else
                        _unwrapper.StartTcpPing(server, singlePort, Count, Timeout, Interval, FailedThreshold, Continuous,
                            IncludeJitter, PrintFqdn, Force, Single, OutputFile, Append, CmdletContext, out isCancel);

                    if (isCancel)
                        return;

                    if (isFirst)
                        isFirst = false;
                }
            }
        }
    }
}