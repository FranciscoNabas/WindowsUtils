#pragma once

#pragma unmanaged

#include "../Support/WuStdException.h"

#pragma managed

namespace WindowsUtils
{
	using namespace System;
	using namespace System::Runtime::Serialization;

	public enum class ErrorType
	{
		SystemError,
		NtError,
		FdiError,
		FciError
	};


	[Serializable()]
	public ref class NativeException : public Exception
	{
	public:
		property Int32 ErrorCode { Int32 get() { return _errorCode; } }

#if defined(_DEBUG)
		property String^ CompactTrace { String^ get() { return _compactTrace; } }

		NativeException(const Core::WuStdException& ex)
			: Exception(gcnew String(ex.Message().GetBuffer())),
			_errorCode(ex.ErrorCode()),
			_compactTrace(gcnew String(ex.CompactTrace().GetBuffer()))
		{ }
#else
		NativeException(const Core::WuStdException& ex)
			: Exception(gcnew String(ex.Message().GetBuffer())),
			_errorCode(ex.ErrorCode())
		{ }
#endif
		NativeException(Int32 errorCode)
			: Exception((gcnew String(Core::WuStdException(errorCode, __FILEW__, __LINE__).Message().GetBuffer()))->Trim()),
			_errorCode(errorCode)
		{ }

		NativeException(Int32 errorCode, String^ message)
			: Exception(message), _errorCode(errorCode)
		{ }

		NativeException(Int32 errorCode, String^ message, String^ compactTrace)
			: Exception(message), _errorCode(errorCode), _compactTrace(compactTrace)
		{ }

		NativeException(Int32 errorCode, String^ message, Exception^ inner_exception)
			: Exception(message, inner_exception), _errorCode(errorCode)
		{ }

	protected:
		NativeException()
			: Exception()
		{ }

		NativeException(SerializationInfo^ info, StreamingContext context)
			: Exception(info, context)
		{ }

	private:
		Int32 _errorCode;
		String^ _compactTrace;
	};
}