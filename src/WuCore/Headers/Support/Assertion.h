#pragma once

#include <stdlib.h>
#include <strsafe.h>

#include "../../pch.h"

#if defined(_DEBUG)
#define _WU_ASSERT(expr, msg) \
    __Wu_do_ass(_CRT_WIDE(#expr), expr, __FILEW__, __LINE__, msg)
#else
#define _WU_ASSERT(expr, msg) ;
#endif

constexpr size_t SIZE_T_DIGIT_COUNT = 20;

static void __Wu_do_ass(const wchar_t* exprStr, bool expr, const wchar_t* file, const int line, const wchar_t* message)
{
	wchar_t buffer[260]{ };
	GetModuleFileName(nullptr, buffer, 260);

	const auto format = L"Debug Assertion Failed!\n\nProgram: %ws\nFile: %ws\nLine: %d\n\nExpression: %ws\nMessage: %ws\n\n(Press Retry to debug the application)";
	const size_t buffLen = wcslen(buffer) + wcslen(file) + wcslen(exprStr) + wcslen(message) + SIZE_T_DIGIT_COUNT + 121 + 1;

	wchar_t* messageBuffer = new wchar_t[buffLen];
	const wchar_t* finalText = messageBuffer;

	if (FAILED(StringCchPrintf(messageBuffer, buffLen, format, buffer, file, line, exprStr, message)))
		finalText = L"Debug Assertion Failed!\n\n(Press Retry to debug the application)";

	if (!expr) {
		int res = MessageBox(nullptr, finalText, L"WindowsUtils Core", MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_DEFBUTTON1 | MB_SYSTEMMODAL);
		if (res == IDABORT) {
			exit(ERROR_ASSERTION_FAILURE);
		}

		else if (res == IDRETRY) {
			DebugBreak();
		}
	}
}