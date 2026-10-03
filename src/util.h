#pragma once
#include "pch.h"

enum OSVersionFlags
{
    OSVF_NONE = 0,
    OSVF_WINNT = 1 << 0,
    OSVF_WINE =  1 << 1,
};

struct OSVersion
{
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    int flags;

    inline bool IsWindowsNT()
    {
        return (flags & OSVF_WINNT);
    }
};

OSVersion *GetOSVersion();
void _tcstrim(TCHAR *psz);
const TCHAR *PathFindFileName(const TCHAR *pszPath);
const TCHAR *PathFindFileExtension(const TCHAR *pszPath);
HRESULT PathPopFileName(TCHAR *pszPath);
HRESULT PathAppend(TCHAR *pszPath, const TCHAR *pszPath2);
HRESULT CopyBitmap(HBITMAP *phbmDest, HBITMAP hbmSrc, RECT *prcCrop = nullptr);
