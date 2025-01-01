#pragma once

#pragma unmanaged

#include "../Support/WuException.h"

#pragma managed

namespace WindowsUtils
{
	using namespace System;
	using namespace System::Runtime::Serialization;
	using namespace System::Management::Automation;

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
		property Int32 ErrorCode { Int32 get() { return m_errorCode; } }
		property ErrorRecord^ Record { ErrorRecord^ get() { return m_record; } }

#if defined(_DEBUG)
		property String^ CompactTrace { String^ get() { return m_compactTrace; } }

		NativeException(const Core::WuException& ex)
			: Exception(gcnew String(ex.Message().Raw())),
			m_errorCode(ex.ErrorCode()),
			m_compactTrace(gcnew String(ex.CompactTrace().Raw())),
			m_record(gcnew ErrorRecord(this, gcnew String(ex.Id().Raw()), static_cast<ErrorCategory>(ex.Category()), nullptr))
		{ }
#else
		NativeException(const Core::WuException& ex)
			: Exception(gcnew String(ex.Message().Raw())),
			m_errorCode(ex.ErrorCode())
		{ }
#endif
		NativeException(Int32 errorCode)
			: Exception(gcnew String(Core::WuNativeException::GetErrorMessage(errorCode).Raw())),
			m_errorCode(errorCode), m_record(gcnew ErrorRecord(this, nullptr, ErrorCategory::NotSpecified, nullptr))
		{ }
		
		NativeException(Int32 errorCode, String^ message)
			: Exception(message), m_errorCode(errorCode), m_record(gcnew ErrorRecord(this, nullptr, ErrorCategory::NotSpecified, nullptr))
		{ }

		NativeException(Int32 errorCode, String^ message, String^ compactTrace)
			: Exception(message), m_errorCode(errorCode), m_compactTrace(compactTrace),
			m_record(gcnew ErrorRecord(this, nullptr, ErrorCategory::NotSpecified, nullptr))
		{ }

		NativeException(Int32 errorCode, String^ message, Exception^ innerException)
			: Exception(message, innerException), m_errorCode(errorCode),
			m_record(gcnew ErrorRecord(this, nullptr, ErrorCategory::NotSpecified, nullptr))
		{ }

	protected:
		NativeException()
			: Exception()
		{ }

		NativeException(SerializationInfo^ info, StreamingContext context)
			: Exception(info, context)
		{ }

	private:
		Int32 m_errorCode;
		String^ m_compactTrace;
		ErrorRecord^ m_record;
	};
}