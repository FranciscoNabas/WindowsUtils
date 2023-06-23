using System.Runtime.InteropServices;
using WindowsUtils.Core;

namespace WindowsUtils.Interop
{
    internal partial class NativeFunctions
    {
        [DllImport("Wtsapi32.dll", CharSet = CharSet.Unicode, SetLastError = true, EntryPoint = "WTSOpenServerW")]
        internal extern static SafeSystemHandle WTSOpenServer(string pServerName);
    }
}

namespace WindowsUtils.TerminalServices
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
}