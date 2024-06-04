#pragma unmanaged

#include "../../Headers/Support/IO.h"
#include "../../Headers/Support/WuStdException.h"

#pragma managed

#include "../../Headers/Wrappers/ContainersWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	// Expand-Cabinet
	void ContainersWrapper::ExpandArchiveFile(Object^ archiveObject, String^ destination, Core::ArchiveFileType fileType)
	{
		switch (fileType) {
			case WindowsUtils::Core::ArchiveFileType::Cabinet:
			{
				Core::WuManagedCabinet^ cabinet = (Core::WuManagedCabinet^)archiveObject;
				try {
					cabinet->ExpandCabinetFile(destination);
				}
				catch (const Core::WuStdException& ex) {
					throw gcnew NativeException(ex);
				}
			} break;

			default:
				throw gcnew NotSupportedException(fileType.ToString());
		}
	}

	// Compress-ArchiveFile
	void ContainersWrapper::CompressArchiveFile(String^ path, String^ destination, String^ namePrefix, int maxCabSize, Core::CabinetCompressionType compressionType, Core::ArchiveFileType type, Core::CmdletContextProxy^ context)
	{
		switch (type) {
			case WindowsUtils::Core::ArchiveFileType::Cabinet:
			{
				Core::AbstractPathTree apt;
				UtilitiesWrapper::GetAptFromPath(path, &apt);
				Core::WuCabinet cabinet(apt, UtilitiesWrapper::GetWideStringFromSystemString(namePrefix), static_cast<USHORT>(compressionType), context->GetUnderlyingContext());

				try {
					cabinet.CompressCabinetFile(UtilitiesWrapper::GetWideStringFromSystemString(destination), maxCabSize);
				}
				catch (const Core::WuStdException& ex) {
					throw gcnew NativeException(ex);
				}
			} break;
		}
	}
}