#pragma once
#pragma unmanaged

#include "../../Support/Notification.h"
#include "../../Engine/ProcessAndThread.h"

#pragma managed

#include "NativeException.h"
#include "TcpingProbeInfo.h"
#include "TcpingStatistics.h"
#include "TestPortInfo.h"
#include "ProcessModuleInfo.h"

namespace WindowsUtils::Core
{
	using namespace System;
	using namespace System::Threading;
	using namespace System::Management::Automation;
	using namespace System::Runtime::InteropServices;

	// A wrapper for the Cmdlet context
	public ref class CmdletContextBase
	{
	public:
		delegate void WriteProgressWrapper(ProgressRecord^ data);
		delegate void WriteWarningWrapper(String^ data);
		delegate void WriteInformationWrapper(InformationRecord^ data);
		delegate void WriteErrorWrapper(ErrorRecord^ data);
		delegate void WriteObjectWrapper(Object^ data);

		delegate void _WriteProgressProxy(const PMAPPED_PROGRESS_DATA data);
		delegate void _WriteWarningProxy(const WWuString& data);
		delegate void _WriteInformationProxy(const PMAPPED_INFORMATION_DATA data);
		delegate void _WriteErrorProxy(const PMAPPED_ERROR_DATA data);
		delegate void _WriteObjectProxy(const PVOID data, const WriteOutputType type);

		CmdletContextBase(
			WriteProgressWrapper^ progWrapper,
			WriteWarningWrapper^ warnWrapper,
			WriteInformationWrapper^ infoWrapper,
			WriteObjectWrapper^ objWrapper,
			WriteErrorWrapper^ errorWrapper
		)
		{
			m_progDelegate = progWrapper;
			_WriteProgressProxy^ progDelWrapper = gcnew _WriteProgressProxy(this, &CmdletContextBase::WriteProgressProxy);
			m_progHandle = GCHandle::Alloc(progDelWrapper);
			IntPtr progressDelegatePtr = Marshal::GetFunctionPointerForDelegate(progWrapper);
			auto progressPtr = static_cast<UnmanagedWriteProgress>(progressDelegatePtr.ToPointer());

			m_warningDelegate = warnWrapper;
			_WriteWarningProxy^ warningDelWrapper = gcnew _WriteWarningProxy(this, &CmdletContextBase::WriteWarningProxy);
			m_warnHandle = GCHandle::Alloc(warningDelWrapper);
			IntPtr warningDelegatePtr = Marshal::GetFunctionPointerForDelegate(warningDelWrapper);
			auto warningPtr = static_cast<UnmanagedWriteWarning>(warningDelegatePtr.ToPointer());

			m_infoDelegate = infoWrapper;
			_WriteInformationProxy^ infoDelWrapper = gcnew _WriteInformationProxy(this, &CmdletContextBase::WriteInformationProxy);
			m_infoHandle = GCHandle::Alloc(infoDelWrapper);
			IntPtr informationDelegatePtr = Marshal::GetFunctionPointerForDelegate(infoDelWrapper);
			auto infoPtr = static_cast<UnmanagedWriteInformation>(informationDelegatePtr.ToPointer());

			m_objectDelegate = objWrapper;
			_WriteObjectProxy^ objDelWrapper = gcnew _WriteObjectProxy(this, &CmdletContextBase::WriteObjectProxy);
			m_objHandle = GCHandle::Alloc(objDelWrapper);
			IntPtr objectDelegatePtr = Marshal::GetFunctionPointerForDelegate(objDelWrapper);
			auto objPtr = static_cast<UnmanagedWriteObject>(objectDelegatePtr.ToPointer());

			m_errorDelegate = errorWrapper;
			_WriteErrorProxy^ errorDelWrapper = gcnew _WriteErrorProxy(this, &CmdletContextBase::WriteErrorProxy);
			m_errorHandle = GCHandle::Alloc(errorDelWrapper);
			IntPtr errorDelegatePtr = Marshal::GetFunctionPointerForDelegate(errorDelWrapper);
			auto errorPtr = static_cast<UnmanagedWriteError>(errorDelegatePtr.ToPointer());

			m_nativeContext = new WuNativeContext(
				progressPtr,
				warningPtr,
				infoPtr,
				objPtr,
				errorPtr
			);
		}

		~CmdletContextBase()
		{
			m_progHandle.Free();
			m_warnHandle.Free();
			m_infoHandle.Free();
			m_objHandle.Free();
			m_errorHandle.Free();
		}

		WuNativeContext* GetUnderlyingContext() { return m_nativeContext; }

		void WriteProgressProxy(const PMAPPED_PROGRESS_DATA progressData)
		{
			ProgressRecord^ record = gcnew ProgressRecord(progressData->ActivityId, gcnew String(progressData->Activity.GetBuffer()), gcnew String(progressData->StatusDescription.GetBuffer()));
			record->CurrentOperation = gcnew String(progressData->CurrentOperation.GetBuffer());
			record->ParentActivityId = progressData->ParentActivityId;
			record->PercentComplete = progressData->PercentComplete;
			record->RecordType = static_cast<System::Management::Automation::ProgressRecordType>(progressData->RecordType);
			record->SecondsRemaining = progressData->SecondsRemaining;

			m_progDelegate(record);
		}

		void WriteWarningProxy(const WWuString& text)
		{
			m_warningDelegate(gcnew String(text.GetBuffer()));
		}

		void WriteInformationProxy(const PMAPPED_INFORMATION_DATA infoData)
		{
			InformationRecord^ record = gcnew InformationRecord(gcnew String(infoData->Text.GetBuffer()), gcnew String(infoData->Source.GetBuffer()));
			record->Computer = gcnew String(infoData->Computer.GetBuffer());
			record->ManagedThreadId = Thread::CurrentThread->ManagedThreadId;
			record->NativeThreadId = infoData->NativeThreadId;
			record->TimeGenerated = DateTime::FromFileTime(infoData->TimeGenerated);
			record->User = gcnew String(infoData->User.GetBuffer());

			for (WWuString& tag : infoData->Tags)
				record->Tags->Add(gcnew String(tag.GetBuffer()));

			m_infoDelegate(record);
		}

		void WriteErrorProxy(const PMAPPED_ERROR_DATA errorData)
		{
			ErrorRecord^ record = gcnew ErrorRecord(
				gcnew NativeException(errorData->ErrorCode, gcnew String(errorData->ErrorMessage.GetBuffer()), gcnew String(errorData->CompactTrace.GetBuffer())),
				gcnew String(errorData->ErrorId.GetBuffer()),
				static_cast<ErrorCategory>(errorData->Category),
				gcnew String(errorData->TargetObject.GetBuffer())
			);

			m_errorDelegate(record);
		}

		void WriteObjectProxy(const PVOID obj, WriteOutputType type)
		{
			{
				switch (type) {
					case WriteOutputType::TcpingOutput:
					{
						TcpingProbeInfo^ probeInfo = gcnew TcpingProbeInfo(*reinterpret_cast<PTCPING_OUTPUT>(obj));
						m_objectDelegate(probeInfo);
					} break;

					case WriteOutputType::TcpingStatistics:
					{
						TcpingStatistics^ statistics = gcnew TcpingStatistics(*reinterpret_cast<PTCPING_STATISTICS>(obj));
						m_objectDelegate(statistics);
					} break;

					case WriteOutputType::TestportOutput:
					{
						TestPortInfo^ portInfo = gcnew TestPortInfo(*reinterpret_cast<PTESTPORT_OUTPUT>(obj));
						m_objectDelegate(portInfo);
					} break;

					case WriteOutputType::WWuString:
					{
						String^ text = gcnew String(reinterpret_cast<WWuString*>(obj)->GetBuffer());
						m_objectDelegate(text);
					} break;

					case WriteOutputType::ProcessModuleInfo:
					{
						ProcessModuleInfo^ modInfo = gcnew ProcessModuleInfo(*reinterpret_cast<PPROCESS_MODULE_INFO>(obj));
						m_objectDelegate(modInfo);
					} break;
				}
			}
		}

	protected:
		!CmdletContextBase()
		{
			m_progHandle.Free();
			m_warnHandle.Free();
			m_infoHandle.Free();
			m_objHandle.Free();
			m_errorHandle.Free();
		}

	private:
		WuNativeContext* m_nativeContext;
		WriteProgressWrapper^ m_progDelegate;
		WriteWarningWrapper^ m_warningDelegate;
		WriteInformationWrapper^ m_infoDelegate;
		WriteObjectWrapper^ m_objectDelegate;
		WriteErrorWrapper^ m_errorDelegate;

		GCHandle m_progHandle;
		GCHandle m_warnHandle;
		GCHandle m_infoHandle;
		GCHandle m_objHandle;
		GCHandle m_errorHandle;
	};
}