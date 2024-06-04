#pragma unmanaged

#include "../../Headers/Support/WuStdException.h"
#include "../../Headers/Support/NtUtilities.h"

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
		WWuString wuPass = L"";
		return GetRegistryValue(L"", L"", wuPass, hive, subKey, valueName);
	}

	Object^ RegistryWrapper::GetRegistryValue(String^ computerName, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WWuString wuPass = L"";
		return GetRegistryValue(computerName, L"", wuPass, hive, subKey, valueName);
	}

	Object^ RegistryWrapper::GetRegistryValue(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValue(L"", userName, wuPass, hive, subKey, valueName);
	}

	Object^ RegistryWrapper::GetRegistryValue(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValue(computerName, userName, wuPass, hive, subKey, valueName);
	}

	Object^ RegistryWrapper::GetRegistryValue(IntPtr hRegistry, String^ subKey, String^ valueName)
	{
		DWORD dwValueType = 0;
		DWORD dwBytesReturned = 0;
		wuunique_ha_ptr<void> data;

		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);
		WWuString wuValueName = UtilitiesWrapper::GetWideStringFromSystemString(valueName);

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		try {
			m_reg->GetRegistryKeyValue(whReg, wuSubKey, wuValueName, dwValueType, data, dwBytesReturned);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		Object^ output;
		switch (dwValueType) {
			case REG_BINARY:
			{
				output = gcnew array<Byte>(dwBytesReturned);
				Marshal::Copy((IntPtr)data.get(), (array<Byte>^)output, 0, dwBytesReturned);
			}
			break;

			case REG_DWORD:
			{
				DWORD* dwData = reinterpret_cast<DWORD*>(data.get());
				output = gcnew Int32(*dwData);
			}
			break;

			case REG_EXPAND_SZ:
			{
				// Getting the necessary buffer.
				LPCWSTR strData = reinterpret_cast<LPCWSTR>(data.get());
				DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
				if (charCount == 0)
					throw gcnew NativeException(GetLastError());


				DWORD bytesNeeded = charCount * 2;
				wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
				charCount = ExpandEnvironmentStrings(strData, buffer.get(), bytesNeeded);
				if (charCount == 0)
					throw gcnew NativeException(GetLastError());

				output = gcnew String(buffer.get());
			}
			break;

			case REG_LINK:
				output = gcnew String(reinterpret_cast<LPWSTR>(data.get()));
				break;

			case REG_MULTI_SZ:
				output = UtilitiesWrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray(reinterpret_cast<LPWSTR>(data.get()), dwBytesReturned);
				break;

			case REG_QWORD:
			{
				long long* qwData = reinterpret_cast<long long*>(data.get());
				output = gcnew Int64(*qwData);
			}
			break;

			case REG_SZ:
				output = gcnew String(reinterpret_cast<LPWSTR>(data.get()));
				break;

			default:
				throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", dwValueType));
				break;
		}

		return output;
	}

	Object^ RegistryWrapper::GetRegistryValue(
		String^ computerName,	  // The computer name. If the computer is remote, it needs Remote Registry enabled.
		String^ userName,		  // User name to impersonate before connecting to the registry.
		WWuString& password,		  // User password.
		RegistryHive hive,		  // The root hive.
		String^ subKey,			  // The subkey path.
		String^ valueName		  // The value property name.
	)
	{
		DWORD dwValueType = 0;
		DWORD dwBytesReturned = 0;
		wuunique_ha_ptr<void> data;

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			UtilitiesWrapper::LogonAndImpersonateUser(userName, password);

		WWuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);
		WWuString wuValueName = UtilitiesWrapper::GetWideStringFromSystemString(valueName);


		try {
			m_reg->GetRegistryKeyValue(wuComputerName, (HKEY)hive, wuSubKey, wuValueName, dwValueType, data, dwBytesReturned);
		}
		catch (const Core::WuStdException& ex) {
			RevertToSelf();
			throw gcnew NativeException(ex);
		}

		Object^ output;
		switch (dwValueType) {
			case REG_BINARY:
			{
				output = gcnew array<Byte>(dwBytesReturned);
				Marshal::Copy((IntPtr)data.get(), (array<Byte>^)output, 0, dwBytesReturned);
			}
			break;

			case REG_DWORD:
			{
				DWORD* dwData = reinterpret_cast<DWORD*>(data.get());
				output = gcnew Int32(*dwData);
			}
			break;

			case REG_EXPAND_SZ:
			{
				// Getting the necessary buffer.
				LPCWSTR strData = reinterpret_cast<LPCWSTR>(data.get());
				DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
				if (charCount == 0)
					throw gcnew NativeException(GetLastError());


				DWORD bytesNeeded = charCount * 2;
				wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
				charCount = ExpandEnvironmentStrings(strData, buffer.get(), bytesNeeded);
				if (charCount == 0)
					throw gcnew NativeException(GetLastError());

				output = gcnew String(buffer.get());
			}
			break;

			case REG_LINK:
				output = gcnew String(reinterpret_cast<LPWSTR>(data.get()));
				break;

			case REG_MULTI_SZ:
				output = UtilitiesWrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray(reinterpret_cast<LPWSTR>(data.get()), dwBytesReturned);
				break;

			case REG_QWORD:
			{
				long long* qwData = reinterpret_cast<long long*>(data.get());
				output = gcnew Int64(*qwData);
			}
			break;

			case REG_SZ:
				output = gcnew String(reinterpret_cast<LPWSTR>(data.get()));
				break;

			default:
				throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", dwValueType));
				break;
		}

		RevertToSelf();

		return output;
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(RegistryHive hive, String^ subKey)
	{
		WWuString wuPass = L"";
		return GetRegistrySubKeyNames(L"", L"", wuPass, hive, subKey);
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(String^ computerName, RegistryHive hive, String^ subKey)
	{
		WWuString wuPass = L"";
		return GetRegistrySubKeyNames(computerName, L"", wuPass, hive, subKey);
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistrySubKeyNames(L"", userName, wuPass, hive, subKey);
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistrySubKeyNames(computerName, userName, wuPass, hive, subKey);
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(IntPtr hRegistry, String^ subKey)
	{
		wusunique_vector<WWuString> subkeyNameVec = make_wusunique_vector<WWuString>();

		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		try {
			m_reg->GetRegistrySubkeyNames(whReg, wuSubKey, 0, subkeyNameVec.get());
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		List<String^>^ output = gcnew List<String^>(0);
		for (WWuString singleName : *subkeyNameVec)
			output->Add(gcnew String(singleName.GetBuffer()));

		RevertToSelf();

		return output->ToArray();
	}

	array<String^>^ RegistryWrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey)
	{
		wusunique_vector<WWuString> subkeyNameVec = make_wusunique_vector<WWuString>();

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			UtilitiesWrapper::LogonAndImpersonateUser(userName, password);

		WWuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);

		try {
			m_reg->GetRegistrySubkeyNames(wuComputerName, (HKEY)hive, wuSubKey, 0, subkeyNameVec.get());
		}
		catch (const Core::WuStdException& ex) {
			RevertToSelf();
			throw gcnew NativeException(ex);
		}

		List<String^>^ output = gcnew List<String^>(0);
		for (WWuString singleName : *subkeyNameVec)
			output->Add(gcnew String(singleName.GetBuffer()));

		RevertToSelf();

		return output->ToArray();
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValueList(L"", userName, wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValueList(computerName, userName, wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(String^ computerName, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuPass = L"";
		return GetRegistryValueList(computerName, L"", wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuPass = L"";
		return GetRegistryValueList(L"", L"", wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(IntPtr hRegistry, String^ subKey, array<String^>^ valueNameList)
	{
		DWORD dwValCount = valueNameList->Length;
		PVALENT pValList = new VALENT[valueNameList->Length];
		WWuString* wuValueNames = new WWuString[valueNameList->Length];

		wuunique_ha_ptr<void> buffer;
		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);

		for (DWORD i = 0; i < dwValCount; i++) {
			wuValueNames[i] = UtilitiesWrapper::GetWideStringFromSystemString(valueNameList[i]);
			pValList[i].ve_valuename = wuValueNames[i].GetBuffer();
		}

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		try {
			m_reg->GetRegistryKeyValueList(whReg, wuSubKey, pValList, dwValCount, buffer);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		array<Object^>^ output = gcnew array<Object^>(dwValCount);
		for (DWORD i = 0; i < dwValCount; i++) {
			PVOID pvData = (PVOID)pValList[i].ve_valueptr;
			switch (pValList[i].ve_type) {
				case REG_BINARY:
				{
					output[i] = gcnew array<Byte>(pValList[i].ve_valuelen);
					Marshal::Copy((IntPtr)pvData, (array<Byte>^)output[i], 0, pValList[i].ve_valuelen);
				}
				break;

				case REG_DWORD:
				{
					DWORD* dwData = static_cast<DWORD*>(pvData);
					output[i] = gcnew Int32(*dwData);
				}
				break;

				case REG_EXPAND_SZ:
				{
					LPCWSTR strData = reinterpret_cast<LPCWSTR>((LPCWSTR)pvData);
					DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
					if (charCount == 0)
						throw gcnew NativeException(GetLastError());


					DWORD bytesNeeded = charCount * 2;
					wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
					charCount = ExpandEnvironmentStrings(strData, buffer.get(), bytesNeeded);
					if (charCount == 0)
						throw gcnew NativeException(GetLastError());

					output[i] = gcnew String(buffer.get());
				}
				break;

				case REG_LINK:
					output[i] = gcnew String((LPWSTR)pvData);
					break;

				case REG_MULTI_SZ:
					output[i] = UtilitiesWrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray((LPWSTR)pvData, pValList[i].ve_valuelen);
					break;

				case REG_QWORD:
				{
					long long* qwData = static_cast<long long*>(pvData);
					output[i] = gcnew Int64(*qwData);
				}
				break;

				case REG_SZ:
					output[i] = gcnew String((LPWSTR)pvData);
					break;

				default:
					throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", pValList[i].ve_type));
			}
		}

		return output;
	}

	array<Object^>^ RegistryWrapper::GetRegistryValueList(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		DWORD dwValCount = valueNameList->Length;
		PVALENT pValList = new VALENT[valueNameList->Length];
		WWuString* wuValueNames = new WWuString[valueNameList->Length];
		wuunique_ha_ptr<void> buffer;

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			UtilitiesWrapper::LogonAndImpersonateUser(userName, password);

		WWuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = UtilitiesWrapper::GetWideStringFromSystemString(subKey);

		for (DWORD i = 0; i < dwValCount; i++) {
			wuValueNames[i] = UtilitiesWrapper::GetWideStringFromSystemString(valueNameList[i]);
			pValList[i].ve_valuename = wuValueNames[i].GetBuffer();
		}

		try {
			m_reg->GetRegistryKeyValueList(wuComputerName, (HKEY)hive, wuSubKey, pValList, dwValCount, buffer);
		}
		catch (const Core::WuStdException& ex) {
			RevertToSelf();
			throw gcnew NativeException(ex);
		}

		array<Object^>^ output = gcnew array<Object^>(dwValCount);
		for (DWORD i = 0; i < dwValCount; i++) {
			PVOID pvData = (PVOID)pValList[i].ve_valueptr;
			switch (pValList[i].ve_type) {
				case REG_BINARY:
				{
					output[i] = gcnew array<Byte>(pValList[i].ve_valuelen);
					Marshal::Copy((IntPtr)pvData, (array<Byte>^)output[i], 0, pValList[i].ve_valuelen);
				}
				break;

				case REG_DWORD:
				{
					DWORD* dwData = static_cast<DWORD*>(pvData);
					output[i] = gcnew Int32(*dwData);
				}
				break;

				case REG_EXPAND_SZ:
				{
					LPCWSTR strData = reinterpret_cast<LPCWSTR>((LPCWSTR)pvData);
					DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
					if (charCount == 0)
						throw gcnew NativeException(GetLastError());


					DWORD bytesNeeded = charCount * 2;
					wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
					charCount = ExpandEnvironmentStrings(strData, buffer.get(), bytesNeeded);
					if (charCount == 0)
						throw gcnew NativeException(GetLastError());

					output[i] = gcnew String(buffer.get());
				}
				break;

				case REG_LINK:
					output[i] = gcnew String((LPWSTR)pvData);
					break;

				case REG_MULTI_SZ:
					output[i] = UtilitiesWrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray((LPWSTR)pvData, pValList[i].ve_valuelen);
					break;

				case REG_QWORD:
				{
					long long* qwData = static_cast<long long*>(pvData);
					output[i] = gcnew Int64(*qwData);
				}
				break;

				case REG_SZ:
					output[i] = gcnew String((LPWSTR)pvData);
					break;

				default:
				{
					RevertToSelf();
					throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", pValList[i].ve_type));
				}
				break;
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

		HKEY subKey;
		pin_ptr<const wchar_t> wrappedKey = PtrToStringChars(subkeyPath);
		LSTATUS regStatus = RegOpenKey(hiveKey, wrappedKey, &subKey);
		if (regStatus != ERROR_SUCCESS)
			throw gcnew NativeException(regStatus);

		ULONG bufferSize = 1 << 10;
		wuunique_ptr<BYTE[]> buffer = make_wuunique<BYTE[]>(bufferSize);
		NTSTATUS result = Core::NtFunctions::GetInstance()->NtQueryKey(subKey, KEY_INFORMATION_CLASS::KeyNameInformation, buffer.get(), bufferSize, &bufferSize);
		if (result != STATUS_SUCCESS) {
			RegCloseKey(subKey);
			throw gcnew NativeException(Core::WuStdException(result, __FILEW__, __LINE__, Core::CoreErrorType::NtError));
		}

		auto keyNameInfo = reinterpret_cast<PKEY_NAME_INFORMATION>(buffer.get());

		if (keyNameInfo->NameLength > 0) {
			RegCloseKey(subKey);
			return gcnew String(keyNameInfo->Name);
		}

		RegCloseKey(subKey);

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
			String^ currentUserRoot = "\\REGISTRY\\USER\\" + gcnew String(Core::AccessControl::GetCurrentTokenUserSid().GetBuffer());
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