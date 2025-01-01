# WindowsUtils Stub Dispatching system

## Motivation

Originally calls from the CLR were routed to the unmanaged APIs directly through the wrappers.  
This works fine for the purpose of calling the APIs, but we were having issues catching exceptions.  
In mixed mode, if an unmanaged object is thrown, when crossing back to managed land the object gets
wrapped in an exception of type `SEHException`.  
When searching for the appropriate `catch` clause, there are two possibilities:

- If a native C++ type is encountered, the exception is unwrapped and compared to the type encountered. This comparison allows a native C++ type to be caught in the normal way.
- However, if a catch clause of type `SEHException` or any of its base classes is examined first, the clause will intercept the exception. Therefore, you should place all catch clauses that catch native C++ types first before any catch clauses of CLR types.  

With this in mind we should be able to catch unmanaged objects thrown in unmanaged code as long as we catch them before the managed exception,
but this was not the case in some places.  
No mater what object or object model we threw we'd never catch it.  
  
Because of this we wrote a dispatching system that wraps unmanaged calls and catches exceptions.  
Exceptions caught by these stubs are routed to a managed marshaler that converts it to the appropriate managed exception and throws it.  
To route the exception we use a similar system to the one used writing to the PowerShell streams from unmanaged code, a delegate/function pointer system.  
  
With this new exception system we also improved the overall managed and unmanaged exceptions.  
Now it's possible to throw .NET exceptions, E.g., `System.ArgumentException` straight from unmanaged code.
We also hold more information about exceptions that allows us to translate them seamlessly to a `System.Management.Automation.ErrorRecord`.  
  
## Workflow

Using this system calling into unmanaged code follows this workflow:  
  
- The user calls a `Cmdlet` through PowerShell.
- The `Cmdlet` works the parameters to a state the API can handle and calls a `Wrapper`.
- The `Wrapper` marshals arguments to their unmanaged counterparts and calls the `Dispatch` function from the corresponding stub.
- The `Dispatch` function wraps the call to the corresponding unmanaged API in a `try-catch` block and calls it by forwarding the arguments.
- The unmanaged API performs the work, calling as many nested functions as necessary.
- The result is usually routed back through the `Stub` to the caller via a `by-ref` parameter, although there are cases where it's purely returned.

If an exception is thrown in unmanaged code this happens:  
  
- The `Stub` catches the exception and calls the marshaler via a function pointer.
- The marshaler, already in managed code checks if the exception can be marshaled as a default `COR_*` exception, if not it wraps it in a `NativeException`.
- The marshaler is a non-returning method that always throws the newly marshaled exception.

The reason for this system is to be able to centralize exception marshaling and avoid having specific `try-catch` blocks throughout unmanaged code that
would be a nightmare to maintain.  
Furthermore, if we need to expand this system to accommodate new managed and unmanaged exceptions we can do so without much work.  

## Drawbacks and future improvement

Some of the drawbacks for this system are:

- Relative unmanaged API obfuscation. Now the wrapper calls a `Dispatch` function where although descriptive enough it adds an extra step not so straight forward to understand.
- Stack trace pollution. Since unmanaged exceptions are **always** thrown by the marshaler these steps are caught in the stack trace, adding to the confusion.
- The dispatchers are templated functions so we can have generic code, but with enough complexity so the compiler knows what to call.

For future improvements we might add new exceptions or somehow fix the stack trace problem.  
I would also like to further improve the stack trace to include deeper unmanaged function calls.  
We might also change the way we dispatch calls to simplify it and/or reduce the boiler plate.

## Reference

[Basic Concepts in Using Managed Exceptions](https://learn.microsoft.com/cpp/dotnet/basic-concepts-in-using-managed-exceptions)  
[Differences in Exception Handling Behavior Under /CLR](https://learn.microsoft.com/cpp/dotnet/differences-in-exception-handling-behavior-under-clr)
