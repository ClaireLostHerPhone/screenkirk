#pragma once
#include "pch.h"

class CConfigManager
{
    HKEY _hkey;

public:
    static HRESULT CreateInstance();
    static CConfigManager *GetInstance();

    CConfigManager()
        : _hkey(nullptr)
    {
    }

    HRESULT Initialize();
    HRESULT GetBool(const TCHAR *pszName, OUT bool *pfOut);
    HRESULT GetString(const TCHAR *pszName, OUT TCHAR **ppszOut);
};