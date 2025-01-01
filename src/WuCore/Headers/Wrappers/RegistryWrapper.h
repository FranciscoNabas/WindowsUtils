#pragma once
#pragma unmanaged

#include "Marshalers.h"
#include "NativeException.h"
#include "../Engine/Registry.h"
#include "../Engine/AccessControl.h"
#include "../Support/ScopedBuffer.h"
#include "../Stubs/RegistryStub.h"

#pragma managed

#include "CmdletContextProxy.h"

namespace WindowsUtils::Wrappers
{
	using namespace System;
	using namespace System::Security;
	using namespace Microsoft::Win32;

	public ref class RegistryWrapper
	{
	public:
		static Object^ GetRegistryValue(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName);
		static Object^ GetRegistryValue(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName);
		static Object^ GetRegistryValue(String^ computerName, RegistryHive hive, String^ subKey, String^ valueName);
		static Object^ GetRegistryValue(RegistryHive hive, String^ subKey, String^ valueName);
		static Object^ GetRegistryValue(IntPtr hRegistry, String^ subKey, String^ valueName);

		static array<String^>^ GetRegistrySubKeyNames(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey);
		static array<String^>^ GetRegistrySubKeyNames(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey);
		static array<String^>^ GetRegistrySubKeyNames(String^ computerName, RegistryHive hive, String^ subKey);
		static array<String^>^ GetRegistrySubKeyNames(RegistryHive hive, String^ subKey);
		static array<String^>^ GetRegistrySubKeyNames(IntPtr hRegistry, String^ subKey);

		static array<Object^>^ GetRegistryValueList(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		static array<Object^>^ GetRegistryValueList(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		static array<Object^>^ GetRegistryValueList(String^ computerName, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		static array<Object^>^ GetRegistryValueList(RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		static array<Object^>^ GetRegistryValueList(IntPtr hRegistry, String^ subKey, array<String^>^ valueNameList);

		static String^ GetRegistryNtPath(String^ keyPath);
		static String^ GetRegistryPathFromNtPath(String^ ntKeyPath);
	};
}