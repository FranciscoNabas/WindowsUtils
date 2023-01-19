#pragma once
#pragma unmanaged

#include "pch.h"
#include "Utilities.h"

#include <Objbase.h>
#include <taskschd.h>
#include <comdef.h>

#pragma comment(lib, "taskschd.lib")

#define RETURNIFFAIL(hr) if (FAILED(hr)) { return hr; }
#define SAFEISHRELEASE(i) if (NULL != i) { i->Release(); }

namespace WindowsUtils::Core
{
	extern "C" public class TaskScheduler
	{
	public:
		/*
		* Task trigger types
		*/
		typedef enum _TASK_TRIGGER_TYPE2
		{
			TASK_TRIGGER_TIME = 1,
			TASK_TRIGGER_BOOT = 8,
		} TASK_TRIGGER_TYPE2;

		

		typedef struct _WU_TASK_TRIGGER_REPETITION_PATTERN
		{
			LPWSTR	Duration;
			LPWSTR	Interval;
			BOOL	StopAtDuration;

			_WU_TASK_TRIGGER_REPETITION_PATTERN();
			_WU_TASK_TRIGGER_REPETITION_PATTERN(LPWSTR&& duration, LPWSTR&& interval, BOOL&& stpatdur);
			~_WU_TASK_TRIGGER_REPETITION_PATTERN();

		}WU_TASK_TRIGGER_REPETITION_PATTERN, * PWU_TASK_TRIGGER_REPETITION_PATTERN;

		typedef struct _WU_TASK_TRIGGER
		{
			BOOL Enabled;
			LPWSTR EndBoundary;
			LPWSTR ExecutionTimeLimit;
			LPWSTR Id;
			LPWSTR StartBoundary;
			TASK_TRIGGER_TYPE2 Type;
			PWU_TASK_TRIGGER_REPETITION_PATTERN Repetition;

			_WU_TASK_TRIGGER(LPCWSTR&& startbound, TASK_TRIGGER_TYPE2 type);
			~_WU_TASK_TRIGGER();

		}WU_TASK_TRIGGER, * PWU_TASK_TRIGGER;

		typedef struct _WU_TASK_TRIGGER_TIME : public WU_TASK_TRIGGER
		{
			LPWSTR	RandomDelay;
			_WU_TASK_TRIGGER_TIME();
			~_WU_TASK_TRIGGER_TIME();

		}WU_TASK_TRIGGER_TIME, * PWU_TASK_TRIGGER_TIME;

		typedef struct _WU_TASK_TRIGGER_BOOT : public WU_TASK_TRIGGER
		{
			LPWSTR	RandomDelay;

			_WU_TASK_TRIGGER_BOOT();
			~_WU_TASK_TRIGGER_BOOT();

		}WU_TASK_TRIGGER_BOOT, * PWU_TASK_TRIGGER_BOOT;

		/*
		* Other trigger types to be used as needed in the future.
		*/

		/*typedef struct _WU_TASK_TRIGGER_DAILY : public WU_TASK_TRIGGER
		{
			SHORT	DaysInterval;
			LPWSTR	RandomDelay;

			_WU_TASK_TRIGGER_DAILY(SHORT daysint, LPWSTR randelay);
			~_WU_TASK_TRIGGER_DAILY();

		}WU_TASK_TRIGGER_DAILY, *PWU_TASK_TRIGGER_DAILY;

		typedef struct _WU_TASK_TRIGGER_WEEKLY : public WU_TASK_TRIGGER
		{
			SHORT	DaysInterval;
			LPWSTR	RandomDelay;
			SHORT	WeeksInterval;

			_WU_TASK_TRIGGER_WEEKLY(SHORT daysint, LPWSTR, SHORT weeksint);
			~_WU_TASK_TRIGGER_WEEKLY();

		}WU_TASK_TRIGGER_WEEKLY, *PWU_TASK_TRIGGER_WEEKLY;

		typedef struct _WU_TASK_TRIGGER_MONTHLY : public WU_TASK_TRIGGER
		{
			LONG	DaysOfMonth;
			SHORT	MonthsOfYear;
			LPWSTR	RandomDelay;
			BOOL	RunOnLastDayOfMonth;

			_WU_TASK_TRIGGER_MONTHLY(LONG daysmonth, SHORT monthsyear, LPWSTR randelay, BOOL runonlastdayofmonth);
			~_WU_TASK_TRIGGER_MONTHLY();

		}WU_TASK_TRIGGER_MONTHLY, *PWU_TASK_TRIGGER_MONTHLY;

		typedef struct _WU_TASK_TRIGGER_REGISTRATION : public WU_TASK_TRIGGER
		{
			LPWSTR	RandomDelay;

			_WU_TASK_TRIGGER_REGISTRATION(LPWSTR randelay);
			~_WU_TASK_TRIGGER_REGISTRATION();

		}WU_TASK_TRIGGER_REGISTRATION, *PWU_TASK_TRIGGER_REGISTRATION;

		typedef struct _WU_TASK_TRIGGER_LOGON : public WU_TASK_TRIGGER
		{
			LPWSTR	RandomDelay;
			LPWSTR	UserId;

			_WU_TASK_TRIGGER_LOGON(LPWSTR randelay, LPWSTR userid);
			~_WU_TASK_TRIGGER_LOGON();

		}WU_TASK_TRIGGER_LOGON, *PWU_TASK_TRIGGER_LOGON;

		typedef struct _WU_TASK_TRIGGER_SESSION_STATE_CHANGE : public WU_TASK_TRIGGER
		{
			LPWSTR	RandomDelay;
			LPWSTR	UserId;
			TASK_SESSION_STATE_CHANGE_TYPE StateChange;

			_WU_TASK_TRIGGER_SESSION_STATE_CHANGE(LPWSTR randelay, LPWSTR userid, TASK_SESSION_STATE_CHANGE_TYPE statechange);
			~_WU_TASK_TRIGGER_SESSION_STATE_CHANGE();

		}WU_TASK_TRIGGER_SESSION_STATE_CHANGE, *PWU_TASK_TRIGGER_SESSION_STATE_CHANGE;*/

		/*
		* Task action types
		*/
		typedef struct _WU_TASK_ACTION
		{
			LPWSTR	Id;
			TASK_ACTION_TYPE Type;

			_WU_TASK_ACTION(TASK_ACTION_TYPE type);
			~_WU_TASK_ACTION();

		}WU_TASK_ACTION, * PWU_TASK_ACTION;

		typedef struct _WU_TASK_ACTION_EXEC : public WU_TASK_ACTION
		{
			LPWSTR	Path;
			LPWSTR	Arguments;
			LPWSTR	WorkingDirectory;

			_WU_TASK_ACTION_EXEC(LPWSTR&& path, LPWSTR&& args, LPWSTR&& workingdir);
			~_WU_TASK_ACTION_EXEC();

		}WU_TASK_ACTION_EXEC, * PWU_TASK_ACTION_EXEC;

		/*
		* Task properties, settings and definition
		*/
		typedef struct _WU_TASK_SETTINGS
		{
			BOOL	AllowDemandStart;
			BOOL	AllowHardTerminate;
			BOOL	DisallowStartIfOnBatteries;
			BOOL	StopIfGoingOnBatteries;
			BOOL	Enabled;
			BOOL	Hidden;
			BOOL	RunOnlyIfIdle;
			BOOL	RunOnlyIfNetworkAvailable;
			BOOL	StartWhenAvailable;
			BOOL	WakeToRun;
			INT		Priority;
			INT		RestartCount;
			LPWSTR	RestartInterval;
			LPWSTR	DeleteExpiredTaskAfter;
			LPWSTR	ExecutionTimeLimit;
			TASK_COMPATIBILITY	Compatibility;
			TASK_INSTANCES_POLICY MultipleInstances;

			_WU_TASK_SETTINGS();
			~_WU_TASK_SETTINGS();

		}WU_TASK_SETTINGS, * PWU_TASK_SETTINGS;

		typedef struct _WU_TASK_PRINCIPAL
		{
			LPWSTR	DisplayName;
			LPWSTR	UserId;
			LPWSTR	GroupId;
			LPWSTR	Id;
			TASK_LOGON_TYPE LogonType;
			TASK_RUNLEVEL_TYPE RunLevel;

			_WU_TASK_PRINCIPAL();
			~_WU_TASK_PRINCIPAL();

		}WU_TASK_PRINCIPAL, * PWU_TASK_PRINCIPAL;

	public:
		/*========================================
		==		  Object identification			==
		==========================================*/

		typedef struct _WU_TASK_DEFINITION
		{
			PWU_TASK_PRINCIPAL				Principal;
			PWU_TASK_SETTINGS				Settings;
			std::vector<PWU_TASK_TRIGGER>	Triggers;
			std::vector<PWU_TASK_ACTION>	Actions;

			_WU_TASK_DEFINITION();
			~_WU_TASK_DEFINITION();

			PWU_TASK_TRIGGER CreateTrigger(TASK_TRIGGER_TYPE2 type);
			PWU_TASK_ACTION CreateAction();

		}WU_TASK_DEFINITION, *PWU_TASK_DEFINITION;

		/*=========================================
		==		  Function identification		 ==
		===========================================*/


		::HRESULT CopyTaskSettings(ITaskSettings*& isettings, TaskScheduler::PWU_TASK_SETTINGS& wusettings);
		::HRESULT CopyTaskPrincipal(IPrincipal*& iprincipal, TaskScheduler::PWU_TASK_PRINCIPAL wuprincipal);
		::HRESULT CopyTaskTriggerProperties(void** ptrigger, TaskScheduler::PWU_TASK_TRIGGER& wutrigger);
	};
}