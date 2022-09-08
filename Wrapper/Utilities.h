#pragma once

#include "Unmanaged.h"
#include <shlobj.h>
#include <strsafe.h>
#include <shlwapi.h>

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Default usage type flag to use with our IFileIsInUse implementation
#define FUT_DEFAULT FUT_EDITING

// Default capability flags to use with our IFileIsInUse implementation
#define OF_CAP_DEFAULT  OF_CAP_CANCLOSE | OF_CAP_CANSWITCHTO

class FileIsInUse : public IFileIsInUse
{
public:
    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // IFileIsInUse
    IFACEMETHODIMP GetAppName(PWSTR* ppszName);
    IFACEMETHODIMP GetUsage(FILE_USAGE_TYPE* pfut);
    IFACEMETHODIMP GetCapabilities(DWORD* pdwCapabilitiesFlags);
    IFACEMETHODIMP GetSwitchToHWND(HWND* phwnd);
    IFACEMETHODIMP CloseFile();

    static HRESULT CreateInstance(HWND hwnd, PCWSTR pszFilePath, FILE_USAGE_TYPE fut, DWORD dwCapabilities, REFIID riid, void** ppv);
    FileIsInUse();


private:
    ~FileIsInUse();

    HRESULT _Initialize(HWND hwnd, PCWSTR pszFilePath, FILE_USAGE_TYPE fut, DWORD dwCapabilities);
    HRESULT _AddFileToROT();
    HRESULT _RemoveFileFromROT();

    long _cRef;
    WCHAR _szFilePath[MAX_PATH];
    HWND _hwnd;
    DWORD _dwCapabilities;
    DWORD _dwCookie;
    FILE_USAGE_TYPE _fut;
};

FileIsInUse::FileIsInUse() : _cRef(1), _hwnd(NULL), _fut(FUT_GENERIC), _dwCapabilities(0), _dwCookie(0) { _szFilePath[0] = '\0'; }
FileIsInUse::~FileIsInUse() { _RemoveFileFromROT(); }
ULONG FileIsInUse::AddRef() { return InterlockedIncrement(&_cRef); }

HRESULT FileIsInUse::_Initialize(HWND hwnd, PCWSTR pszFilePath, FILE_USAGE_TYPE fut, DWORD dwCapabilities)
{
    _hwnd = hwnd;
    _fut = fut;
    _dwCapabilities = dwCapabilities;
    HRESULT hr = StringCchCopy(_szFilePath, ARRAYSIZE(_szFilePath), pszFilePath);
    if (SUCCEEDED(hr))
    {
        hr = _AddFileToROT();
    }
    return hr;
}

HRESULT FileIsInUse::CreateInstance(HWND hwnd, PCWSTR pszFilePath, FILE_USAGE_TYPE fut, DWORD dwCapabilities, REFIID riid, void** ppv)
{
    FileIsInUse* pfiu = new (std::nothrow) FileIsInUse();
    HRESULT hr = (pfiu) ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pfiu->_Initialize(hwnd, pszFilePath, fut, dwCapabilities);
        if (SUCCEEDED(hr))
        {
            hr = pfiu->QueryInterface(riid, ppv);
        }
        pfiu->Release();
    }
    return hr;
}

HRESULT FileIsInUse::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(FileIsInUse, IFileIsInUse),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG FileIsInUse::Release()
{
    ULONG cRef = InterlockedDecrement(&_cRef);
    if (!cRef)
    {
        delete this;
    }
    return cRef;
}

// IFileIsInUse

HRESULT FileIsInUse::CloseFile()
{
    _RemoveFileFromROT();
    return S_OK;
}

HRESULT FileIsInUse::GetAppName(PWSTR* ppszName)
{
    HRESULT hr = E_FAIL;
    WCHAR szModule[MAX_PATH];
    UINT cch = GetModuleFileName(NULL, szModule, ARRAYSIZE(szModule));
    if (cch != 0)
    {
        hr = SHStrDup(PathFindFileName(szModule), ppszName);
    }
    return hr;
}

HRESULT FileIsInUse::GetUsage(FILE_USAGE_TYPE* pfut)
{
    *pfut = _fut;
    return S_OK;
}

HRESULT FileIsInUse::GetCapabilities(DWORD* pdwCapabilitiesFlags)
{
    *pdwCapabilitiesFlags = _dwCapabilities;
    return S_OK;
}

HRESULT FileIsInUse::GetSwitchToHWND(HWND* phwnd)
{
    *phwnd = _hwnd;
    return S_OK;
}

HRESULT FileIsInUse::_AddFileToROT()
{
    IRunningObjectTable* prot;
    HRESULT hr = GetRunningObjectTable(NULL, &prot);
    if (SUCCEEDED(hr))
    {
        IMoniker* pmk;
        hr = CreateFileMoniker(_szFilePath, &pmk);
        if (SUCCEEDED(hr))
        {
            // Add ROTFLAGS_ALLOWANYCLIENT to make this work accross security boundaries
            hr = prot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE | ROTFLAGS_ALLOWANYCLIENT,
                static_cast<IFileIsInUse*>(this), pmk, &_dwCookie);
            if (hr == CO_E_WRONG_SERVER_IDENTITY)
            {
                // this failure is due to ROTFLAGS_ALLOWANYCLIENT and the fact that we don't
                // have the AppID registered for our CLSID. Try again without ROTFLAGS_ALLOWANYCLIENT
                // knowing that this means this can only work in the scope of apps running with the
                // same MIC level.
                hr = prot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE,
                    static_cast<IFileIsInUse*>(this), pmk, &_dwCookie);
            }
            pmk->Release();
        }
        prot->Release();
    }
    return hr;
}

HRESULT FileIsInUse::_RemoveFileFromROT()
{
    IRunningObjectTable* prot;
    HRESULT hr = GetRunningObjectTable(NULL, &prot);
    if (SUCCEEDED(hr))
    {
        if (_dwCookie)
        {
            hr = prot->Revoke(_dwCookie);
            _dwCookie = 0;
        }

        prot->Release();
    }
    return hr;
}

HRESULT _RegisterThisAppRunAsInteractiveUser(PCWSTR pszCLSID)
{
    HRESULT hr = E_INVALIDARG;

    WCHAR szModule[MAX_PATH];
    if (GetModuleFileName(NULL, szModule, ARRAYSIZE(szModule)))
    {
        WCHAR szKey[MAX_PATH];
        hr = StringCchPrintf(szKey, ARRAYSIZE(szKey), L"Software\\Classes\\AppID\\%s", PathFindFileName(szModule));
        if (SUCCEEDED(hr))
        {
            HKEY hk;
            LSTATUS ls = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, NULL, 0, KEY_WRITE, NULL, &hk, NULL);
            hr = HRESULT_FROM_WIN32(ls);
            if (SUCCEEDED(hr))
            {
                RegSetValueEx(hk, L"AppID", 0, REG_SZ, (BYTE*)pszCLSID, sizeof(*pszCLSID) * (lstrlen(pszCLSID) + 1));
                RegCloseKey(hk);

                hr = StringCchPrintf(szKey, ARRAYSIZE(szKey), L"Software\\Classes\\AppID\\%s", pszCLSID);
                if (SUCCEEDED(hr))
                {
                    ls = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, NULL, 0, KEY_WRITE, NULL, &hk, NULL);
                    hr = HRESULT_FROM_WIN32(ls);
                    if (SUCCEEDED(hr))
                    {
                        RegSetValueEx(hk, L"RunAs", 0, REG_SZ, (BYTE*)L"Interactive User", sizeof(L"Interactive User"));
                        RegCloseKey(hk);
                    }
                }
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}