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
		DWORD GetMsiProperties(std::map<LPWSTR, LPWSTR>& ppmapout, LPWSTR fileName);
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

			class SessionEnumOutput
			{
			public:
				int				SessionId;
				LPWSTR			UserName;
				LPWSTR			SessionName;
				WtsSessionState	SessionState;

				SessionEnumOutput(LPWSTR usrName, LPWSTR sessName) {
					size_t usrSz = wcslen(usrName) + 1;
					size_t sesSz = wcslen(sessName) + 1;

					UserName = new wchar_t[usrSz];
					SessionName = new wchar_t[sesSz];

					wcscpy_s(UserName, usrSz, usrName);
					wcscpy_s(SessionName, sesSz, sessName);
				};
				~SessionEnumOutput() {};
			}*PSessionEnumOutput;

			void GetEnumeratedSession(std::vector<TerminalServices::SessionEnumOutput>& ppOutVec, HANDLE session, BOOL onlyActive, BOOL excludeSystemSessions);
			std::vector<DWORD> InvokeMessage(LPWSTR pTitle, LPWSTR pMessage, DWORD style, DWORD timeout, BOOL bWait, std::vector<DWORD> sessionId, HANDLE session);
		};
	}
}