#pragma once
#pragma unmanaged

#include "../../Engine/Utilities.h"

#pragma managed

namespace WindowsUtils
{
	public ref class ResourceMessageTable
	{
	public:
		property Int64 Id { Int64 get() { return m_wrapper->Id; } }
		property String^ Message { String^ get() { return (gcnew String(m_wrapper->Message.GetBuffer()))->Trim(); } }

		ResourceMessageTable()
			: m_wrapper { new Core::WU_RESOURCE_MESSAGE_TABLE } { }
		
		ResourceMessageTable(Core::WU_RESOURCE_MESSAGE_TABLE& messageTable)
			: m_wrapper { new Core::WU_RESOURCE_MESSAGE_TABLE(messageTable.Id, messageTable.Message.GetBuffer()) } { }

		~ResourceMessageTable() { delete m_wrapper; }

	protected:
		!ResourceMessageTable() { delete m_wrapper; }

	private:
		Core::PWU_RESOURCE_MESSAGE_TABLE m_wrapper;
	};
}