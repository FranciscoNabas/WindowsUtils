#pragma once
#pragma unmanaged

#include "../Engine/Registry.h"
#include "../Engine/AccessControl.h"

#pragma managed

namespace WindowsUtils::Wrappers
{
	using namespace System;
	using namespace System::Security;
	using namespace Microsoft::Win32;

	public ref class RegistryWrapper
	{
	public:
		Object^ GetRegistryValue(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName);
		Object^ GetRegistryValue(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName);
		Object^ GetRegistryValue(String^ computerName, RegistryHive hive, String^ subKey, String^ valueName);
		Object^ GetRegistryValue(RegistryHive hive, String^ subKey, String^ valueName);
		Object^ GetRegistryValue(IntPtr hRegistry, String^ subKey, String^ valueName);

		array<String^>^ GetRegistrySubKeyNames(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey);
		array<String^>^ GetRegistrySubKeyNames(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey);
		array<String^>^ GetRegistrySubKeyNames(String^ computerName, RegistryHive hive, String^ subKey);
		array<String^>^ GetRegistrySubKeyNames(RegistryHive hive, String^ subKey);
		array<String^>^ GetRegistrySubKeyNames(IntPtr hRegistry, String^ subKey);

		array<Object^>^ GetRegistryValueList(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<Object^>^ GetRegistryValueList(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<Object^>^ GetRegistryValueList(String^ computerName, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<Object^>^ GetRegistryValueList(RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<Object^>^ GetRegistryValueList(IntPtr hRegistry, String^ subKey, array<String^>^ valueNameList);

		static String^ GetRegistryNtPath(String^ keyPath);
		static String^ GetRegistryPathFromNtPath(String^ ntKeyPath);

	private:
		Core::Registry* m_reg;

		Object^ GetRegistryValue(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey, String^ valueName);
		array<Object^>^ GetRegistryValueList(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<String^>^ GetRegistrySubKeyNames(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey);
	};
}