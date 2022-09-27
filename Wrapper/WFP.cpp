#pragma unmanaged

#include "pch.h"
#include "Unmanaged.h"

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // UNICODE

#define EXIT_ON_ERROR(fnName) \
   if (result != ERROR_SUCCESS) \
   { \
      printf(#fnName " = 0x%08X\n", result); \
      goto CLEANUP; \
   }

using namespace std;

namespace WindowsUtils
{
	/**********************************
	'Installing a provider', an example from MS Docs.
	*
	Source: https://docs.microsoft.com/en-us/windows/win32/fwp/installing-a-provider
	***********************************/
	DWORD InstallProvider(
		PCWSTR providerName,
		const GUID* providerKey,
		PCWSTR subLayerName,
		const GUID* subLayerKey
	)
	{
		DWORD result = ERROR_SUCCESS;
		HANDLE engine = NULL;
		FWPM_SESSION0 session;
		FWPM_PROVIDER0 provider;
		FWPM_SUBLAYER0 subLayer;

		memset(&session, 0, sizeof(session));
		session.txnWaitTimeoutInMSec = INFINITE;

		result = FwpmEngineOpen0(
			NULL,
			RPC_C_AUTHN_DEFAULT,
			NULL,
			&session,
			&engine
		);
		EXIT_ON_ERROR(FwpmEngineOpen0);

		result = FwpmTransactionBegin0(engine, 0);
		EXIT_ON_ERROR(FwpmTransactionBegin0);

		memset(&provider, 0, sizeof(provider));
		provider.providerKey = *providerKey;
		provider.displayData.name = (PWSTR)providerName;
		provider.flags = FWPM_PROVIDER_FLAG_PERSISTENT;
		result = FwpmProviderAdd0(engine, &provider, NULL);
		if (result != FWP_E_ALREADY_EXISTS) { EXIT_ON_ERROR(FwpmProviderAdd0); }

		memset(&subLayer, 0, sizeof(subLayer));
		subLayer.subLayerKey = *subLayerKey;
		subLayer.displayData.name = (PWSTR)subLayerName;
		subLayer.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
		subLayer.providerKey = (GUID*)providerKey;
		subLayer.weight = 0x8000;
		result = FwpmSubLayerAdd0(engine, &subLayer, NULL);
		if (result != FWP_E_ALREADY_EXISTS) { EXIT_ON_ERROR(FwpmSubLayerAdd0); }

		result = FwpmTransactionCommit0(engine);
		EXIT_ON_ERROR(FwpmTransactionCommit0);

	CLEANUP:
		FwpmEngineClose0(engine);
		return result;
	}

	/**********************************
	'Uninstalling a provider', an example from MS Docs.
	*
	Source: https://docs.microsoft.com/en-us/windows/win32/fwp/uninstalling-a-provider
	***********************************/
	DWORD UninstallProvider(
		const GUID* providerKey,
		const GUID* subLayerKey
	)
	{
		DWORD result = ERROR_SUCCESS;
		HANDLE engine = NULL;
		FWPM_SESSION0 session;

		memset(&session, 0, sizeof(session));
		session.txnWaitTimeoutInMSec = INFINITE;

		result = FwpmEngineOpen0(
			NULL,
			RPC_C_AUTHN_DEFAULT,
			NULL,
			&session,
			&engine
		);
		EXIT_ON_ERROR(FwpmEngineOpen0);

		result = FwpmTransactionBegin0(engine, 0);
		EXIT_ON_ERROR(FwpmTransactionBegin0);

		result = FwpmSubLayerDeleteByKey0(engine, subLayerKey);
		if (result != FWP_E_SUBLAYER_NOT_FOUND) { EXIT_ON_ERROR(FwpmSubLayerDeleteByKey0); }

		result = FwpmProviderDeleteByKey0(engine, providerKey);
		if (result != FWP_E_PROVIDER_NOT_FOUND) { EXIT_ON_ERROR(FwpmProviderDeleteByKey0); }

		result = FwpmTransactionCommit0(engine);
		EXIT_ON_ERROR(FwpmTransactionCommit0);

	CLEANUP:
		FwpmEngineClose0(engine);
		return result;
	}
}