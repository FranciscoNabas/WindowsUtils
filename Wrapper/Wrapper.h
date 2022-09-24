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

#define SharedVecPtr(T) std::shared_ptr<std::vector<T>>
#define MakeVecPtr(T) std::make_shared<std::vector<T>>()

using namespace System;
using namespace System::Collections::Generic;

using namespace System::Runtime::InteropServices;
using namespace System::Management::Automation;
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

		enum class WtsSessionState
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

		enum class AppType
		{
			UnknownApp = 0,
			MainWindow = 1,
			OtherWindow = 2,
			Service = 3,
			Explorer = 4,
			Console = 5,
			Critical = 1000
		};

		ref class SessionEnumOutput
		{
		public:
			int SessionId;
			String^ UserName;
			String^ SessionName;
			Nullable<TimeSpan> IdleTime;
			Nullable<DateTime> LogonDate;
			WtsSessionState SessionState;
		};

		ref class MessageDumpOutput
		{
		public:
			Int64 Id;
			String^ Message;
		};

		ref class RpcMapperOutput
		{
		public:
			String^	BindingString;
			String^ Annotation;

			RpcMapperOutput(String^ bs, String^ an) : BindingString(bs), Annotation(an) { }
			~RpcMapperOutput() { }
		};

		ref class FileHandleOutput
		{
		public:
			UInt32^ ProcessId;
			AppType AppType;
			String^ AppName;
			String^ ImagePath;
		};

		List<SessionEnumOutput^>^ GetEnumeratedSession(IntPtr session, bool onlyActive, bool includeSystemSessions)
		{
			List<SessionEnumOutput^>^ output = gcnew List<SessionEnumOutput^>();
			std::shared_ptr<std::vector<TerminalServices::SessionEnumOutput>> result = std::make_shared<std::vector<TerminalServices::SessionEnumOutput>>();
			DWORD opresult = wtsPtr->GetEnumeratedSession(*result, (HANDLE)session, onlyActive, includeSystemSessions);
			if (opresult != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormatedError(opresult));

			for (size_t it = 0; it < result->size(); it++)
			{
				SessionEnumOutput^ inner = gcnew SessionEnumOutput();
				inner->SessionId = result->at(it).SessionId;
				inner->UserName = gcnew String(result->at(it).UserName);
				inner->SessionName = gcnew String(result->at(it).SessionName);
				inner->SessionState = (WtsSessionState)result->at(it).SessionState;
				inner->LogonDate = DateTime::FromFileTime(result->at(it).LogonTime.QuadPart);
				if (result->at(it).IdleTime.QuadPart > 0)
					inner->IdleTime = DateTime::Now - DateTime::FromFileTime(result->at(it).IdleTime.QuadPart);
				else
					inner->IdleTime = TimeSpan::Zero;
				
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

		List<MessageDumpOutput^>^ GetResourceMessageTable(String^ libPath)
		{
			SharedVecPtr(Utilities::MessageDumpOutput) ppResult = MakeVecPtr(Utilities::MessageDumpOutput);
			List<MessageDumpOutput^>^ output = gcnew List<MessageDumpOutput^>();
			pin_ptr<const wchar_t> wLibPath = PtrToStringChars(libPath);

			DWORD result = utlPtr->GetResourceMessageTable(*ppResult, (LPWSTR)wLibPath);

			if (result != ERROR_SUCCESS)
			{
				wLibPath = nullptr;
				throw gcnew SystemException(GetFormatedError(result));
			}

			for (size_t i = 0; i < ppResult->size(); i++)
			{
				Utilities::MessageDumpOutput single = ppResult->at(i);
				MessageDumpOutput^ inner = gcnew MessageDumpOutput();

				inner->Id = single.Id;
				inner->Message = gcnew String(single.Message);

				output->Add(inner);
			}

			wLibPath = nullptr;
			return output;
		}

		List<RpcMapperOutput^>^ MapRpcEndpoints()
		{
			List<RpcMapperOutput^>^ output = gcnew List<RpcMapperOutput^>();
			SharedVecPtr(Utilities::RpcMapperOutput) result = MakeVecPtr(Utilities::RpcMapperOutput);;

			utlPtr->MapRpcEndpoints(*result);

			for (size_t i = 0; i < result->size(); i++)
			{
				output->Add(gcnew RpcMapperOutput(gcnew String(result->at(i).BindingString), gcnew String(result->at(i).Annotation)));
			}

			return output;
		}

		String^ GetFormatedError(int errorCode)
		{
			LPWSTR result = utlPtr->GetFormatedError((DWORD)errorCode);
			String^ output = gcnew String(result);
			GlobalFree(result);
			return output;
		}
		String^ GetFormatedWin32Error()
		{
			LPWSTR result = utlPtr->GetFormatedWin32Error();
			String^ output = gcnew String(result);
			GlobalFree(result);
			return output;
		}
		String^ GetFormatedWSError()
		{
			LPWSTR result = utlPtr->GetFormatedWSError();
			String^ output = gcnew String(result);
			GlobalFree(result);
			return output;
		}

		List<FileHandleOutput^>^ GetProcessFileHandle(String^ fileName)
		{
			List<FileHandleOutput^>^ output = gcnew List<FileHandleOutput^>();
			SharedVecPtr(Unmanaged::Utilities::FileHandleOutput) ppOutput = MakeVecPtr(Unmanaged::Utilities::FileHandleOutput);
			pin_ptr<const wchar_t> wFileName = PtrToStringChars(fileName);
			UINT result = utlPtr->GetProcessFileHandle(*ppOutput, (PCWSTR)wFileName);

			if (result != ERROR_SUCCESS)
			{
				wFileName = nullptr;
				throw gcnew SystemException(GetFormatedError(result));
			}

			if (ppOutput->size() == 0)
			{
				wFileName = nullptr;
				return nullptr;
			}

			for (size_t i = 0; i < ppOutput->size(); i++)
			{
				FileHandleOutput^ single = gcnew FileHandleOutput();
				single->ProcessId = ppOutput->at(i).ProcessId;
				single->AppType = (AppType)ppOutput->at(i).AppType;
				single->AppName = gcnew String(ppOutput->at(i).AppName);
				single->ImagePath = gcnew String(ppOutput->at(i).ImagePath);

				output->Add(single);
			}

			wFileName = nullptr;
			return output;
		}
	
		PSObject^ GetMsiProperties(String^ fileName)
		{
			PSObject^ output = gcnew PSObject();
			std::shared_ptr<std::map<std::wstring, std::wstring>> ppresult = std::make_shared<std::map<std::wstring, std::wstring>>();
			pin_ptr<const wchar_t> wfilename = PtrToStringChars(fileName);

			DWORD result = utlPtr->GetMsiProperties(*ppresult, (LPWSTR)wfilename);
			if (ERROR_SUCCESS != result && ERROR_NO_MORE_ITEMS != result)
			{
				wfilename = nullptr;
				LPWSTR pextmsierr;
				DWORD inResu = utlPtr->GetMsiExtendedErrorMessage(pextmsierr);
				if (ERROR_SUCCESS == inResu)
					throw gcnew SystemException(gcnew String(pextmsierr));
				else
					throw gcnew SystemException(GetFormatedError(result));
			}

			std::map<std::wstring, std::wstring>::iterator itr;
			for (itr = ppresult->begin(); itr != ppresult->end(); itr++)
			{
				output->Members->Add(gcnew PSNoteProperty(gcnew String(itr->first.c_str()), gcnew String(itr->second.c_str())));
			}

			return output;
		}
	};
}