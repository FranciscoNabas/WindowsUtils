#pragma once
#pragma unmanaged

#include "../../Support/String.h"
#include "../../Engine/ProcessAndThread.h"

#include <Shlwapi.h>

#pragma managed

namespace WindowsUtils
{
	public enum class ObjectHandleType
	{
		FileSystem = 0,
		Registry = 1
	};

	public ref class ObjectHandle
	{
	public:
		property ObjectHandleType Type { ObjectHandleType get() { return static_cast<ObjectHandleType>(m_wrapper->Type); } }
		property String^ InputObject { String^ get() { return gcnew String(m_wrapper->InputObject.GetBuffer()); } }
		property String^ Name {
			String^ get()
			{
				if (!WWuString::IsNullOrEmpty(m_wrapper->Name))
					return gcnew String(m_wrapper->Name.GetBuffer());

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
							::PathStripPathW(name.GetBuffer());

							return gcnew String(name.GetBuffer());
						}
					}
					else
						return gcnew String(search->second.GetBuffer());
				}
				else {
					if (!WWuString::IsNullOrEmpty(m_wrapper->Name)) {
						WWuString name = m_wrapper->Name;
						::PathStripPathW(name.GetBuffer());

						return gcnew String(name.GetBuffer());
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
						return gcnew String(search->second.GetBuffer());

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
						return gcnew String(search->second.GetBuffer());

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
						return gcnew String(search->second.GetBuffer());

				return nullptr;
			}
		}
		property String^ ImagePath {
			String^ get()
			{
				if (!WWuString::IsNullOrEmpty(m_wrapper->ImagePath))
					return gcnew String(m_wrapper->ImagePath.GetBuffer());

				return nullptr;
			}
		}

		ObjectHandle()
			: m_wrapper { new Core::WU_OBJECT_HANDLE } { }
		
		ObjectHandle(const Core::WU_OBJECT_HANDLE& objectHandle)
			: m_wrapper { new Core::WU_OBJECT_HANDLE(objectHandle) } { }

		~ObjectHandle() { delete m_wrapper; }

	protected:
		!ObjectHandle() { delete m_wrapper; }

	private:
		Core::PWU_OBJECT_HANDLE m_wrapper;
	};

	public ref class ObjectHandleInput
	{
	public:
		property String^ Path { String^ get() { return m_Path; } }
		property ObjectHandleType Type { ObjectHandleType get() { return m_Type; } }

		ObjectHandleInput(String^ path, ObjectHandleType type)
			: m_Path(path), m_Type(type) { }

	private:
		String^ m_Path;
		ObjectHandleType m_Type;
	};
}