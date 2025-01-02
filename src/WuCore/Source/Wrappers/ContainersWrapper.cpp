#pragma unmanaged

#include "../../Headers/Support/IO.h"
#include "../../Headers/Support/WuException.h"

#pragma managed

#include "../../Headers/Wrappers/ContainersWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	// Expand-Cabinet
	void ContainersWrapper::ExpandArchiveFile(String^ path, String^ destination, ArchiveFileType type)
	{
		WWuString wrappedPath = UtilitiesWrapper::GetWideStringFromSystemString(path);
		WWuString wrappedDest = UtilitiesWrapper::GetWideStringFromSystemString(destination);

		switch (type) {
		case ArchiveFileType::Cabinet:
		{
			try {
				Stubs::Containers::Dispatch<ContainersOperation::Expand>(wrappedPath, wrappedDest, Context->GetUnderlyingContext());
			}
			catch (NativeException^ ex) {
				Context->WriteError(ex->Record);
				throw;
			}
		} break;

		default:
			throw gcnew NotSupportedException();
		}
	}

	// Compress-ArchiveFile
	void ContainersWrapper::CompressArchiveFile(String^ path, String^ destination, String^ namePrefix, int maxCabSize, CabinetCompressionType compressionType, ArchiveFileType type)
	{
		WWuString wrappedDest     = UtilitiesWrapper::GetWideStringFromSystemString(destination);
		WWuString wrappedNamPref  = UtilitiesWrapper::GetWideStringFromSystemString(namePrefix);
		switch (type) {
			case ArchiveFileType::Cabinet:
			{
				Core::AbstractPathTree apt;
				UtilitiesWrapper::GetAptFromPath(path, &apt);

				try {
					Stubs::Containers::Dispatch<ContainersOperation::Compress>(apt, wrappedDest, wrappedNamPref,
						maxCabSize, static_cast<const Core::CabinetCompressionType>(compressionType), Context->GetUnderlyingContext());
				}
				catch (NativeException^ ex) {
					Context->WriteError(ex->Record);
					throw;
				}
			} break;

			default:
				throw gcnew NotSupportedException();
		}
	}
}