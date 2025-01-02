using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Creates a new cabinet file.</para>
    /// <para type="description">This Cmdlet creates a new cabinet based on the input path.</para>
    /// <para type="description">If the path is a directory it looks recursively for all files.</para>
    /// <para type="description">You can use the 'MaxCabSize'(Kb) parameter if you want to split in multiple cabs, and the 'NamePrefix' to chose a prefix name.</para>
    /// <para type="description">Important! Cabinet files accepts only files smaller than 2Gb, and the maximum size for a cabinet file is 2Gb.</para>
    /// <example>
    ///     <para></para>
    ///     <code>New-Cabinet -Path 'C:\Path\To\Files' -Destination 'C:\Path\To\Destination'</code>
    ///     <para>Compresses all files in 'C:\Path\To\Files'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>New-Cabinet 'C:\Path\To\Files' 'C:\Path\To\Destination' -MaxCabSize 20000 -NamePrefix 'CoolCab'</code>
    ///     <para>Compress all files in 'C:\Path\To\Files' with a max cab size of 20Mb and a prefix name of 'CoolCab'</para>
    ///     <para>If the size exceeds 20Mb, files will span over multiple cabs, following the pattern 'CoolCab01.cab', 'CoolCab02.cab'...</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.New, "Cabinet")]
    public class CompressArchiveFileCommand : CoreCommandBase
    {
        private string _path;
        private string _destination;

        /// <summary>
        /// <para type="description">The path to the files to be compressed.</para>
        /// <para type="description">It can be a path to a file, or a directory.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0
        )]
        [ValidateNotNullOrEmpty]
        public string Path {
            get { return _path; }
            set {
                if (!Directory.Exists(value) && !File.Exists(value))
                    throw new FileNotFoundException($"Path {value} not found.");

                _path = value;
            }
        }

        /// <summary>
        /// <para type="description">A path to the destination. It needs to be a path to a valid folder.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 1
        )]
        [ValidateNotNullOrEmpty]
        public string Destination {
            get { return _destination; }
            set {
                if (!Directory.Exists(value))
                    throw new FileNotFoundException($"Directory {value} not found.");

                _destination = value;
            }
        }

        /// <summary>
        /// <para type="description">A name prefix. The cabinet will be created in the format 'Prefix01.cab'</para>
        /// <para type="description">If the files span through multiple cabs, the count is incremented. 'Prefix01.cab', 'Prefix02.cab'...</para>
        /// </summary>
        [Parameter()]
        public string NamePrefix { get; set; } = "Cabinet";

        /// <summary>
        /// <para type="description">The maximum cabinet size in kilobytes. This size will determine if files are going to span over multiple cabs.</para>
        /// <para type="description">The maximum supported cabinet size, and the default is 2Gb.</para>
        /// </summary>
        [Parameter()]
        [ValidateRange(1, 0x1FFFFF)]
        public int MaxCabSize { get; set; } = 0x1FFFFF;

        /// <summary>
        /// <para type="description">The compression algorithm. The higher the compression level, the longer it takes to compress all files.</para>
        /// <para type="description">From lower to higher: None, MSZip, LZXLow, LZXHigh.</para>
        /// </summary>
        [Parameter()]
        public CabinetCompressionType CompressionType { get; set; } = CabinetCompressionType.MSZip;

        protected override void ProcessRecord()
        {
            try {
                Containers.CompressArchiveFile(Path, _destination, NamePrefix, (MaxCabSize * 1024), CompressionType, ArchiveFileType.Cabinet);
            }
            // Exception already written to the stream.
            catch (NativeException) { }
        }
    }
}