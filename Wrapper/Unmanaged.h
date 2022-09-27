#pragma once
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

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

namespace WindowsUtils
{
	extern "C" public class __declspec(dllexport) Unmanaged
	{
	public:

		enum class AppType
		{
			UnknownApp = RM_APP_TYPE::RmUnknownApp,
			MainWindow = RM_APP_TYPE::RmMainWindow,
			OtherWindow = RM_APP_TYPE::RmOtherWindow,
			Service = RM_APP_TYPE::RmService,
			Explorer = RM_APP_TYPE::RmExplorer,
			Console = RM_APP_TYPE::RmConsole,
			Critical = RM_APP_TYPE::RmCritical
		};

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
			AppType		AppType;
			DWORD		ProcessId;
			LPWSTR		AppName;
			LPWSTR		ImagePath;
			FileHandle() { }
			FileHandle(Unmanaged::AppType apptype, DWORD pid, LPWSTR appname, LPWSTR imgpath) : AppType(apptype), ProcessId(pid), AppName(appname), ImagePath(imgpath) { }
		};

		class RpcEndpoint
		{
		public:
			LPWSTR	BindingString;
			LPWSTR	Annotation;
			RpcEndpoint() { }
			RpcEndpoint(LPWSTR bstr, LPWSTR ann) : BindingString(bstr), Annotation(ann) { }
		};

		DWORD GetResourceMessageTable(std::vector<ResourceMessageTable>& ppvecmdo, LPTSTR libName);
		DWORD MapRpcEndpoints(std::vector<RpcEndpoint>& ppOutVec);
		LPWSTR GetFormatedWSError();
		LPWSTR GetFormatedWin32Error();
		LPWSTR GetFormatedError(DWORD errorCode);
		DWORD GetProcessFileHandle(std::vector<FileHandle>& ppvecfho, PCWSTR fileName);
		DWORD GetMsiProperties(std::map<std::wstring, std::wstring>& ppmapout, LPWSTR fileName);
		DWORD GetMsiExtendedErrorMessage(LPWSTR& pErrorMessage);
		DWORD GetEnumeratedSession(std::vector<ComputerSession>& ppOutVec, HANDLE session, BOOL onlyActive, BOOL includeSystemSessions);
		std::vector<DWORD> InvokeMessage(LPWSTR pTitle, LPWSTR pMessage, DWORD style, DWORD timeout, BOOL bWait, std::vector<DWORD> sessionId, HANDLE session);
		DWORD DisconnectSession(HANDLE session, DWORD sessionid, BOOL wait);
	};
}