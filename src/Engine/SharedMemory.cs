using System.Management.Automation;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WindowsUtils.Interop;

namespace WindowsUtils.Interop
{
    internal enum MappedDataType : uint
    {
        Progress,
        Warning
    }

    internal enum MappedPageProtection : uint
    {
        PAGE_READONLY = 0x02,
        PAGE_READWRITE = 0x04,
        PAGE_WRITECOPY = 0x08,
        PAGE_EXECUTE_READ = 0x20,
        PAGE_EXECUTE_READWRITE = 0x40,
        PAGE_EXECUTE_WRITECOPY = 0x80
    }

    internal enum SectionAccess : uint
    {
        SECTION_QUERY = 0x0001,
        SECTION_MAP_WRITE = 0x0002,
        SECTION_MAP_READ = 0x0004,
        SECTION_MAP_EXECUTE = 0x0008,
        SECTION_EXTEND_SIZE = 0x0010,
        SECTION_MAP_EXECUTE_EXPLICIT = 0x0020,
        SECTION_ALL_ACCESS = AccessType.STANDARD_RIGHTS_REQUIRED |
                             SECTION_QUERY |
                             SECTION_MAP_WRITE |
                             SECTION_MAP_READ |
                             SECTION_MAP_EXECUTE |
                             SECTION_EXTEND_SIZE
    }

    internal enum FileMapAccess : uint
    {
        FILE_MAP_WRITE = SectionAccess.SECTION_MAP_WRITE,
        FILE_MAP_READ = SectionAccess.SECTION_MAP_READ,
        FILE_MAP_ALL_ACCESS = SectionAccess.SECTION_ALL_ACCESS,
        FILE_MAP_EXECUTE = SectionAccess.SECTION_MAP_EXECUTE_EXPLICIT,
        FILE_MAP_COPY = 0x00000001,
        FILE_MAP_RESERVE = 0x80000000,
        FILE_MAP_TARGETS_INVALID = 0x40000000,
        FILE_MAP_LARGE_PAGES = 0x20000000
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct SYSTEM_INFO
    {
        [FieldOffset(0x0)] internal uint dwOemId;
        [FieldOffset(0x0)] internal ushort wProcessorArchitecture;
        [FieldOffset(0x2)] internal ushort wReserved;
        [FieldOffset(0x4)] internal uint dwPageSize;
        [FieldOffset(0x8)] internal IntPtr lpMinimumApplicationAddress;
        [FieldOffset(0x10)] internal IntPtr lpMaximumApplicationAddress;
        [FieldOffset(0x18)] internal ulong dwActiveProcessorMask;
        [FieldOffset(0x20)] internal uint dwNumberOfProcessors;
        [FieldOffset(0x24)] internal uint dwProcessorType;
        [FieldOffset(0x28)] internal uint dwAllocationGranularity;
        [FieldOffset(0x2C)] internal ushort wProcessorLevel;
        [FieldOffset(0x2E)] internal ushort wProcessorRevision;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct MAPPED_PROGRESS_DATA
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Activity;
        internal int ActivityId;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string CurrentOperation;
        internal int ParentActivityId;
        internal int PercentComplete;
        internal ProgressRecordType RecordType;
        internal int SecondsRemaining;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string StatusDescription;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal unsafe struct MAPPED_INFORMATION_DATA
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Computer;
        internal uint NativeThreadId;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Text;
        
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Source;
        
        // To avoid problems with string marshaling, and to keep it as simple
        // as possible, we're going pointer.
        internal IntPtr* Tags;
        internal uint TagCount;
        internal long TimeGenerated;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string User;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct TCPING_OUTPUT
    {
        internal long Timestamp;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Destination;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string DestAddress;
        
        internal uint Port;
        internal TcpingStatus Status;
        internal double RoundTripTime;
        internal double Jitter;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TCPING_STATISTICS
    {
        internal uint Sent;
        internal uint Successful;
        internal uint Failed;
        internal double FailedPercent;
        internal double MinRtt;
        internal double MaxRtt;
        internal double AvgRtt;
        internal double MinJitter;
        internal double MaxJitter;
        internal double AvgJitter;
        internal double TotalJitter;
        internal double TotalMilliseconds;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct TESTPORT_OUTPUT
    {
        internal long Timestamp;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Destination;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string DestAddress;

        internal uint Port;
        internal TcpingStatus Status;
    }

    internal partial class NativeFunctions
    {
        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "CreateFileMappingW")]
        internal static extern SafeMemoryMappedFileHandle CreateFileMapping(
            IntPtr hFile,
            IntPtr lpFileMappingAttributes,
            MappedPageProtection flProtect,
            uint dwMaximumSizeHigh,
            uint dwMaximumSizeLow,
            string lpName
        );

        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern SafeMemoryMappedViewHandle MapViewOfFile(
            SafeMemoryMappedFileHandle hFileMappingObject,
            FileMapAccess dwDesiredAccess,
            uint dwFileOffsetHigh,
            uint dwFileOffsetLow,
            uint dwNumberOfBytesToMap
        );

        [DllImport("Kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern void GetSystemInfo(ref SYSTEM_INFO lpSystemInfo);
    }
}

namespace WindowsUtils
{
    internal abstract class MemoryMappedShare : IDisposable
    {
        internal ulong Size => _size.QuadPart;

        private readonly SafeMemoryMappedFileHandle _memoryHandle;
        private readonly uint _allocGranularity = 65536;
        private readonly Dictionary<Guid, SafeMemoryMappedViewHandle> _mappedViews;

        private LARGE_UINTEGER _size;

        protected MemoryMappedShare(ulong size, string shareName)
        {
            _size = new(size);
            _mappedViews = new();

            _memoryHandle = NativeFunctions.CreateFileMapping(
                NativeConstants.INVALID_HANDLE_VALUE,
                IntPtr.Zero,
                MappedPageProtection.PAGE_READWRITE,
                _size.HighPart,
                _size.LowPart,
                shareName
            );
            if (_memoryHandle is null || _memoryHandle.IsInvalid || _memoryHandle.IsClosed)
                throw new NativeException(Marshal.GetLastWin32Error());
        }

        internal IntPtr DangerousGetMappedFileHandle()
            => _memoryHandle.DangerousGetHandle();

        internal SafeMemoryMappedViewHandle MapView(out Guid viewId, FileMapAccess desiredAccess, LARGE_UINTEGER offset, ulong numberOfBytes)
        {
            SafeMemoryMappedViewHandle mappedView;
            try
            {
                if (offset.QuadPart > numberOfBytes)
                    throw new ArgumentOutOfRangeException(nameof(offset));

                if (numberOfBytes == 0)
                    throw new ArgumentOutOfRangeException(nameof(numberOfBytes));

                // The maximum bytes number returned is the size of the mapped file.
                numberOfBytes = numberOfBytes > _size.QuadPart ? _size.QuadPart : numberOfBytes;

                // The offset needs to be a multiple of the system allocation granularity.
                // If it's not, we round to the closest value.
                ulong remainder = offset.QuadPart % _allocGranularity;
                if (remainder != 0)
                {
                    offset.SetQuadPart(offset.QuadPart - remainder);
                    if (remainder >= (_allocGranularity / 2))
                        offset.SetQuadPart(offset.QuadPart + _allocGranularity);
                }

                mappedView = NativeFunctions.MapViewOfFile(_memoryHandle, desiredAccess, offset.HighPart, offset.LowPart, (uint)numberOfBytes);
                if (mappedView is null || mappedView.IsInvalid || mappedView.IsClosed)
                    throw new NativeException(Marshal.GetLastWin32Error());

                viewId = Guid.NewGuid();
                _mappedViews.Add(viewId, mappedView);
            }
            catch (Exception ex)
            {
                // Get stack trace for the exception with source file information
                var st = new System.Diagnostics.StackTrace(ex, true);
                // Get the top stack frame
                var frame = st.GetFrame(0);
                // Get the line number from the stack frame
                var line = frame.GetFileLineNumber();
                // Only works if there is a symbol file for the assembly.
                Console.WriteLine(line.ToString());
                throw ex;
            }


            return mappedView;
        }

        internal void UnmapView(Guid id)
        {
            if (_mappedViews.TryGetValue(id, out SafeMemoryMappedViewHandle? mappedView))
            {
                if (mappedView is not null && !mappedView.IsClosed && !mappedView.IsInvalid)
                    mappedView.Dispose();

                _mappedViews.Remove(id);
            }
        }

        public void Dispose()
        {
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (disposing)
            {
                foreach (var mappedView in _mappedViews)
                    if (!mappedView.Value.IsClosed && !mappedView.Value.IsInvalid)
                        mappedView.Value.Dispose();

                _memoryHandle.Dispose();
            }
        }
    }

    internal sealed class ProgressMappedShare : MemoryMappedShare
    {
        private static ProgressMappedShare? _instance;

        private ProgressMappedShare()
            : base(409600, "WuProgressInfoShare") { }

        internal static ProgressMappedShare GetShare()
        {
            _instance ??= new();
            return _instance;
        }
    }

    internal sealed class WarningMappedShare : MemoryMappedShare
    {
        private static WarningMappedShare? _instance;

        private WarningMappedShare()
            : base(204800, "WuWarningInfoShare") { }

        internal static WarningMappedShare GetShare()
        {
            _instance ??= new();
            return _instance;
        }
    }

    internal sealed class InformationMappedShare : MemoryMappedShare
    {
        private static InformationMappedShare? _instance;

        private InformationMappedShare()
            : base(409600, "WuInformationInfoShare") { }

        internal static InformationMappedShare GetShare()
        {
            _instance ??= new();
            return _instance;
        }
    }
}