#pragma once
#pragma unmanaged

#include "../../Engine/TerminalServices.h"

#pragma managed

namespace WindowsUtils
{
	using namespace System;

	public ref class MessageResponse
	{
	public:
		property UInt32 SessionId { UInt32 get() { return m_wrapper->SessionId; } }
		property UInt32 Response { UInt32 get() { return m_wrapper->Response; } }

		MessageResponse()
			: m_wrapper { new Core::WU_MESSAGE_RESPONSE } { }

		MessageResponse(const Core::WU_MESSAGE_RESPONSE& messageResponse)
			: m_wrapper { new Core::WU_MESSAGE_RESPONSE { messageResponse.SessionId, messageResponse.Response } } { }

		~MessageResponse() { delete m_wrapper; }

	protected:
		!MessageResponse() { delete m_wrapper; }

	private:
		Core::PWU_MESSAGE_RESPONSE m_wrapper;
	};
}