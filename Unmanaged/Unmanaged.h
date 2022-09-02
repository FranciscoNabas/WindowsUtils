#pragma once
#pragma unmanaged

#include <WtsApi32.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <Windows.h>
#include <iostream>
#include <memory>
#include <fwpmu.h>
#include <Rpc.h>
#include <WinSock2.h>
#include <RestartManager.h>

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
			std::wstring	Id;
			std::wstring	Message;
		}MessageDumpOutput, * PMessageDumpOutput;

		typedef struct FileHandleOutput {
			RM_APP_TYPE	AppType;
			DWORD		ProcessId;
			LPWSTR		AppName;
			LPWSTR		ImagePath;

			FileHandleOutput() { };
			FileHandleOutput(RM_APP_TYPE appType, DWORD procId, LPWSTR appName)
				: AppType(appType), ProcessId(procId)
			{
				size_t nameSz = wcslen(appName) + 1;
				
				AppName = new wchar_t[nameSz];
				wcscpy_s(AppName, nameSz, appName);
				
				ImagePath = new WCHAR[MAX_PATH];
			}
			~FileHandleOutput() { }
		}FileHandleOutput, * PFileHandleOutput;

		typedef struct RpcMapperOutput {
			LPWSTR	BindingString;
			LPWSTR	Annotation;

			RpcMapperOutput(LPWSTR binStr, LPWSTR ann) {
				size_t binSz = wcslen(binStr) + 1;
				size_t annSz = wcslen(ann) + 1;

				BindingString = new wchar_t[binSz];
				Annotation = new wchar_t[annSz];

				wcscpy_s(BindingString, binSz, binStr);
				wcscpy_s(Annotation, annSz, ann);
			}
			~RpcMapperOutput() { }
		}RpcMapperOutput, *PRpcMapperOutput;

		std::vector<Utilities::MessageDumpOutput> Utilities::GetResourceMessageTable(LPTSTR libName);
		DWORD MapRpcEndpoints(std::vector<Utilities::RpcMapperOutput>& ppOutVec);
		LPWSTR GetFormatedWSError();
		LPWSTR GetFormatedWin32Error();
		LPWSTR GetFormatedError(DWORD errorCode);
		DWORD GetProcessFileHandle(std::vector<FileHandleOutput>& ppvecfho, PCWSTR fileName);
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