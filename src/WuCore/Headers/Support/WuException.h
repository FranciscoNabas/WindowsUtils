#pragma once
#pragma unmanaged

#include <CorError.h>

#if defined(_DEBUG)
#include <Shlwapi.h>
#endif

#include "WuString.h"

#define _WU_RAISE_NATIVE_EXCEPTION(code, id, category) \
	throw WindowsUtils::Core::WuNativeException(code, id, category, __FILEW__, __LINE__)

#define _WU_RAISE_NATIVE_NT_EXCEPTION(code, id, category) \
	throw WindowsUtils::Core::WuNativeException(code, id, category, __FILEW__, __LINE__, CoreErrorType::NtError)

#define _WU_RAISE_NATIVE_FDI_EXCEPTION(code, id, category) \
	throw WindowsUtils::Core::WuNativeException(code, id, category, __FILEW__, __LINE__, CoreErrorType::FdiError)

#define _WU_RAISE_NATIVE_FCI_EXCEPTION(code, id, category) \
	throw WindowsUtils::Core::WuNativeException(code, id, category, __FILEW__, __LINE__, CoreErrorType::FciError)

#define _WU_RAISE_NATIVE_EXCEPTION_WMESS(code, id, category, message) \
	throw WindowsUtils::Core::WuNativeException(code, id, category, message, __FILEW__, __LINE__)


// These exceptions will be translated to CLR exceptions.

#define _WU_RAISE_COR_EXCEPTION(code, id, category) \
	throw WindowsUtils::Core::WuException(code, id, category, true, __FILEW__, __LINE__)

#define _WU_RAISE_COR_EXCEPTION_WMESS(code, id, category, message) \
	throw WindowsUtils::Core::WuException(code, id, category, message, true, __FILEW__, __LINE__)


// Constructor facilitators.
#define _WU_NEW_NATIVE_EXCEPTION(code, id, category) \
	WindowsUtils::Core::WuNativeException(code, id, category, __FILEW__, __LINE__)

namespace WindowsUtils::Core
{
	/// <summary>
	/// Error type. Utilized to generate the error
	/// message based on the error code.
	/// </summary>
	enum class CoreErrorType
	{
		SystemError,
		NtError,
		FdiError,
		FciError,
	};

	/// <summary>
	/// Maps to System.Management.Automation.ErrorCategory.
	/// </summary>
	enum class WriteErrorCategory
	{
		NotSpecified,
		OpenError,
		CloseError,
		DeviceError,
		DeadlockDetected,
		InvalidArgument,
		InvalidData,
		InvalidOperation,
		InvalidResult,
		InvalidType,
		MetadataError,
		NotImplemented,
		NotInstalled,
		ObjectNotFound,
		OperationStopped,
		OperationTimeout,
		SyntaxError,
		ParserError,
		PermissionDenied,
		ResourceBusy,
		ResourceExists,
		ResourceUnavailable,
		ReadError,
		WriteError,
		FromStdErr,
		SecurityError,
		ProtocolError,
		ConnectionError,
		AuthenticationError,
		LimitsExceeded,
		QuotaExceeded,
		NotEnabled,
	};

	/// <summary>
	/// Generic exception class. Base to all WindowsUtils exceptions.
	/// </summary>
	class WuException : public std::exception
	{
	public:
		WuException(const WuException& other);
		WuException(const int errorCode, const WWuString& id, const WriteErrorCategory category, const bool resolveCor, const LPWSTR filePath, const int lineNumber);
		WuException(const int errorCode, const WWuString& id, const WriteErrorCategory category, const WWuString& message, const bool resolveCor, const LPWSTR filePath, const int lineNumber);
		~WuException();

		const int ErrorCode() const;
		const WWuString& Id() const;
		const bool ResolveCor() const;
		const WWuString& Message() const;
		const WWuString& CompactTrace() const;
		const WriteErrorCategory Category() const;

	protected:
		int p_errorCode;
		WWuString p_id;
		WWuString p_message;
		bool p_isResolveCorErr;
		WWuString p_compactTrace;
		WriteErrorCategory p_category;
	};

	/// <summary>
	/// A native exception.
	/// </summary>
	/// <remarks>
	/// This exception is usually thrown when a system API fails.
	/// </remarks>
	class WuNativeException : public WuException
	{
	public:
		WuNativeException(const WuNativeException& other);
		WuNativeException(const int errorCode, const WWuString& id, const WriteErrorCategory category, const WWuString& message, const LPWSTR filePath, const int lineNumber);
		WuNativeException(const int errorCode, const WWuString& id, const WriteErrorCategory category, const LPWSTR filePath, const int lineNumber, CoreErrorType type = CoreErrorType::SystemError);
		WuNativeException(const UINT errorCode, const WWuString& id, const WriteErrorCategory category, const WWuString& message, const LPWSTR filePath, const int lineNumber);
		WuNativeException(const UINT errorCode, const WWuString& id, const WriteErrorCategory category, const LPWSTR filePath, const int lineNumber, CoreErrorType type = CoreErrorType::SystemError);
		WuNativeException(const ULONG errorCode, const WWuString& id, const WriteErrorCategory category, const WWuString& message, const LPWSTR filePath, const int lineNumber);
		WuNativeException(const ULONG errorCode, const WWuString& id, const WriteErrorCategory category, const LPWSTR filePath, const int lineNumber, CoreErrorType type = CoreErrorType::SystemError);
		~WuNativeException();

		_NODISCARD static WWuString GetErrorMessage(const int errorCode, const CoreErrorType type = CoreErrorType::SystemError);
		_NODISCARD static WWuString GetErrorMessage(const UINT errorCode, const CoreErrorType type = CoreErrorType::SystemError);
		_NODISCARD static WWuString GetErrorMessage(const ULONG errorCode, const CoreErrorType type = CoreErrorType::SystemError);
		_NODISCARD static WWuString GetFdiErrorMessage(const ULONG errorCode);
		_NODISCARD static WWuString GetFciErrorMessage(const ULONG errorCode);

	private:
		bool m_isNt;

		void SetMessage(bool isNt);
		void SetFdiMessage();
		void SetFciMessage();
	};
}