// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.


// System environment definitions.
#ifndef PCH_H
#define PCH_H
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600

// Utility definitions.
#define DWERRORCHECKV(result) if (ERROR_SUCCESS != result) { return WuResult(result, __FILEW__, __LINE__); }
#define DWERRORCHECKF(result) if (ERROR_SUCCESS != result) { return WuResult(GetLastError(), __FILEW__, __LINE__); }

#define FILE_ATTRIBUTE_UTC_TIME 0x100

// To avoid warning C4005. This definition is done via compiler command line.
#undef __CLR_VER

// Encoding definition.
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

// Maximum username character count. 'lmcons.h'.
#ifndef UNLEN
#define UNLEN 256
#endif

// Globals.
bool _CtrlCHit_ = false;

// Library imports.
#pragma comment (lib, "Cabinet.lib")
#pragma comment (lib, "Ole32.lib")
#pragma comment (lib, "Msi.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "WtsApi32.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "Shlwapi.lib")
#pragma comment (lib, "Version.lib")
#pragma comment (lib, "PathCch.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

// Include list.
#include <Windows.h>
#include <WtsApi32.h>
#include <winternl.h>
#include <strsafe.h>
#include <Shlwapi.h>
#include <MsiQuery.h>
#include <xstring>
#include <vector>
#include <map>
#include <memory>
#include <AclAPI.h>
#include <sddl.h>
#include <fdi.h>
#include <fcntl.h>
#include <io.h>
#include <cmath>
#include <psapi.h>
#include <new>
#include <system_error>
#include <PathCch.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#endif //PCH_H