#pragma once
#pragma unmanaged

#include "../Support/Notification.h"
#include "../Engine/ProcessAndThread.h"

#pragma managed

#include "Marshalers.h"
#include "NativeException.h"
#include "Types/NetworkTypes.h"
#include "Types/ProcThreadTypes.h"

namespace WindowsUtils::Core
{
	using namespace System;
	using namespace System::Threading;
	using namespace System::Management::Automation;
	using namespace System::Runtime::InteropServices;
	using namespace WindowsUtils::Network;

	public delegate void WriteProgressWrapper(ProgressRecord^ data);
	public delegate void WriteWarningWrapper(String^ data);
	public delegate void WriteInformationWrapper(InformationRecord^ data);
	public delegate void WriteErrorWrapper(ErrorRecord^ data);
	public delegate void WriteObjectWrapper(Object^ data);

	// A wrapper for the Cmdlet context
	public ref class CmdletContextProxy
	{
	public:
		delegate void _WriteProgressProxy(const PMAPPED_PROGRESS_DATA data);
		delegate void _WriteWarningProxy(const WWuString& data);
		delegate void _WriteInformationProxy(const PMAPPED_INFORMATION_DATA data);
		delegate void _WriteExceptionProxy(const WuException& ex);
		delegate void _WriteObjectProxy(const PVOID data, const WriteOutputType type);

		CmdletContextProxy(
			WriteProgressWrapper^ progWrapper,
			WriteWarningWrapper^ warnWrapper,
			WriteInformationWrapper^ infoWrapper,
			WriteObjectWrapper^ objWrapper,
			WriteErrorWrapper^ errorWrapper
		)
		{
			m_progDelegate = progWrapper;
			_WriteProgressProxy^ progDelWrapper = gcnew _WriteProgressProxy(this, &CmdletContextProxy::WriteProgressProxy);
			m_progHandle = GCHandle::Alloc(progDelWrapper);
			IntPtr progressDelegatePtr = Marshal::GetFunctionPointerForDelegate(progDelWrapper);
			auto progressPtr = static_cast<UnmanagedWriteProgress>(progressDelegatePtr.ToPointer());

			m_warningDelegate = warnWrapper;
			_WriteWarningProxy^ warningDelWrapper = gcnew _WriteWarningProxy(this, &CmdletContextProxy::WriteWarningProxy);
			m_warnHandle = GCHandle::Alloc(warningDelWrapper);
			IntPtr warningDelegatePtr = Marshal::GetFunctionPointerForDelegate(warningDelWrapper);
			auto warningPtr = static_cast<UnmanagedWriteWarning>(warningDelegatePtr.ToPointer());

			m_infoDelegate = infoWrapper;
			_WriteInformationProxy^ infoDelWrapper = gcnew _WriteInformationProxy(this, &CmdletContextProxy::WriteInformationProxy);
			m_infoHandle = GCHandle::Alloc(infoDelWrapper);
			IntPtr informationDelegatePtr = Marshal::GetFunctionPointerForDelegate(infoDelWrapper);
			auto infoPtr = static_cast<UnmanagedWriteInformation>(informationDelegatePtr.ToPointer());

			m_objectDelegate = objWrapper;
			_WriteObjectProxy^ objDelWrapper = gcnew _WriteObjectProxy(this, &CmdletContextProxy::WriteObjectProxy);
			m_objHandle = GCHandle::Alloc(objDelWrapper);
			IntPtr objectDelegatePtr = Marshal::GetFunctionPointerForDelegate(objDelWrapper);
			auto objPtr = static_cast<UnmanagedWriteObject>(objectDelegatePtr.ToPointer());

			m_errorDelegate = errorWrapper;
			_WriteExceptionProxy^ exDelWrapper = gcnew _WriteExceptionProxy(this, &CmdletContextProxy::WriteExceptionProxy);
			m_exHandle = GCHandle::Alloc(exDelWrapper);
			auto exPtr = static_cast<UnmanagedWriteException>(Marshal::GetFunctionPointerForDelegate(exDelWrapper).ToPointer());

			m_nativeContext = new WuNativeContext(
				progressPtr,
				warningPtr,
				infoPtr,
				objPtr,
				exPtr,
				ExceptionMarshaler::NativePtr
			);
		}

		~CmdletContextProxy()
		{
			m_progHandle.Free();
			m_warnHandle.Free();
			m_infoHandle.Free();
			m_objHandle.Free();
			m_errorHandle.Free();
		}

		WuNativeContext* GetUnderlyingContext() { return m_nativeContext; }

		void WriteProgress(ProgressRecord^ record) { m_progDelegate(record); }
		void WriteWarning(String^ text) { m_warningDelegate(text); }
		void WriteInformation(InformationRecord^ record) { m_infoDelegate(record); }
		void WriteError(ErrorRecord^ record) { m_errorDelegate(record); }
		void WriteObject(Object^ object) { m_objectDelegate(object); }

		void WriteProgressProxy(const PMAPPED_PROGRESS_DATA progressData)
		{
			ProgressRecord^ record = gcnew ProgressRecord(progressData->ActivityId, gcnew String(progressData->Activity.Raw()), gcnew String(progressData->StatusDescription.Raw()));
			record->CurrentOperation = gcnew String(progressData->CurrentOperation.Raw());
			record->ParentActivityId = progressData->ParentActivityId;
			record->PercentComplete = progressData->PercentComplete;
			record->RecordType = static_cast<System::Management::Automation::ProgressRecordType>(progressData->RecordType);
			record->SecondsRemaining = progressData->SecondsRemaining;

			m_progDelegate(record);
		}

		void WriteWarningProxy(const WWuString& text)
		{
			m_warningDelegate(gcnew String(text.Raw()));
		}

		void WriteInformationProxy(const PMAPPED_INFORMATION_DATA infoData)
		{
			InformationRecord^ record = gcnew InformationRecord(gcnew String(infoData->Text.Raw()), gcnew String(infoData->Source.Raw()));
			record->Computer = gcnew String(infoData->Computer.Raw());
			record->ManagedThreadId = Thread::CurrentThread->ManagedThreadId;
			record->NativeThreadId = infoData->NativeThreadId;
			record->TimeGenerated = DateTime::FromFileTime(infoData->TimeGenerated);
			record->User = gcnew String(infoData->User.Raw());

			for (WWuString& tag : infoData->Tags)
				record->Tags->Add(gcnew String(tag.Raw()));

			m_infoDelegate(record);
		}

		void WriteExceptionProxy(const WuException& exception)
		{
			NativeException^ manEx = gcnew NativeException(exception);
			m_errorDelegate(manEx->Record);
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
						String^ text = gcnew String(reinterpret_cast<WWuString*>(obj)->Raw());
						m_objectDelegate(text);
					} break;

					case WriteOutputType::ProcessModuleInfo:
					{
						ProcessModuleInfo^ modInfo = gcnew ProcessModuleInfo(*reinterpret_cast<PROCESS_MODULE_INFO*>(obj));
						m_objectDelegate(modInfo);
					} break;

					case WriteOutputType::ObjectHandle:
					{
						ObjectHandle^ objHandle = gcnew ObjectHandle(*reinterpret_cast<OBJECT_HANDLE*>(obj));
						m_objectDelegate(objHandle);
					} break;
				}
			}
		}

	protected:
		!CmdletContextProxy()
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
		GCHandle m_exHandle;
	};
}