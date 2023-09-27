using System.Net;
using System.Text.Json;
using System.Reflection;
using System.Runtime.InteropServices;
using WindowsUtils.Core;
using WindowsUtils.Interop;
using WindowsUtils.Commands;

namespace WindowsUtils
{
    public enum MessageBoxReturn : int
    {
        Ok = 1,
        Cancel = 2,
        Abort = 3,
        Retry = 4,
        Ignore = 5,
        Yes = 6,
        No = 7,
        TryAgain = 10,
        Continue = 11,
        TimeOut = 32000,
        AsyncReturn = 32001
    }

    public enum DotNetEditionInfo : uint
    {
        FullFramework,
        Core,
        Clr
    }

    public enum TcpingStatus : ushort
    {
        Open,
        Closed,
        Timeout
    }

    public enum HResultSeverity
    {
        Success = 0,
        Failure
    }

    public enum HResultFacility
    {
        Default = 0,
        RPC,
        Dispatch,
        Storage,
        ITF,
        Win32 = 7,
        Windows,
        Security,
        Control,
        Cert,
        internet,
        MediaServer,
        MSMQ,
        SetupAPI,
        SmartCard,
        COMPlus,
        AAF,
        NetClr,
        AuditCollectionService,
        DirectPlay,
        UMI,
        SXS,
        WindowsCE,
        HTTP,
        CommonLog,
        FilterManager = 31,
        BackgroundCopy,
        Configuration,
        StateManagement,
        MsIdentityServer,
        WindowsUpdate,
        ActiveDirectory,
        Graphics,
        Shell,
        TPMServices,
        TPMSoftware,
        PLA = 48,
        FVE,
        FirewallPlatform,
        WinRM,
        NDIS,
        Hypervisor,
        CMI,
        Virtualization,
        VolumeManager,
        BCD,
        VHD,
        SystemDiagnostics = 60,
        WebServices,
        WindowsDefender = 80,
        OPC
    }

    public abstract class Enumeration : IComparable
    {
        internal string Name { get; set; }
        internal uint Id { get; set; }
        protected Enumeration(uint id, string name)
        {
            Id = id;
            Name = name;
        }

        public override string ToString() => Name;
        
        internal static IEnumerable<T> GetAll<T>() where T: Enumeration =>
            typeof(T).GetFields(BindingFlags.Public |
                                BindingFlags.Static |
                                BindingFlags.DeclaredOnly)
                     .Select(f => f.GetValue(null))
                     .Cast<T>();
        
        public static T GetById<T>(uint id) where T : Enumeration =>
            GetAll<T>().First(f => f.Id == id);
            
        public override bool Equals(object? obj)
        {
            if (obj is not Enumeration otherValue)
            {
                return false;
            }

            var typeMatches = GetType().Equals(obj.GetType());
            var valueMatches = Id.Equals(otherValue.Id);

            return typeMatches && valueMatches;
        }

        public override int GetHashCode()
        {
            return Name.GetHashCode();
        }

        public int CompareTo(object? other)
        {
            if (other == null) { return 0; }
            else { return Id.CompareTo(((Enumeration)other).Id); }
        }
    }

    public abstract class MessageBoxOption : Enumeration
    {
        public new string Name { get; set; }
        public uint Value { get; set; }
        public Type Type { get; set; }
        public MessageBoxOption(uint value, string name, Type type)
            : base(value, name)
        {
            Name = name;
            Value = value;
            Type = type;
        }

        public static MessageBoxOption[] GetAvailableOptions()
        {
            MessageBoxOption[] output = new MessageBoxOption[23];
            int arrindex = 0;
            GetAll<MessageBoxButton>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));
            GetAll<MessageBoxDefaultButton>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));
            GetAll<MessageBoxIcon>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));
            GetAll<MessageBoxModal>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));
            GetAll<MessageBoxType>().Select(it => (MessageBoxOption)it).ToList().ForEach(f => output[arrindex++] = (f));

            return output;
        }

        public static uint MbOptionsResolver(string[] input)
        {
            SendRemoteMessageCommand pshook = new();
            List<string> processed = new();
            MessageBoxOption[] allNames = GetAvailableOptions();
            uint output = 0;

            foreach (string item in input)
            {
                MessageBoxOption current = allNames.Where(p => p.Name == item).FirstOrDefault();

                if (current is not null)
                {
                    if (processed.Contains(item)) { pshook.WriteWarning("Duplicate item " + item + ". Ignoring."); }
                    else
                    {
                        output |= current.Value;
                        processed.Add(item);
                    }
                }
                else { throw new ArgumentOutOfRangeException("invalid type '" + item + "'."); }
            }

            return output;
        }
    }

    internal class Utilities
    {
        private static readonly Wrapper unWrapper = new();

        internal static string GetLastWin32Error() => unWrapper.GetLastWin32Error();
        internal static string GetLastWin32Error(int errorCode) => unWrapper.GetFormattedError(errorCode, ErrorType.SystemError);
    }

    public class AppType : Enumeration
    {
        public static AppType UnknownApp = new(0, "UnknownApp");
        public static AppType MainWindow = new(1, "MainWindow");
        public static AppType OtherWindow = new(2, "OtherWindow");
        public static AppType Service = new(3, "Service");
        public static AppType Explorer = new(4, "Explorer");
        public static AppType Console = new(5, "Console");
        public static AppType Critical = new(1000, "Critical");
        AppType(uint id, string name) : base(id, name) { }
    }
    public class MessageBoxButton : MessageBoxOption
    {
        public static MessageBoxButton MB_ABORTRETRYIGNORE = new(0x00000002, "MB_ABORTRETRYIGNORE");
        public static MessageBoxButton MB_CANCELTRYCONTINUE = new(0x00000006, "MB_CANCELTRYCONTINUE");
        public static MessageBoxButton MB_HELP = new(0x00004000, "MB_HELP");
        public static MessageBoxButton MB_OK = new(0x00000000, "MB_OK");
        public static MessageBoxButton MB_OKCANCEL = new(0x00000001, "MB_OKCANCEL");
        public static MessageBoxButton MB_RETRYCANCEL = new(0x00000005, "MB_RETRYCANCEL");
        public static MessageBoxButton MB_YESNO = new(0x00000004, "MB_YESNO");
        public static MessageBoxButton MB_YESNOCANCEL = new(0x00000003, "MB_YESNOCANCEL");

        public MessageBoxButton(uint value, string name) : base(value, name, typeof(MessageBoxButton)) { }
    }
    public class MessageBoxIcon : MessageBoxOption
    {
        public static MessageBoxIcon MB_ICONWARNING = new(0x00000030, "MB_ICONWARNING");
        public static MessageBoxIcon MB_ICONINFORMATION = new(0x00000040, "MB_ICONINFORMATION");
        public static MessageBoxIcon MB_ICONQUESTION = new(0x00000020, "MB_ICONQUESTION");
        public static MessageBoxIcon MB_ICONERROR = new(0x00000010, "MB_ICONERROR");

        public MessageBoxIcon(uint value, string name) : base(value, name, typeof(MessageBoxIcon)) { }
    }
    public class MessageBoxDefaultButton : MessageBoxOption
    {
        public static MessageBoxDefaultButton MB_DEFBUTTON1 = new(0x00000000, "MB_DEFBUTTON1");
        public static MessageBoxDefaultButton MB_DEFBUTTON2 = new(0x00000100, "MB_DEFBUTTON2");
        public static MessageBoxDefaultButton MB_DEFBUTTON3 = new(0x00000200, "MB_DEFBUTTON3");
        public static MessageBoxDefaultButton MB_DEFBUTTON4 = new(0x00000300, "MB_DEFBUTTON4");

        public MessageBoxDefaultButton(uint value, string name) : base(value, name, typeof(MessageBoxDefaultButton)) { }
    }
    public class MessageBoxModal : MessageBoxOption
    {
        public static MessageBoxModal MB_SYSTEMMODAL = new(0x00001000, "MB_SYSTEMMODAL");
        public static MessageBoxModal MB_TASKMODAL = new(0x00002000, "MB_TASKMODAL");

        public MessageBoxModal(uint value, string name) : base(value, name, typeof(MessageBoxModal)) { }
    }
    public class MessageBoxType : MessageBoxOption
    {
        public static MessageBoxType MB_RIGHT = new(0x00080000, "MB_RIGHT");
        public static MessageBoxType MB_RTLREADING = new(0x00100000, "MB_RTLREADING");
        public static MessageBoxType MB_SETFOREGROUND = new(0x00010000, "MB_SETFOREGROUND");
        public static MessageBoxType MB_TOPMOST = new(0x00040000, "MB_TOPMOST");
        public static MessageBoxType MB_SERVICE_NOTIFICATION = new(0x00200000, "MB_SERVICE_NOTIFICATION");

        public MessageBoxType(uint value, string name) : base(value, name, typeof(MessageBoxType)) { }
    }

    public sealed class DotNetVersionInfo
    {
        private readonly long _release;
        
        public Version Version { get; }
        public DotNetEditionInfo Edition { get; }
        public string ComputerName { get; }

        public DotNetVersionInfo(long minVersion, Version version, DotNetEditionInfo edition)
        {
            _release = minVersion;
            Version = version;
            Edition = edition;
            ComputerName = Environment.MachineName;
        }

        public DotNetVersionInfo(long minVersion, Version version, DotNetEditionInfo edition, string computerName)
        {
            _release = minVersion;
            Version = version;
            Edition = edition;
            ComputerName = Environment.MachineName;
        }

        public DotNetVersionInfo(Version version, DotNetEditionInfo edition)
        {
            _release = 0;
            Version = version;
            Edition = edition;
            ComputerName = Environment.MachineName;
        }

        public static DotNetVersionInfo GetInfoFromRelease(long release)
        {
            DotNetVersionInfo? previous = null;
            foreach (DotNetVersionInfo versionInfo in GetAvailableVersionInfo().Where(i => i._release != 0).OrderBy(i => i._release))
            {
                if (release == versionInfo._release)
                    return versionInfo;

                if (previous is null && release < versionInfo._release)
                    throw new ArgumentException("Invalid release number.");
                
                else if (previous is not null && release < versionInfo._release)
                    return previous;
                
                previous = versionInfo;
            }

            throw new ArgumentException("Invalid release number.");
        }

        public static DotNetVersionInfo[] GetAvailableVersionInfo(bool includeCoreVersion = false)
        {
            List<DotNetVersionInfo> infoList = new();
            using JsonDocument fromResourceDocument = JsonDocument.Parse(Properties.Resources.DotNetFrameworkVersionTable);
            foreach (JsonElement element in fromResourceDocument.RootElement.EnumerateArray())
                infoList.Add(new(
                    element.GetProperty("MinimumVersion").GetInt64(),
                    Version.Parse(element.GetProperty("Version").GetString()),
                    (DotNetEditionInfo)element.GetProperty("Edition").GetUInt32()
                ));

            if (includeCoreVersion)
            {
                using WebClient client = new();
                string dotnetCoreVerInfo = client.DownloadString(@"https://raw.githubusercontent.com/dotnet/core/main/release-notes/releases-index.json");
                using JsonDocument fromWebDocument = JsonDocument.Parse(dotnetCoreVerInfo);
                foreach (JsonElement element in fromWebDocument.RootElement.GetProperty("releases-index").EnumerateArray())
                    infoList.Add(new(
                        Version.Parse(element.GetProperty("channel-version").GetString()),
                        DotNetEditionInfo.Core
                    ));
            }

            return infoList.ToArray();
        }
    }

    public sealed class DotNetInstalledUpdateInfo
    {
        public string Version { get; }
        public string[] InstalledUpdates { get; }
        public string ComputerName { get; }

        public DotNetInstalledUpdateInfo(string version, string[] installedUpdates)
        {
            ComputerName = Environment.MachineName;
            Version = version;
            InstalledUpdates = installedUpdates;
        }

        public DotNetInstalledUpdateInfo(string computerName, string version, string[] installedUpdates)
        {
            ComputerName = computerName;
            Version = version;
            InstalledUpdates = installedUpdates;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                                                                                       //
    // ~ HResult format                                                                                      //
    //                                                                                                       //
    // +-------------------------------------------------------------------------------------------------+   //
    // | S | R | C | N | X |            Facility            |                    Code                    |   //
    // +-------------------------------------------------------------------------------------------------+   //
    //                                                                                                       //
    // S: Severity - 1 bit                                                                                   //
    // R: Reserved - 1 bit                                                                                   //
    // C: Customer - 1 bit                                                                                   //
    // N: Is NT    - 1 bit                                                                                   //
    // X: Reserved - 1 bit                                                                                   //
    //                                                                                                       //
    // Facility - 11 bits                                                                                    //
    // Code     - 16 bits                                                                                    //
    //                                                                                                       //
    // https://learn.microsoft.com/openspecs/windows_protocols/ms-erref/0642cb2f-2075-4469-918c-4441e69c548a //
    //                                                                                                       //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////

    public sealed class HResultInfo
    {
        public HResultSeverity Severity { get; }
        public HResultFacility Facility { get; }
        public int Code { get; }

        public HResultInfo(int hResult)
        {
            string binary = Convert.ToString(hResult, 2);
            if (binary.Length < 32)
            {
                Severity = HResultSeverity.Failure;
                Facility = HResultFacility.Default;
                Code = hResult;
            }
            else
            {
                Severity = (HResultSeverity)Convert.ToInt32(binary[0].ToString());
                Facility = (HResultFacility)Convert.ToInt32(binary.Substring(5, 11), 2);
                Code = Convert.ToInt32(binary.Substring(16), 2);
            }
        }
    }

    public sealed class ErrorInformation
    {
        private readonly int _errorCode;

        public int ErrorCode { get { return _errorCode; } }
        public string HexCode { get { return $"0x{Convert.ToString(_errorCode, 16).ToUpper()}"; } }
        public int? HResultCode {
            get {
                if (HResultInfo is not null)
                    return HResultInfo.Code;

                return null;
            }
        }
        public string? HResultHexCode {
            get {
                if (HResultCode is not null)
                    return $"0x{Convert.ToString(HResultCode.Value, 16).ToUpper()}";

                return null;
            }
        }
        public string SymbolicName { get; }
        public string? Description { get; }
        public HResultInfo? HResultInfo { get; }

        internal ErrorInformation(int errorCode, string symName, string? desc, HResultInfo? hrInfo)
            => (_errorCode, SymbolicName, Description, HResultInfo) = (errorCode, symName, desc, hrInfo);
    }
}