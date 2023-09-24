#include "../../pch.h"

#include "../../Headers/Support/CoreUtils.h"
#include "../../Headers/Support/WuStdException.h"

#include <cmath>

namespace WindowsUtils::Core
{
	/*
	*	~ WindowsUtils TimeSpan
	*/

	WuTimeSpan::WuTimeSpan(__int64 ticks)
	{
		m_ticks = ticks;
		SetValues();
	}

	WuTimeSpan::WuTimeSpan(int hours, int minutes, int seconds)
	{
		m_ticks = TimeToTicks(hours, minutes, seconds);
		SetValues();
	}

	WuTimeSpan::WuTimeSpan(int days, int hours, int minutes, int seconds)
	{
		__int64 num = (static_cast<__int64>(days) * 3600L * 24 + static_cast<__int64>(hours) * 3600L + static_cast<__int64>(minutes) * 60L + seconds) * 1000;
		if (num > 922337203685477L || num < -922337203685477L) {
			throw "TimeSpan to long";
		}
		m_ticks = num * 10000;
		SetValues();
	}

	WuTimeSpan::WuTimeSpan(int days, int hours, int minutes, int seconds, int milliseconds)
	{
		__int64 num = (static_cast<__int64>(days) * 3600L * 24 + static_cast<__int64>(hours) * 3600L + static_cast<__int64>(minutes) * 60L + seconds) * 1000 + milliseconds;
		if (num > 922337203685477L || num < -922337203685477L) {
			throw "TimeSpan to long";
		}
		m_ticks = num * 10000;
		SetValues();
	}

	WuTimeSpan::~WuTimeSpan() { }

	const __int64 WuTimeSpan::Ticks() const { return m_ticks; }
	const WuTimeSpan WuTimeSpan::Zero() { return WuTimeSpan(0LL); }
	const WuTimeSpan WuTimeSpan::MaxValue() { return WuTimeSpan(9223372036854775807); }
	const WuTimeSpan WuTimeSpan::MinValue() { return WuTimeSpan(-static_cast<__int64>(9223372036854775808)); }

	WuTimeSpan WuTimeSpan::Add(const WuTimeSpan other) const
	{
		__int64 ticks = m_ticks + other.m_ticks;
		if (m_ticks >> 63 == other.m_ticks >> 63 && m_ticks >> 63 != ticks >> 63) {
			throw WuStdException(-1, L"Overflow exception. TimeSpan to long.", __FILEW__, __LINE__);
		}
		
		return WuTimeSpan(ticks);
	}

	const __int64 WuTimeSpan::TimeToTicks(int hour, int minute, int second)
	{
		__int64 num = static_cast<__int64>(hour) * 3600L + static_cast<__int64>(minute) * 60L + second;
		if (num > 922337203685L || num < -922337203685L) {
			throw "TimeSpan to long";
		}
		num *= -10000000;

		return num;
	}

	void WuTimeSpan::SetValues()
	{
		Days = std::lround(static_cast<double>(m_ticks) / 864000000000L);
		Hours = static_cast<int>(m_ticks / 36000000000L % 24);
		Minutes = static_cast<int>(m_ticks / 600000000 % 60);
		Seconds = static_cast<int>(m_ticks / 10000000 % 60);
		Milliseconds = static_cast<int>(m_ticks / 10000 % 1000);

		TotalDays = static_cast<double>(m_ticks) / 864000000000.0;
		TotalHours = static_cast<double>(m_ticks) / 36000000000.0;
		TotalMinutes = static_cast<double>(m_ticks) / 600000000.0;
		TotalSeconds = static_cast<double>(m_ticks) / 10000000.0;
		TotalMicroseconds = static_cast<double>(m_ticks) / 10.0;
		TotalNanoseconds = static_cast<double>(m_ticks) * 100.0;

		double num = static_cast<double>(m_ticks) / 10000.0;
		if (num > 922337203685477.0)
			TotalMilliseconds = 922337203685477.0;
		else if (num < -922337203685477.0)
			TotalMilliseconds = -922337203685477.0;
		else
			TotalMilliseconds = num;
	}

	/*
	*	~ WindowsUtils StopWatch
	*/

	WuStopWatch::WuStopWatch() { Reset(); }

	void WuStopWatch::Start()
	{
		if (!m_isRunning) {
			m_startTimeStamp = GetTimestamp();
			m_isRunning = true;
		}
	}

	WuStopWatch WuStopWatch::StartNew()
	{
		WuStopWatch spw;
		spw.Start();

		return spw;
	}

	void WuStopWatch::Stop()
	{
		if (m_isRunning) {
			__int64 timestamp = GetTimestamp();
			__int64 num = timestamp - m_startTimeStamp;
			m_elapsed += num;
			m_isRunning = false;
			if (m_elapsed < 0)
				m_elapsed = 0LL;
		}
	}

	void WuStopWatch::Reset()
	{
		m_elapsed = 0L;
		m_isRunning = false;
		m_startTimeStamp = 0LL;
	}

	void WuStopWatch::Restart()
	{
		m_elapsed = 0L;
		m_startTimeStamp = GetTimestamp();
		m_isRunning = true;
	}

	__int64 WuStopWatch::GetTimestamp()
	{
		return GetPerformanceCounter();
	}

	const bool WuStopWatch::IsRunning() const
	{
		return m_isRunning;
	}

	const __int64 WuStopWatch::ElapsedTicks() const
	{
		__int64 num = m_elapsed;
		if (m_isRunning) {
			__int64 timestamp = GetTimestamp();
			__int64 num2 = timestamp - m_startTimeStamp;
			num += num2;
		}

		return num;
	}

	const double WuStopWatch::ElapsedMilliseconds() const
	{
		__int64 totalTicks = ElapsedTicks();
		return static_cast<double>(totalTicks) / 10000.0;
	}

	const WuTimeSpan WuStopWatch::Elapsed() const
	{
		__int64 elapsedDateTimeTicks = static_cast<__int64>(static_cast<double>(ElapsedTicks() * m_tickFrequency));
		return WuTimeSpan(elapsedDateTimeTicks);
	}

	__int64 WuStopWatch::GetPerformanceFrequency()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		return frequency.QuadPart;
	}

	__int64 WuStopWatch::GetPerformanceCounter()
	{
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);

		return counter.QuadPart;
	}
}