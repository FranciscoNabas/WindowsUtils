#pragma once

#include "../../Engine/Containers.h"

#pragma managed

#include "CmdletContextBase.h"

#include <vcclr.h>


namespace WindowsUtils::Core
{
	public enum class ArchiveFileType
	{
		Cabinet
	};

	using namespace System::Collections::Generic;

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
			List<String^>^ get()
			{
				return _bundleCabinetPaths;
			}
		}

		WuManagedCabinet(String^ filePath, CmdletContextBase^ context)
		{
			_bundleCabinetPaths = gcnew List<String^>(0);

			pin_ptr<const wchar_t> pinnedString = PtrToStringChars(filePath);
			WWuString wrappedFilePath { (LPWSTR)pinnedString };

			_nativeCabinet = new WuCabinet(wrappedFilePath, context->GetUnderlyingContext());

			for (const CABINET_PROCESSING_INFO& cabInfo : _nativeCabinet->GetCabinetInfo()) {
				_bundleCabinetPaths->Add(gcnew String(cabInfo.Path.GetBuffer()));
			}
		}

		~WuManagedCabinet() { }
		
		void ExpandCabinetFile(String^ destination)
		{
			pin_ptr<const wchar_t> pinnedString = PtrToStringChars(destination);
			WWuString wrappedDestination { (LPWSTR)pinnedString };

			try {
				_nativeCabinet->ExpandCabinetFile(wrappedDestination);
			}
			catch (const WuStdException& ex) {
				throw gcnew NativeException(ex);
			}
		}

	protected:
		!WuManagedCabinet() { }

	private:
		List<String^>^ _bundleCabinetPaths;
		WuCabinet* _nativeCabinet;
	};
}