#pragma once
#pragma unmanaged

#include "../../Engine/Containers.h"
#include "../../Support/WuStdException.h"

#pragma managed

#include <vcclr.h>

#include "../CmdletContextProxy.h"

namespace WindowsUtils::Core
{
	using namespace System;
	using namespace System::Collections::Generic;

	public enum class ArchiveFileType
	{
		Cabinet
	};

	public enum class CabinetCompressionType
	{
		None = tcompTYPE_NONE,
		MSZip = tcompTYPE_MSZIP,
		LZXLow = tcompTYPE_LZX | tcompLZX_WINDOW_LO,
		LZXHigh = tcompTYPE_LZX | tcompLZX_WINDOW_HI
	};

	public ref class WuManagedCabinet
	{
	public:
		property List<String^>^ BundleCabinetPaths {
			List<String^>^ get() { return m_cabPaths; }
		}

		WuManagedCabinet(String^ filePath, CmdletContextProxy^ context)
		{
			m_cabPaths = gcnew List<String^>(0);

			pin_ptr<const wchar_t> pinnedString = PtrToStringChars(filePath);
			WWuString wrappedFilePath { (LPWSTR)pinnedString };

			m_cab = new WuCabinet(wrappedFilePath, context->GetUnderlyingContext());

			for (const CABINET_PROCESSING_INFO& cabInfo : m_cab->GetCabinetInfo()) {
				m_cabPaths->Add(gcnew String(cabInfo.Path.GetBuffer()));
			}
		}

		~WuManagedCabinet() { }

		void ExpandCabinetFile(String^ destination)
		{
			pin_ptr<const wchar_t> pinnedString = PtrToStringChars(destination);
			WWuString wrappedDestination { (LPWSTR)pinnedString };

			try {
				m_cab->ExpandCabinetFile(wrappedDestination);
			}
			catch (const WuStdException& ex) {
				throw gcnew NativeException(ex);
			}
		}

	protected:
		!WuManagedCabinet() { }

	private:
		WuCabinet* m_cab;
		List<String^>^ m_cabPaths;
	};
}