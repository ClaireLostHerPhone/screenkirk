#pragma once
#include "pch.h"

struct OSVersion
{
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
#ifndef _UNICODE
    bool fIsNt;
#endif

    inline bool IsWindowsNT()
    {
#ifndef _UNICODE
        return fIsNt;
#else
        return true;
#endif
    }
};

OSVersion *GetOSVersion();
const TCHAR *PathFindFileName(const TCHAR *pszPath);
HRESULT PathPopFileName(TCHAR *pszPath);
HRESULT PathAppend(TCHAR *pszPath, const TCHAR *pszPath2);
HRESULT CopyBitmap(HBITMAP *phbmDest, HBITMAP hbmSrc, RECT *prcCrop = nullptr);
