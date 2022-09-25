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

namespace Unmanaged
{
	extern "C" public class __declspec(dllexport) Utilities
	{
	public:
		typedef struct MessageDumpOutput {
			MessageDumpOutput() {};
			DWORD	Id;
			LPWSTR	Message;
		}MessageDumpOutput, * PMessageDumpOutput;

		typedef struct FileHandleOutput {
			RM_APP_TYPE	AppType;
			DWORD		ProcessId;
			LPWSTR		AppName;
			LPWSTR		ImagePath;

			FileHandleOutput() { };
			~FileHandleOutput() { }
		}FileHandleOutput, * PFileHandleOutput;

		typedef struct RpcMapperOutput {
			LPWSTR	BindingString;
			LPWSTR	Annotation;

			RpcMapperOutput() { }
			~RpcMapperOutput() { }
		}RpcMapperOutput, *PRpcMapperOutput;

		DWORD Utilities::GetResourceMessageTable(std::vector<Utilities::MessageDumpOutput>& ppvecmdo, LPTSTR libName);
		DWORD MapRpcEndpoints(std::vector<Utilities::RpcMapperOutput>& ppOutVec);
		LPWSTR GetFormatedWSError();
		LPWSTR GetFormatedWin32Error();
		LPWSTR GetFormatedError(DWORD errorCode);
		DWORD GetProcessFileHandle(std::vector<FileHandleOutput>& ppvecfho, PCWSTR fileName);
		DWORD GetMsiProperties(std::map<std::wstring, std::wstring>& ppmapout, LPWSTR fileName);
		DWORD GetMsiExtendedErrorMessage(LPWSTR& pErrorMessage);
	};

	namespace WindowsTerminalServices
	{
		extern "C" public class __declspec(dllexport) TerminalServices
		{
		public:
			enum class WtsSessionState {
				Active = WTSActive,
				Connected = WTSConnected,
				ConnectQuery = WTSConnectQuery,
				Shadow = WTSShadow,
				Disconnected = WTSDisconnected,
				Idle = WTSIdle,
				Listen = WTSListen,
				Reset = WTSReset,
				Down = WTSDown,
				Init = WTSInit,
			};

			typedef struct SessionEnumOutput
			{
			public:
				int				SessionId;
				LPWSTR			UserName;
				LPWSTR			SessionName;
				LARGE_INTEGER	IdleTime;
				LARGE_INTEGER	LogonTime;
				WtsSessionState	SessionState;

				SessionEnumOutput() { };
				~SessionEnumOutput() { };
			}SessionEnumOutput, *PSessionEnumOutput;

			DWORD GetEnumeratedSession(std::vector<TerminalServices::SessionEnumOutput>& ppOutVec, HANDLE session, BOOL onlyActive, BOOL includeSystemSessions);
			std::vector<DWORD> InvokeMessage(LPWSTR pTitle, LPWSTR pMessage, DWORD style, DWORD timeout, BOOL bWait, std::vector<DWORD> sessionId, HANDLE session);
			DWORD DisconnectSession(HANDLE session, DWORD sessionid, BOOL wait);
		};
	}
}