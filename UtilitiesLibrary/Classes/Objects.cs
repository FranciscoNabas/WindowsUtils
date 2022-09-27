using WindowsUtils.Abstraction;

#nullable enable
namespace WindowsUtils
{
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
        SessionState(uint id, string name) : base(id, name) { }
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
}

#nullable enable
namespace WindowsUtils.TerminalServices
{
    internal class Session
    {
        internal string? ComputerName { get; set; }
        internal SystemSafeHandle? SessionHandle { get; set; }
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