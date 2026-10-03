#include "pch.h"
#include "cfgmgr.h"

#define REGSTR_SCREENKIRK TEXT("SOFTWARE\\ClaireLostHerPhone\\screenkirk")

CConfigManager *g_pCfgMgrInst = nullptr;

HRESULT CConfigManager::Initialize()
{
    HKEY hkey = nullptr;
    LSTATUS ls = RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_SCREENKIRK, 0, KEY_READ, &hkey);
    if (ls != ERROR_SUCCESS)
    {
        ls = RegCreateKey(HKEY_CURRENT_USER, REGSTR_SCREENKIRK, &hkey);

        if (ls != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    _hkey = hkey;
    return S_OK;
}

HRESULT CConfigManager::GetString(const TCHAR *pszName, OUT TCHAR **ppszOut)
{
    if (!pszName || !ppszOut)
        return E_POINTER;

    DWORD dwcch;
    LSTATUS ls = RegGetValue(_hkey, nullptr, pszName, RRF_RT_REG_SZ, nullptr, nullptr, &dwcch);

    if (ls == ERROR_SUCCESS)
    {
        *ppszOut = (TCHAR *)CoTaskMemAlloc((dwcch + 1) * sizeof(TCHAR));
        ls = RegGetValue(_hkey, nullptr, pszName, RRF_RT_REG_SZ, nullptr, *ppszOut, &dwcch);
        if (ls == ERROR_SUCCESS)
        {
            return S_OK;
        }

        CoTaskMemFree(ppszOut);
        return E_FAIL;
    }

    return E_NOT_SET;
}

// static
HRESULT CConfigManager::CreateInstance()
{
    g_pCfgMgrInst = new (std::nothrow) CConfigManager();
    if (g_pCfgMgrInst)
    {
        HRESULT hr = g_pCfgMgrInst->Initialize();
        if (SUCCEEDED(hr))
        {
            DBGPRINT(TEXT("Initialized config manager."));
            return S_OK;
        }

        return hr;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

// static
CConfigManager *CConfigManager::GetInstance()
{
    return g_pCfgMgrInst;
}
