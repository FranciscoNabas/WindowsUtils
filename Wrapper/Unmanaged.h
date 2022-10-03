﻿#pragma once
#pragma unmanaged

#include <WtsApi32.h>
#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <Windows.h>
#include <iostream>
#include <memory>
#include <fwpmu.h>
#include <Rpc.h>
#include <WinSock2.h>
#include <RestartManager.h>
#include <MsiQuery.h>
#include <ntddkbd.h>
#include <winternl.h>
#include <Psapi.h>
#include <ShlObj.h>    // Shell API
#include <Propkey.h>   // PKEY_* constants
#include <atlbase.h>   // CComPtr, CComHeapPtr

#define CHECKNTSTATUS(stat) if (STATUS_SUCCESS != stat) { return stat; }
#define STATUS_SUCCESS ERROR_SUCCESS
constexpr auto STATUS_INFO_LENGTH_MISMATCH = -1073741820;

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

namespace WindowsUtils
{
	extern "C" public class __declspec(dllexport) Unmanaged
	{
	public:

		class ComputerSession
		{
		public:
			int						SessionId;
			LPWSTR					Domain;
			LPWSTR					UserName;
			LPWSTR					SessionName;
			LARGE_INTEGER			IdleTime;
			LARGE_INTEGER			LogonTime;
			WTS_CONNECTSTATE_CLASS	SessionState;
			ComputerSession() { }
			ComputerSession(int sessid, LPWSTR domain, LPWSTR usrname, LPWSTR sessname, LARGE_INTEGER idltime, LARGE_INTEGER lgtime, WTS_CONNECTSTATE_CLASS sesstate)
				: SessionId(sessid), UserName(usrname), SessionName(sessname), IdleTime(idltime), LogonTime(lgtime), SessionState(sesstate) { }
		};

		class ResourceMessageTable
		{
		public:
			DWORD	Id;
			LPWSTR	Message;
			ResourceMessageTable() { }
			ResourceMessageTable(DWORD id, LPWSTR message) : Id(id), Message(message) { }
		};

		class FileHandle
		{
		public:
			LPWSTR		FileName;
			DWORD		ProcessId;
			LPWSTR		Application;
			LPWSTR		ProductName;
			LPWSTR		FileVersion;
			LPWSTR		ImagePath;
			FileHandle() { }
			FileHandle(LPWSTR filename, DWORD pid, LPWSTR appname, LPWSTR procname, LPWSTR fileversion, LPWSTR imgpath)
				: FileName(filename), ProcessId(pid), Application(appname), ProductName(procname), FileVersion(fileversion), ImagePath(imgpath) { }
		};

		class RpcEndpoint
		{
		public:
			LPWSTR	BindingString;
			LPWSTR	Annotation;
			RpcEndpoint() { }
			RpcEndpoint(LPWSTR bstr, LPWSTR ann) : BindingString(bstr), Annotation(ann) { }
		};

		DWORD GetProcessFileHandle(std::vector<FileHandle>& ppvecfho, std::vector<LPCWSTR> reslist);
		DWORD GetResourceMessageTable(std::vector<ResourceMessageTable>& ppvecmdo, LPTSTR libName);
		DWORD MapRpcEndpoints(std::vector<RpcEndpoint>& ppOutVec);
		LPWSTR GetFormatedWSError();
		LPWSTR GetFormatedWin32Error();
		LPWSTR GetFormatedError(DWORD errorCode);
		DWORD GetMsiProperties(std::map<std::wstring, std::wstring>& ppmapout, LPWSTR fileName);
		DWORD GetMsiExtendedErrorMessage(LPWSTR& pErrorMessage);
		DWORD GetEnumeratedSession(std::vector<ComputerSession>& ppOutVec, HANDLE session, BOOL onlyActive, BOOL includeSystemSessions);
		std::vector<DWORD> InvokeMessage(LPWSTR pTitle, LPWSTR pMessage, DWORD style, DWORD timeout, BOOL bWait, std::vector<DWORD> sessionId, HANDLE session);
		DWORD DisconnectSession(HANDLE session, DWORD sessionid, BOOL wait);
	};
}