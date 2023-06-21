using System.Management.Automation;

namespace WindowsUtils.Engine;

[AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
public sealed class ValidateFileExistsAttribute : ValidateArgumentsAttribute
{
    public ValidateFileExistsAttribute() : base() { }

    protected override void Validate(object arguments, EngineIntrinsics engineIntrinsics)
    {
        if (arguments is string value)
        {
            if (!File.Exists(value))
                throw new ValidationMetadataException("File not found.");
        }
        else
            throw new ValidationMetadataException($"Wrong type of '{arguments.GetType()}'.");
    }
}

[AttributeUsage(AttributeTargets.Parameter, Inherited = false)]
public sealed class NotNullWhenAttribute : Attribute
{
    public bool ReturnValue { get; }

    public NotNullWhenAttribute(bool returnValue) => ReturnValue = returnValue;
}