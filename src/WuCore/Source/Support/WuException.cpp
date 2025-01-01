#include "../../pch.h"

#include "../../Headers/Support/WuException.h"

#include <fci.h>
#include <fdi.h>

namespace WindowsUtils::Core
{
#pragma region WuException

	WuException::WuException(const WuException& other)
		: p_errorCode(other.p_errorCode), p_id(other.p_id), p_category(other.p_category),
			p_message(other.p_message), p_compactTrace(other.p_compactTrace) { }

	WuException::WuException(const int errorCode, const WWuString& id, const WriteErrorCategory category, const bool resolveCor, const LPWSTR filePath, const int lineNumber)
		: p_errorCode(errorCode), p_id(id), p_category(category), p_isResolveCorErr(resolveCor)
	{
#if defined(_DEBUG)
		WWuString fileName(filePath);
		PathStripPath(fileName.Raw());
		p_compactTrace = WWuString::Format(L"(%ws:%d)", fileName.Raw(), lineNumber);
#endif
	}

	WuException::WuException(const int errorCode, const WWuString& id, const WriteErrorCategory category, const WWuString& message, const bool resolveCor, const LPWSTR filePath, const int lineNumber)
		: p_errorCode(errorCode), p_id(id), p_category(category), p_message(message), p_isResolveCorErr(resolveCor)
	{
#if defined(_DEBUG)
		WWuString fileName(filePath);
		PathStripPath(fileName.Raw());
		p_compactTrace = WWuString::Format(L"(%ws:%d)", fileName.Raw(), lineNumber);
#endif
	}

	WuException::~WuException() { }

	const WWuString& WuException::Id() const { return p_id; }
	const int WuException::ErrorCode() const { return p_errorCode; }
	const WWuString& WuException::Message() const { return p_message; }
	const bool WuException::ResolveCor() const { return p_isResolveCorErr; }
	const WWuString& WuException::CompactTrace() const { return p_compactTrace; }
	const WriteErrorCategory WuException::Category() const { return p_category; }

#pragma endregion

#pragma region WuNativeException

	WuNativeException::WuNativeException(const WuNativeException& other)
		: WuException{ other }, m_isNt{ other.m_isNt } { }

	WuNativeException::WuNativeException(const ULONG errorCode, const WWuString& id, const WriteErrorCategory category, const LPWSTR filePath, const int lineNumber, CoreErrorType type)
		: WuException{ static_cast<int>(errorCode), id, category, false, filePath, lineNumber }
	{
		m_isNt = false;
		switch (type) {
		case CoreErrorType::SystemError:
			SetMessage(false);
			break;
		case CoreErrorType::NtError:
			m_isNt = true;
			SetMessage(true);
			break;
		case CoreErrorType::FdiError:
			SetFdiMessage();
			break;
		case CoreErrorType::FciError:
			SetFciMessage();
			break;
		}
	}

	WuNativeException::WuNativeException(const int errorCode, const WWuString& id, const WriteErrorCategory category, const LPWSTR filePath, const int lineNumber, CoreErrorType type)
		: WuException{ errorCode, id, category, false, filePath, lineNumber }
	{
		m_isNt = false;
		switch (type) {
		case CoreErrorType::SystemError:
			SetMessage(false);
			break;
		case CoreErrorType::NtError:
			m_isNt = true;
			SetMessage(true);
			break;
		case CoreErrorType::FdiError:
			SetFdiMessage();
			break;
		case CoreErrorType::FciError:
			SetFciMessage();
			break;
		}
	}

	WuNativeException::WuNativeException(const UINT errorCode, const WWuString& id, const WriteErrorCategory category, const LPWSTR filePath, const int lineNumber, CoreErrorType type)
		: WuException{ static_cast<int>(errorCode), id, category, false, filePath, lineNumber }
	{
		m_isNt = false;
		switch (type) {
		case CoreErrorType::SystemError:
			SetMessage(false);
			break;
		case CoreErrorType::NtError:
			m_isNt = true;
			SetMessage(true);
			break;
		case CoreErrorType::FdiError:
			SetFdiMessage();
			break;
		case CoreErrorType::FciError:
			SetFciMessage();
			break;
		}
	}

	WuNativeException::WuNativeException(const UINT errorCode, const WWuString& id, const WriteErrorCategory category, const WWuString& message, const LPWSTR filePath, const int lineNumber)
		: WuException{ static_cast<int>(errorCode), id, category, message, false, filePath, lineNumber } { }

	WuNativeException::WuNativeException(const ULONG errorCode, const WWuString& id, const WriteErrorCategory category, const WWuString& message, const LPWSTR filePath, const int lineNumber)
		: WuException{ static_cast<int>(errorCode), id, category, message, false, filePath, lineNumber }, m_isNt{ false } { }

	WuNativeException::WuNativeException(const int errorCode, const WWuString& id, const WriteErrorCategory category, const WWuString& message, const LPWSTR filePath, const int lineNumber)
		: WuException{ errorCode, id, category, message, false, filePath, lineNumber }, m_isNt{ false } { }

	WuNativeException::~WuNativeException() { }

	_NODISCARD WWuString WuNativeException::GetErrorMessage(const int errorCode, const CoreErrorType type)
	{
		return GetErrorMessage(static_cast<ULONG>(errorCode), type);
	}

	_NODISCARD WWuString WuNativeException::GetErrorMessage(const UINT errorCode, const CoreErrorType type)
	{
		return GetErrorMessage(static_cast<ULONG>(errorCode), type);
	}

	_NODISCARD WWuString WuNativeException::GetErrorMessage(const ULONG errorCode, const CoreErrorType type)
	{
		if (type == CoreErrorType::FciError) {
			return GetFciErrorMessage(errorCode);
		}
		else if (type == CoreErrorType::FdiError) {
			return GetFdiErrorMessage(errorCode);
		}
		else {
			LPWSTR buffer{ };
			if (type == CoreErrorType::NtError) {
				HMODULE hModule = GetModuleHandle(L"ntdll.dll");
				if (!hModule) {
					return WWuString();
				}
				else {
					DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
					DWORD result = FormatMessage(flags, hModule, errorCode, NULL, (LPWSTR)&buffer, 0, NULL);
					if (result != 0) {
						const auto message = WWuString(buffer);
						LocalFree(buffer);

						return message;
					}
					else {
						return WWuString();
					}
				}
			}
			else {
				DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
				DWORD result = FormatMessage(flags, NULL, errorCode, NULL, (LPWSTR)&buffer, 0, NULL);
				if (result != 0) {
					const auto message = WWuString(buffer);
					LocalFree(buffer);

					return message;
				}
				else {
					return WWuString();
				}
			}
		}
	}

	_NODISCARD WWuString WuNativeException::GetFdiErrorMessage(const ULONG errorCode)
	{
		switch (errorCode) {
		case FDIERROR_CABINET_NOT_FOUND:
			return L"Cabinet not found.";

		case FDIERROR_NOT_A_CABINET:
			return L"File is not a cabinet.";

		case FDIERROR_UNKNOWN_CABINET_VERSION:
			return L"Unknown cabinet version.";

		case FDIERROR_CORRUPT_CABINET:
			return L"Corrupt cabinet.";

		case FDIERROR_ALLOC_FAIL:
			return L"Memory allocation failed.";

		case FDIERROR_BAD_COMPR_TYPE:
			return L"Unknown compression type.";

		case FDIERROR_MDI_FAIL:
			return L"Failure decompressing data.";

		case FDIERROR_TARGET_FILE:
			return L"Failure writing to target file.";

		case FDIERROR_RESERVE_MISMATCH:
			return L"Cabinets in set have different RESERVE sizes.";

		case FDIERROR_WRONG_CABINET:
			return L"Cabinet returned on fdintNEXT_CABINET is incorrect.";

		case FDIERROR_USER_ABORT:
			return L"Application aborted.";

		default:
			return GetErrorMessage(errorCode);
		}
	}

	_NODISCARD WWuString WuNativeException::GetFciErrorMessage(const ULONG errorCode)
	{
		switch (errorCode) {
		case FCIERR_OPEN_SRC:
			return L"Failure opening the file to be stored in the cabinet.";

		case FCIERR_READ_SRC:
			return L"Failure reading the file to be stored in the cabinet.";

		case FCIERR_ALLOC_FAIL:
			return L"Out of memory in FCI.";

		case FCIERR_TEMP_FILE:
			return L"Could not create a temporary file.";

		case FCIERR_BAD_COMPR_TYPE:
			return L"Unknown compression type.";

		case FCIERR_CAB_FILE:
			return L"Could not create the cabinet file.";

		case FCIERR_USER_ABORT:
			return L"FCI aborted.";

		case FCIERR_MCI_FAIL:
			return L"Failure compressing data.";

		case FCIERR_CAB_FORMAT_LIMIT:
			return L"Data-size or file-count exceeded CAB format limits.";

		default:
			return GetErrorMessage(errorCode);
		}
	}

	
	void WuNativeException::SetMessage(bool isNt)
	{
		p_message = GetErrorMessage(p_errorCode, isNt ? CoreErrorType::NtError : CoreErrorType::SystemError);
	}

	void WuNativeException::SetFciMessage()
	{
		p_message = GetFciErrorMessage(p_errorCode);
	}

	void WuNativeException::SetFdiMessage()
	{
		p_message = GetFdiErrorMessage(p_errorCode);
	}

#pragma endregion
}