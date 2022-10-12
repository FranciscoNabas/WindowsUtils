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
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace System::Management::Automation;

namespace WindowsUtils::Core
{
	public ref class ComputerSessionBase
	{
	public:
		// Allocate the native object on the C++ Heap via a constructor
		ComputerSessionBase() : wrapper( new Unmanaged::ComputerSession ) { }
		
		ComputerSessionBase(Unmanaged::ComputerSession item) {
			wrapper = new Unmanaged::ComputerSession(
				item.SessionId,
				item.Domain,
				item.UserName,
				item.SessionName,
				item.IdleTime,
				item.LogonTime,
				item.SessionState
			);
		}

		ComputerSessionBase(Unmanaged::ComputerSession item, String^ _computername) {
			wrapper = new Unmanaged::ComputerSession(
				item.SessionId,
				item.Domain,
				item.UserName,
				item.SessionName,
				item.IdleTime,
				item.LogonTime,
				item.SessionState
			);
			computerName = _computername;
		}

		// Deallocate the native object on a destructor
		~ComputerSessionBase() { delete wrapper; }

		// Defining managed properties
		property Int32 SessionId {
			Int32 get() { return wrapper->SessionId; }
		}
		property String^ UserName {
			String^ get() {
				return gcnew String(wrapper->UserName);
			}
		}
		property String^ SessionName {
			String^ get() { return gcnew String(wrapper->SessionName); }
		}
		property TimeSpan IdleTime {
			TimeSpan get() {
				if (wrapper->IdleTime.QuadPart == 0)
					return TimeSpan::Zero;
				else
					return DateTime::Now - DateTime::FromFileTime(wrapper->IdleTime.QuadPart);
			}
		}
		property DateTime LogonTime {
			DateTime get() {
				return DateTime::FromFileTime(wrapper->LogonTime.QuadPart);
			}
		}
		property UInt32 SessionState {
			UInt32 get() {
				return wrapper->SessionState;
			}
		}
		property String^ ComputerName { 
			String^ get() {
				return computerName;
			}
		
		}

	protected:
		// Deallocate the native object on the finalizer just in case no destructor is called
		!ComputerSessionBase() { delete wrapper; }

	private:
		String^ computerName;
		Unmanaged::ComputerSession* wrapper;
	};

	public ref class ResourceMessageTableCore
	{
	public:
		property Int64 Id { Int64 get() { return wrapper->Id; } }
		property String^ Message { String^ get() { return gcnew String(wrapper->Message); } }
		
		ResourceMessageTableCore() : wrapper( new Unmanaged::ResourceMessageTable ) { }
		ResourceMessageTableCore(Unmanaged::ResourceMessageTable item) {
			wrapper = new Unmanaged::ResourceMessageTable(item.Id, item.Message);
		}
		~ResourceMessageTableCore() { delete wrapper; }

	protected:
		!ResourceMessageTableCore() { delete wrapper; }

	private:
		Unmanaged::ResourceMessageTable* wrapper;
	};

	public ref class RpcEndpoint
	{
	public:
		property String^ BindingString { String^ get() { return gcnew String(wrapper->BindingString); } }
		property String^ Annotation { String^ get() { return gcnew String(wrapper->Annotation); } }

		RpcEndpoint() : wrapper( new Unmanaged::RpcEndpoint ) { }
		RpcEndpoint(Unmanaged::RpcEndpoint item) {
			wrapper = new Unmanaged::RpcEndpoint(item.BindingString, item.Annotation);
		}
		~RpcEndpoint() { delete wrapper; }

	protected:
		!RpcEndpoint() { delete wrapper; }

	private:
		Unmanaged::RpcEndpoint* wrapper;
	};

	public ref class ObjectHandleBase
	{
	public:
		property String^ InputObject { String^ get() { return gcnew String(wrapper->InputObject); } }
		property UInt32 ProcessId { UInt32 get() { return wrapper->ProcessId; } }
		property String^ Application {
			String^ get() {
				if (NULL != wrapper->Application)
					return gcnew String(wrapper->Application);

				return nullptr;
			}
		}
		property String^ ProductName {
			String^ get() {
				if (NULL != wrapper->ProductName)
					return gcnew String(wrapper->ProductName);
				
				return nullptr;
			}
		}
		property String^ FileVersion {
			String^ get() {
				if (NULL != wrapper->FileVersion)
					return gcnew String(wrapper->FileVersion);

				return nullptr;
			}
		}
		property String^ CompanyName {
			String^ get() {
				if (NULL != wrapper->CompanyName)
					return gcnew String(wrapper->CompanyName);

				return nullptr;
			}
		}
		property String^ ImagePath { String^ get() { return gcnew String(wrapper->ImagePath); } }

		ObjectHandleBase() : wrapper( new Unmanaged::ObjectHandle) { }
		ObjectHandleBase(Unmanaged::ObjectHandle item) {
			wrapper = new Unmanaged::ObjectHandle(
				item.InputObject,
				item.ProcessId,
				item.Application,
				item.ProductName,
				item.FileVersion,
				item.CompanyName,
				item.ImagePath
			);
		}
		~ObjectHandleBase() { delete wrapper; }

	protected:
		!ObjectHandleBase() { delete wrapper; }

	private:
		Unmanaged::ObjectHandle* wrapper;
	};

	public ref class MessageResponseBase
	{
	public:
		property UInt32 SessionId { UInt32 get() { return wrapper->SessionId; } }
		property UInt32 Response { UInt32 get() { return wrapper->Response; } }

		MessageResponseBase() : wrapper( new Unmanaged::MessageResponse ) { }
		MessageResponseBase(Unmanaged::MessageResponse item) {
			wrapper = new Unmanaged::MessageResponse(
				item.SessionId,
				item.Response
			);
		}
		~MessageResponseBase() { delete wrapper; }

	protected:
		!MessageResponseBase() { delete wrapper; }

	private:
		Unmanaged::MessageResponse* wrapper;
	};


	public ref class SystemHandle
	{
	public:
		UInt32 ProcessId;
		Byte ObjectTypeNumber;
		String^ ObjectTypeName;
		String^ ExtPropertyInfo;
		Byte Flags;
	};

	/*
		All calls to native code functions
	*/
	public ref class WrappedFunctions
	{
	public:
		Unmanaged* extptr;

		array<ComputerSessionBase^>^ GetEnumeratedSession(String^ computerName, IntPtr session, bool onlyActive, bool includeSystemSessions)
		{
			SharedVecPtr(Unmanaged::ComputerSession) result = MakeVecPtr(Unmanaged::ComputerSession);
			DWORD opresult = extptr->GetEnumeratedSession(*result, (HANDLE)session, onlyActive, includeSystemSessions);
			if (opresult != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormattedError(opresult));

			array<ComputerSessionBase^>^ output = gcnew array<ComputerSessionBase^>((int)result->size());
			
			if (nullptr != computerName)
				for (size_t i = 0; i < result->size(); i++)
					output[(int)i] = gcnew ComputerSessionBase(result->at(i), computerName);
			else
				for (size_t i = 0; i < result->size(); i++)
					output[(int)i] = gcnew ComputerSessionBase(result->at(i));

			return output;
		}
		array<MessageResponseBase^>^ InvokeMessage(IntPtr session, array<int>^ sessionId, String^ title, String^ message, UInt32 style, int timeout, bool wait)
		{
			DWORD dwresult = ERROR_SUCCESS;
			SharedVecPtr(Unmanaged::MessageResponse) presult = MakeVecPtr(Unmanaged::MessageResponse);
			SharedVecPtr(DWORD) psessid = MakeVecPtr(DWORD);

			pin_ptr<const wchar_t> wTitle = PtrToStringChars(title);
			pin_ptr<const wchar_t> wMessage = PtrToStringChars(message);

			if (sessionId == nullptr)
				dwresult = extptr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *psessid, *presult, (HANDLE)session);
			
			else
			{
				for (int i = 0; i < sessionId->Length; i++)
					psessid->push_back((DWORD)sessionId[i]);

				dwresult = extptr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *psessid, *presult, (HANDLE)session);
			}

			if (ERROR_SUCCESS != dwresult)
				throw gcnew SystemException(GetFormattedError(dwresult));
			
			array<MessageResponseBase^>^ output = gcnew array<MessageResponseBase^>((int)presult->size());
			
			for (size_t i = 0; i < presult->size(); i++)
				if (presult->at(i).Response != 0)
					output[(int)i] = gcnew MessageResponseBase(presult->at(i));

			return output;
		}
		array<ResourceMessageTableCore^>^ GetResourceMessageTable(String^ libPath)
		{
			SharedVecPtr(Unmanaged::ResourceMessageTable) ppResult = MakeVecPtr(Unmanaged::ResourceMessageTable);
			pin_ptr<const wchar_t> wLibPath = PtrToStringChars(libPath);

			DWORD result = extptr->GetResourceMessageTable(*ppResult, (LPWSTR)wLibPath);
			if (result != ERROR_SUCCESS)
			{
				wLibPath = nullptr;
				throw gcnew SystemException(GetFormattedError(result));
			}

			array<ResourceMessageTableCore^>^ output = gcnew array<ResourceMessageTableCore^>((int)ppResult->size());
			for (size_t i = 0; i < ppResult->size(); i++)
				output[(int)i] = gcnew ResourceMessageTableCore(ppResult->at(i));

			wLibPath = nullptr;
			return output;
		}
		array<RpcEndpoint^>^ MapRpcEndpoints()
		{
			SharedVecPtr(Unmanaged::RpcEndpoint) result = MakeVecPtr(Unmanaged::RpcEndpoint);;
			DWORD opresult = extptr->MapRpcEndpoints(*result);
			if (opresult != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormattedError(opresult));

			array<RpcEndpoint^>^ output = gcnew array<RpcEndpoint^>((int)result->size());
			for (size_t i = 0; i < result->size(); i++)
				output[(int)i] = gcnew RpcEndpoint(result->at(i));

			return output;
		}
		String^ GetFormattedError(int errorCode)
		{
			LPWSTR result = extptr->GetFormattedError((DWORD)errorCode);
			String^ output = gcnew String(result);
			GlobalFree(result);
			return output;
		}
		String^ GetFormatedWin32Error()
		{
			LPWSTR result = extptr->GetFormatedWin32Error();
			String^ output = gcnew String(result);
			GlobalFree(result);
			return output;
		}
		String^ GetFormatedWSError()
		{
			LPWSTR result = extptr->GetFormatedWSError();
			String^ output = gcnew String(result);
			GlobalFree(result);
			return output;
		}
		array<ObjectHandleBase^>^ GetProcessObjectHandle(array<String^>^ fileName)
		{
			SharedVecPtr(Unmanaged::ObjectHandle) ppOutput = MakeVecPtr(Unmanaged::ObjectHandle);
			SharedVecPtr(LPCWSTR) reslist = MakeVecPtr(LPCWSTR);
			
			for (int i = 0; i < fileName->Length; i++)
			{
				pin_ptr<const WCHAR> single = PtrToStringChars(fileName[i]);
				reslist->push_back((LPCWSTR)single);
				single = nullptr;
			}

			UINT result = extptr->GetProcessObjectHandle(*ppOutput, *reslist);
			if (result != ERROR_SUCCESS)
			{
				throw gcnew SystemException(GetFormattedError(result));
			}
			if (ppOutput->size() == 0)
			{
				return nullptr;
			}

			array<ObjectHandleBase^>^ output = gcnew array<ObjectHandleBase^>((int)ppOutput->size());
			for (size_t i = 0; i < ppOutput->size(); i++)
				output[(int)i] = gcnew ObjectHandleBase(ppOutput->at(i));

			return output;
		}
		PSObject^ GetMsiProperties(String^ fileName)
		{
			PSObject^ output = gcnew PSObject();
			std::shared_ptr<std::map<std::wstring, std::wstring>> ppresult = std::make_shared<std::map<std::wstring, std::wstring>>();
			pin_ptr<const wchar_t> wfilename = PtrToStringChars(fileName);

			DWORD result = extptr->GetMsiProperties(*ppresult, (LPWSTR)wfilename);
			if (ERROR_SUCCESS != result && ERROR_NO_MORE_ITEMS != result)
			{
				wfilename = nullptr;
				LPWSTR pextmsierr;
				DWORD inResu = extptr->GetMsiExtendedErrorMessage(pextmsierr);
				if (ERROR_SUCCESS == inResu)
					throw gcnew SystemException(gcnew String(pextmsierr));
				else
					throw gcnew SystemException(GetFormattedError(result));
			}

			std::map<std::wstring, std::wstring>::iterator itr;
			for (itr = ppresult->begin(); itr != ppresult->end(); itr++)
			{
				output->Members->Add(gcnew PSNoteProperty(gcnew String(itr->first.c_str()), gcnew String(itr->second.c_str())));
			}

			return output;
		}
		void DisconnectSession(IntPtr session, UInt32 sessionid, bool wait)
		{
			DWORD result = extptr->DisconnectSession((HANDLE)session, (DWORD)sessionid, wait);
			if (result != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormattedError(result));
		}
		void DisconnectSession(IntPtr session, bool wait)
		{
			DWORD result = extptr->DisconnectSession((HANDLE)session, NULL, wait);
			if (result != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormattedError(result));
		}

		void SendClick()
		{
			DWORD result = extptr->SendClick();
			
			if (ERROR_SUCCESS != result)
				throw gcnew SystemException(GetFormattedError(result));
		}
	
		/*
			Testing stuff
		*/

		

		List<SystemHandle^>^ GetNtSystemHandleInformation()
		{
			SharedVecPtr(Unmanaged::SystemHandleOutInfo) pvout = MakeVecPtr(Unmanaged::SystemHandleOutInfo);
			NTSTATUS result = extptr->GetNtSystemInformation(*pvout);
			List<SystemHandle^>^ output = gcnew List<SystemHandle^>();

			for (size_t i = 0; i < pvout->size(); i++)
			{
				Unmanaged::SystemHandleOutInfo single = pvout->at(i);
				SystemHandle^ inner = gcnew SystemHandle();
				
				inner->ProcessId = single.ProcessId;
				inner->ObjectTypeNumber = single.ObjectTypeNumber;
				inner->Flags = single.Flags;
				inner->ObjectTypeName = gcnew String(single.ObjectTypeName);
				inner->ExtPropertyInfo = gcnew String(single.ExtPropertyInfo);

				output->Add(inner);
			}

			return output;
		}

	};
}