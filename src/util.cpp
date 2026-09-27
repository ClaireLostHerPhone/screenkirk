#include "pch.h"
#include "util.h"

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
