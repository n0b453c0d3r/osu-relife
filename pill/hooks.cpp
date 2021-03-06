#include "hooks.h"
#include "factory.h"

GETADAPTERSADDRESSES fpsGetAdaptersAddresses = NULL;
WMIGET fpsWMIGet = NULL;
REGQUERYVALUEEXW fpsRegQuery = NULL;
GETADDRINFOW fpsGetAddrInfoW = NULL;

void Initialize_Hooks() {
	MH_Initialize();
	//std::cout << "hooks initialized" << std::endl;
}

void MAC_Hook() {

	auto target = GetProcAddress(LoadLibrary(L"IPHLPAPI.DLL"), "GetAdaptersAddresses");

	MH_CreateHook(target, &DetourGetAdaptersAddresses, (LPVOID*)&fpsGetAdaptersAddresses);
	MH_EnableHook(target);
	//std::cout << "created hook for 'iphlpapi.dll@GetAdaptersAddresses'" << std::endl;
}

ULONG __stdcall DetourGetAdaptersAddresses(
	ULONG                 Family,
	ULONG                 Flags,
	PVOID                 Reserved,
	PIP_ADAPTER_ADDRESSES AdapterAddresses,
	PULONG                SizePointer) {

	auto hRet = fpsGetAdaptersAddresses(Family, Flags, Reserved, AdapterAddresses, SizePointer);
	
	if (AdapterAddresses != NULL)
		*AdapterAddresses = *GetMAC();

	//std::cout << "called 'iphlpapi.dll@GetAdaptersAddresses'" << std::endl;
	
	return hRet;
}

void WMIGet_Hook() {
	auto target = GetProcAddress(LoadLibrary(L"fastprox.dll"), "?Get@CWbemObject@@UAGJPBGJPAUtagVARIANT@@PAJ2@Z");
	MH_CreateHook(target, &DetourWMIGet, (LPVOID*)&fpsWMIGet);
	MH_EnableHook(target);
	//std::cout << "created hook for 'fastprox.dll@Get'" << std::endl;
}

HRESULT __stdcall DetourWMIGet(
	void* pThis,
	LPCWSTR wszName,
	LONG lFlags,
	VARIANTARG *pVal,
	CIMTYPE pvtType,
	LONG plFlavor) {

	//std::wcout << "called 'fastprox.dll@Get' : " << wszName << std::endl;

	auto hRet = fpsWMIGet(pThis, wszName, lFlags, pVal, pvtType, plFlavor);

	if (wcscmp(wszName, L"Signature") == 0) {
		VariantClear(pVal);
	}
	else if (wcscmp(wszName, L"SerialNumber") == 0) {
		VariantClear(pVal);
		pVal->vt = VT_BSTR;
		pVal->bstrVal = GetSerialNumber();
	}

	return hRet;
}

void RegQuery_Hook() {
	auto target = GetProcAddress(LoadLibrary(L"ADVAPI32.DLL"), "RegQueryValueExW");

	MH_CreateHook(target, &DetourRegQueryValueExW, (LPVOID*)&fpsRegQuery);
	MH_EnableHook(target);
	//std::cout << "created hook for 'advapi32.dll@RegQueryValueExW'" << std::endl;
}

LSTATUS __stdcall DetourRegQueryValueExW(
	HKEY    hKey,
	LPCSTR  lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE  lpData,
	LPDWORD lpcbData
) {

	auto hRet = fpsRegQuery(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

	//std::wcout << "called 'advapi32.dll@RegQueryValueExW'" << std::endl;
	
	if (lstrcmp((LPCWSTR)lpValueName, L"UninstallID") == 0 && lpData != NULL) {
		auto tab = GetUninstallID();

		int i = 0;
		while (lpData[i] != 0) {
			lpData[i] = tab[i / 2];
			i += 2;
		}
	}
	
	return hRet;
}

void AddrInfoW_Hook() {
	auto target = GetProcAddress(LoadLibrary(L"WS2_32.dll"), "GetAddrInfoW");
	MH_CreateHook(target, &DetourGetAddrInfoW, (LPVOID*)&fpsGetAddrInfoW);
	MH_EnableHook(target);
	//std::cout << "created hook for 'WS2_32.dll@GetAddrInfoW'" << std::endl;
}

INT32 __stdcall DetourGetAddrInfoW(
	PCWSTR          pNodeName,
	PCWSTR          pServiceName,
	const ADDRINFOW *pHints,
	PADDRINFOW      *ppResult) {

	//std::wcout << "called 'WS2_32.dll@GetAddrInfoW' : " << pNodeName << std::endl;

	if (last_selected->serverID == 0)
		return fpsGetAddrInfoW(pNodeName, pServiceName, pHints, ppResult);

	return fpsGetAddrInfoW(server_host, pServiceName, pHints, ppResult);
}

CERTGETCERTIFICATECHAIN fpsCertGetCertificateChain = NULL;

void Hook_CertGetCertificateChain() {
	auto target = GetProcAddress(LoadLibrary(L"crypt32.dll"), "CertGetCertificateChain");
	MH_CreateHook(target, &DetourCertGetCertificateChain, (LPVOID*)&fpsCertGetCertificateChain);
	MH_EnableHook(target);
	//std::cout << "created hook for 'crypt32.dll@CertGetCertificateChain'" << std::endl;
}

BOOL __stdcall DetourCertGetCertificateChain(HCERTCHAINENGINE arg0, PCCERT_CONTEXT arg1, LPFILETIME arg2, HCERTSTORE arg3, PCERT_CHAIN_PARA arg4, DWORD arg5, LPVOID arg6, CERT_CHAIN_CONTEXT** arg7) {
	//std::cout << "called 'crypt32.dll@CertGetCertificateChain'" << std::endl;
	
	auto ret = fpsCertGetCertificateChain(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);

	if (last_selected->serverID != 0)
		(*arg7)->TrustStatus.dwErrorStatus = 0;

	return ret;
}
