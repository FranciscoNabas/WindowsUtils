#pragma once

#include "..\pch.h"

#include "String.h"

typedef enum _ErrorType
{
	SystemError,
	NtError,
	FdiError
} ErrorType;

class WuStdException : public std::exception
{
public:
	const int ErrorCode() const { return m_windowsError; }
	const WWuString& Message() const { return m_message; }
	const WWuString& CompactTrace() const { return m_compactTrace; }

	void Crier() const noexcept
	{
#if defined(_IS_POWERSHELL)

#else
		DWORD bytesWritten;
		HANDLE stdErr = GetStdHandle(STD_ERROR_HANDLE);
		WWuString finalMessage = WWuString::Format(L"[%d] - %ws%ws", m_windowsError, m_message.GetBuffer(), m_compactTrace.GetBuffer());
		WriteFile(stdErr, finalMessage.GetBuffer(), static_cast<DWORD>((finalMessage.Length() + 1) * 2), &bytesWritten, NULL);
#endif
	}

	WuStdException(int errorCode, LPCWSTR filePath, int lineNumber, ErrorType type = ErrorType::SystemError)
	{
		m_windowsError = errorCode;

		switch (type) {
			case SystemError:
				SetMessage(false);
				break;
			case NtError:
				SetMessage(true);
				break;
			case FdiError:
				SetFdiMessage();
				break;
		}

#if defined(_DEBUG)
		WWuString fileName(filePath);
		PathStripPath(fileName.GetBuffer());
		m_compactTrace = WWuString::Format(L" (%ws:%d)", fileName.GetBuffer(), lineNumber);
#else
		m_compactTrace = WWuString();
#endif
	}

	~WuStdException() { }

private:
	bool m_isNt;
	int m_windowsError;
	WWuString m_message;
	WWuString m_compactTrace;

	void SetMessage(bool isNt)
	{
		LPWSTR buffer = NULL;
		if (isNt) {
			HMODULE hModule = GetModuleHandle(L"ntdll.dll");
			if (hModule == NULL) {
				m_message = WWuString();
			}
			else {
				DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
				DWORD result = FormatMessage(flags, hModule, m_windowsError, NULL, (LPWSTR)&buffer, 0, NULL);
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
			DWORD result = FormatMessage(flags, NULL, m_windowsError, NULL, (LPWSTR)&buffer, 0, NULL);
			if (result != 0) {
				m_message = WWuString(buffer);
				LocalFree(buffer);
			}
			else {
				m_message = WWuString();
			}
		}
	}

	void SetFdiMessage()
	{
		switch (m_windowsError) {
			case FDIERROR_CABINET_NOT_FOUND:
				m_message = L"Cabinet not found";

			case FDIERROR_NOT_A_CABINET:
				m_message = L"File is not a cabinet";

			case FDIERROR_UNKNOWN_CABINET_VERSION:
				m_message = L"Unknown cabinet version";

			case FDIERROR_CORRUPT_CABINET:
				m_message = L"Corrupt cabinet";

			case FDIERROR_ALLOC_FAIL:
				m_message = L"Memory allocation failed";

			case FDIERROR_BAD_COMPR_TYPE:
				m_message = L"Unknown compression type";

			case FDIERROR_MDI_FAIL:
				m_message = L"Failure decompressing data";

			case FDIERROR_TARGET_FILE:
				m_message = L"Failure writing to target file";

			case FDIERROR_RESERVE_MISMATCH:
				m_message = L"Cabinets in set have different RESERVE sizes";

			case FDIERROR_WRONG_CABINET:
				m_message = L"Cabinet returned on fdintNEXT_CABINET is incorrect";

			case FDIERROR_USER_ABORT:
				m_message = L"Application aborted";

			default:
				SetMessage(false);
		}
	}
};

class WuResult
{
public:
	long Result;
	WWuString Message;
	WWuString CompactTrace;

	WuResult()
		: Result(ERROR_SUCCESS)
	{ }

	WuResult(long errorCode, LPWSTR fileName, DWORD lineNumber, bool isNt = false)
		: Result(errorCode)
	{
		Message = GetErrorMessage(errorCode, isNt);
		CompactTrace = GetCompactTrace(fileName, lineNumber);
	}

	WuResult(long errorCode, const WWuString& message, LPWSTR fileName, DWORD lineNumber)
		: Result(errorCode), Message(message)
	{
		CompactTrace = GetCompactTrace(fileName, lineNumber);
	}

	~WuResult() { }

	_NODISCARD static WWuString GetErrorMessage(long errorCode, bool isNt)
	{
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

	_NODISCARD static WWuString GetCompactTrace(LPWSTR fileName, DWORD lineNumber)
	{
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

	static WuResult GetResultFromFdiError(const FDIERROR& err, LPWSTR fileName, DWORD lineNumber)
	{
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

////////////////////////////////////////////////////
//
//		~ Thou shalt not reinvent the wheel ~		
//
//	- System.TimeSpan
//	- System.Diagnostics.StopWatch
//
//	Everything... Even the variable names.
//
////////////////////////////////////////////////////

struct WuTimeSpan
{
	const __int64 TicksPerDay = 864000000000LL;
	const __int64 TicksPerHour = 36000000000LL;
	const __int64 TicksPerMinute = 600000000LL;
	const __int64 TicksPerSecond = 10000000LL;
	const __int64 TicksPerMillisecond = 10000LL;

	int Days;
	int Hours;
	int Minutes;
	int Seconds;
	int Milliseconds;

	double TotalDays;
	double TotalHours;
	double TotalMinutes;
	double TotalSeconds;
	double TotalMilliseconds;
	double TotalMicroseconds;
	double TotalNanoseconds;

	WuTimeSpan(__int64 ticks)
	{
		_ticks = ticks;
		SetValues();
	}
	WuTimeSpan(int hours, int minutes, int seconds)
	{
		__int64 ticks = TimeToTicks(hours, minutes, seconds);
		SetValues();
	}
	WuTimeSpan(int days, int hours, int minutes, int seconds)
	{
		__int64 num = (static_cast<__int64>(days) * 3600L * 24 + static_cast<__int64>(hours) * 3600L + static_cast<__int64>(minutes) * 60L + seconds) * 1000;
		if (num > 922337203685477L || num < -922337203685477L) {
			throw "TimeSpan to long";
		}
		_ticks = num * 10000;
		SetValues();
	}
	WuTimeSpan(int days, int hours, int minutes, int seconds, int milliseconds)
	{
		__int64 num = (static_cast<__int64>(days) * 3600L * 24 + static_cast<__int64>(hours) * 3600L + static_cast<__int64>(minutes) * 60L + seconds) * 1000 + milliseconds;
		if (num > 922337203685477L || num < -922337203685477L) {
			throw "TimeSpan to long";
		}
		_ticks = num * 10000;
		SetValues();
	}

	~WuTimeSpan() { }

	const __int64 Ticks() { return _ticks; }
	const WuTimeSpan Zero() { return WuTimeSpan(0LL); }
	const WuTimeSpan MaxValue() { return WuTimeSpan(9223372036854775807); }
	const WuTimeSpan MinValue() { return WuTimeSpan(-static_cast<__int64>(9223372036854775808)); }

	WuTimeSpan Add(const WuTimeSpan other)
	{
		__int64 ticks = other._ticks;
		if (_ticks > ticks) {
			return 1;
		}
		if (_ticks < ticks) {
			return -1;
		}
		return 0;
	}

private:
	__int64 _ticks;

	inline static const __int64 TimeToTicks(int hour, int minute, int second)
	{
		__int64 num = static_cast<__int64>(hour) * 3600L + static_cast<__int64>(minute) * 60L + second;
		if (num > 922337203685L || num < -922337203685L) {
			throw "TimeSpan to long";
		}
		num *= -10000000;

		return num;
	}

	inline void SetValues()
	{
		Days = std::lround(static_cast<double>(_ticks) / 864000000000L);
		Hours = static_cast<int>(_ticks / 36000000000L % 24);
		Minutes = static_cast<int>(_ticks / 600000000 % 60);
		Seconds = static_cast<int>(_ticks / 10000000 % 60);
		Milliseconds = static_cast<int>(_ticks / 10000 % 1000);

		TotalDays = static_cast<double>(_ticks) / 864000000000.0;
		TotalHours = static_cast<double>(_ticks) / 36000000000.0;
		TotalMinutes = static_cast<double>(_ticks) / 600000000.0;
		TotalSeconds = static_cast<double>(_ticks) / 10000000.0;
		TotalMicroseconds = static_cast<double>(_ticks) / 10.0;
		TotalNanoseconds = static_cast<double>(_ticks) * 100.0;

		double num = static_cast<double>(_ticks) / 10000.0;
		if (num > 922337203685477.0)
			TotalMilliseconds = 922337203685477.0;
		else if (num < -922337203685477.0)
			TotalMilliseconds = -922337203685477.0;
		else
			TotalMilliseconds = num;
	}
};

class WuStopWatch
{
public:
	WuStopWatch()
	{
		// From tcping.exe.
		// apparently... QueryPerformanceCounter isn't thread safe unless we do this
		SetThreadAffinityMask(GetCurrentThread(), 1);
		Reset();
	}

	void Start()
	{
		if (!_isRunning) {
			_startTimeStamp = GetTimestamp();
			_isRunning = true;
		}
	}

	static WuStopWatch StartNew()
	{
		WuStopWatch spw;
		spw.Start();

		return spw;
	}

	void Stop()
	{
		if (_isRunning) {
			__int64 timestamp = GetTimestamp();
			__int64 num = timestamp - _startTimeStamp;
			_elapsed += num;
			_isRunning = false;
			if (_elapsed < 0)
				_elapsed = 0LL;
		}
	}

	void Reset()
	{
		_elapsed = 0L;
		_isRunning = false;
		_startTimeStamp = 0LL;
	}

	void Restart()
	{
		_elapsed = 0L;
		_startTimeStamp = GetTimestamp();
		_isRunning = true;
	}

	static __int64 GetTimestamp()
	{
		return GetPerformanceCounter();
	}

	const bool IsRunning() const
	{
		return _isRunning;
	}

	const __int64 ElapsedTicks() const
	{
		__int64 num = _elapsed;
		if (_isRunning) {
			__int64 timestamp = GetTimestamp();
			__int64 num2 = timestamp - _startTimeStamp;
			num += num2;
		}

		return num;
	}

	const double ElapsedMilliseconds() const
	{
		__int64 totalTicks = ElapsedTicks();
		return static_cast<double>(totalTicks) / 10000.0;
	}

	const WuTimeSpan Elapsed() const
	{
		__int64 elapsedDateTimeTicks = static_cast<__int64>(static_cast<double>(ElapsedTicks() * _tickFrequency));
		return WuTimeSpan(elapsedDateTimeTicks);
	}

private:
	__int64 _elapsed;
	__int64 _startTimeStamp;
	bool _isRunning;

	__int64 _frequency = GetPerformanceFrequency();
	double _tickFrequency = 10000000.0 / static_cast<double>(_frequency);

	static __int64 GetPerformanceFrequency()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		return frequency.QuadPart;
	}

	static __int64 GetPerformanceCounter()
	{
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);

		return counter.QuadPart;
	}
};