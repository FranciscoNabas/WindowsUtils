#pragma once
#pragma unmanaged

#include "WuException.h"

namespace WindowsUtils::Core
{
	////////////////////////////////////////////////////
	//
	//		~ Thou shalt not reinvent the wheel ~		
	//
	//	- System.TimeSpan
	//	- System.Diagnostics.StopWatch
	//
	////////////////////////////////////////////////////

	struct WuTimeSpan
	{
		static constexpr __int64 TicksPerDay          = 864000000000LL;
		static constexpr __int64 TicksPerHour         = 36000000000LL;
		static constexpr __int64 TicksPerMinute       = 600000000LL;
		static constexpr __int64 TicksPerSecond       = 10000000LL;
		static constexpr __int64 TicksPerMillisecond  = 10000LL;

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

		WuTimeSpan(__int64 ticks);
		WuTimeSpan(int hours, int minutes, int seconds);
		WuTimeSpan(int days, int hours, int minutes, int seconds);
		WuTimeSpan(int days, int hours, int minutes, int seconds, int milliseconds);
		~WuTimeSpan();

		const __int64 Ticks() const;
		const WuTimeSpan Zero();
		const WuTimeSpan MaxValue();
		const WuTimeSpan MinValue();

		WuTimeSpan Add(const WuTimeSpan other) const;

	private:
		__int64 m_ticks;

		static const __int64 TimeToTicks(int hour, int minute, int second);
		void SetValues();
	};

	class WuStopWatch
	{
	public:
		WuStopWatch();

		static WuStopWatch StartNew();

		void Start();
		void Stop();
		void Reset();
		void Restart();
		static __int64 GetTimestamp();
		const bool IsRunning() const;
		const __int64 ElapsedTicks() const;
		const double ElapsedMilliseconds() const;
		const WuTimeSpan Elapsed() const;

	private:
		__int64 m_elapsed;
		__int64 m_startTimeStamp;
		bool m_isRunning;

		__int64 m_frequency = GetPerformanceFrequency();
		double m_tickFrequency = 10000000.0 / static_cast<double>(m_frequency);

		static __int64 GetPerformanceFrequency();
		static __int64 GetPerformanceCounter();
	};
}