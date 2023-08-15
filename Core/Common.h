#pragma once

#include "String.h"

class WuOsException {
public:
	long ErrorCode;
	WWuString Message;

	WuOsException()
		: ErrorCode(ERROR_SUCCESS) { }

	WuOsException(long errorCode, bool isNt = false)
		: ErrorCode(errorCode) {
		Message = GetErrorMessage(errorCode, isNt);
	}

	WuOsException(long errorCode, WWuString message)
		: ErrorCode(errorCode), Message(message) { }

	_NODISCARD static WWuString GetErrorMessage(long errorCode, bool isNt) {
		if (isNt) {
			HMODULE hModule = GetModuleHandle(L"ntdll.dll");
			if (hModule == NULL)
				return WWuString();

			LPWSTR buffer = NULL;
			DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS;
			if (FormatMessage(flags, hModule, errorCode, NULL, (LPWSTR)&buffer, 0, NULL) == 0)
				return WWuString();

			WWuString output(buffer);
			LocalFree(buffer);

			return output;
		}
		else {
			LPWSTR buffer = NULL;
			DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
			if (FormatMessage(flags, NULL, errorCode, NULL, (LPWSTR)&buffer, 0, NULL) == 0)
				return WWuString();

			WWuString output(buffer);
			LocalFree(buffer);

			return output;
		}
	}
};