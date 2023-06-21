using WindowsUtils.Core;
using System.Runtime.InteropServices;

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
    /// Object from Invoke-RemoteMessage
    /// </summary>
    public class MessageResponse
    {
        public uint SessionId => wrapper.SessionId;
        public MessageBoxReturn Response => (MessageBoxReturn)wrapper.Response;

        public static explicit operator MessageResponse(MessageResponseBase mrbase) => new (mrbase);
        public MessageResponse(MessageResponseBase mrbase) => wrapper = mrbase;

        private readonly MessageResponseBase wrapper;
    }

    internal sealed class WtsSession : IDisposable
    {
        internal string ComputerName { get; set; }
        internal Interop.SafeSystemHandle SessionHandle { get; set; }
        
        internal WtsSession(string computerName)
        {
            SessionHandle = Interop.NativeFunctions.WTSOpenServer(computerName);
            if (SessionHandle is null || SessionHandle.IsInvalid || SessionHandle.IsClosed)
                throw new NativeException(Marshal.GetLastWin32Error());

            ComputerName = computerName;
        }
        
        public void Dispose()
        {
            SessionHandle.Dispose();
            GC.SuppressFinalize(this);
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
    
}