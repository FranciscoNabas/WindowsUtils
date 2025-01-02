#pragma once
#pragma unmanaged

#include "../Support/IO.h"
#include "../Engine/Utilities.h"
#include "../Stubs/UtilitiesStub.h"

#pragma managed

#include "WrapperBase.h"
#include "NativeException.h"

namespace WindowsUtils
{
	public ref class ResourceMessageTable
	{
	public:
		property Int64 Id { Int64 get() { return m_wrapper->Id; } }
		property String^ Message { String^ get() { return (gcnew String(m_wrapper->Message.Raw()))->Trim(); } }

		ResourceMessageTable()
			: m_wrapper { new Core::WU_RESOURCE_MESSAGE_TABLE }
		{ }

		ResourceMessageTable(Core::WU_RESOURCE_MESSAGE_TABLE& messageTable)
			: m_wrapper { new Core::WU_RESOURCE_MESSAGE_TABLE(messageTable.Id, messageTable.Message.Raw()) }
		{ }

		~ResourceMessageTable() { delete m_wrapper; }

	protected:
		!ResourceMessageTable() { delete m_wrapper; }

	private:
		Core::PWU_RESOURCE_MESSAGE_TABLE m_wrapper;
	};
}

namespace WindowsUtils::Wrappers
{
	using namespace System::Security;

	public ref class UtilitiesWrapper : public WrapperBase
	{
	public:
		UtilitiesWrapper(Core::CmdletContextProxy^ context)
			: WrapperBase(context) { }

		// Get-FormattedError
		static String^ GetFormattedError(Int32 errorcode, ErrorType source);

		// Get-LastWin32Error
		static String^ GetLastWin32Error();

		// Send-Click
		Void SendClick();

		// Get-ResourceMessageTable
		array<ResourceMessageTable^>^ GetResourceMessageTable(String^ libpath);

		static array<String^>^ GetStringArrayFromDoubleNullTerminatedCStyleArray(const LPWSTR pvNativeArray, DWORD dwszBytes);
		static array<String^>^ GetStringArrayFromDoubleNullTerminatedCStyleArray(IntPtr nativeArray, DWORD dwszBytes);
		static void LogonAndImpersonateUser(String^ userName, SecureString^ password);
		static WWuString GetWideStringFromSystemString(String^ string);
		static void GetAptFromPath(String^ path, Core::AbstractPathTree* apt);

		static inline DateTime^ GetDateTimeFromFileTime(const ::FILETIME fileTime)
		{
			__int64 ftQuadPart = static_cast<__int64>(fileTime.dwHighDateTime) << 32 | fileTime.dwLowDateTime;
			return DateTime::FromFileTime(ftQuadPart);
		}
	};
}