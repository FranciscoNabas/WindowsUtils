#pragma once

#pragma unmanaged
#include "Unmanaged.h"

#pragma managed
#include <vcclr.h>
#include <iostream>
#include <vector>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace Unmanaged;
using namespace Unmanaged::WindowsTerminalServices;

namespace Wrapper {

	public ref class Managed
	{
	public:
		TerminalServices* wtsPtr;
		Utilities* utlPtr;

		int strucSize = sizeof(TerminalServices::SessionEnumOutput);
		Type^ strucType = TerminalServices::SessionEnumOutput::typeid;

		enum class wWtsSessionState
		{
			Active,
			Connected,
			ConnectQuery,
			Shadow,
			Disconnected,
			Idle,
			Listen,
			Reset,
			Down,
			Init
		};

		ref class wSessionEnumOutput
		{
		public:
			int SessionId;
			String^ UserName;
			String^ SessionName;
			wWtsSessionState SessionState;
		};

		ref class wMessageDumpOutput
		{
		public:
			Int64 Id;
			String^ Message;
		};

		List<wSessionEnumOutput^>^ GetEnumeratedSession(IntPtr session, bool onlyActive, bool excludeSystemSessions)
		{
			List<wSessionEnumOutput^>^ output = gcnew List<wSessionEnumOutput^>();
			std::shared_ptr<std::vector<TerminalServices::SessionEnumOutput>> result = std::make_shared<std::vector<TerminalServices::SessionEnumOutput>>();
			wtsPtr->GetEnumeratedSession(*result, (HANDLE)session, onlyActive, excludeSystemSessions);

			for (size_t it = 0; it < result->size(); it++)
			{
				wSessionEnumOutput^ inner = gcnew wSessionEnumOutput();
				inner->SessionId = result->at(it).SessionId;
				inner->UserName = gcnew String(result->at(it).UserName);
				inner->SessionName = gcnew String(result->at(it).SessionName);
				inner->SessionState = (wWtsSessionState)result->at(it).SessionState;
				output->Add(inner);
			}

			return output;
		}

		List<int>^ InvokeMessage(IntPtr session, array<int>^ sessionId, String^ title, String^ message, UInt32 style, int timeout, bool wait)
		{
			std::vector<DWORD>* result = new std::vector<DWORD>();
			std::vector<DWORD>* unSessionId = (std::vector<DWORD>*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(std::vector<DWORD>));
			unSessionId->clear();
			pin_ptr<const wchar_t> wTitle = PtrToStringChars(title);
			pin_ptr<const wchar_t> wMessage = PtrToStringChars(message);

			if (sessionId == nullptr)
			{
				*result = wtsPtr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *unSessionId, (HANDLE)session);
			}
			else
			{
				for (int i = 0; i < sessionId->Length; i++) { unSessionId->push_back((DWORD)sessionId[i]); }
				*result = wtsPtr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *unSessionId, (HANDLE)session);
			}
			
			List<int>^ output = gcnew List<int>();
			for (size_t i = 0; i < result->size(); i++) { output->Add(result->at(i)); }

			delete result;
			HeapFree(GetProcessHeap(), NULL, unSessionId);
			return output;
		}

		List<wMessageDumpOutput^>^ GetResourceMessageTable(String^ libPath)
		{
			List<wMessageDumpOutput^>^ output = gcnew List<wMessageDumpOutput^>();
			std::vector<Utilities::MessageDumpOutput>* result = new std::vector<Utilities::MessageDumpOutput>();
			pin_ptr<const wchar_t> wLibPath = PtrToStringChars(libPath);

			*result = utlPtr->GetResourceMessageTable((LPWSTR)wLibPath);
			for (size_t i = 0; i < result->size(); i++)
			{
				Utilities::MessageDumpOutput single = result->at(i);
				wMessageDumpOutput^ inner = gcnew wMessageDumpOutput();

				inner->Id = std::stoll(single.Id, nullptr, 16);
				inner->Message = gcnew String(single.Message.c_str());

				output->Add(inner);
			}
			delete result;
			return output;
		}
	};
}
