#pragma once
#pragma managed

#include "CmdletContextProxy.h"

namespace WindowsUtils::Wrappers
{
	public ref class WrapperBase
	{
	public:
		WrapperBase(Core::CmdletContextProxy^ context)
			: m_context(context) { }

	protected:
		property Core::CmdletContextProxy^ Context { Core::CmdletContextProxy^ get() { return m_context; } }

	private:
		Core::CmdletContextProxy^ m_context;
	};
}