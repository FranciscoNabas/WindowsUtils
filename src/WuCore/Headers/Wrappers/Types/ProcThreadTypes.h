#pragma once
#pragma unmanaged

#include <Shlwapi.h>

#include "../../Support/Nt/NtUtilities.h"
#include "../../Engine/ProcessAndThread.h"
#include "../../Support/IO.h"
#include "../../Engine/Registry.h"

#pragma managed

/*
*	Important note about types:
*
*	PowerShell for some reason lists the properties in the reverse order than declared here.
*	An easy way to deal with it is to declare properties 'upside-down'.
*	This avoids us having to mess with 'format'/'types' files.
*
*	For classes with inheritance, it seems to list first the child properties, then
*	the parent ones.
*
*	Remember: what matters is what shows up to the user.
*/

namespace WindowsUtils
{
	using namespace System;

	public enum class ObjectHandleType
	{
		FileSystem = 0,
		Registry = 1
	};

	public ref class ObjectHandle
	{
	public:
		property ObjectHandleType Type { ObjectHandleType get() { return static_cast<ObjectHandleType>(m_wrapper->Type); } }
		property String^ InputObject { String^ get() { return gcnew String(m_wrapper->InputObject.Raw()); } }
		property String^ Name {
			String^ get()
			{
				if (!WWuString::IsNullOrEmpty(m_wrapper->Name))
					return gcnew String(m_wrapper->Name.Raw());

				return nullptr;
			}
		}
		property UInt32 ProcessId { UInt32 get() { return m_wrapper->ProcessId; } }
		property String^ Description
		{
			String^ get()
			{
				auto search = m_wrapper->VersionInfo.find(Core::VersionInfoProperty::FileDescription);
				if (search != m_wrapper->VersionInfo.end()) {
					if (WWuString::IsNullOrEmpty(search->second)) {
						if (!WWuString::IsNullOrEmpty(m_wrapper->Name)) {
							WWuString name = m_wrapper->Name;
							PathStripPath(name.Raw());

							return gcnew String(name.Raw());
						}
					}
					else
						return gcnew String(search->second.Raw());
				}
				else {
					if (!WWuString::IsNullOrEmpty(m_wrapper->Name)) {
						WWuString name = m_wrapper->Name;
						PathStripPath(name.Raw());

						return gcnew String(name.Raw());
					}
				}

				return nullptr;
			}
		}
		property String^ ProductName
		{
			String^ get()
			{
				auto search = m_wrapper->VersionInfo.find(Core::VersionInfoProperty::ProductName);
				if (search != m_wrapper->VersionInfo.end())
					if (!WWuString::IsNullOrEmpty(search->second))
						return gcnew String(search->second.Raw());

				return nullptr;
			}
		}
		property String^ FileVersion
		{
			String^ get()
			{
				auto search = m_wrapper->VersionInfo.find(Core::VersionInfoProperty::FileVersion);
				if (search != m_wrapper->VersionInfo.end())
					if (!WWuString::IsNullOrEmpty(search->second))
						return gcnew String(search->second.Raw());

				return nullptr;
			}
		}
		property String^ CompanyName
		{
			String^ get()
			{
				auto search = m_wrapper->VersionInfo.find(Core::VersionInfoProperty::CompanyName);
				if (search != m_wrapper->VersionInfo.end())
					if (!WWuString::IsNullOrEmpty(search->second))
						return gcnew String(search->second.Raw());

				return nullptr;
			}
		}
		property String^ ImagePath {
			String^ get()
			{
				if (!WWuString::IsNullOrEmpty(m_wrapper->ImagePath))
					return gcnew String(m_wrapper->ImagePath.Raw());

				return nullptr;
			}
		}

		ObjectHandle()
			: m_wrapper { new Core::WU_OBJECT_HANDLE }
		{ }

		ObjectHandle(const Core::WU_OBJECT_HANDLE& objectHandle)
			: m_wrapper { new Core::WU_OBJECT_HANDLE(objectHandle) }
		{ }

		~ObjectHandle() { delete m_wrapper; }

	protected:
		!ObjectHandle() { delete m_wrapper; }

	private:
		Core::PWU_OBJECT_HANDLE m_wrapper;
	};

	public ref class ObjectHandleInput
	{
	public:
		property String^ Path { String^ get() { return m_path; } }
		property ObjectHandleType Type { ObjectHandleType get() { return m_type; } }

		ObjectHandleInput(String^ path, ObjectHandleType type)
			: m_path(path), m_type(type)
		{ }

	private:
		String^ m_path;
		ObjectHandleType m_type;
	};

	public ref class ImageVersionInfo
	{
	public:
		property String^ FileDescription {
			String^ get() { return m_fileDescription; }
			void set(String^ value) { m_fileDescription = value; }
		}

		property String^ ProductName {
			String^ get() { return m_productName; }
			void set(String^ value) { m_productName = value; }
		}

		property String^ FileVersion {
			String^ get() { return m_fileVersion; }
			void set(String^ value) { m_fileVersion = value; }
		}

		property String^ CompanyName {
			String^ get() { return m_companyName; }
			void set(String^ value) { m_companyName = value; }
		}

		ImageVersionInfo()
			: m_fileDescription(nullptr), m_productName(nullptr), m_fileVersion(nullptr), m_companyName(nullptr)
		{ }

	private:
		String^ m_fileDescription;
		String^ m_productName;
		String^ m_fileVersion;
		String^ m_companyName;
	};

	public ref class ModuleInfo
	{
	public:
		property String^ ModuleName { String^ get() { return gcnew String(m_wrapper->ModuleName.Raw()); } }
		property String^ ModulePath { String^ get() { return gcnew String(m_wrapper->ModulePath.Raw()); } }
		property ImageVersionInfo^ VersionInfo { ImageVersionInfo^ get() { return m_versionInfo; } }

		ModuleInfo(const Core::WU_MODULE_INFO& info)
		{
			m_wrapper = new Core::WU_MODULE_INFO(info);

			m_versionInfo = gcnew ImageVersionInfo();
			m_versionInfo->FileDescription = gcnew String(info.VersionInfo.FileDescription.Raw());
			m_versionInfo->ProductName = gcnew String(info.VersionInfo.ProductName.Raw());
			m_versionInfo->FileVersion = gcnew String(info.VersionInfo.FileVersion.Raw());
			m_versionInfo->CompanyName = gcnew String(info.VersionInfo.CompanyName.Raw());
		}

		~ModuleInfo() { delete m_wrapper; }

	protected:
		!ModuleInfo() { delete m_wrapper; }

	private:
		Core::PWU_MODULE_INFO m_wrapper;
		ImageVersionInfo^ m_versionInfo;
	};

	public ref class ProcessModuleInfo
	{
	public:
		property UInt32 ProcessId { UInt32 get() { return m_wrapper->ProcessId; } }
		property String^ ImagePath { String^ get() { return gcnew String(m_wrapper->ImagePath.Raw()); } }
		property String^ ImageName { String^ get() { return gcnew String(m_wrapper->ImageFileName.Raw()); } }
		property String^ CommandLine { String^ get() { return gcnew String(m_wrapper->CommandLine.Raw()); } }
		property array<ModuleInfo^>^ Info { array<ModuleInfo^>^ get() { return m_moduleInfo; } }

		ProcessModuleInfo(const Core::PROCESS_MODULE_INFO& info)
		{
			m_wrapper = new Core::PROCESS_MODULE_INFO(info);

			m_moduleInfo = gcnew array<ModuleInfo^>(static_cast<int>(info.ModuleInfo.size()));
			int index = 0;
			for (const Core::WU_MODULE_INFO& modInfo : info.ModuleInfo) {
				m_moduleInfo[index] = gcnew ModuleInfo(modInfo);
				index++;
			}
		}

		~ProcessModuleInfo() { delete m_wrapper; }

	protected:
		!ProcessModuleInfo() { delete m_wrapper; }

	private:
		Core::PPROCESS_MODULE_INFO m_wrapper;
		array<ModuleInfo^>^ m_moduleInfo;
	};

	public ref class ObjectHandleInfo
	{
	public:
		property IntPtr HandleValue { IntPtr get() { return static_cast<IntPtr>(m_wrapper->HandleValue); } }
		property String^ Type {
			String^ get()
			{
				if (m_wrapper->Type == L"File")
					return "FileSystem";

				if (m_wrapper->Type == L"Key")
					return "Registry";

				return gcnew String(m_wrapper->Type.Raw());
			}
		}
		property String^ Name {
			String^ get()
			{
				if (!WWuString::IsNullOrEmpty(m_wrapper->Name)) {
					if (m_wrapper->Type == L"File") {
						Core::IO::GetFileDosPathFromDevicePath(m_wrapper->Name);
						return gcnew String(m_wrapper->Name.Raw());
					}
					else if (m_wrapper->Type == L"Key") {
						WWuString path;
						Core::Registry::GetRegistryPathFromNtPath(m_wrapper->Name, path);
						
						return gcnew String(path.Raw());
					}

					return (gcnew String(m_wrapper->Name.Raw()))->Trim();
				}

				return String::Empty;
			}
		}

		ObjectHandleInfo(const Core::WU_OBJECT_HANDLE_INFO& info)
			: m_wrapper(m_wrapper = new Core::WU_OBJECT_HANDLE_INFO(info)) { }

		~ObjectHandleInfo() { delete m_wrapper; }

	protected:
		!ObjectHandleInfo() { delete m_wrapper; }

	private:
		Core::PWU_OBJECT_HANDLE_INFO m_wrapper;
	};
}