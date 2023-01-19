using System.Management;
using System.Security.Principal;
using System.Security.AccessControl;
using Microsoft.Win32;
using WindowsUtils.Core;
using System.Text;

namespace WindowsUtils
{
    /// <summary>
    /// Object used on WTS Cmdlets
    /// </summary>
    public class ComputerSession
    {
        public int SessionId => wrapper.SessionId;
        public string UserName => wrapper.UserName;
        public string SessionName => wrapper.SessionName;
        public TimeSpan IdleTime => wrapper.IdleTime;
        public DateTime LogonTime => (DateTime)wrapper.LogonTime;
        public SessionState SessionState => (SessionState)wrapper.SessionState;
        public string ComputerName => wrapper.ComputerName;
        
        public static explicit operator ComputerSession(ComputerSessionBase csbase) => new(csbase);
        public ComputerSession(ComputerSessionBase csbase) => wrapper = csbase;

        private readonly ComputerSessionBase wrapper;
    }

    /// <summary>
    /// Object from Get-ResourceMessageTable
    /// </summary>
    public class ResourceMessageTable
    {
        public long Id => wrapper.Id;
        public string Message => wrapper.Message;

        public static explicit operator ResourceMessageTable(ResourceMessageTableCore resmesbase) => new(resmesbase);
        public ResourceMessageTable(ResourceMessageTableCore resmesbase) => wrapper = resmesbase;

        private readonly ResourceMessageTableCore wrapper;
    }

    /// <summary>
    /// Object from Get-ObjectHandle
    /// </summary>
    public class ObjectHandle
    {
        public string InputObject => wrapper.InputObject;
        public string Name => wrapper.Name;
        public uint ProcessId => wrapper.ProcessId;
        public string Description => wrapper.Description;
        public string ProductName => wrapper.ProductName;
        public object FileVersion
        {
            get
            {
                try
                {
                    return Version.Parse(wrapper.FileVersion);
                }
                catch (Exception)
                {
                    return wrapper.FileVersion;
                }
            }
        }
        public string CompanyName => wrapper.CompanyName;
        public string ImagePath => wrapper.ImagePath;

        public static explicit operator ObjectHandle(ObjectHandleBase ohbase) => new (ohbase);
        public ObjectHandle(ObjectHandleBase ohbase) => wrapper = ohbase;

        private readonly ObjectHandleBase wrapper;
    }

    /// <summary>
    /// Oject from Invoke-RemoteMessage
    /// </summary>
    public class MessageResponse
    {
        public uint SessionId => wrapper.SessionId;
        public MessageBoxReturn Response => (MessageBoxReturn)wrapper.Response;

        public static explicit operator MessageResponse(MessageResponseBase mrbase) => new (mrbase);
        public MessageResponse(MessageResponseBase mrbase) => wrapper = mrbase;

        private readonly MessageResponseBase wrapper;
    }

    public static class WtsSession
    {
        public static string? ComputerName { get; set; }
        public static SystemSafeHandle? SessionHandle { get; set; }

        public static void StageComputerSession(string computerName)
        {
            if (SessionHandle == null || string.IsNullOrEmpty(ComputerName))
            {
                //TODO: Error handling.
                SessionHandle = Interop.WTSOpenServerW(computerName);
                if (SessionHandle is null)
                    throw new SystemException(Utilites.GetLastWin32Error());

                ComputerName = computerName;
            }
            else
            {
                if (ComputerName != computerName)
                {
                    if (!SessionHandle.IsInvalid && !SessionHandle.IsClosed)
                        Interop.CloseHandle(SessionHandle.ToIntPtr());

                    SessionHandle = Interop.WTSOpenServerW(computerName);
                    if (SessionHandle is null)
                        throw new SystemException(Utilites.GetLastWin32Error());

                    ComputerName = computerName;
                }
            }
        }
    }
    
    public class SessionState : Enumeration
    {
        public static SessionState Active = new(0, "Active");
        public static SessionState Connected = new(1, "Connected");
        public static SessionState ConnectQuery = new(2, "ConnectQuery");
        public static SessionState Shadow = new(3, "Shadow");
        public static SessionState Disconnected = new(4, "Disconnected");
        public static SessionState Idle = new(5, "Idle");
        public static SessionState Listen = new(6, "Listen");
        public static SessionState Reset = new(7, "Reset");
        public static SessionState Down = new(8, "Down");
        public static SessionState Init = new(9, "Init");
        
        public static implicit operator uint(SessionState s) => s.Id;
        public static explicit operator SessionState(uint id) => new SessionState(id);
        SessionState(uint id, string name) : base(id, name) { }
        SessionState(uint id) : base(id, GetById<SessionState>(id).ToString()) { }
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

    //internal class ProcessManager
    //{
    //    internal string ComputerName { get; private set; }
    //    internal int SessionId { get; private set; }
    //    internal uint ProcessId { get; private set; }
    //    internal string CommandLine { get; private set; }
    //    internal bool HasExited { get; private set; }
    //    internal OperationType Operation { get; }
    //    internal int ProcessTtl { get; private set; }

    //    internal static readonly string CurrentUser = WindowsIdentity.GetCurrent().Name;
    //    private static RegistryKey? regapproot;

    //    internal enum OperationType
    //    {
    //        DisconnectSession
    //    }

    //    private ProcessManager(string pcname, int sessionid, uint procid, string commandline, OperationType optype, int procttl)
    //        => (ComputerName, SessionId, ProcessId, CommandLine, HasExited, Operation, ProcessTtl) = (pcname, sessionid, procid, commandline, false, optype, procttl);

    //    internal static ProcessManager Create(string pcname, int sessionid, OperationType optype, string commandline, int procttl)
    //    {
    //        /*
    //         * Checking if a process already exists for this session and PC.
    //         */
    //        regapproot ??= Registry.LocalMachine.OpenSubKey(
    //            "SOFTWARE\\WindowsUtils\\ProcessManager"
    //            , RegistryKeyPermissionCheck.ReadWriteSubTree
    //            , RegistryRights.QueryValues | RegistryRights.CreateSubKey | RegistryRights.EnumerateSubKeys
    //        );
    //        if (regapproot is null)
    //            throw new ArgumentNullException("Error loading module registry key: not found.");

    //        object rgsearch = CheckOperationRegistry(pcname, sessionid, optype, ref regapproot);
    //        if (rgsearch.ToString() != "False")
    //            throw new InvalidOperationException((string)rgsearch);

    //        /*
    //         * Creating remote process.
    //         */
    //        ManagementScope scope = new($"\\\\{pcname}\\root\\cimv2");
    //        scope.Connect();

    //        using ManagementClass winprocess = new(scope, new ManagementPath("Win32_Process"), null);
    //        using ManagementBaseObject inparam =  winprocess.GetMethodParameters("Create");
    //        inparam.Properties["CommandLine"].Value = commandline;

    //        using ManagementBaseObject result = winprocess.InvokeMethod("Create", inparam, null);

    //        if ((uint)result.Properties["ReturnValue"].Value != 0)
    //            throw new SystemException($"Error creating remote process. Error code: {(long)result.Properties["ReturnValue"].Value}.");

    //        /*
    //         * Storing in the registry
    //         */
    //        uint _procid = (uint)result.Properties["ProcessId"].Value;
    //        string entrykeyname = $"{sessionid}_{pcname}";
    //        using RegistryKey newpmentry = regapproot.CreateSubKey(entrykeyname, true);
    //        newpmentry.SetValue("CallerUserName", CurrentUser);
    //        newpmentry.SetValue("OperationType", (uint)optype, RegistryValueKind.DWord);
    //        newpmentry.SetValue("ProcessId", _procid, RegistryValueKind.DWord);
    //        newpmentry.SetValue("CommandLine", commandline);
    //        newpmentry.SetValue("InputDate", DateTime.Now.ToString());

    //        /*
    //         * Creating the registry cleanup task
    //         */
    //        byte[] scrbytes = Encoding.Unicode.GetBytes(string.Format(Properties.Resources.DisconnectWMessageCleanRegPS1, entrykeyname, _procid));
    //        string encodedscript = Convert.ToBase64String(scrbytes);
    //        string tstartboundary = DateTime.Now.AddMinutes(procttl).ToString("yyyy-MM-ddThh:mm:ss");

    //        string taskdefinition = string.Format(Properties.Resources.DisconnectWMessageCleanRegTask, tstartboundary, $"-ExecutionPolicy Bypass -EncodedCommand {encodedscript}");

    //        ProcessManager newprocess = new(
    //            pcname
    //            ,sessionid
    //            ,(uint)result.Properties["ProcessId"].Value
    //            ,commandline
    //            ,optype
    //            ,procttl
    //        );

    //        return newprocess;
    //    }

    //    private static object CheckOperationRegistry(string pcname, int sessionid, OperationType type, ref RegistryKey provider)
    //    {
    //        foreach (string key in provider.GetSubKeyNames())
    //        {
    //            if (key == $"{sessionid}_{pcname}")
    //            {
    //                using RegistryKey? tempkey = provider.OpenSubKey(key);
    //                if (tempkey is not null)
    //                {
    //                    object? optype = tempkey.GetValue("OperationType");
    //                    if (optype is not null)
    //                        if ((int)optype == (int)type)
    //                        {
    //                            object? caller = tempkey.GetValue("OperationType");
    //                            if (caller is not null)
    //                                return caller.ToString();
    //                        }
    //                }
    //            }
    //        }

    //        return false;
    //    }
    //}
}