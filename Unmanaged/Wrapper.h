#pragma once

#include "Unmanaged.h"
#include <vcclr.h>
#include <iostream>
#include <vector>

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
			vector<Unmanaged::SessionEnumOutput> result = ptr->GetEnumeratedSession((LPWSTR)wName, onlyActive, excludeSystemSessions);
			
			for (size_t it = 0; it < result.size(); it++)
			{
				wSessionEnumOutput^ inner = gcnew wSessionEnumOutput();
				inner->UserName = Marshal::PtrToStringUni((IntPtr)result[it].UserName);
				inner->SessionName = Marshal::PtrToStringUni((IntPtr)result[it].SessionName);
				inner->SessionState = (wWtsSessionState)result[it].SessionState;
				output->Add(inner);
			}
			
			return output;
		};
	};
}
