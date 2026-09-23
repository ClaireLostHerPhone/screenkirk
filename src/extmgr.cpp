#include "pch.h"
#include "dynarray.h"

using DllGetClassObject_t = decltype(&DllGetClassObject);

class CLoadedExtension
{
public:
    HMODULE _hmod;
    IScreenshotEditorExtension *_pExt;
    const TCHAR *_pszName;
    const TCHAR *_pszVersionStr;
    const TCHAR *_pszAuthor;

    ~CLoadedExtension();
};

CLoadedExtension::~CLoadedExtension()
{
    if (_pszName)
        CoTaskMemFree((void *)_pszName);
    if (_pszVersionStr)
        CoTaskMemFree((void *)_pszVersionStr);
    if (_pszAuthor)
        CoTaskMemFree((void *)_pszAuthor);

    if (_pExt)
        _pExt->Release();
}

class CExtensionManager
{
    CDynamicArray<CLoadedExtension> _vLoadedExts;

public:
    HRESULT LoadExtension(const TCHAR *pszPath);
};

HRESULT CExtensionManager::LoadExtension(const TCHAR *pszPath)
{
    CLoadedExtension le;

    HMODULE hmod = LoadLibrary(pszPath);
    HRESULT hr = E_FAIL;
    if (hmod)
    {
        le._hmod = hmod;

        DllGetClassObject_t pfnDllGetClassObject = (DllGetClassObject_t)GetProcAddress(hmod, "DllGetClassObject");
        if (!pfnDllGetClassObject)
        {
            FreeLibrary(hmod);
            return E_FAIL;
        }

        IClassFactory *pFac = nullptr;
        hr = pfnDllGetClassObject(CLSID_ScreenshotEditorExtension, IID_PPV_ARGS(&pFac));
        if (SUCCEEDED(hr))
        {
            IScreenshotEditorExtension *pExt = nullptr;
            hr = pFac->CreateInstance(nullptr, IID_PPV_ARGS(&pExt));
            if (SUCCEEDED(hr))
            {
                le._pExt = pExt;
                
                if (FAILED(pExt->GetName(&le._pszName)))
                    le._pszName = nullptr;
                if (FAILED(pExt->GetVersionString(&le._pszVersionStr)))
                    le._pszVersionStr = nullptr;
                if (FAILED(pExt->GetAuthor(&le._pszAuthor)))
                    le._pszAuthor = nullptr;

                _vLoadedExts.Push(std::move(le));
                hr = S_OK;
            }
            pFac->Release();
        }
    }

    return hr;
}