#include "pch.h"
#include "screenshot_manager.h"
#include "screenshot_editor.h"
#include "cfgmgr.h"
#include "util.h"
#include "dynarray.h"

// WIC is currently disabled for VS 2005 builds since the wincodec header cannot be used with it.
#if defined(_UNICODE) && (defined(_MSVC_LANG) && _MSVC_LANG >= 201103L || !defined(_MSVC_LANG) && __cplusplus >= 201103L)
#define COMPILETIME_ENABLE_WIC
#endif

#ifdef COMPILETIME_ENABLE_WIC
    #include <wincodec.h>
#endif

struct KeyboardShortcut
{
    UINT uiModifiers;
    UINT uiVirtualKey;
};

static HRESULT ParseKeyboardShortcutString(const TCHAR *psz, KeyboardShortcut *pShortcut)
{
    static struct KeyMap
    {
        const UINT uiVirtualKey;
        const TCHAR sz[12];
    };

    static constexpr KeyMap rgModifierMap[] = {
        { MOD_CONTROL,               TEXT("control") },
        { MOD_CONTROL,               TEXT("ctrl") },
        { MOD_CONTROL | MOD_LEFT,    TEXT("lcontrol") },
        { MOD_CONTROL | MOD_RIGHT,   TEXT("rcontrol") },
        { MOD_CONTROL | MOD_LEFT,    TEXT("lctrl") },
        { MOD_CONTROL | MOD_RIGHT,   TEXT("rctrl") },
        { MOD_ALT,                   TEXT("alt") },
        { MOD_ALT,                   TEXT("menu") },
        { MOD_ALT | MOD_LEFT,        TEXT("lalt") },
        { MOD_ALT | MOD_RIGHT,       TEXT("ralt") },
        { MOD_ALT | MOD_LEFT,        TEXT("lmenu") },
        { MOD_ALT | MOD_RIGHT,       TEXT("rmenu") },
        { MOD_SHIFT,                 TEXT("shift") },
        { MOD_SHIFT | MOD_LEFT,      TEXT("lshift") },
        { MOD_SHIFT | MOD_RIGHT,     TEXT("rshift") },
        { MOD_WIN,                   TEXT("win") },
        { MOD_WIN | MOD_LEFT,        TEXT("lwin") },
        { MOD_WIN | MOD_RIGHT,       TEXT("rwin") },
        { 0 },
    };

    static constexpr KeyMap rgNameMap[] = {
        { VK_CANCEL,      TEXT("cancel") },
        { VK_BACK,        TEXT("back") },
        { VK_BACK,        TEXT("backspace") },
        { VK_TAB,         TEXT("tab") },
        { VK_CLEAR,       TEXT("clear") },
        { VK_RETURN,      TEXT("return") },
        { VK_RETURN,      TEXT("enter") },
        { VK_PAUSE,       TEXT("pause") },
        { VK_CAPITAL,     TEXT("capital") },
        { VK_CAPITAL,     TEXT("caps") },
        { VK_CAPITAL,     TEXT("capslock") },
        { VK_CAPITAL,     TEXT("capslk") },
        { VK_KANA,        TEXT("kana") },
        { VK_HANGUL,      TEXT("hangul") },
        { VK_JUNJA,       TEXT("junja") },
        { VK_FINAL,       TEXT("final") },
        { VK_HANJA,       TEXT("hanja") },
        { VK_KANJI,       TEXT("kanji") },
        { VK_ESCAPE,      TEXT("escape") },
        { VK_ESCAPE,      TEXT("esc") },
        { VK_CONVERT,     TEXT("convert") },
        { VK_NONCONVERT,  TEXT("nonconvert") },
        { VK_ACCEPT,      TEXT("accept") },
        { VK_MODECHANGE,  TEXT("modechange") },
        { VK_SPACE,       TEXT("space") },
        { VK_SPACE,       TEXT("spc") },
        { VK_PRIOR,       TEXT("prior") },
        { VK_PRIOR,       TEXT("pageup") },
        { VK_NEXT,        TEXT("next") },
        { VK_NEXT,        TEXT("pagedown") },
        { VK_END,         TEXT("end") },
        { VK_HOME,        TEXT("home") },
        { VK_LEFT,        TEXT("left") },
        { VK_UP,          TEXT("up") },
        { VK_RIGHT,       TEXT("right") },
        { VK_DOWN,        TEXT("down") },
        { VK_SELECT,      TEXT("select") },
        { VK_PRINT,       TEXT("select") },
        { VK_EXECUTE,     TEXT("execute") },
        { VK_SNAPSHOT,    TEXT("snapshot") },
        { VK_SNAPSHOT,    TEXT("printscreen") },
        { VK_SNAPSHOT,    TEXT("prtscr") },
        { VK_INSERT,      TEXT("insert") },
        { VK_DELETE,      TEXT("delete") },
        { VK_DELETE,      TEXT("del") },
        { VK_HELP,        TEXT("help") },
        { 0 },
    };

    size_t cb = sizeof(TCHAR) * _tcslen(psz);
    TCHAR *pszCopy = new TCHAR[cb];
    _tcscpy_s(pszCopy, cb, psz);

    TCHAR *pszContext = nullptr;
    TCHAR *pszToken = _tcstok_s(pszCopy, TEXT("+"), &pszContext);

    UINT uiModifiers = 0;
    UINT uiVirtualKey = 0;

    while (pszToken != nullptr)
    {
        DBGPRINT(TEXT("Before trim: %s"), pszToken);
        _tcstrim(pszToken);
        DBGPRINT(TEXT("After trim: %s"), pszToken);

        bool fHandled = false;

        FOR_EACH(KeyMap km, rgModifierMap)
        {
            if (km.sz && _tcsicmp(pszToken, km.sz) == 0)
            {
                fHandled = true;
                uiModifiers |= km.uiVirtualKey;
                break;
            }
        }

        if (!fHandled)
        {
            FOR_EACH(KeyMap km, rgNameMap)
            {
                if (km.sz && _tcsicmp(pszToken, km.sz) == 0)
                {
                    fHandled = true;
                    uiVirtualKey = km.uiVirtualKey;
                    break;
                }
            }

            if (!fHandled)
            {
                if (!_tcslen(pszToken) != 1)
                {
                    // This is an error case. The main key should only ever be one
                    // character long.
                }

                uiVirtualKey = pszToken[0];
            }
        }

        DBGPRINT(TEXT("uiModifiers: %d"), uiModifiers);
        DBGPRINT(TEXT("uiVirtualKey: %c"), uiVirtualKey);

        pszToken = _tcstok_s(nullptr, TEXT("+"), &pszContext);
    }

    delete[] pszCopy;

    pShortcut->uiModifiers = uiModifiers;
    pShortcut->uiVirtualKey = uiVirtualKey;
    return S_OK;
}

HRESULT RegisterScreenshotShortcut()
{
    TCHAR *pszShortcut = nullptr;
    bool fIsAllocated = false;
    if (FAILED(CConfigManager::GetInstance()->GetString(TEXT("ShortcutKey"), &pszShortcut)))
    {
        pszShortcut = TEXT("Win+Shift+S");
    }
    else
    {
        fIsAllocated = true;
    }

    HRESULT hr = E_FAIL;

    KeyboardShortcut ks = { 0 };
    hr = ParseKeyboardShortcutString(pszShortcut, &ks);
    if (SUCCEEDED(hr))
    {
        BOOL fResult = RegisterHotKey(nullptr, ID_HOTKEY_SCREENSHOT, ks.uiModifiers, ks.uiVirtualKey);
        hr = fResult ? S_OK : E_FAIL;
    }

    if (fIsAllocated)
        CoTaskMemFree(pszShortcut);

    return hr;
}

static DWORD WINAPI ScreenshotWindowThreadProc(void *lpParameter)
{
    CScreenshotContext *pScreenshotCtx = (CScreenshotContext *)lpParameter;
    CScreenshotEditorWindow *pScreenshotWnd = CScreenshotEditorWindow::CreateAndShow(pScreenshotCtx);
    if (!pScreenshotWnd)
    {
        MessageBox(nullptr,
            TEXT("Failed to create the screenshot editor window."),
            TEXT("screenkirk"),
            MB_OK | MB_ICONERROR
        );
        delete pScreenshotCtx;
        return 1;
    }

    MSG msg = { 0 };
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(nullptr, nullptr, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

void OnScreenshotKeyPressed()
{
    CScreenshotContext *pScreenshotCtx = nullptr;

    HRESULT hr = TakeScreenshot(&pScreenshotCtx);
    if (SUCCEEDED(hr))
    {
        HANDLE hThread = CreateThread(nullptr, 0, ScreenshotWindowThreadProc, pScreenshotCtx, 0, nullptr);
        if (!hThread)
        {
            MessageBox(nullptr,
                TEXT("Failed to create thread for the screenshot editor window."),
                TEXT("screenkirk"),
                MB_OK | MB_ICONERROR
            );
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
    
    pContext->_ptVirtualScreen.x = xDesktop;
    pContext->_ptVirtualScreen.y = yDesktop;
    pContext->_sizeDesktop.cx = cxDesktop;
    pContext->_sizeDesktop.cy = cyDesktop;

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
        union
        {
            // When the time comes, this will need to be freed when this fucker is
            // killed.
            IScreenshotEditorExtension *pOwnerExtension;

            GUID guidWicContainerFormat;
        } providerData;
    };

    struct SupportedExtension
    {
        TCHAR szExtension[64];
        CDynamicArray<UINT> vuiOffsetProviders;
    };
    
    struct FilterItem
    {
        TCHAR szDisplayName[MAX_PATH];
        SupportedExtension extension;
    };

    HBITMAP _hbm;
    CDynamicArray<CodecInfo> _vCodecInfo;

#ifdef COMPILETIME_ENABLE_WIC
    IWICImagingFactory *_pWicFactory = nullptr;
#endif

    HRESULT _GetFilterExtensions(CDynamicArray<SupportedExtension> *pvse);
    HRESULT _GetFilterItemList(CDynamicArray<FilterItem> *pvfi);

    // Caller must free the string.
    HRESULT _BuildFilterString(CDynamicArray<FilterItem> *pvfi, TCHAR **ppszOut);

    HRESULT _GetFileTypeName(const TCHAR *pszExtension, TCHAR *pszOut, int cch);
    HRESULT _GetLocalizedAllFilesString(TCHAR *pszOut, int cch);

    HRESULT _FetchWicCodecs();

    HRESULT _EncodeImage(char **ppcData, int *pcbData, const TCHAR *pszExtension, FilterItem *pfi);
    HRESULT _EncodeBitmap(char **ppcData, int *pcbData);
    HRESULT _EncodeWIC(GUID *pEncoderGuid, char **ppcData, int *pcbData);

public:
    CSaveImage(HBITMAP hbm)
        : _hbm(hbm)
    {
    }

    ~CSaveImage();

    HRESULT Initialize();
    HRESULT OpenSaveDialog();
    inline bool IsWicAvailable()
    {
#ifdef COMPILETIME_ENABLE_WIC
        return _pWicFactory != nullptr;
#else
        // Non Unicode builds target Windows 9x, which does not support WIC.
        return false;
#endif
    }

    HRESULT FetchSupportedCodecs();
};

CSaveImage::~CSaveImage()
{
#ifdef COMPILETIME_ENABLE_WIC
    if (_pWicFactory)
        _pWicFactory->Release();
#endif
}

HRESULT CSaveImage::Initialize()
{
#ifdef COMPILETIME_ENABLE_WIC
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
    ofn.hwndOwner = FindWindow(CScreenshotEditorWindow::GetWindowClass(), nullptr); // lazy

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

        hr = E_ABORT;
    }
    else
    {
        FilterItem *pSelectedFilterItem = &vFilterItems.At(ofn.nFilterIndex - 1);

        const TCHAR *pszExtension = PathFindFileExtension(szFileName);
        char *pcData = nullptr;
        int cbData = 0;
        if (FAILED(_EncodeImage(&pcData, &cbData, pszExtension, pSelectedFilterItem)))
        {
            return E_FAIL;
        }

        HANDLE hf = CreateFile(szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hf != INVALID_HANDLE_VALUE)
        {
            DWORD dwBytesWritten = 0;
            WriteFile(hf, pcData, cbData, &dwBytesWritten, nullptr);
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        delete[] pcData;
        CloseHandle(hf);
    }

    delete[] pszFilter;
    return hr;
}

HRESULT CSaveImage::FetchSupportedCodecs()
{
    _vCodecInfo.Clear();

    // Built-in BMP codec:
    CodecInfo bmpCodec;
    bmpCodec.provider = CP_BUILTIN;
    _tcscpy_s(bmpCodec.szName, TEXT("Windows Bitmap Codec"));
    _tcscpy_s(bmpCodec.szExtensions, TEXT(".bmp"));
    _vCodecInfo.Push(bmpCodec);

#ifdef COMPILETIME_ENABLE_WIC
    // If WIC is available, then load all codecs from it.
    _FetchWicCodecs();
#endif

    return S_OK;
}

HRESULT CSaveImage::_GetFilterExtensions(CDynamicArray<SupportedExtension> *pvse)
{
    FOR_EACH_DYNARR(CodecInfo &ci, _vCodecInfo)
    {
        TCHAR szExtensionList[MAX_PATH] = { 0 };
        _tcscpy_s(szExtensionList, ci.szExtensions);

        TCHAR *pszContext = nullptr;
        TCHAR *pszToken = _tcstok_s(szExtensionList, TEXT(","), &pszContext);
        while (pszToken != nullptr)
        {
            UINT uiIndex = i;
            bool fHandled = false;

            // Does this extension already exist in the map?
            FOR_EACH_DYNARR(SupportedExtension &se, *pvse)
            {
                if (_tcscmp(se.szExtension, pszToken) == 0)
                {
                    // Then add this provider to the existing supported extension.
                    se.vuiOffsetProviders.Push(uiIndex);
                    fHandled = true;
                }
            }

            // Otherwise, add a new entry to the map.
            if (!fHandled)
            {
                SupportedExtension se = { 0 };
                _tcscpy_s(se.szExtension, pszToken);
                se.vuiOffsetProviders.Push(uiIndex);
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

    FOR_EACH_DYNARR(SupportedExtension &se, vse)
    {
        FilterItem fi = { 0 };
        fi.extension = se;

        TCHAR szTypeDisplayName[MAX_PATH] = { 0 };
        if (SUCCEEDED(_GetFileTypeName(se.szExtension, szTypeDisplayName, ARRAYSIZE(szTypeDisplayName))))
        {
            _stprintf_s(fi.szDisplayName, TEXT("%s (%s)"), szTypeDisplayName, fi.extension.szExtension);
        }
        else
        {
            _tcscpy_s(fi.szDisplayName, fi.extension.szExtension);
        }

        pvfi->Push(fi);
    }

    // All items:
    FilterItem fiAllItems = { 0 };
    _tcscpy_s(fiAllItems.extension.szExtension, TEXT("*.*"));

    TCHAR szTypeDisplayName[MAX_PATH] = { 0 };
    if (SUCCEEDED(_GetLocalizedAllFilesString(szTypeDisplayName, ARRAYSIZE(szTypeDisplayName))))
    {
        _tcscpy_s(fiAllItems.szDisplayName, szTypeDisplayName);
    }
    else
    {
        _tcscpy_s(fiAllItems.szDisplayName, fiAllItems.extension.szExtension);
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
    FOR_EACH_DYNARR(FilterItem &fi, *pvfi)
    {
        cchNeeded += (_tcslen(fi.szDisplayName) + 1);
        cchNeeded += (_tcslen(fi.extension.szExtension) + 1);
    }

    // For the final zero terminator.
    cchNeeded += 1;

    // 2. Suffer.
    *ppszOut = new TCHAR[cchNeeded];
    TCHAR *ppszCur = *ppszOut;
    ZeroMemory(*ppszOut, cchNeeded * sizeof(TCHAR));

    bool fIsEmpty = true;
    FOR_EACH_DYNARR(FilterItem &fi, *pvfi)
    {
        // The compiler really wants me to use the "safe" version of this function, but we already
        // know the buffer size and it would be pointless to keep recalculating it. The buffer was
        // allocated so that everything can fit.
        _tcscpy_s(ppszCur, _tcslen(fi.szDisplayName) + 1, fi.szDisplayName);
        ppszCur += _tcslen(ppszCur) + 1;

        _tcscpy_s(ppszCur, _tcslen(fi.extension.szExtension) + 1, fi.extension.szExtension);
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

    typedef HRESULT (WINAPI *SHLoadIndirectString_t)(PCTSTR pszSource, PTSTR pszOutBuf, UINT cchOutBuf, void **ppvReserved);
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

#ifdef COMPILETIME_ENABLE_WIC
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
                CodecInfo ci;
                ci.provider = CP_WIC;
                _tcscpy_s(ci.szName, szFriendlyName);
                _tcscpy_s(ci.szExtensions, szExtensions);
                pCodecInfo->GetContainerFormat(&ci.providerData.guidWicContainerFormat);
                _vCodecInfo.Push(ci);

                DBGPRINT(TEXT("Fetched codec \"%s\" with provided extensions \"%s\""), szFriendlyName, szExtensions);

                pCodecInfo->Release();
            }

            pUnk->Release();
        }

        pEnum->Release();
    }

    return hr;
#else
    return E_NOTIMPL;
#endif
}

HRESULT CSaveImage::_EncodeImage(char **ppcData, int *pcbData, const TCHAR *pszExtension, FilterItem *pfi)
{
    if (!ppcData || !pcbData || !pszExtension || !pfi)
        return E_POINTER;

    FOR_EACH_DYNARR(int idx, pfi->extension.vuiOffsetProviders)
    {
        CodecInfo *pci = &_vCodecInfo[idx];

        if (pci->provider == CP_EXTENSION)
        {
            // TODO: Extension API here is not yet planned.
        }
        else if (pci->provider == CP_WIC)
        {
            return _EncodeWIC(&pci->providerData.guidWicContainerFormat, ppcData, pcbData);
        }
        else if (pci->provider == CP_BUILTIN)
        {
            // This only supports bmp at the moment.
            return _EncodeBitmap(ppcData, pcbData);
        }
    }

    // There is no good provider for the selected extension.
    return E_FAIL;
}

HRESULT CSaveImage::_EncodeBitmap(char **ppcData, int *pcbData)
{
    BITMAP bm;
    if (!GetObject(_hbm, sizeof(bm), &bm))
    {
        return E_FAIL;
    }

    HDC hdc = GetDC(HWND_DESKTOP);

    BITMAPINFOHEADER bih = { sizeof(bih) };
    bih.biWidth = bm.bmWidth;
    bih.biHeight = bm.bmHeight;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = bm.bmWidth * bm.bmHeight * 4;

    HRESULT hr = E_FAIL;
    char *pcDIBits = new (std::nothrow) char[bih.biSizeImage];
    if (pcDIBits)
    {
        BITMAPINFO bmi;
        bmi.bmiHeader = bih;
        if (GetDIBits(hdc, _hbm, 0, (UINT)bm.bmHeight, pcDIBits, &bmi, DIB_RGB_COLORS))
        {
            BITMAPFILEHEADER bfh;
            bfh.bfType = 0x4D42; // "BM"
            bfh.bfSize = sizeof(bfh) + sizeof(bih) + bih.biSizeImage;
            bfh.bfReserved1 = bfh.bfReserved2 = 0;
            bfh.bfOffBits = sizeof(bfh) + sizeof(bih);

            // Write the data into the data buffer:
            *ppcData = new char[bfh.bfSize];
            *pcbData = bfh.bfSize;
            memcpy_s(*ppcData, bfh.bfSize, &bfh, sizeof(bfh));
            memcpy_s((*ppcData) + sizeof(bfh), bfh.bfSize - sizeof(bfh), &bih, sizeof(bih));
            memcpy_s((*ppcData) + sizeof(bfh) + sizeof(bih), bfh.bfSize - sizeof(bfh) - sizeof(bih), pcDIBits, bih.biSizeImage);

            hr = S_OK;
        }

        delete[] pcDIBits;
    }
    ReleaseDC(HWND_DESKTOP, hdc);

    return hr;
}

HRESULT CSaveImage::_EncodeWIC(GUID *pEncoderGuid, char **ppcData, int *pcbData)
{
#ifdef COMPILETIME_ENABLE_WIC
    CComPtr<IWICBitmap> spBitmap;
    HRESULT hr = _pWicFactory->CreateBitmapFromHBITMAP(_hbm, nullptr, WICBitmapIgnoreAlpha, &spBitmap);
    
    if (FAILED(hr))
        return hr;

    CComPtr<IStream> spMemStream;
    hr = CreateStreamOnHGlobal(nullptr, TRUE, &spMemStream);
    if (FAILED(hr))
        return hr;

    CComPtr<IWICStream> spStream;
    hr = _pWicFactory->CreateStream(&spStream);
    if (FAILED(hr))
        return hr;

    hr = spStream->InitializeFromIStream(spMemStream.Get());
    if (FAILED(hr))
        return hr;

    CComPtr<IWICBitmapEncoder> spEncoder;
    hr = _pWicFactory->CreateEncoder(*pEncoderGuid, nullptr, &spEncoder);
    if (FAILED(hr))
        return hr;

    hr = spEncoder->Initialize(spStream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr))
        return hr;

    CComPtr<IWICBitmapFrameEncode> spFrameEncode;
    CComPtr<IPropertyBag2> spPropertyBag;
    hr = spEncoder->CreateNewFrame(&spFrameEncode, &spPropertyBag);
    if (FAILED(hr))
        return hr;

    hr = spFrameEncode->Initialize(spPropertyBag.Get());
    if (FAILED(hr))
        return hr;

    UINT uiWidth = 0;
    UINT uiHeight = 0;
    hr = spBitmap->GetSize(&uiWidth, &uiHeight);
    if (FAILED(hr))
        return hr;

    hr = spFrameEncode->SetSize(uiWidth, uiHeight);
    if (FAILED(hr))
        return hr;

    WICPixelFormatGUID formatGuid;
    hr = spBitmap->GetPixelFormat(&formatGuid);
    if (FAILED(hr))
        return hr;

    hr = spFrameEncode->SetPixelFormat(&formatGuid);
    if (FAILED(hr))
        return hr;

    hr = spFrameEncode->WriteSource(spBitmap.Get(), nullptr);
    if (FAILED(hr))
        return hr;

    hr = spFrameEncode->Commit();
    if (FAILED(hr))
        return hr;

    hr = spEncoder->Commit();
    if (SUCCEEDED(hr))
    {
        HGLOBAL hGlobal = nullptr;
        hr = GetHGlobalFromStream(spMemStream.Get(), &hGlobal);
        if (SUCCEEDED(hr))
        {
            size_t cbStream = GlobalSize(hGlobal);
            if (cbStream > 0)
            {
                ULARGE_INTEGER streamPos = { 0 };
                LARGE_INTEGER zeroSeek = { 0 };
                hr = spMemStream->Seek(zeroSeek, STREAM_SEEK_CUR, &streamPos);
                if (SUCCEEDED(hr))
                {
                    DWORD cbFinal = (DWORD)streamPos.QuadPart;

                    void *pBytes = GlobalLock(hGlobal);
                    if (pBytes)
                    {
                        char *pcBytes = new (std::nothrow) char[cbFinal];
                        if (pcBytes)
                        {
                            memcpy_s(pcBytes, cbFinal, pBytes, cbFinal);

                            *ppcData = pcBytes;
                            *pcbData = cbFinal;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    GlobalUnlock(hGlobal);
                }
            }
        }
    }

    return hr;
#else
    return E_NOTIMPL;
#endif
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
    CSaveImage si(_hbmModified);
    si.Initialize();

    // TEMPORARY -- I just need to get this called somehow so I can analyze
    // shit in real time.
    si.FetchSupportedCodecs();

    return si.OpenSaveDialog();
}
