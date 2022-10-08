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

	public ref class ComputerSession
	{
	public:
		// Allocate the native object on the C++ Heap via a constructor
		ComputerSession() : wrapper( new Unmanaged::ComputerSession ) { }
		
		ComputerSession(Unmanaged::ComputerSession item) {
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

		ComputerSession(Unmanaged::ComputerSession item, String^ _computername) {
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
		~ComputerSession() { delete wrapper; }

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
		property WindowsUtils::Core::SessionState^ SessionState {
			WindowsUtils::Core::SessionState^ get() {
				return WindowsUtils::Core::Enumeration::GetById<WindowsUtils::Core::SessionState^>(wrapper->SessionState);
			}
		}
		property String^ ComputerName { 
			String^ get() {
				return computerName;
			}
		
		}

	protected:
		// Deallocate the native object on the finalizer just in case no destructor is called
		!ComputerSession() { delete wrapper; }

	private:
		String^ computerName;
		Unmanaged::ComputerSession* wrapper;
	};

	public ref class ResourceMessageTable
	{
	public:
		property Int64 Id { Int64 get() { return wrapper->Id; } }
		property String^ Message { String^ get() { return gcnew String(wrapper->Message); } }
		
		ResourceMessageTable() : wrapper( new Unmanaged::ResourceMessageTable ) { }
		ResourceMessageTable(Unmanaged::ResourceMessageTable item) {
			wrapper = new Unmanaged::ResourceMessageTable(item.Id, item.Message);
		}
		~ResourceMessageTable() { delete wrapper; }

	protected:
		!ResourceMessageTable() { delete wrapper; }

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

	public ref class ObjectHandle
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

		ObjectHandle() : wrapper( new Unmanaged::ObjectHandle) { }
		ObjectHandle(Unmanaged::ObjectHandle item) {
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
		~ObjectHandle() { delete wrapper; }

	protected:
		!ObjectHandle() { delete wrapper; }

	private:
		Unmanaged::ObjectHandle* wrapper;
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

		array<ComputerSession^>^ GetEnumeratedSession(String^ computerName, IntPtr session, bool onlyActive, bool includeSystemSessions)
		{
			SharedVecPtr(Unmanaged::ComputerSession) result = MakeVecPtr(Unmanaged::ComputerSession);
			DWORD opresult = extptr->GetEnumeratedSession(*result, (HANDLE)session, onlyActive, includeSystemSessions);
			if (opresult != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormatedError(opresult));

			array<ComputerSession^>^ output = gcnew array<ComputerSession^>((int)result->size());
			
			if (nullptr != computerName)
				for (size_t i = 0; i < result->size(); i++)
					output[(int)i] = gcnew ComputerSession(result->at(i), computerName);
			else
				for (size_t i = 0; i < result->size(); i++)
					output[(int)i] = gcnew ComputerSession(result->at(i));

			return output;
		}
		List<int>^ InvokeMessage(IntPtr session, array<int>^ sessionId, String^ title, String^ message, UInt32 style, int timeout, bool wait)
		{
			// To do: convert to smart pointer.
			std::vector<DWORD>* result = new std::vector<DWORD>();
			std::vector<DWORD>* unSessionId = (std::vector<DWORD>*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(std::vector<DWORD>));
			unSessionId->clear();

			pin_ptr<const wchar_t> wTitle = PtrToStringChars(title);
			pin_ptr<const wchar_t> wMessage = PtrToStringChars(message);

			if (sessionId == nullptr)
			{
				*result = extptr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *unSessionId, (HANDLE)session);
			}
			else
			{
				for (int i = 0; i < sessionId->Length; i++) { unSessionId->push_back((DWORD)sessionId[i]); }
				*result = extptr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *unSessionId, (HANDLE)session);
			}
			
			List<int>^ output = gcnew List<int>();
			for (size_t i = 0; i < result->size(); i++) { output->Add(result->at(i)); }

			delete result;
			HeapFree(GetProcessHeap(), NULL, unSessionId);
			return output;
		}
		array<ResourceMessageTable^>^ GetResourceMessageTable(String^ libPath)
		{
			SharedVecPtr(Unmanaged::ResourceMessageTable) ppResult = MakeVecPtr(Unmanaged::ResourceMessageTable);
			pin_ptr<const wchar_t> wLibPath = PtrToStringChars(libPath);

			DWORD result = extptr->GetResourceMessageTable(*ppResult, (LPWSTR)wLibPath);
			if (result != ERROR_SUCCESS)
			{
				wLibPath = nullptr;
				throw gcnew SystemException(GetFormatedError(result));
			}

			array<ResourceMessageTable^>^ output = gcnew array<ResourceMessageTable^>((int)ppResult->size());
			for (size_t i = 0; i < ppResult->size(); i++)
				output[(int)i] = gcnew ResourceMessageTable(ppResult->at(i));

			wLibPath = nullptr;
			return output;
		}
		array<RpcEndpoint^>^ MapRpcEndpoints()
		{
			SharedVecPtr(Unmanaged::RpcEndpoint) result = MakeVecPtr(Unmanaged::RpcEndpoint);;
			DWORD opresult = extptr->MapRpcEndpoints(*result);
			if (opresult != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormatedError(opresult));

			array<RpcEndpoint^>^ output = gcnew array<RpcEndpoint^>((int)result->size());
			for (size_t i = 0; i < result->size(); i++)
				output[(int)i] = gcnew RpcEndpoint(result->at(i));

			return output;
		}
		String^ GetFormatedError(int errorCode)
		{
			LPWSTR result = extptr->GetFormatedError((DWORD)errorCode);
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
		array<ObjectHandle^>^ GetProcessObjectHandle(array<String^>^ fileName)
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
				throw gcnew SystemException(GetFormatedError(result));
			}
			if (ppOutput->size() == 0)
			{
				return nullptr;
			}

			array<ObjectHandle^>^ output = gcnew array<ObjectHandle^>((int)ppOutput->size());
			for (size_t i = 0; i < ppOutput->size(); i++)
				output[(int)i] = gcnew ObjectHandle(ppOutput->at(i));

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
					throw gcnew SystemException(GetFormatedError(result));
			}

			std::map<std::wstring, std::wstring>::iterator itr;
			for (itr = ppresult->begin(); itr != ppresult->end(); itr++)
			{
				output->Members->Add(gcnew PSNoteProperty(gcnew String(itr->first.c_str()), gcnew String(itr->second.c_str())));
			}

			return output;
		}
		void DisconnectSession(IntPtr session, Int32 sessionid, bool wait)
		{
			DWORD result = extptr->DisconnectSession((HANDLE)session, (DWORD)sessionid, wait);
			if (result != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormatedError(result));
		}
		void DisconnectSession(IntPtr session, bool wait)
		{
			DWORD result = extptr->DisconnectSession((HANDLE)session, NULL, wait);
			if (result != ERROR_SUCCESS)
				throw gcnew SystemException(GetFormatedError(result));
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