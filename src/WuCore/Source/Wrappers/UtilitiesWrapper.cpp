#pragma unmanaged

#include "../../Headers/Support/WuStdException.h"

#pragma managed

#include <vcclr.h>

#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;
	using namespace System::Runtime::InteropServices;

	String^ UtilitiesWrapper::GetFormattedError(Int32 errorCode, ErrorType source)
	{
		Core::WuStdException ex(errorCode, __FILEW__, __LINE__, static_cast<Core::CoreErrorType>(source));

		return (gcnew String(ex.Message().GetBuffer()))->Trim();
	}
	
	// Get-LastWin32Error
	String^ UtilitiesWrapper::GetLastWin32Error()
	{
		WWuString errorMessage;
		try {
			m_utl->GetFormattedWin32Error(errorMessage);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		return (gcnew String(errorMessage.GetBuffer()))->Trim();
	}

	// Send-Click
	Void UtilitiesWrapper::SendClick()
	{
		try {
			m_utl->SendClick();
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	// Get-ResourceMessageTable
	array<ResourceMessageTable^>^ UtilitiesWrapper::GetResourceMessageTable(String^ libPath)
	{
		wuvector<Core::WU_RESOURCE_MESSAGE_TABLE> messageTable;

		WWuString wuLibPath = GetWideStringFromSystemString(libPath);

		try {
			m_utl->GetResourceMessageTable(messageTable, wuLibPath);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

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

	void UtilitiesWrapper::LogonAndImpersonateUser(String^ userName, SecureString^ password)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		LogonAndImpersonateUser(userName, wuPass);
	}

	// Logs on the given user and impersonates it.
	// You must call 'RevertToSelf()' to revert to the caller.
	void UtilitiesWrapper::LogonAndImpersonateUser(
		String^ userName,	 // The user name. If the user belongs to a domain, enter the down-level logon name: 'DOMAIN\UserName'.
		WWuString& password	 // The user's password, if any.
	)
	{
		if (!String::IsNullOrEmpty(userName)) {
			WWuString wuDomain;
			WWuString wuUserName;
			HANDLE hToken;

			if (userName->Contains(L"\\")) {
				wuDomain = GetWideStringFromSystemString(userName->Split('\\')[0]);
				wuUserName = GetWideStringFromSystemString(userName->Split('\\')[1]);
			}
			else
				wuUserName = GetWideStringFromSystemString(userName);

			if (!LogonUser(wuUserName.GetBuffer(), wuDomain.GetBuffer(), password.GetBuffer(), LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken)) {
				password.SecureErase();
				throw gcnew NativeException(GetLastError());
			}

			if (!ImpersonateLoggedOnUser(hToken)) {
				password.SecureErase();
				CloseHandle(hToken);

				throw gcnew NativeException(GetLastError());
			}

			// Zeroing the memory here instead of Marshal::ZeroFreeGlobalAllocUnicode
			// So the plain text stays less time in memory.
			password.SecureErase();

			CloseHandle(hToken);
		}
	}

	void UtilitiesWrapper::GetAptFromPath(String^ path, Core::AbstractPathTree* apt)
	{
		WWuString wrappedPath = GetWideStringFromSystemString(path);

		wuvector<Core::FS_INFO> fsInfo = Core::IO::EnumerateFileSystemInfo(wrappedPath);
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