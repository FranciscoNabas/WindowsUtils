#include "../../pch.h"

#include "../../Headers/Support/WuStdException.h"

#include <fci.h>
#include <fdi.h>

#if defined(_DEBUG)
#include <Shlwapi.h>
#endif

namespace WindowsUtils::Core
{
	WuStdException::WuStdException(const WuStdException& other)
	{
		m_errorCode = other.m_errorCode;
		m_message = other.m_message;
		m_compactTrace = other.m_compactTrace;
		m_isNt = other.m_isNt;
	}

	WuStdException::WuStdException(int errorCode, LPCWSTR filePath, int lineNumber, CoreErrorType type)
	{
		m_errorCode = errorCode;
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

#if defined(_DEBUG)
		WWuString fileName(filePath);
		PathStripPath(fileName.GetBuffer());
		m_compactTrace = WWuString::Format(L"(%ws:%d)", fileName.GetBuffer(), lineNumber);
#endif
	}

	WuStdException::WuStdException(int errorCode, const WWuString& message, LPCWSTR filePath, int lineNumber)
	{
		m_message = message;
		m_isNt = false;
		m_errorCode = errorCode;

#if defined(_DEBUG)
		WWuString fileName(filePath);
		PathStripPath(fileName.GetBuffer());
		m_compactTrace = WWuString::Format(L"(%ws:%d)", fileName.GetBuffer(), lineNumber);
#endif
	}

	WuStdException::~WuStdException() { }

	const int WuStdException::ErrorCode() const { return m_errorCode; }
	const WWuString& WuStdException::Message() const { return m_message; }
	const WWuString& WuStdException::CompactTrace() const { return m_compactTrace; }

	void WuStdException::SetMessage(bool isNt)
	{
		LPWSTR buffer = NULL;
		if (isNt) {
			HMODULE hModule = GetModuleHandle(L"ntdll.dll");
			if (hModule == NULL) {
				m_message = WWuString();
			}
			else {
				DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
				DWORD result = FormatMessage(flags, hModule, m_errorCode, NULL, (LPWSTR)&buffer, 0, NULL);
				if (result != 0) {
					m_message = WWuString(buffer);
					LocalFree(buffer);
				}
				else {
					m_message = WWuString();
				}
			}
		}
		else {
			DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
			DWORD result = FormatMessage(flags, NULL, m_errorCode, NULL, (LPWSTR)&buffer, 0, NULL);
			if (result != 0) {
				m_message = WWuString(buffer);
				LocalFree(buffer);
			}
			else {
				m_message = WWuString();
			}
		}
	}

	void WuStdException::SetFciMessage()
	{
		switch (m_errorCode) {
			case FCIERR_OPEN_SRC:
				m_message = L"Failure opening the file to be stored in the cabinet.";
				break;

			case FCIERR_READ_SRC:
				m_message = L"Failure reading the file to be stored in the cabinet.";
				break;

			case FCIERR_ALLOC_FAIL:
				m_message = L"Out of memory in FCI.";
				break;

			case FCIERR_TEMP_FILE:
				m_message = L"Could not create a temporary file.";
				break;

			case FCIERR_BAD_COMPR_TYPE:
				m_message = L"Unknown compression type.";
				break;

			case FCIERR_CAB_FILE:
				m_message = L"Could not create the cabinet file.";
				break;

			case FCIERR_USER_ABORT:
				m_message = L"FCI aborted.";
				break;

			case FCIERR_MCI_FAIL:
				m_message = L"Failure compressing data.";
				break;

			case FCIERR_CAB_FORMAT_LIMIT:
				m_message = L"Data-size or file-count exceeded CAB format limits.";
				break;

			default:
				SetMessage(false);
				break;
		}
	}

	void WuStdException::SetFdiMessage()
	{
		switch (m_errorCode) {
			case FDIERROR_CABINET_NOT_FOUND:
				m_message = L"Cabinet not found.";
				break;

			case FDIERROR_NOT_A_CABINET:
				m_message = L"File is not a cabinet.";
				break;

			case FDIERROR_UNKNOWN_CABINET_VERSION:
				m_message = L"Unknown cabinet version.";
				break;

			case FDIERROR_CORRUPT_CABINET:
				m_message = L"Corrupt cabinet.";
				break;

			case FDIERROR_ALLOC_FAIL:
				m_message = L"Memory allocation failed.";
				break;

			case FDIERROR_BAD_COMPR_TYPE:
				m_message = L"Unknown compression type.";
				break;

			case FDIERROR_MDI_FAIL:
				m_message = L"Failure decompressing data.";
				break;

			case FDIERROR_TARGET_FILE:
				m_message = L"Failure writing to target file.";
				break;

			case FDIERROR_RESERVE_MISMATCH:
				m_message = L"Cabinets in set have different RESERVE sizes.";
				break;

			case FDIERROR_WRONG_CABINET:
				m_message = L"Cabinet returned on fdintNEXT_CABINET is incorrect.";
				break;

			case FDIERROR_USER_ABORT:
				m_message = L"Application aborted.";
				break;

			default:
				SetMessage(false);
				break;
		}
	}

	void WuStdException::Cry(const WWuString& errorId, WriteErrorCategory category, const WWuString& targetObject, const WuNativeContext* context) const noexcept
	{
		MAPPED_ERROR_DATA errorReport(
			m_errorCode,
			m_message,
			m_compactTrace,
			errorId,
			category,
			targetObject
		);

		context->NativeWriteError(&errorReport);
	}

	void WuStdException::Cry(const WWuString& errorId, const WWuString& message, WriteErrorCategory category, const WWuString& targetObject, const WuNativeContext* context) const noexcept
	{
		MAPPED_ERROR_DATA errorReport(
			m_errorCode,
			message,
			m_compactTrace,
			errorId,
			category,
			targetObject
		);

		context->NativeWriteError(&errorReport);
	}
}