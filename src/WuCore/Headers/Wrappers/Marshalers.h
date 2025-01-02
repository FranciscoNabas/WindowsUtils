#pragma once
#pragma unmanaged

#include "../Support/WuException.h"
#include "../Support/Notification.h"

#pragma managed

#include "NativeException.h"

namespace WindowsUtils::Core
{
	using namespace System::Runtime::InteropServices;

	delegate void ExceptionMarshalerProxy(const WuException& ex);

	public ref class ExceptionMarshaler abstract sealed
	{
	public:
		static property UnmanagedExceptionMarshaler NativePtr { UnmanagedExceptionMarshaler get() { return m_marshalerPtr; } }

	private:
		static ExceptionMarshaler()
		{
			m_delegate = gcnew ExceptionMarshalerProxy(&MarshalException);
			m_marshalerPtr = static_cast<UnmanagedExceptionMarshaler>(Marshal::GetFunctionPointerForDelegate(m_delegate).ToPointer());
		}

		[[noreturn]] static void MarshalException(const WuException& ex)
		{
			if (ex.ResolveCor())
				throw Marshal::GetExceptionForHR(ex.ErrorCode());

			throw gcnew NativeException(ex);
		}

		static ExceptionMarshalerProxy^ m_delegate;
		static UnmanagedExceptionMarshaler m_marshalerPtr;
	};
}