﻿#pragma once

#include "Unmanaged.h"
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

namespace Wrapper {

	public ref class Managed
	{
	public:
		Unmanaged* ptr;

		int strucSize = sizeof(Unmanaged::SessionEnumOutput);
		Type^ strucType = Unmanaged::SessionEnumOutput::typeid;

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

		List<wSessionEnumOutput^>^ GetEnumeratedSession(IntPtr session, bool onlyActive, bool excludeSystemSessions)
		{
			List<wSessionEnumOutput^>^ output = gcnew List<wSessionEnumOutput^>();
			std::vector<Unmanaged::SessionEnumOutput>* result = new std::vector<Unmanaged::SessionEnumOutput>;
			*result = ptr->GetEnumeratedSession((HANDLE)session, onlyActive, excludeSystemSessions);

			for (size_t it = 0; it < result->size(); it++)
			{
				Unmanaged::SessionEnumOutput single = result->at(it);
				wSessionEnumOutput^ inner = gcnew wSessionEnumOutput();
				inner->SessionId = (int)single.SessionId;
				inner->UserName = gcnew String(single.UserName.c_str());
				inner->SessionName = gcnew String(single.SessionName.c_str());
				inner->SessionState = (wWtsSessionState)single.SessionState;
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
				*result = ptr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *unSessionId, (HANDLE)session);
			}
			else
			{
				for (int i = 0; i < sessionId->Length; i++) { unSessionId->push_back((DWORD)sessionId[i]); }
				*result = ptr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *unSessionId, (HANDLE)session);
			}
			
			List<int>^ output = gcnew List<int>();
			for (size_t i = 0; i < result->size(); i++) { output->Add(result->at(i)); }

			delete result;
			HeapFree(GetProcessHeap(), NULL, unSessionId);
			return output;
		}
	};
}
