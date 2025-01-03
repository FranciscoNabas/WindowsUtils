#pragma unmanaged

#include "../../Headers/Support/WuException.h"

#pragma managed

#include <vcclr.h>

#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;
	using namespace System::Runtime::InteropServices;

	String^ UtilitiesWrapper::GetFormattedError(Int32 errorCode, ErrorType source)
	{
		// Doesn't throw.
		return (gcnew String(Core::WuNativeException::GetErrorMessage(errorCode, static_cast<Core::CoreErrorType>(source)).Raw()))->Trim();
	}
	
	// Get-LastWin32Error
	String^ UtilitiesWrapper::GetLastWin32Error()
	{
		// Doesn't throw.
		return (gcnew String(Core::WuNativeException::GetErrorMessage(GetLastError(), Core::CoreErrorType::SystemError).Raw()))->Trim();
	}

	// Send-Click
	Void UtilitiesWrapper::SendClick()
	{
		_WU_START_TRY
			Stubs::Utilities::Dispatch<UtilitiesOperation::SendClick>(Context->GetUnderlyingContext());
		_WU_MANAGED_CATCH
	}

	// Get-ResourceMessageTable
	array<ResourceMessageTable^>^ UtilitiesWrapper::GetResourceMessageTable(String^ libPath)
	{
		std::vector<Core::WU_RESOURCE_MESSAGE_TABLE> messageTable;

		WWuString wuLibPath = GetWideStringFromSystemString(libPath);

		_WU_START_TRY
			Stubs::Utilities::Dispatch<UtilitiesOperation::GetResMesTable>(Context->GetUnderlyingContext(), messageTable, wuLibPath);
		_WU_MANAGED_CATCH

		List<ResourceMessageTable^>^ output = gcnew List<ResourceMessageTable^>(0);
		for (auto& tableEntry : messageTable)
			output->Add(gcnew ResourceMessageTable(tableEntry));

		return output->ToArray();
	}

	// Utilities
	array<String^>^ UtilitiesWrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray(const LPWSTR pvNativeArray, DWORD dwszBytes)
	{
		List<String^>^ stringList = gcnew List<String^>(0);
		LPWSTR nativeArray = pvNativeArray;
		DWORD offset = 0;
		while (true) {
			String^ current = gcnew String(nativeArray);
			stringList->Add(current);

			offset += current->Length + 1;
			DWORD remaining = dwszBytes - (offset * 2);
			if (remaining <= 4)
				break;

			nativeArray += current->Length + 1;
		}

		return stringList->ToArray();
	}

	array<String^>^ UtilitiesWrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray(IntPtr nativeArray, DWORD dwszBytes)
	{
		List<String^>^ stringList = gcnew List<String^>(0);
		LPWSTR wNativeArray = (LPWSTR)nativeArray.ToPointer();
		DWORD offset = 0;
		while (true) {
			String^ current = gcnew String(wNativeArray);
			stringList->Add(current);

			offset += current->Length + 1;
			DWORD remaining = dwszBytes - (offset * 2);
			if (remaining <= 4)
				break;

			wNativeArray += current->Length + 1;
		}

		return stringList->ToArray();
	}

	// Logs on the given user and impersonates it.
	// You must call 'RevertToSelf()' to revert to the caller.
	void UtilitiesWrapper::LogonAndImpersonateUser(
		String^ userName,			 // The user name. If the user belongs to a domain, enter the down-level logon name: 'DOMAIN\UserName'.
		SecureString^ password		 // The user's password, if any.
	)
	{
		if (!String::IsNullOrEmpty(userName) && password) {
			WWuString wuDomain;
			WWuString wuUserName;
			IntPtr coTaskPass = Marshal::SecureStringToCoTaskMemUnicode(password);
			try {
				WWuString wuPass{ (LPWSTR)coTaskPass.ToPointer() };
				try {
					if (userName->Contains(L"\\")) {
						wuDomain = GetWideStringFromSystemString(userName->Split('\\')[0]);
						wuUserName = GetWideStringFromSystemString(userName->Split('\\')[1]);
					}
					else
						wuUserName = GetWideStringFromSystemString(userName);

					Core::SafeObjectHandle tokenHandle;
					if (!LogonUser(wuUserName.Raw(), wuDomain.Raw(), wuPass.Raw(), LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &tokenHandle))
						throw gcnew NativeException(GetLastError());

					if (!ImpersonateLoggedOnUser(tokenHandle.Get()))
						throw gcnew NativeException(GetLastError());
				}
				finally {
					wuPass.SecureErase();
				}
			}
			finally {
				Marshal::ZeroFreeCoTaskMemUnicode(coTaskPass);
			}
		}
	}

	void UtilitiesWrapper::GetAptFromPath(String^ path, Core::AbstractPathTree* apt)
	{
		WWuString wrappedPath = GetWideStringFromSystemString(path);

		WuList<Core::FS_INFO> fsInfo = Core::IO::EnumerateFileSystemInfo(wrappedPath);
		for (Core::FS_INFO& info : fsInfo) {
			if (info.Length > 0x7FFFFFFF)
				throw gcnew ArgumentException("Cabinet does not support files bigger than 2Gb.");

			apt->PushEntry(Core::AbstractPathTree::AptEntry(info.Name, info.FullName, wrappedPath, info.Length, info.Type));
		}

	}

	WWuString UtilitiesWrapper::GetWideStringFromSystemString(String^ string)
	{
		pin_ptr<const wchar_t> pinnedString = PtrToStringChars(string);
		return WWuString((LPWSTR)pinnedString);
	}
}