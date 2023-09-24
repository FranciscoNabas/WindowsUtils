#pragma once
#pragma unmanaged

#include "../../Engine/ProcessAndThread.h"

#pragma managed

namespace WindowsUtils
{
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
		property String^ ModuleName { String^ get() { return gcnew String(m_wrapper->ModuleName.GetBuffer()); } }
		property String^ ModulePath { String^ get() { return gcnew String(m_wrapper->ModulePath.GetBuffer()); } }
		property ImageVersionInfo^ VersionInfo { ImageVersionInfo^ get() { return m_versionInfo; } }

		ModuleInfo(const Core::WU_MODULE_INFO& info)
		{
			m_wrapper = new Core::WU_MODULE_INFO(info);

			m_versionInfo = gcnew ImageVersionInfo();
			m_versionInfo->FileDescription = gcnew String(info.VersionInfo.FileDescription.GetBuffer());
			m_versionInfo->ProductName = gcnew String(info.VersionInfo.ProductName.GetBuffer());
			m_versionInfo->FileVersion = gcnew String(info.VersionInfo.FileVersion.GetBuffer());
			m_versionInfo->CompanyName = gcnew String(info.VersionInfo.CompanyName.GetBuffer());
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
		property String^ ImagePath { String^ get() { return gcnew String(m_wrapper->ImagePath.GetBuffer()); } }
		property String^ ImageName { String^ get() { return gcnew String(m_wrapper->ImageFileName.GetBuffer()); } }
		property String^ CommandLine { String^ get() { return gcnew String(m_wrapper->CommandLine.GetBuffer()); } }
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
}