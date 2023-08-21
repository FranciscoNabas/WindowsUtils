using System.Management.Automation;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using System.Management.Automation.Provider;
using Microsoft.Win32.SafeHandles;
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

internal sealed class CmdletNativeContext : IDisposable
{
    private static ProgressMappedShare _progressShare
        = ProgressMappedShare.GetShare();

    private static WarningMappedShare _warningShare
        = WarningMappedShare.GetShare();

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
    }
    
    // Operators
    public static explicit operator CmdletContextBase(CmdletNativeContext context)
    {
        return new CmdletContextBase(
            new CmdletContextBase.WriteProgressWrapper(context.NativeWriteProgress),
            new CmdletContextBase.WriteWarningWrapper(context.NativeWriteWarning),
            _progressShare.DangerousGetMappedFileHandle(),
            _warningShare.DangerousGetMappedFileHandle()
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
            _progressShare.Dispose();
            _warningShare.Dispose();

#pragma warning disable CS8625
            _progressShare = null;
            _warningShare = null;
#pragma warning restore CS8625
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
    private void NativeWriteProgress(ulong dataSize)
    {
        SafeMemoryMappedViewHandle view = _progressShare.MapView(
            out Guid viewId,
            FileMapAccess.FILE_MAP_READ,
            new LARGE_UINTEGER(0),
            dataSize
        );

        MAPPED_PROGRESS_DATA progressData = (MAPPED_PROGRESS_DATA)Marshal.PtrToStructure(view.DangerousGetHandle(), typeof(MAPPED_PROGRESS_DATA));
        ProgressRecord record = new(progressData.ActivityId, progressData.Activity, progressData.StatusDescription) {
            CurrentOperation = progressData.CurrentOperation,
            ParentActivityId = progressData.ParentActivityId,
            PercentComplete = progressData.PercentComplete,
            RecordType = progressData.RecordType,
            SecondsRemaining = progressData.SecondsRemaining
        };

        _command.WriteProgress(record);
        _progressShare.UnmapView(viewId);
    }

    private void NativeWriteWarning(ulong dataSize)
    {
        SafeMemoryMappedViewHandle view = _warningShare.MapView(
            out Guid viewId,
            FileMapAccess.FILE_MAP_READ,
            new LARGE_UINTEGER(0),
            dataSize
        );

        string text = Marshal.PtrToStringUni(view.DangerousGetHandle());

        _command.WriteWarning(text);
        _warningShare.UnmapView(viewId);
    }
    
    internal bool HasErrors()
        => _accumulatedErrors.Count > 0;
    
    // To be implemented in the future, if necessary.
    internal void StopProcessing()
    {
        Stopping = true;
    }
}