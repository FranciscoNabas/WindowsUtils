#pragma once

#include "String.h"

class WuResult {
public:
	long Result;
	WWuString Message;
	WWuString CompactTrace;

	WuResult()
		: Result(ERROR_SUCCESS) { }

	WuResult(long errorCode, LPWSTR fileName, DWORD lineNumber, bool isNt = false)
		: Result(errorCode) {
		Message = GetErrorMessage(errorCode, isNt);
		CompactTrace = GetCompactTrace(fileName, lineNumber);
	}

	WuResult(long errorCode, const WWuString& message, LPWSTR fileName, DWORD lineNumber)
		: Result(errorCode), Message(message) {
		CompactTrace = GetCompactTrace(fileName, lineNumber);
	}

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

	_NODISCARD static WWuString GetCompactTrace(LPWSTR fileName, DWORD lineNumber) {
		WWuString wrappedName(fileName);
		auto fileNameSplit = wrappedName.Split('\\');
		WWuString relPath;
		bool isProject = false;
		for (WWuString fsItem : fileNameSplit) {
			if (fsItem.EndsWith(L"WindowsUtils")) {
				isProject = true;
				continue;
			}

			if (isProject) {
				relPath = WWuString::Format(L"%ws\\%ws", relPath.GetBuffer(), fsItem.GetBuffer());
			}
		}

		return WWuString::Format(L"%ws:%d", relPath.GetBuffer(), lineNumber);
	}

	static WuResult GetResultFromFdiError(const FDIERROR& err, LPWSTR fileName, DWORD lineNumber) {
		switch (err) {
			case FDIERROR_CABINET_NOT_FOUND:
				return WuResult(FDIERROR_CABINET_NOT_FOUND, L"Cabinet not found", fileName, lineNumber);

			case FDIERROR_NOT_A_CABINET:
				return WuResult(FDIERROR_NOT_A_CABINET, L"File is not a cabinet", fileName, lineNumber);

			case FDIERROR_UNKNOWN_CABINET_VERSION:
				return WuResult(FDIERROR_UNKNOWN_CABINET_VERSION, L"Unknown cabinet version", fileName, lineNumber);

			case FDIERROR_CORRUPT_CABINET:
				return WuResult(FDIERROR_CORRUPT_CABINET, L"Corrupt cabinet", fileName, lineNumber);

			case FDIERROR_ALLOC_FAIL:
				return WuResult(FDIERROR_ALLOC_FAIL, L"Memory allocation failed", fileName, lineNumber);

			case FDIERROR_BAD_COMPR_TYPE:
				return WuResult(FDIERROR_BAD_COMPR_TYPE, L"Unknown compression type", fileName, lineNumber);

			case FDIERROR_MDI_FAIL:
				return WuResult(FDIERROR_MDI_FAIL, L"Failure decompressing data", fileName, lineNumber);

			case FDIERROR_TARGET_FILE:
				return WuResult(FDIERROR_TARGET_FILE, L"Failure writing to target file", fileName, lineNumber);

			case FDIERROR_RESERVE_MISMATCH:
				return WuResult(FDIERROR_RESERVE_MISMATCH, L"Cabinets in set have different RESERVE sizes", fileName, lineNumber);

			case FDIERROR_WRONG_CABINET:
				return WuResult(FDIERROR_WRONG_CABINET, L"Cabinet returned on fdintNEXT_CABINET is incorrect", fileName, lineNumber);

			case FDIERROR_USER_ABORT:
				return WuResult(FDIERROR_USER_ABORT, L"Application aborted", fileName, lineNumber);

			default:
				return WuResult(err, L"Unknown error", fileName, lineNumber);
		}
	}
};