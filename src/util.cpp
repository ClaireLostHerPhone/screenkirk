#include "pch.h"
#include "util.h"

//
// Operating system detection
//

OSVersion g_osVersion = { 0 };
OSVersion *GetOSVersion()
{
    if (g_osVersion.dwMajorVersion != 0)
        return &g_osVersion;

#ifndef _WIN16
    typedef struct _RTL_OSVERSIONINFOW {
        ULONG dwOSVersionInfoSize;
        ULONG dwMajorVersion;
        ULONG dwMinorVersion;
        ULONG dwBuildNumber;
        ULONG dwPlatformId;
        WCHAR szCSDVersion[128];
    } RTL_OSVERSIONINFOW;

    // RtlGetVersion is available since 5.0. If it is available, then the OS is guaranteed to be
    // NT.
    typedef LONG (WINAPI *RtlGetVersion_t)(RTL_OSVERSIONINFOW *lpVersionInformation);
    RtlGetVersion_t pfnRtlGetVersion = nullptr;
    HMODULE hmNtdll = GetModuleHandle(TEXT("ntdll.dll"));
    if (hmNtdll && (pfnRtlGetVersion = (RtlGetVersion_t)GetProcAddress(hmNtdll, "RtlGetVersion")))
    {
        RTL_OSVERSIONINFOW ovi = { sizeof(ovi) };
        if (pfnRtlGetVersion(&ovi) == 0)
        {
            g_osVersion.dwMajorVersion = ovi.dwMajorVersion;
            g_osVersion.dwMinorVersion = ovi.dwMinorVersion;
            g_osVersion.dwBuildNumber = ovi.dwBuildNumber;
#ifndef _UNICODE
            g_osVersion.fIsNt = true;
#endif
            return &g_osVersion;
        }
    }
#endif

#pragma warning(push)
#pragma warning(disable : 4996) // Disable deprecation warning when using Windows 8 or higher SDKs.
    OSVERSIONINFO osvi = { sizeof(osvi) };
    if (GetVersionEx(&osvi))
    {
        g_osVersion.dwMajorVersion = osvi.dwMajorVersion;
        g_osVersion.dwMinorVersion = osvi.dwMinorVersion;
        g_osVersion.dwBuildNumber = osvi.dwBuildNumber;
#ifndef _UNICODE
        g_osVersion.fIsNt = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT;
#endif
        return &g_osVersion;
    }
#pragma warning(pop)

    return nullptr;
}

//
// Path handling
//

const TCHAR *PathFindFileName(const TCHAR *pszPath)
{
    const TCHAR *pc = pszPath;

    for (const TCHAR *c = pszPath; *c; c++)
    {
        if (*c == TEXT('\\'))
        {
            pc = c + 1;
        }
    }

    return pc;
}

HRESULT PathPopFileName(TCHAR *pszPath)
{
    TCHAR *pszFileName = (TCHAR *)PathFindFileName(pszPath);
    _tcscpy_s(pszFileName, MAX_PATH - (pszFileName - pszPath), TEXT("\0"));
    return S_OK;
}

HRESULT PathAppend(TCHAR *pszPath, const TCHAR *pszPath2)
{
    if (pszPath[_tcslen(pszPath) - 1] != TEXT('\\'))
    {
        _tcscat_s(pszPath, MAX_PATH, TEXT("\\"));
    }

    _tcscat_s(pszPath, MAX_PATH, pszPath2);
    return S_OK;
}

//
// Image manipulation
//

HRESULT CopyBitmap(HBITMAP *phbmDest, HBITMAP hbmSrc, RECT *prcCrop)
{
    if (!phbmDest)
        return E_POINTER;

    HRESULT hr = E_FAIL;
    RECT rcCrop = { 0 };

    if (prcCrop)
    {
        rcCrop = *prcCrop;
    }
    else
    {
        BITMAP bm;
        if (!GetObject(hbmSrc, sizeof(BITMAP), &bm))
        {
            return E_FAIL;
        }
        rcCrop = { 0, 0, bm.bmWidth, bm.bmHeight };
    }

    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    HBITMAP hbmNew = nullptr;
    if (hdcDesktop)
    {
        HDC hdcSrc = CreateCompatibleDC(hdcDesktop);
        HDC hdcDst = CreateCompatibleDC(hdcDesktop);
        if (hdcSrc && hdcDst)
        {
            hbmNew = CreateCompatibleBitmap(hdcDesktop, RECTWIDTH(rcCrop), RECTHEIGHT(rcCrop));
            if (hbmNew)
            {
                HGDIOBJ hObjOldSrc = SelectObject(hdcSrc, hbmSrc);
                HGDIOBJ hObjOldDst = SelectObject(hdcDst, hbmNew);

                BitBlt(hdcDst, 0, 0, RECTWIDTH(rcCrop), RECTHEIGHT(rcCrop), hdcSrc, rcCrop.left, rcCrop.top, SRCCOPY);

                SelectObject(hdcDst, hObjOldDst);
                SelectObject(hdcSrc, hObjOldSrc);

                hr = S_OK;
            }
        }
        if (hdcSrc)
            DeleteDC(hdcSrc);
        if (hdcDst)
            DeleteDC(hdcDst);
        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

    if (hbmNew)
    {
        if (SUCCEEDED(hr))
        {
            *phbmDest = hbmNew;
        }
        else
        {
            DeleteObject(hbmNew);
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
