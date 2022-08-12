#pragma once

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
			String^ UserName;
			String^ SessionName;
			wWtsSessionState SessionState;
		};

		List<wSessionEnumOutput^>^ GetEnumeratedSession(String^ computerName, bool onlyActive, bool excludeSystemSessions)
		{
			pin_ptr<const wchar_t> wName = PtrToStringChars(computerName);
			List<wSessionEnumOutput^>^ output = gcnew List<wSessionEnumOutput^>();
			vector<Unmanaged::SessionEnumOutput> *result = new vector<Unmanaged::SessionEnumOutput>;
			*result = ptr->GetEnumeratedSession((LPWSTR)wName, onlyActive, excludeSystemSessions);
			
			for (size_t it = 0; it < result->size(); it++)
			{
				Unmanaged::SessionEnumOutput single = result->at(it);
				wSessionEnumOutput^ inner = gcnew wSessionEnumOutput();
				inner->UserName = gcnew String(single.UserName.c_str());
				inner->SessionName = gcnew String(single.SessionName.c_str());
				inner->SessionState = (wWtsSessionState)single.SessionState;
				output->Add(inner);
			}
			
			return output;
		};
	};
}
