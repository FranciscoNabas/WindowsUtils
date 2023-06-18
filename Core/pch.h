// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H
#define WIN32_LEAN_AND_MEAN

#define _WIN32_WINNT 0x0600

#define SharedVecPtr(T) std::shared_ptr<std::vector<T>>
#define MakeVecPtr(T) std::make_shared<std::vector<T>>()

// To avoid warning C4005. This definition is done via compiler command line.
#undef __CLR_VER

// add headers that you want to pre-compile here

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

#include <Windows.h>
#include <WtsApi32.h>
#include <winternl.h>
#include <strsafe.h>
#include <Shlwapi.h>
#include <MsiQuery.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <AclAPI.h>
#include <sddl.h>

#endif //PCH_H