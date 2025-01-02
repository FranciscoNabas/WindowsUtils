#pragma unmanaged

#include "../../Headers/Support/WuException.h"
#include "../../Headers/Support/Nt/NtUtilities.h"

#pragma managed

#include <vcclr.h>

#include "../../Headers/Wrappers/RegistryWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;
	using namespace System::Runtime::InteropServices;

	// Registry operations
	Object^ RegistryWrapper::GetRegistryValue(RegistryHive hive, String^ subKey, String^ valueName)
	{
		return GetRegistryValue(nullptr, nullptr, nullptr, hive, subKey, valueName);
	}

	Object^ RegistryWrapper::GetRegistryValue(String^ computerName, RegistryHive hive, String^ subKey, String^ valueName)
	{
		return GetRegistryValue(computerName, nullptr, nullptr, hive, subKey, valueName);
	}

	Object^ RegistryWrapper::GetRegistryValue(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		return GetRegistryValue(nullptr, userName, password, hive, subKey, valueName);
	}

	Object^ RegistryWrapper::GetRegistryValue(IntPtr hRegistry, String^ subKey, String^ valueName)
	{
		return GetRegistryValue(nullptr, nullptr, nullptr, static_cast<RegistryHive>(static_cast<Int64>(hRegistry)), subKey, valueName);
	}

	Object^ RegistryWrapper::GetRegistryValue(
		String^ computerName,		// The computer name. If the computer is remote, it needs Remote Registry enabled.
		String^ userName,			// User name to impersonate before connecting to the registry.
		SecureString^ password,     // User password.
		RegistryHive hive,			// The root hive.
		String^ subKey,				// The subkey path.
		String^ valueName			// The value property name.
	)
	{
		DWORD valueType{ };
		WWuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);
		WWuString wuValueName = UtilitiesWrapper::GetWideStringFromSystemString(valueName);

		Core::ScopedBuffer data;
		try {
			// Managing logon.
			if (!String::IsNullOrEmpty(userName))
				UtilitiesWrapper::LogonAndImpersonateUser(userName, password);

			Core::RegistryHandle regHandle{ (HKEY)hive, false };
			data = Stubs::Registry::Dispatch<RegistryOperation::GetValue>(Core::ExceptionMarshaler::NativePtr,
				&wuComputerName, regHandle, wuSubKey, wuValueName, &valueType);
		}
		catch (...) {
			RevertToSelf();
			throw;
		}

		Object^ output;
		switch (valueType) {
		case REG_BINARY:
		{
			output = gcnew array<Byte>(static_cast<int>(data.Size()));
			pin_ptr<Byte> outputPtr = &((array<Byte>^)output)[0];
			RtlCopyMemory(outputPtr, data.Get(), data.Size());
		} break;

		case REG_DWORD:
			output = gcnew Int32(*reinterpret_cast<PDWORD>(data.Get()));
			break;

		case REG_EXPAND_SZ:
		{
			// Getting the necessary buffer.
			LPCWSTR strData = reinterpret_cast<LPCWSTR>(data.Get());
			DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
			if (charCount == 0)
				throw gcnew NativeException(GetLastError());

			charCount++;
			std::unique_ptr<WCHAR[]> buffer = std::make_unique<WCHAR[]>(charCount);
			charCount = ExpandEnvironmentStrings(strData, buffer.get(), charCount);
			if (charCount == 0)
				throw gcnew NativeException(GetLastError());

			output = gcnew String(buffer.get());
		}
		break;

		case REG_LINK:
			output = gcnew String(reinterpret_cast<LPWSTR>(data.Get()));
			break;

		case REG_MULTI_SZ:
			output = UtilitiesWrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray(reinterpret_cast<LPWSTR>(data.Get()), static_cast<DWORD>(data.Size()));
			break;

		case REG_QWORD:
			output = gcnew Int64(*reinterpret_cast<__int64*>(data.Get()));
			break;

		case REG_SZ:
			output = gcnew String(reinterpret_cast<LPWSTR>(data.Get()));
			break;

		default:
			throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", valueType));
			break;
		}

		RevertToSelf();

		return output;
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(RegistryHive hive, String^ subKey)
	{
		return GetRegistrySubKeyNames(nullptr, nullptr, nullptr, hive, subKey);
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(String^ computerName, RegistryHive hive, String^ subKey)
	{
		return GetRegistrySubKeyNames(computerName, nullptr, nullptr, hive, subKey);
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		return GetRegistrySubKeyNames(nullptr, userName, password, hive, subKey);
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(IntPtr hRegistry, String^ subKey)
	{
		return GetRegistrySubKeyNames(nullptr, nullptr, nullptr, static_cast<RegistryHive>(static_cast<Int32>(hRegistry)), subKey);
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		WWuString wuComputerName;
		WuList<WWuString> subkeyNameVec;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);

		try {
			// Managing logon.
			if (!String::IsNullOrEmpty(userName))
				UtilitiesWrapper::LogonAndImpersonateUser(userName, password);

			Core::RegistryHandle regHandle{ (HKEY)hive, false };
			Stubs::Registry::Dispatch<RegistryOperation::GetSubkeys>(Core::ExceptionMarshaler::NativePtr,
				&wuComputerName, regHandle, wuSubKey, 0, subkeyNameVec);
		}
		catch (...) {
			RevertToSelf();
			throw;
		}

		List<String^>^ output = gcnew List<String^>(0);
		for (const WWuString& singleName : subkeyNameVec)
			output->Add(gcnew String(singleName.Raw()));

		RevertToSelf();

		return output->ToArray();
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		return GetRegistryValueList(nullptr, userName, password, hive, subKey, valueNameList);
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(String^ computerName, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		return GetRegistryValueList(computerName, nullptr, nullptr, hive, subKey, valueNameList);
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		return GetRegistryValueList(nullptr, nullptr, nullptr, hive, subKey, valueNameList);
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(IntPtr hRegistry, String^ subKey, array<String^>^ valueNameList)
	{
		return GetRegistryValueList(nullptr, nullptr, nullptr, static_cast<RegistryHive>(static_cast<Int32>(hRegistry)), subKey, valueNameList);
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);

		DWORD valueCount = valueNameList->Length;
		std::unique_ptr<VALENT[]> valList = std::make_unique<VALENT[]>(valueCount);
		std::unique_ptr<WWuString[]> valueNames = std::make_unique<WWuString[]>(valueCount);
		for (DWORD i = 0; i < valueCount; i++) {
			valueNames[i] = UtilitiesWrapper::GetWideStringFromSystemString(valueNameList[i]);
			valList[i].ve_valuename = valueNames[i].Raw();
		}

		Core::ScopedBuffer buffer;
		try {
			// Managing logon.
			if (!String::IsNullOrEmpty(userName))
				UtilitiesWrapper::LogonAndImpersonateUser(userName, password);

			Core::RegistryHandle regHandle{ (HKEY)hive, false };
			buffer = Stubs::Registry::Dispatch<RegistryOperation::GetValueList>(Core::ExceptionMarshaler::NativePtr,
				&wuComputerName, regHandle, wuSubKey, valList.get(), valueCount);
		}
		catch (...) {
			RevertToSelf();
			throw;
		}

		array<Object^>^ output = gcnew array<Object^>(valueCount);
		for (DWORD i = 0; i < valueCount; i++) {
			PVOID data = (PVOID)valList[i].ve_valueptr;
			switch (valList[i].ve_type) {
				case REG_BINARY:
				{
					output[i] = gcnew array<Byte>(valList[i].ve_valuelen);
					pin_ptr<Byte> dataPtr = &((array<Byte>^)output[i])[0];
					RtlCopyMemory(dataPtr, data, valList[i].ve_valuelen);
				} break;

				case REG_DWORD:
					output[i] = gcnew Int32(*reinterpret_cast<PDWORD>(data));
					break;

				case REG_EXPAND_SZ:
				{
					LPCWSTR strData = reinterpret_cast<LPCWSTR>(data);
					DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
					if (charCount == 0)
						throw gcnew NativeException(GetLastError());

					charCount++;
					std::unique_ptr<WCHAR[]> buffer = std::make_unique<WCHAR[]>(charCount);
					charCount = ExpandEnvironmentStrings(strData, buffer.get(), charCount);
					if (charCount == 0)
						throw gcnew NativeException(GetLastError());

					output[i] = gcnew String(buffer.get());
				} break;

				case REG_LINK:
					output[i] = gcnew String(reinterpret_cast<LPWSTR>(data));
					break;

				case REG_MULTI_SZ:
					output[i] = UtilitiesWrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray(reinterpret_cast<LPWSTR>(data), valList[i].ve_valuelen);
					break;

				case REG_QWORD:
					output[i] = gcnew Int64(*reinterpret_cast<__int64*>(data));
					break;

				case REG_SZ:
					output[i] = gcnew String(reinterpret_cast<LPWSTR>(data));
					break;

				default:
				{
					RevertToSelf();
					throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", valList[i].ve_type));
				} break;
			}
		}

		RevertToSelf();

		return output;
	}

	// Utilities
	String^ RegistryWrapper::GetRegistryNtPath(String^ keyPath)
	{
		array<String^>^ splitPath = keyPath->Split('\\');
		String^ subkeyPath = String::Join("\\", gcnew ArraySegment<String^>(splitPath, 1, splitPath->Length - 1));

		HKEY hiveKey;
		if (splitPath[0] == "HKEY_CLASSES_ROOT") { hiveKey = HKEY_CLASSES_ROOT; }
		else if (splitPath[0] == "HKEY_CURRENT_USER") { hiveKey = HKEY_CURRENT_USER; }
		else if (splitPath[0] == "HKEY_LOCAL_MACHINE") { hiveKey = HKEY_LOCAL_MACHINE; }
		else if (splitPath[0] == "HKEY_USERS") { hiveKey = HKEY_USERS; }
		else if (splitPath[0] == "HKEY_CURRENT_CONFIG") { hiveKey = HKEY_CURRENT_CONFIG; }
		else { throw gcnew ArgumentException("Invalid hive '" + splitPath[0] + "'."); }

		Core::RegistryHandle subKey;
		pin_ptr<const wchar_t> wrappedKey = PtrToStringChars(subkeyPath);
		LSTATUS regStatus = RegOpenKey(hiveKey, wrappedKey, &subKey);
		if (regStatus != ERROR_SUCCESS)
			throw gcnew NativeException(regStatus);

		ULONG bufferSize = 1 << 10;
		Core::ScopedBuffer buffer{ bufferSize };
		NTSTATUS result = NtQueryKey(subKey.Get(), KEY_INFORMATION_CLASS::KeyNameInformation, buffer.Get(), bufferSize, &bufferSize);
		if (result != STATUS_SUCCESS)
			throw gcnew NativeException(Core::WuNativeException(result, L"QueryKey", Core::WriteErrorCategory::InvalidResult, __FILEW__, __LINE__, Core::CoreErrorType::NtError));

		auto keyNameInfo = reinterpret_cast<PKEY_NAME_INFORMATION>(buffer.Get());

		if (keyNameInfo->NameLength > 0)
			return gcnew String(keyNameInfo->Name);

		return nullptr;
	}

	// Converts a NT registry path to user-mode registry path.
	// TODO: Deal with non-kernel 'hives' like 'HKEY_CURRENT_CONFIG'.
	String^ RegistryWrapper::GetRegistryPathFromNtPath(String^ ntKeyPath)
	{
		String^ root = ntKeyPath->Split('\\')[2];
		if (root == "MACHINE") {
			String^ path = ntKeyPath->Replace("\\REGISTRY\\MACHINE", "HKLM:");
			if (path == "HKLM:")
				return "HKLM:\\";

			return path;
		}
		else if (root == "USER") {
			String^ currentUserRoot = "\\REGISTRY\\USER\\" + gcnew String(Core::AccessControl::GetCurrentTokenUserSid().Raw());
			String^ path;
			if (ntKeyPath->StartsWith(currentUserRoot))
				path = ntKeyPath->Replace(currentUserRoot, "HKCU:");
			else
				path = ntKeyPath->Replace("\\REGISTRY\\USER", "HKU:");
			
			if (path == "HKCU:" || path == "HKU:")
				return path + "\\";

			return path;
		}

		return ntKeyPath;
	}
}