using System.Collections;
using System.ServiceProcess;
using System.Management.Automation;
using System.Management.Automation.Language;

namespace WindowsUtils.ArgumentCompletion;

#region Service completers
internal abstract class ServiceArgumentCompleterBase : IArgumentCompleter
{
    public abstract IEnumerable<CompletionResult> CompleteArgument(string commandName, string parameterName, string wordToComplete, CommandAst commandAst, IDictionary fakeBoundParamenters);
}

internal class ServiceNameCompleter : ServiceArgumentCompleterBase
{
    public override IEnumerable<CompletionResult> CompleteArgument(string commandName, string parameterName, string wordToComplete, CommandAst commandAst, IDictionary fakeBoundParameters)
        => ServiceController.GetServices().Where(s => s.ServiceName.StartsWith(wordToComplete, StringComparison.OrdinalIgnoreCase)).Select(s => new CompletionResult(s.ServiceName));
}

internal class ServiceDisplayNameCompleter : ServiceArgumentCompleterBase
{
    public override IEnumerable<CompletionResult> CompleteArgument(string commandName, string parameterName, string wordToComplete, CommandAst commandAst, IDictionary fakeBoundParameters)
        => ServiceController.GetServices().Where(s => s.DisplayName.StartsWith(wordToComplete, StringComparison.OrdinalIgnoreCase)).Select(s => new CompletionResult($"'{s.DisplayName}'"));
}
#endregion