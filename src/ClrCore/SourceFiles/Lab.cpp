#include "..\pch.h"

#include "..\Headers\Lab.h"

namespace WindowsUtils::Core
{
#pragma unmanaged
	void NativeLab::WriteWWuStringToPowerShellOutput(WuNativeContext* context)
	{
		WWuString testString = L"Tits!";
		context->NativeWriteObject<WWuString>(&testString, Notification::WWUSTRING);
	}

	void NativeLab::WriteStructToPowerShellOutput(WuNativeContext* context)
	{
		LAB_STRUCT obj = {
			666,
			L"Tits!"
		};

		context->NativeWriteObject<LAB_STRUCT>(&obj, Notification::LAB_STRUCT);
	}

	void NativeLab::WriteVectorToPowerShellOutput(WuNativeContext* context)
	{
		wuvector<ProcessAndThread::WU_MODULE_INFO> vec;
		ProcessAndThread::WU_MODULE_INFO info;
		info.ModuleName = L"Module name";
		info.ModulePath = L"Module path";
		info.VersionInfo.FileDescription = L"Version info File Description";
		info.VersionInfo.ProductName = L"Version info Product Name";
		info.VersionInfo.FileVersion = L"Version info File Version";
		info.VersionInfo.CompanyName = L"Version info Company Name";

		vec.push_back(info);

		
	}

#pragma managed
	void LabWrapper::WriteWWuStringAsObject(CmdletContextBase^ context)
	{
		labPtr->WriteWWuStringToPowerShellOutput(context->GetUnderlyingContext());
	}

	void LabWrapper::WriteStructAsObject(CmdletContextBase^ context)
	{
		labPtr->WriteStructToPowerShellOutput(context->GetUnderlyingContext());
	}

	void LabWrapper::WriteVectorAsObject(CmdletContextBase^ context)
	{
		labPtr->WriteVectorToPowerShellOutput(context->GetUnderlyingContext());
	}
}