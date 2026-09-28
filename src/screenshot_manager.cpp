#include "pch.h"
#include "screenshot_manager.h"
#include "screenshot_editor.h"
#include "util.h"
#include "dynarray.h"
#include <wincodec.h>

void OnScreenshotKeyPressed()
{
    CScreenshotContext *pScreenshotCtx = nullptr;

    HRESULT hr = TakeScreenshot(&pScreenshotCtx);
    if (SUCCEEDED(hr))
    {
        // TODO: Make window run in another thread.
        CScreenshotEditorWindow *pScreenshotWnd = CScreenshotEditorWindow::CreateAndShow(pScreenshotCtx);
        if (!pScreenshotWnd)
        {
            delete pScreenshotCtx;
        }
    }
}

HRESULT TakeScreenshot(OUT CScreenshotContext **ppContextOut)
{
    if (!ppContextOut)
        return E_INVALIDARG;

    CScreenshotContext *pContext = new CScreenshotContext();

    //
    // Take the screenshot of the desktop:
    //
    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    int cxDesktop = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int cyDesktop = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int xDesktop = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int yDesktop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    pContext->_ptVirtualScreen = { xDesktop, yDesktop };
    pContext->_sizeDesktop = { cxDesktop, cyDesktop };

    HDC hdcCopy = CreateCompatibleDC(hdcDesktop);
    pContext->_hbmScreenshot = CreateCompatibleBitmap(hdcDesktop, cxDesktop, cyDesktop);
    HGDIOBJ hObjOld = SelectObject(hdcCopy, pContext->_hbmScreenshot);
    BitBlt(hdcCopy, 0, 0, cxDesktop, cyDesktop, hdcDesktop, xDesktop, yDesktop, SRCCOPY);

    SelectObject(hdcCopy, hObjOld);
    DeleteDC(hdcCopy);
    ReleaseDC(HWND_DESKTOP, hdcDesktop);

    //
    // Get cursor information.
    //
    CURSORINFO ci = { sizeof(CURSORINFO) };
    if (GetCursorInfo(&ci))
    {
        pContext->_fCursorVisible = ci.flags & CURSOR_SHOWING;
        pContext->_ptCursor = ci.ptScreenPos;
        if (ci.flags & CURSOR_SHOWING)
        {
            ICONINFO ii = { 0 };
            if (GetIconInfo(ci.hCursor, &ii))
            {
                pContext->_hbmCursorColor = ii.hbmColor;
                pContext->_hbmCursorMask = ii.hbmMask;
            }
        }
    }

    *ppContextOut = pContext;
    return S_OK;
}

class CSaveImage
{
    enum CodecProvider
    {
        CP_BUILTIN,
        CP_WIC,
        CP_EXTENSION,
    };

    struct CodecInfo
    {
        CodecProvider provider;
        TCHAR szName[MAX_PATH];
        TCHAR szExtensions[MAX_PATH]; // Comma-separated list of extensions.
        IScreenshotEditorExtension *pOwnerExtension;
    };
    
    struct FilterItem
    {
        TCHAR szDisplayName[MAX_PATH];
        TCHAR szExtensions[MAX_PATH];
    };

    struct SupportedExtension
    {
        TCHAR szExtension[64];
        CDynamicArray<CodecInfo *> _vpProviders;
    };

    CDynamicArray<CodecInfo> _vCodecInfo;

#ifdef _UNICODE
    IWICImagingFactory *_pWicFactory = nullptr;
#endif

    HRESULT _GetFilterExtensions(CDynamicArray<SupportedExtension> *pvse);
    HRESULT _GetFilterItemList(CDynamicArray<FilterItem> *pvfi);

    // Caller must free the string.
    HRESULT _BuildFilterString(CDynamicArray<FilterItem> *pvfi, TCHAR **ppszOut);

    HRESULT _GetFileTypeName(const TCHAR *pszExtension, TCHAR *pszOut, int cch);
    HRESULT _GetLocalizedAllFilesString(TCHAR *pszOut, int cch);
    HRESULT _FetchWicCodecs();

public:
    CSaveImage()
    {
    }

    HRESULT Initialize();
    HRESULT OpenSaveDialog();
    inline bool IsWicAvailable()
    {
#ifdef _UNICODE
        return _pWicFactory != nullptr;
#else
        // Non Unicode builds target Windows 9x, which does not support WIC.
        return false;
#endif
    }

    HRESULT FetchSupportedCodecs();
};

HRESULT CSaveImage::Initialize()
{
#ifndef _WIN16
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // We don't care if the WIC factory fails to be created.
    (void)CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_pWicFactory));
#endif

    return S_OK;
}

HRESULT CSaveImage::OpenSaveDialog()
{
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = g_hinst;
    ofn.hwndOwner = FindWindow(c_szScreenshotEditorWindowClassName, nullptr); // lazy

    TCHAR szFileName[MAX_PATH];
    ZeroMemory(szFileName, MAX_PATH);
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;

    CDynamicArray<FilterItem> vFilterItems;
    HRESULT hr = _GetFilterItemList(&vFilterItems);
    if (FAILED(hr))
    {
        return hr;
    }

    TCHAR *pszFilter = nullptr;
    hr = _BuildFilterString(&vFilterItems, &pszFilter);
    if (FAILED(hr))
    {
        return hr;
    }

    ofn.lpstrFilter = pszFilter;
    ofn.nFilterIndex = 1;

    if (!GetSaveFileName(&ofn))
    {
        DWORD dwError = CommDlgExtendedError();
        if (dwError)
        {
            TCHAR szBuffer[MAX_PATH];
            _stprintf_s(szBuffer, TEXT("Failed to open save dialog. (%d)"), dwError);
            MessageBox(ofn.hwndOwner, szBuffer, TEXT("Error"), MB_OK | MB_ICONERROR);
        }
    }

    delete[] pszFilter;

    return S_OK;
}

HRESULT CSaveImage::FetchSupportedCodecs()
{
    _vCodecInfo.Clear();

    // Built-in BMP codec:
    CodecInfo bmpCodec = {};
    bmpCodec.provider = CP_BUILTIN;
    _tcscpy_s(bmpCodec.szName, TEXT("Windows Bitmap Codec"));
    _tcscpy_s(bmpCodec.szExtensions, TEXT(".bmp"));
    _vCodecInfo.Push(bmpCodec);

#ifdef _UNICODE
    // If WIC is available, then load all codecs from it.
    _FetchWicCodecs();
#endif

    return S_OK;
}

HRESULT CSaveImage::_GetFilterExtensions(CDynamicArray<SupportedExtension> *pvse)
{
    for (CodecInfo &ci : _vCodecInfo)
    {
        TCHAR szExtensionList[MAX_PATH] = {};
        _tcscpy_s(szExtensionList, ci.szExtensions);

        TCHAR *pszContext = nullptr;
        TCHAR *pszToken = _tcstok_s(szExtensionList, TEXT(","), &pszContext);
        while (pszToken != nullptr)
        {
            bool fHandled = false;

            // Does this extension already exist in the map?
            for (SupportedExtension &se : *pvse)
            {
                if (_tcscmp(se.szExtension, pszToken) == 0)
                {
                    // Then add this provider to the existing supported extension.
                    se._vpProviders.Push(&ci);
                    fHandled = true;
                }
            }

            // Otherwise, add a new entry to the map.
            if (!fHandled)
            {
                SupportedExtension se = {};
                _tcscpy_s(se.szExtension, pszToken);
                se._vpProviders.Push(&ci);
                pvse->Push(se);
            }

            pszToken = _tcstok_s(nullptr, TEXT(","), &pszContext);
        }
    }

    return S_OK;
}

HRESULT CSaveImage::_GetFilterItemList(CDynamicArray<FilterItem> *pvfi)
{
    CDynamicArray<SupportedExtension> vse;
    if (FAILED(_GetFilterExtensions(&vse)))
    {
        return E_FAIL;
    }

    for (SupportedExtension &se : vse)
    {
        FilterItem fi = {};
        _tcscpy_s(fi.szExtensions, se.szExtension);

        WCHAR szTypeDisplayName[MAX_PATH] = { 0 };
        if (SUCCEEDED(_GetFileTypeName(se.szExtension, szTypeDisplayName, ARRAYSIZE(szTypeDisplayName))))
        {
            _stprintf_s(fi.szDisplayName, TEXT("%s (%s)"), szTypeDisplayName, fi.szExtensions);
        }
        else
        {
            _tcscpy_s(fi.szDisplayName, fi.szExtensions);
        }

        pvfi->Push(fi);
    }

    // All items:
    FilterItem fiAllItems = {};
    _tcscpy_s(fiAllItems.szExtensions, TEXT("*.*"));

    WCHAR szTypeDisplayName[MAX_PATH] = { 0 };
    if (SUCCEEDED(_GetLocalizedAllFilesString(szTypeDisplayName, ARRAYSIZE(szTypeDisplayName))))
    {
        _tcscpy_s(fiAllItems.szDisplayName, szTypeDisplayName);
    }
    else
    {
        _tcscpy_s(fiAllItems.szDisplayName, fiAllItems.szExtensions);
    }

    pvfi->Push(fiAllItems);

    return S_OK;
}

HRESULT CSaveImage::_BuildFilterString(CDynamicArray<FilterItem> *pvfi, TCHAR **ppszOut)
{
    if (!pvfi || !ppszOut)
        return E_POINTER;

    // 1. Calculate the number of bytes necessary for the string.
    int cchNeeded = 0;
    for (FilterItem &fi : *pvfi)
    {
        cchNeeded += (_tcslen(fi.szDisplayName) + 1);
        cchNeeded += (_tcslen(fi.szExtensions) + 1);
    }

    // For the final zero terminator.
    cchNeeded += 1;

    // 2. Suffer.
    *ppszOut = new TCHAR[cchNeeded];
    TCHAR *ppszCur = *ppszOut;
    ZeroMemory(*ppszOut, cchNeeded * sizeof(TCHAR));

    bool fIsEmpty = true;
    for (FilterItem &fi : *pvfi)
    {
        // The compiler really wants me to use the "safe" version of this function, but we already
        // know the buffer size and it would be pointless to keep recalculating it. The buffer was
        // allocated so that everything can fit.
        _tcscpy_s(ppszCur, _tcslen(fi.szDisplayName) + 1, fi.szDisplayName);
        ppszCur += _tcslen(ppszCur) + 1;

        _tcscpy_s(ppszCur, _tcslen(fi.szExtensions) + 1, fi.szExtensions);
        ppszCur += _tcslen(ppszCur) + 1;

        fIsEmpty = false;
    }

    return S_OK;
}

HRESULT CSaveImage::_GetFileTypeName(const TCHAR *pszExtension, TCHAR *pszOut, int cch)
{
    if (!pszExtension || !pszOut)
        return E_POINTER;

    HRESULT hr = E_FAIL;

    typedef DWORD_PTR (WINAPI *SHGetFileInfo_t)(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO *psfi, UINT cbFileInfo, UINT uFlags);
    HMODULE hmShell32 = LoadLibrary(TEXT("shell32.dll"));
    if (hmShell32)
    {
        SHGetFileInfo_t pfnSHGetFileInfo = (SHGetFileInfo_t)GetProcAddress(hmShell32, "SHGetFileInfo" _AWSTR);
        if (pfnSHGetFileInfo)
        {
            SHFILEINFO fi = { 0 };

            DWORD_PTR result = pfnSHGetFileInfo(
                pszExtension, FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), 
                SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME
            );

            if (result != 0 && fi.szTypeName[0] != L'\0')
            {
                _tcscpy_s(pszOut, cch, fi.szTypeName);
                hr = S_OK;
            }
        }

        FreeLibrary(hmShell32);
    }

    return hr;
}

HRESULT CSaveImage::_GetLocalizedAllFilesString(TCHAR *pszOut, int cch)
{
    if (!pszOut)
        return E_POINTER;

    HRESULT hr = E_FAIL;

    typedef HRESULT (WINAPI *SHLoadIndirectString_t)(PCWSTR pszSource, PWSTR pszOutBuf, UINT cchOutBuf, void **ppvReserved);
    HMODULE hmShlwapi = LoadLibrary(TEXT("shlwapi.dll"));
    if (hmShlwapi)
    {
        SHLoadIndirectString_t pfnSHLoadIndirectString = (SHLoadIndirectString_t)GetProcAddress(hmShlwapi, "SHLoadIndirectString");
        if (pfnSHLoadIndirectString)
        {
            hr = pfnSHLoadIndirectString(TEXT("@comdlg32.dll,-10022"), pszOut, cch, nullptr);
        }

        FreeLibrary(hmShlwapi);
    }

    if (FAILED(hr))
    {
        _tcscpy_s(pszOut, cch, TEXT("All Files (*.*)"));
        hr = S_OK;
    }

    return hr;
}

HRESULT CSaveImage::_FetchWicCodecs()
{
    if (!IsWicAvailable())
    {
        DBGPRINT(TEXT("WIC is not available on this platform. Skipping..."));
        return E_NOTIMPL; // This result code works well enough.
    }

    IEnumUnknown *pEnum = nullptr;
    HRESULT hr = _pWicFactory->CreateComponentEnumerator(
        WICEncoder,
        WICComponentEnumerateDefault,
        &pEnum
    );

    if (SUCCEEDED(hr))
    {
        IUnknown *pUnk = nullptr;
        ULONG uFetched = 0;
        while (pEnum->Next(1, &pUnk, &uFetched) == S_OK)
        {
            IWICBitmapCodecInfo *pCodecInfo = nullptr;
            hr = pUnk->QueryInterface(IID_PPV_ARGS(&pCodecInfo));

            if (SUCCEEDED(hr))
            {
                UINT cchFriendlyName = 0;
                WCHAR szFriendlyName[MAX_PATH] = { 0 };
                pCodecInfo->GetFriendlyName(MAX_PATH, szFriendlyName, &cchFriendlyName);

                UINT cchExtensions = 0;
                WCHAR szExtensions[MAX_PATH] = { 0 };
                pCodecInfo->GetFileExtensions(MAX_PATH, szExtensions, &cchExtensions);

                // The file extensions are provided in a format that we already support, so we can just
                // make a codec out of that.
                CodecInfo ci = {};
                ci.provider = CP_WIC;
                _tcscpy_s(ci.szName, szFriendlyName);
                _tcscpy_s(ci.szExtensions, szExtensions);
                _vCodecInfo.Push(ci);

                DBGPRINT(TEXT("Fetched codec \"%s\" with provided extensions \"%s\""), szFriendlyName, szExtensions);

                pCodecInfo->Release();
            }

            pUnk->Release();
        }

        pEnum->Release();
    }

    return hr;
}

HRESULT CScreenshotContext::GetWindowPositions(OnGetWindowPositionsCB cb)
{
    return E_NOTIMPL;
}

HRESULT CScreenshotContext::_EnsureModificationBuffer()
{
    if (!_hbmModified)
    {
        return CopyBitmap(&_hbmModified, _hbmScreenshot);
    }

    return S_FALSE;
}

HRESULT CScreenshotContext::Crop(RECT *prcCrop)
{
    // If the rectangle isn't inbounds, then we will fail.
    if (prcCrop->left < 0 || prcCrop->top < 0
        || prcCrop->right > _sizeDesktop.cx || prcCrop->bottom > _sizeDesktop.cy)
    {
        return E_FAIL;
    }

    HRESULT hr = _EnsureModificationBuffer();

    if (FAILED(hr))
    {
        return hr;
    }

    return CopyBitmap(&_hbmModified, _hbmModified, prcCrop);
}

HRESULT CScreenshotContext::CopyToClipboard()
{
    OpenClipboard(nullptr);
    EmptyClipboard();

    SetClipboardData(CF_BITMAP, _hbmModified);
    
    CloseClipboard();
    return S_OK;
}

HRESULT CScreenshotContext::SaveToFile()
{
    CSaveImage si;
    si.Initialize();

    // TEMPORARY -- I just need to get this called somehow so I can analyze
    // shit in real time.
    si.FetchSupportedCodecs();

    si.OpenSaveDialog();

    return S_OK;
}
