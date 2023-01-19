#include "pch.h"
#include "TaskScheduler.h"

namespace WindowsUtils::Core
{

	/*========================================
	==			object definition			==
	==========================================*/

	// Memory manager
	_WuMemoryManagement* PWuMemoryManagement;

	/*
	* Task trigger types
	*/

	// Trigger repetition pattern
	TaskScheduler::_WU_TASK_TRIGGER_REPETITION_PATTERN::_WU_TASK_TRIGGER_REPETITION_PATTERN(
		LPWSTR&& duration
		,LPWSTR&& interval
		,BOOL&& stopatduration
	) : StopAtDuration(stopatduration)
	{
		size_t szdur = wcslen(duration) + 1;
		size_t szinterval = wcslen(interval) + 1;

		Duration = (LPWSTR)PWuMemoryManagement->Allocate(sizeof(WCHAR) * szdur);
		Interval = (LPWSTR)PWuMemoryManagement->Allocate(sizeof(WCHAR) * szinterval);

		wcscpy_s(Duration, szdur, duration);
		wcscpy_s(Interval, szinterval, interval);
	}
	
	TaskScheduler::_WU_TASK_TRIGGER_REPETITION_PATTERN::_WU_TASK_TRIGGER_REPETITION_PATTERN()
		: Duration(NULL), Interval(NULL), StopAtDuration(FALSE) { }
	
	TaskScheduler::_WU_TASK_TRIGGER_REPETITION_PATTERN::~_WU_TASK_TRIGGER_REPETITION_PATTERN()
	{
		PWuMemoryManagement->Free(Duration);
		PWuMemoryManagement->Free(Interval);
	}

	// Trigger
	TaskScheduler::_WU_TASK_TRIGGER::_WU_TASK_TRIGGER(
		LPCWSTR&& startbound
		,TASK_TRIGGER_TYPE2 type
	) : ExecutionTimeLimit(NULL), EndBoundary(NULL), Id(NULL), Enabled(TRUE), Repetition(new WU_TASK_TRIGGER_REPETITION_PATTERN()), Type(type)
	{
		size_t szstartbound = wcslen(startbound) + 1;
		StartBoundary = (LPWSTR)PWuMemoryManagement->Allocate(sizeof(WCHAR) * szstartbound);
		wcscpy_s(StartBoundary, szstartbound, startbound);
	}

	TaskScheduler::_WU_TASK_TRIGGER::~_WU_TASK_TRIGGER()
	{
		PWuMemoryManagement->Free(StartBoundary);
		delete Repetition;
	}

	// Trigger time
	TaskScheduler::_WU_TASK_TRIGGER_TIME::_WU_TASK_TRIGGER_TIME()
		: RandomDelay(NULL), _WU_TASK_TRIGGER((LPCWSTR)L"", TASK_TRIGGER_TIME) { }

	TaskScheduler::_WU_TASK_TRIGGER_TIME::~_WU_TASK_TRIGGER_TIME() { }

	// Task trigger boot
	TaskScheduler::_WU_TASK_TRIGGER_BOOT::_WU_TASK_TRIGGER_BOOT()
		: RandomDelay(NULL), _WU_TASK_TRIGGER((LPCWSTR)L"", TASK_TRIGGER_BOOT) { }

	TaskScheduler::_WU_TASK_TRIGGER_BOOT::~_WU_TASK_TRIGGER_BOOT() { }

	/*
	* Task action types
	*/

	// Action
	TaskScheduler::_WU_TASK_ACTION::_WU_TASK_ACTION(
		TASK_ACTION_TYPE type
	) : Id(NULL), Type(type) { }

	TaskScheduler::_WU_TASK_ACTION::~_WU_TASK_ACTION() { }

	// Action exec
	TaskScheduler::_WU_TASK_ACTION_EXEC::_WU_TASK_ACTION_EXEC(
		LPWSTR&& path = NULL
		,LPWSTR&& arguments = NULL
		,LPWSTR&& workingdir = NULL
	) : _WU_TASK_ACTION(TASK_ACTION_EXEC)
	{
		size_t szpath = wcslen(path) + 1;
		Path = (LPWSTR)PWuMemoryManagement->Allocate(sizeof(WCHAR) * szpath);
		wcscpy_s(Path, szpath, path);

		if (NULL != arguments)
		{
			size_t szargs = wcslen(arguments) + 1;
			Arguments = (LPWSTR)PWuMemoryManagement->Allocate(sizeof(WCHAR) * szargs);
			wcscpy_s(Arguments, szpath, arguments);
		}

		if (NULL != workingdir)
		{
			size_t szwordir = wcslen(workingdir) + 1;
			WorkingDirectory = (LPWSTR)PWuMemoryManagement->Allocate(sizeof(WCHAR) * szwordir);
			wcscpy_s(WorkingDirectory, szwordir, workingdir);
		}
	}

	TaskScheduler::_WU_TASK_ACTION_EXEC::~_WU_TASK_ACTION_EXEC()
	{
		PWuMemoryManagement->Free(Path);
		PWuMemoryManagement->Free(Arguments);
		PWuMemoryManagement->Free(WorkingDirectory);
	}

	/*
	* Task settings
	*/

	TaskScheduler::_WU_TASK_SETTINGS::_WU_TASK_SETTINGS()
		: AllowDemandStart(FALSE)
		,AllowHardTerminate(TRUE)
		,DisallowStartIfOnBatteries(FALSE)
		,StopIfGoingOnBatteries(FALSE)
		,Enabled(TRUE)
		,Hidden(FALSE)
		,RunOnlyIfIdle(FALSE)
		,RunOnlyIfNetworkAvailable(FALSE)
		,StartWhenAvailable(TRUE)
		,WakeToRun(FALSE)
		,Priority(7) // BELOW_NORMAL_PRIORITY_CLASS; THREAD_PRIORITY_BELOW_NORMAL. Default for background tasks.
		,RestartCount(0)
		,RestartInterval(NULL)
		,DeleteExpiredTaskAfter(NULL)
		,ExecutionTimeLimit(L"PT3H")
		,Compatibility(TASK_COMPATIBILITY_V2)
		,MultipleInstances(TASK_INSTANCES_IGNORE_NEW)
	{ }

	TaskScheduler::_WU_TASK_SETTINGS::~_WU_TASK_SETTINGS() { }

	/*
	* Task principal
	*/

	TaskScheduler::_WU_TASK_PRINCIPAL::_WU_TASK_PRINCIPAL()
		: DisplayName(NULL), UserId(NULL), GroupId(NULL), Id(NULL), LogonType(TASK_LOGON_INTERACTIVE_TOKEN), RunLevel(TASK_RUNLEVEL_LUA)
	{ }

	TaskScheduler::_WU_TASK_PRINCIPAL::~_WU_TASK_PRINCIPAL()
	{
		PWuMemoryManagement->Free(DisplayName);
		PWuMemoryManagement->Free(UserId);
		PWuMemoryManagement->Free(GroupId);
		PWuMemoryManagement->Free(Id);
	}

	/*
	* Task definition
	*/
	TaskScheduler::_WU_TASK_DEFINITION::_WU_TASK_DEFINITION()
	{
		Principal = *std::make_shared<PWU_TASK_PRINCIPAL>();
		Settings = *std::make_shared<PWU_TASK_SETTINGS>();
		Triggers = *MakeVecPtr(PWU_TASK_TRIGGER);
		Actions = *MakeVecPtr(PWU_TASK_ACTION);
	}

	TaskScheduler::_WU_TASK_DEFINITION::~_WU_TASK_DEFINITION() { }

	/*TaskScheduler::PWU_TASK_TRIGGER TaskScheduler::_WU_TASK_DEFINITION::CreateTrigger(TASK_TRIGGER_TYPE2 type)
	{
		switch (type)
		{
		case TASK_TRIGGER_BOOT:
			PWU_TASK_TRIGGER_BOOT psingle = new WU_TASK_TRIGGER_BOOT();
			Triggers.push_back(psingle);
			return Triggers.back();

		case TASK_TRIGGER_TIME:
			PWU_TASK_TRIGGER_TIME psingle = new WU_TASK_TRIGGER_TIME();
			Triggers.push_back(psingle);
			return Triggers.back();
		}
	}*/

	TaskScheduler::PWU_TASK_ACTION TaskScheduler::_WU_TASK_DEFINITION::CreateAction()
	{
		PWU_TASK_ACTION_EXEC psingle = new WU_TASK_ACTION_EXEC();
		Actions.push_back(psingle);
		return Actions.back();
	}

	/*========================================
	==			function definition			==
	==========================================*/

	//HRESULT TaskScheduler::CreateScheduledTask(
	//	LPWSTR&& taskname				// Task name.
	//	,PWU_TASK_DEFINITION& taskdef	// Task definition.
	//	,LPWSTR&& taskpath = L"\\"		// Task path.
	//	,LPWSTR&& userpass = NULL
	//)
	//{
	//	HRESULT coresult = S_OK;
	//	LPCOLESTR pprogid = L"Schedule.Service";
	//	ITaskService* ischeduler = NULL;
	//	ITaskFolder* ifolder = NULL;
	//	ITaskDefinition* idefinition = NULL;
	//	IPrincipal* iprincipal = NULL;
	//	ITaskSettings* isettings = NULL;
	//	ITriggerCollection* itriggercol = NULL;
	//	IActionCollection* iactioncol = NULL;
	//	IRegisteredTask* itask = NULL;
	//	_variant_t varuserid = VT_NULL;
	//	_variant_t varuserpass = VT_NULL;

	//	/*
	//	* On C++/CLI, the runtime calls CoInitializeEx to initialize the COM apartment state.
	//	* Changing System.Threading.ApartmentState before calling unmanaged code can improve performance.
	//	* This is valid even when using P/Invoke.
	//	* 
	//	* https://devblogs.microsoft.com/oldnewthing/20150316-00/?p=44463
	//	* https://learn.microsoft.com/en-us/previous-versions/dotnet/netframework-1.1/5s8ee185(v=vs.71)?redirectedfrom=MSDN
	//	* 
	//	coresult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//	if (FAILED(coresult))
	//		return coresult;

	//	coresult = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
	//	if (FAILED(coresult))
	//		return coresult;
	//	*/

	//	coresult = CoCreateInstance(
	//		CLSID_TaskScheduler		// Schedule.Service program CLSID.
	//		,NULL					// Not null if object is being created as part of an aggregate.
	//		,CLSCTX_INPROC_SERVER	// Value from CLSCTX enum.
	//		,IID_ITaskService		// Interface ID to return.
	//		,(void **)&ischeduler	// The interface.
	//	);
	//	RETURNIFFAIL(coresult);

	//	/*
	//	* At this point we should have a pointer to the ITaskService interface.
	//	* Using this interface we can manage all other Task Scheduler API objects.
	//	* This is equivalent to New-Object -ComObject 'Schedule.Service',
	//	* with .NET runtime callable wrappers on PowerShell.
	//	*/

	//	coresult = ischeduler->Connect(
	//		_variant_t()	// Local computer.
	//		,_variant_t()	// Current user token.
	//		,_variant_t()	// And domain.
	//		,_variant_t()	// Empty password.
	//	);
	//	RETURNIFFAIL(coresult);

	//	coresult = ischeduler->GetFolder(taskpath, &ifolder);
	//	if (FAILED(coresult))
	//	{
	//		ischeduler->Release();
	//		return coresult;
	//	}

	//	coresult = ischeduler->NewTask(0, &idefinition);
	//	ischeduler->Release();
	//	if (FAILED(coresult))
	//	{
	//		ifolder->Release();
	//		return coresult;
	//	}

	//	// Defining task settings.
	//	coresult = idefinition->get_Settings(&isettings);
	//	if (FAILED(coresult))
	//	{
	//		ifolder->Release();
	//		idefinition->Release();
	//		return coresult;
	//	}

	//	coresult = TaskScheduler::CopyTaskSettings(isettings, taskdef->Settings);
	//	isettings->Release();
	//	if (FAILED(coresult))
	//	{
	//		ifolder->Release();
	//		idefinition->Release();
	//		return coresult;
	//	}

	//	// Defining task principal.
	//	coresult = idefinition->get_Principal(&iprincipal);
	//	if (FAILED(coresult))
	//	{
	//		ifolder->Release();
	//		idefinition->Release();
	//		return coresult;
	//	}

	//	coresult = TaskScheduler::CopyTaskPrincipal(iprincipal, taskdef->Principal);
	//	iprincipal->Release();
	//	if (FAILED(coresult))
	//	{
	//		ifolder->Release();
	//		idefinition->Release();
	//		return coresult;
	//	}

	//	// Creating triggers.
	//	coresult = idefinition->get_Triggers(&itriggercol);
	//	if (FAILED(coresult))
	//	{
	//		ifolder->Release();
	//		idefinition->Release();
	//		return coresult;
	//	}

	//	for (PWU_TASK_TRIGGER wutrigger : taskdef->Triggers)
	//	{
	//		ITrigger* itrigger = NULL;
	//		coresult = itriggercol->Create((::TASK_TRIGGER_TYPE2)wutrigger->Type, &itrigger);
	//		itriggercol->Release();
	//		if (FAILED(coresult))
	//		{
	//			ifolder->Release();
	//			idefinition->Release();
	//			return coresult;
	//		}

	//		switch (wutrigger->Type)
	//		{
	//		case TASK_TRIGGER_BOOT:
	//			IBootTrigger* iboottrigger = NULL;
	//			PWU_TASK_TRIGGER_BOOT wuboottrigger = (PWU_TASK_TRIGGER_BOOT)wutrigger;
	//			BSTR delay;

	//			coresult = itrigger->QueryInterface(IID_IBootTrigger, (void**)iboottrigger);
	//			itrigger->Release();
	//			if (FAILED(coresult))
	//			{
	//				ifolder->Release();
	//				idefinition->Release();
	//				return coresult;
	//			}

	//			// ITrigger properties.
	//			coresult = CopyTaskTriggerProperties((void**)iboottrigger, wutrigger);
	//			if (FAILED(coresult))
	//			{
	//				ifolder->Release();
	//				idefinition->Release();
	//				iboottrigger->Release();
	//				return coresult;
	//			}

	//			// IBootTrigger properties.
	//			coresult = iboottrigger->get_Delay(&delay);
	//			if (FAILED(coresult))
	//			{
	//				iboottrigger->Release();
	//				ifolder->Release();
	//				idefinition->Release();
	//				return coresult;
	//			}
	//			if (!wcscmp(delay, wuboottrigger->RandomDelay))
	//				iboottrigger->put_Delay(wuboottrigger->RandomDelay);
	//			iboottrigger->Release();
	//			
	//			break;

	//		case TASK_TRIGGER_TIME:
	//			ITimeTrigger* itimetrigger = NULL;
	//			PWU_TASK_TRIGGER_TIME wutimetrigger = (PWU_TASK_TRIGGER_TIME)wutrigger;
	//			BSTR delay;

	//			coresult = itrigger->QueryInterface(IID_ITimeTrigger, (void**)itimetrigger);
	//			itrigger->Release();
	//			if (FAILED(coresult))
	//			{
	//				ifolder->Release();
	//				idefinition->Release();
	//				return coresult;
	//			}

	//			// ITrigger properties.
	//			coresult = CopyTaskTriggerProperties((void**)itimetrigger, wutrigger);
	//			if (FAILED(coresult))
	//			{
	//				ifolder->Release();
	//				idefinition->Release();
	//				return coresult;
	//			}

	//			// ITimeTrigger properties.
	//			coresult = itimetrigger->get_RandomDelay(&delay);
	//			if (FAILED(coresult))
	//			{
	//				ifolder->Release();
	//				idefinition->Release();
	//				itimetrigger->Release();
	//				return coresult;
	//			}
	//			if (!wcscmp(delay, wutimetrigger->RandomDelay))
	//				itimetrigger->put_RandomDelay(wutimetrigger->RandomDelay);
	//			itimetrigger->Release();

	//			break;
	//		
	//		}
	//	}

	//	// Creating actions.
	//	coresult = idefinition->get_Actions(&iactioncol);
	//	if (FAILED(coresult))
	//	{
	//		ifolder->Release();
	//		idefinition->Release();
	//		return coresult;
	//	}

	//	for (PWU_TASK_ACTION wuaction : taskdef->Actions)
	//	{
	//		IAction* iaction = NULL;
	//		IExecAction* iexecaction = NULL;
	//		PWU_TASK_ACTION_EXEC wuexecaction = (PWU_TASK_ACTION_EXEC)wuaction;
	//		BSTR actionid;
	//		BSTR path;
	//		BSTR args;
	//		BSTR workingdir;

	//		coresult = iactioncol->Create(::TASK_ACTION_TYPE::TASK_ACTION_EXEC, &iaction);
	//		iactioncol->Release();
	//		if (FAILED(coresult))
	//		{
	//			ifolder->Release();
	//			idefinition->Release();
	//			return coresult;
	//		}

	//		coresult = iaction->QueryInterface(IID_IExecAction, (void**)iexecaction);
	//		iaction->Release();
	//		if (FAILED(coresult))
	//		{
	//			ifolder->Release();
	//			idefinition->Release();
	//			return coresult;
	//		}

	//		// Copying proerties.
	//		coresult = iexecaction->get_Id(&actionid);
	//		if (FAILED(coresult))
	//		{
	//			iexecaction->Release();
	//			ifolder->Release();
	//			idefinition->Release();
	//			return  coresult;
	//		}
	//		if (!wcscmp(actionid, wuexecaction->Id))
	//			iexecaction->put_Id(wuexecaction->Id);

	//		coresult = iexecaction->get_Path(&path);
	//		if (FAILED(coresult))
	//		{
	//			iexecaction->Release();
	//			ifolder->Release();
	//			idefinition->Release();
	//			return  coresult;
	//		}
	//		if (!wcscmp(path, wuexecaction->Path))
	//			iexecaction->put_Path(wuexecaction->Path);

	//		coresult = iexecaction->get_Arguments(&args);
	//		if (FAILED(coresult))
	//		{
	//			iexecaction->Release();
	//			ifolder->Release();
	//			idefinition->Release();
	//			return  coresult;
	//		}
	//		if (!wcscmp(args, wuexecaction->Arguments))
	//			iexecaction->put_Arguments(wuexecaction->Arguments);

	//		coresult = iexecaction->get_WorkingDirectory(&workingdir);
	//		if (FAILED(coresult))
	//		{
	//			iexecaction->Release();
	//			ifolder->Release();
	//			idefinition->Release();
	//			return  coresult;
	//		}
	//		if (!wcscmp(workingdir, wuexecaction->WorkingDirectory))
	//			iexecaction->put_WorkingDirectory(wuexecaction->WorkingDirectory);
	//		iexecaction->Release();
	//	}

	//	// Managing final credential.
	//	if (!IsNullOrWhiteSpace(taskdef->Principal->UserId))
	//	{

	//	}

	//	// Registering task.
	//	/*coresult = ifolder->RegisterTaskDefinition(
	//		taskname
	//		,idefinition
	//		,::TASK_CREATE_OR_UPDATE
	//		,
	//	);*/

	//	SAFEISHRELEASE(ifolder);
	//	SAFEISHRELEASE(idefinition);

	//	return coresult;
	//}

	HRESULT TaskScheduler::CopyTaskSettings(ITaskSettings*& isettings, TaskScheduler::PWU_TASK_SETTINGS& wusettings)
	{
		HRESULT result = S_OK;
		VARIANT_BOOL booleanvalues = FALSE;
		int intvalues = 0;
		BSTR restartinterval;
		BSTR deleteexpiredafter;
		BSTR exectimelimit;
		TASK_COMPATIBILITY compatibility;
		TASK_INSTANCES_POLICY instpolicy;

		result = isettings->get_AllowDemandStart(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->AllowDemandStart * -1))
			isettings->put_AllowDemandStart(wusettings->AllowDemandStart * -1);

		result = isettings->get_AllowHardTerminate(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->AllowHardTerminate * -1))
			isettings->put_AllowHardTerminate(wusettings->AllowHardTerminate * -1);

		result = isettings->get_DisallowStartIfOnBatteries(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->DisallowStartIfOnBatteries * -1))
			isettings->put_DisallowStartIfOnBatteries(wusettings->DisallowStartIfOnBatteries * -1);

		result = isettings->get_StopIfGoingOnBatteries(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->DisallowStartIfOnBatteries * -1))
			isettings->put_DisallowStartIfOnBatteries(wusettings->DisallowStartIfOnBatteries * -1);

		result = isettings->get_StopIfGoingOnBatteries(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->StopIfGoingOnBatteries * -1))
			isettings->put_StopIfGoingOnBatteries(wusettings->StopIfGoingOnBatteries * -1);

		result = isettings->get_Enabled(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->Enabled * -1))
			isettings->put_Enabled(wusettings->Enabled * -1);

		result = isettings->get_Hidden(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->Hidden * -1))
			isettings->put_Hidden(wusettings->Hidden * -1);

		result = isettings->get_RunOnlyIfIdle(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->RunOnlyIfIdle * -1))
			isettings->put_RunOnlyIfIdle(wusettings->RunOnlyIfIdle * -1);

		result = isettings->get_RunOnlyIfNetworkAvailable(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->RunOnlyIfNetworkAvailable * -1))
			isettings->put_RunOnlyIfNetworkAvailable(wusettings->RunOnlyIfNetworkAvailable * -1);

		result = isettings->get_StartWhenAvailable(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->StartWhenAvailable * 1))
			isettings->put_StartWhenAvailable(wusettings->StartWhenAvailable * -1);

		result = isettings->get_WakeToRun(&booleanvalues);
		RETURNIFFAIL(result);
		if (booleanvalues != (wusettings->WakeToRun * -1))
			isettings->put_WakeToRun(wusettings->WakeToRun * -1);

		result = isettings->get_Priority(&intvalues);
		RETURNIFFAIL(result);
		if (intvalues != wusettings->Priority)
			isettings->put_Priority(intvalues);

		result = isettings->get_RestartCount(&intvalues);
		RETURNIFFAIL(result);
		if (intvalues != wusettings->RestartCount)
			isettings->put_RestartCount(intvalues);

		result = isettings->get_RestartInterval(&restartinterval);
		RETURNIFFAIL(result);
		if (!wcscmp(restartinterval, wusettings->RestartInterval))
			isettings->put_RestartInterval(wusettings->RestartInterval);

		result = isettings->get_DeleteExpiredTaskAfter(&deleteexpiredafter);
		RETURNIFFAIL(result);
		if (!wcscmp(deleteexpiredafter, wusettings->DeleteExpiredTaskAfter))
			isettings->put_DeleteExpiredTaskAfter(wusettings->DeleteExpiredTaskAfter);

		result = isettings->get_ExecutionTimeLimit(&exectimelimit);
		RETURNIFFAIL(result);
		if (!wcscmp(exectimelimit, wusettings->ExecutionTimeLimit))
			isettings->put_ExecutionTimeLimit(wusettings->ExecutionTimeLimit);

		result = isettings->get_Compatibility(&compatibility);
		RETURNIFFAIL(result);
		if (compatibility != wusettings->Compatibility)
			isettings->put_Compatibility(wusettings->Compatibility);

		result = isettings->get_Compatibility(&compatibility);
		RETURNIFFAIL(result);
		if (compatibility != wusettings->Compatibility)
			isettings->put_Compatibility(wusettings->Compatibility);

		result = isettings->get_MultipleInstances(&instpolicy);
		RETURNIFFAIL(result);
		if (instpolicy != wusettings->MultipleInstances)
			isettings->put_MultipleInstances(wusettings->MultipleInstances);

	}

	HRESULT TaskScheduler::CopyTaskPrincipal(IPrincipal*& iprincipal, TaskScheduler::PWU_TASK_PRINCIPAL wuprincipal)
	{
		HRESULT result = S_OK;
		BSTR displayname;
		BSTR groupinfo;
		BSTR taskid;
		BSTR userid;
		TASK_LOGON_TYPE logontype;
		TASK_RUNLEVEL_TYPE runlevel;

		result = iprincipal->get_DisplayName(&displayname);
		RETURNIFFAIL(result);
		if (!wcscmp(displayname, wuprincipal->DisplayName))
			iprincipal->put_DisplayName(wuprincipal->DisplayName);

		result = iprincipal->get_GroupId(&groupinfo);
		RETURNIFFAIL(result);
		if (!wcscmp(groupinfo, wuprincipal->GroupId))
			iprincipal->put_GroupId(wuprincipal->GroupId);

		result = iprincipal->get_Id(&taskid);
		RETURNIFFAIL(result);
		if (!wcscmp(taskid, wuprincipal->Id))
			iprincipal->put_Id(wuprincipal->Id);

		result = iprincipal->get_UserId(&userid);
		RETURNIFFAIL(result);
		if (!wcscmp(userid, wuprincipal->UserId))
			iprincipal->put_UserId(wuprincipal->UserId);

		result = iprincipal->get_LogonType(&logontype);
		RETURNIFFAIL(result);
		if (logontype != wuprincipal->LogonType)
			iprincipal->put_LogonType(wuprincipal->LogonType);

		result = iprincipal->get_RunLevel(&runlevel);
		RETURNIFFAIL(result);
		if (runlevel != wuprincipal->RunLevel)
			iprincipal->put_RunLevel(wuprincipal->RunLevel);
	}

	HRESULT TaskScheduler::CopyTaskTriggerProperties(void** ptrigger, PWU_TASK_TRIGGER& wutrigger)
	{
		HRESULT result = S_OK;
		ITrigger* itrigger = (ITrigger*)ptrigger;
		IRepetitionPattern* reppattern = NULL;
		VARIANT_BOOL enabled;
		BSTR endboundary;
		BSTR exectimelimit;
		BSTR triggerid;
		BSTR startboundary;

		result = itrigger->get_Enabled(&enabled);
		RETURNIFFAIL(result);
		if (enabled != (wutrigger->Enabled * -1))
			itrigger->put_Enabled(wutrigger->Enabled * -1);

		result = itrigger->get_EndBoundary(&endboundary);
		RETURNIFFAIL(result);
		if (!wcscmp(endboundary, wutrigger->EndBoundary))
			itrigger->put_EndBoundary(wutrigger->EndBoundary);

		result = itrigger->get_ExecutionTimeLimit(&exectimelimit);
		RETURNIFFAIL(result);
		if (!wcscmp(exectimelimit, wutrigger->ExecutionTimeLimit))
			itrigger->put_ExecutionTimeLimit(wutrigger->ExecutionTimeLimit);

		result = itrigger->get_Id(&triggerid);
		RETURNIFFAIL(result);
		if (!wcscmp(triggerid, wutrigger->Id))
			itrigger->put_Id(wutrigger->Id);

		result = itrigger->get_StartBoundary(&startboundary);
		RETURNIFFAIL(result);
		if (!wcscmp(startboundary, wutrigger->StartBoundary))
			itrigger->put_StartBoundary(wutrigger->StartBoundary);
	}
}