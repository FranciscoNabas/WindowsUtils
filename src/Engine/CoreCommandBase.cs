using System.Diagnostics;
using System.Management.Automation;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using System.Management.Automation.Provider;
using WindowsUtils.Core;
using WindowsUtils.Interop;

namespace WindowsUtils.Commands;

public abstract class CoreCommandBase : PSCmdlet, IDynamicParameters
{
    private bool _suppressWildcardExpansion;
    private bool _force;
    private object? _dynamicParameters;

    internal virtual CmdletNativeContext CmdletContext
    {
        get
        {
            return new(this) {
                Force = Force,
                DynamicParameters = DynamicParameters
            };
        }
    }
    internal virtual SwitchParameter SuppressWildcardExpansion
    {
        get { return _suppressWildcardExpansion; }
        set { _suppressWildcardExpansion = value; }
    }
    protected internal object? DynamicParameters
        => _dynamicParameters;
    internal virtual SwitchParameter Force
    {
        get { return _force; }
        set { _force = value; }
    }

    internal virtual object? GetDynamicParameters(CmdletNativeContext context)
        => null;

    public object? GetDynamicParameters()
    {
        CmdletNativeContext cmdletContext = CmdletContext;
		cmdletContext.PassThru = false;
        try
		{
			_dynamicParameters = GetDynamicParameters(cmdletContext);
		}
		catch (ItemNotFoundException)
		{
			_dynamicParameters = null;
		}
		catch (ProviderNotFoundException)
		{
			_dynamicParameters = null;
		}
		catch (System.Management.Automation.DriveNotFoundException)
		{
			_dynamicParameters = null;
		}
        
		return _dynamicParameters;
    }
}

internal unsafe sealed class CmdletNativeContext : IDisposable
{
    private readonly UnmanagedMemoryStream _progressStream;
    private readonly UnmanagedMemoryStream _warningStream;
    private readonly UnmanagedMemoryStream _informationStream;
    private readonly UnmanagedMemoryStream _objectStream;
    private readonly UnmanagedMemoryStream _errorStream;

    private readonly IntPtr _progressBuffer;
    private readonly IntPtr _warningBuffer;
    private readonly IntPtr _informationBuffer;
    private readonly IntPtr _objectBuffer;
    private readonly IntPtr _errorBuffer;

    private readonly PSCmdlet _command;
    private readonly bool _streamErrors;
    private readonly PSCredential _credential = PSCredential.Empty;
    private readonly Collection<PSObject> _accumulatedObjects = new();
    private readonly Collection<ErrorRecord> _accumulatedErrors = new();

    private bool _force;
    private CmdletProvider? _providerInstance;
    
    internal CommandOrigin Origin { get; } = CommandOrigin.Internal;
    internal CmdletProvider? ProviderInstance
    {
        get { return _providerInstance; }
        set { _providerInstance = value; }
    }
    internal object? DynamicParameters { get; set; }
    internal InvocationInfo? MyInvocation
    {
        get
        {
            if (_command is not null)
                return _command.MyInvocation;

            return null;
        }
    }
    internal bool PassThru { get; set; }
    internal PSDriveInfo? Drive { get; set; }
    internal PSCredential Credential
    {
        get
        {
            PSCredential result = _credential;
            if (_credential is null && Drive is not null)
                result = Drive.Credential;

            return result;
        }
    }
    internal SwitchParameter Force
    {
        get { return _force; }
        set { _force = value; }
    }
    internal bool Stopping { get; private set; }

    // Constructors.
    internal CmdletNativeContext(PSCmdlet command)
    {
        _command = command;
        _streamErrors = true;
        Origin = command.CommandOrigin;
        PassThru = true;

        _progressBuffer = Marshal.AllocHGlobal(128);
        _warningBuffer = Marshal.AllocHGlobal(128);
        _informationBuffer = Marshal.AllocHGlobal(128);
        _objectBuffer = Marshal.AllocHGlobal(128);
        _errorBuffer = Marshal.AllocHGlobal(128);

        _progressStream = new((byte*)_progressBuffer.ToPointer(), 128, 128, FileAccess.ReadWrite);
        _warningStream = new((byte*)_warningBuffer.ToPointer(),128, 128, FileAccess.ReadWrite);
        _informationStream = new((byte*)_informationBuffer.ToPointer(), 128, 128, FileAccess.ReadWrite);
        _objectStream = new((byte*)_informationBuffer.ToPointer(), 128, 128, FileAccess.ReadWrite);
        _errorStream = new((byte*)_errorBuffer.ToPointer(), 128, 128, FileAccess.ReadWrite);
    }
    
    // Operators
    public static explicit operator CmdletContextBase(CmdletNativeContext context)
    {
        return new CmdletContextBase(
            new CmdletContextBase.WriteProgressWrapper(context.NativeWriteProgress),
            new CmdletContextBase.WriteWarningWrapper(context.NativeWriteWarning),
            new CmdletContextBase.WriteInformationWrapper(context.NativeWriteInformation),
            new CmdletContextBase.WriteObjectWrapper(context.NativeWriteObject),
            new CmdletContextBase.WriteErrorWrapper(context.NativeWriteError),
            context._progressStream.PositionPointer,
            context._warningStream.PositionPointer,
            context._informationStream.PositionPointer,
            context._objectStream.PositionPointer,
            context._errorStream.PositionPointer
        );
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
            _progressStream.Close();
            _warningStream.Close();
            _informationStream.Close();
            _objectStream.Close();
            _errorStream.Close();

            Marshal.FreeHGlobal(_progressBuffer);
            Marshal.FreeHGlobal(_warningBuffer);
            Marshal.FreeHGlobal(_informationBuffer);
            Marshal.FreeHGlobal(_objectBuffer);
            Marshal.FreeHGlobal(_errorBuffer);
        }
    }

    // Default Cmdlet methods.
    internal void WriteDebug(string text)
        => _command.WriteDebug(text);

    internal void WriteInformation(InformationRecord record)
        => _command.WriteInformation(record);

    internal void WriteProgress(ProgressRecord record)
        => _command.WriteProgress(record);

    internal void WriteVerbose(string text)
        => _command.WriteVerbose(text);

    internal void WriteObject(object obj)
    {
        if (Stopping)
            throw new PipelineStoppedException();

        if (PassThru)
            _command.WriteObject(obj);
        else
            _accumulatedObjects.Add(PSObject.AsPSObject(obj));
    }

    internal void WriteError(ErrorRecord record)
    {
        if (Stopping)
            throw new PipelineStoppedException();

        if (_streamErrors)
            _command.WriteError(record);
        else
            _accumulatedErrors.Add(record);
    }

    // These methods are not meant to be called directly, instead they should be
    // used to create wrapped delegates, who's function pointer will be used by
    // unmanaged code.
    private void NativeWriteProgress()
    {
        MAPPED_PROGRESS_DATA progressData = (MAPPED_PROGRESS_DATA)Marshal.PtrToStructure((IntPtr)_progressStream.PositionPointer, typeof(MAPPED_PROGRESS_DATA));
        ProgressRecord record = new(progressData.ActivityId, progressData.Activity, progressData.StatusDescription) {
            CurrentOperation = progressData.CurrentOperation,
            ParentActivityId = progressData.ParentActivityId,
            PercentComplete = progressData.PercentComplete,
            RecordType = progressData.RecordType,
            SecondsRemaining = progressData.SecondsRemaining
        };

        _command.WriteProgress(record);
    }

    private void NativeWriteWarning()
    {
        string text = Marshal.PtrToStringUni((IntPtr)_warningStream.PositionPointer);
        _command.WriteWarning(text);
    }

    private void NativeWriteInformation()
    {
        MAPPED_INFORMATION_DATA data = (MAPPED_INFORMATION_DATA)Marshal.PtrToStructure((IntPtr)_informationStream.PositionPointer, typeof(MAPPED_INFORMATION_DATA));
        InformationRecord record = new(data.Text, data.Source) {
            Computer = data.Computer,
            ManagedThreadId = (uint)Thread.CurrentThread.ManagedThreadId,
            NativeThreadId = data.NativeThreadId,
            ProcessId = (uint)Process.GetCurrentProcess().Id,
            TimeGenerated = DateTime.FromFileTime((long)data.TimeGenerated),
            User = data.User
        };

        unsafe
        {
            if (data.Tags != null && data.TagCount > 0)
                for (uint i = 0; i < data.TagCount; i++)
                    record.Tags.Add(Marshal.PtrToStringUni(data.Tags[i]));
        }

        _command.WriteInformation(record);
    }

    private void NativeWriteObject(WriteOutputType objectType)
    {
        object output;
        switch (objectType)
        {
            case WriteOutputType.TCPING_OUTPUT:
                TCPING_OUTPUT nativeObject = (TCPING_OUTPUT)Marshal.PtrToStructure((IntPtr)_objectStream.PositionPointer, typeof(TCPING_OUTPUT));
                output = new TcpingProbeInfo(nativeObject);
                break;

            case WriteOutputType.TCPING_STATISTICS:
                TCPING_STATISTICS nativeStats = (TCPING_STATISTICS)Marshal.PtrToStructure((IntPtr)_objectStream.PositionPointer, typeof(TCPING_STATISTICS));
                output = new TcpingStatistics(nativeStats);
                break;

            case WriteOutputType.TESTPORT_OUTPUT:
                TESTPORT_OUTPUT testPortStats = (TESTPORT_OUTPUT)Marshal.PtrToStructure((IntPtr)_objectStream.PositionPointer, typeof(TESTPORT_OUTPUT));
                output = new TestPortInfo(testPortStats);
                break;

            case WriteOutputType.WWUSTRING:
                WWuString nativeString = (WWuString)Marshal.PtrToStructure((IntPtr)_objectStream.PositionPointer, typeof(WWuString));
                output = nativeString;
                break;

            case WriteOutputType.LAB_STRUCT:
                LAB_STRUCT labStruct = (LAB_STRUCT)Marshal.PtrToStructure((IntPtr)_objectStream.PositionPointer, typeof(LAB_STRUCT));
                output = new LabStruct(labStruct);
                break;

            case WriteOutputType.PROCESS_MODULE_INFO:
                PROCESS_MODULE_INFO modInfo = (PROCESS_MODULE_INFO)Marshal.PtrToStructure((IntPtr)_objectStream.PositionPointer, typeof(PROCESS_MODULE_INFO));
                output = new ProcessModuleInfo(modInfo);
                break;

            default:
                return;
        }

        _command.WriteObject(output);
    }

    private void NativeWriteError()
    {
        MAPPED_ERROR_DATA errorData = (MAPPED_ERROR_DATA)Marshal.PtrToStructure(_errorBuffer, typeof(MAPPED_ERROR_DATA));
        _command.WriteError(new(
            new NativeException(errorData.ErrorCode, errorData.ErrorMessage, errorData.CompactTrace),
            errorData.ErrorId,
            errorData.Category,
            errorData.TargetObject
        ));
    }
    
    internal bool HasErrors()
        => _accumulatedErrors.Count > 0;
    
    // To be implemented in the future, if necessary.
    internal void StopProcessing()
    {
        Stopping = true;
    }
}