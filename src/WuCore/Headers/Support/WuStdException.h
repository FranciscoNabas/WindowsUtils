#pragma once
#pragma unmanaged

#include "Notification.h"

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
		FciError
	};

	class WuStdException : public std::exception
	{
	public:
		const int ErrorCode() const;
		const WWuString& Message() const;
		const WWuString& CompactTrace() const;

		/// <summary>
		/// This function writes a 'ErrorRecord' to PowerShell
		/// </summary>
		void Cry(const WWuString& errorId, WriteErrorCategory category, const WWuString& targetObject, const WuNativeContext* context) const noexcept;
		void Cry(const WWuString& errorId, const WWuString& message, WriteErrorCategory category, const WWuString& targetObject, const WuNativeContext* context) const noexcept;

		WuStdException(const WuStdException& other);
		WuStdException(int errorCode, LPCWSTR filePath, int lineNumber, CoreErrorType type = CoreErrorType::SystemError);
		WuStdException(int errorCode, LPCWSTR message, LPCWSTR filePath, int lineNumber);
		~WuStdException();

	private:
		bool m_isNt;
		int m_errorCode;
		WWuString m_message;
		WWuString m_compactTrace;

		void SetMessage(bool isNt);
		void SetFdiMessage();
		void SetFciMessage();
	};
}