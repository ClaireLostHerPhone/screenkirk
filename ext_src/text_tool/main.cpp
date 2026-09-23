#include "pch.h"
#include <new>

//
// CTextEditorTool
//
class CTextEditorTool : public IScreenshotEditorTool
{
    ULONG _uRefCount;
    void *_pvSite;

public:
    //@Begin IUnknown
    STDMETHODIMP QueryInterface(const IID &riid, void **ppvOut) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        InterlockedIncrement(&_uRefCount);
        return _uRefCount;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        InterlockedDecrement(&_uRefCount);
        if (_uRefCount <= 0)
            delete this;
        return _uRefCount;
    }
    //@End IUnknown

    //@Begin IObjectWithSite
    STDMETHODIMP GetSite(void **ppvSite) override;
    STDMETHODIMP SetSite(void *pUnkSite) override;
    //@End IObjectWithSite

    STDMETHODIMP SelectTool() override;
    STDMETHODIMP_(HICON) GetToolIcon() override;
    STDMETHODIMP GetToolName(OUT const TCHAR **pszOut) override;
    STDMETHODIMP OnKeyDown(int iVirtualKey, LPARAM lParam) override;
    STDMETHODIMP OnKeyUp(int iVirtualKey, LPARAM lParam) override;
    STDMETHODIMP OnMouseMove(int x, int y, WPARAM flags) override;
    STDMETHODIMP OnMouseLButtonDown(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseLButtonUp(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseRButtonDown(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseRButtonUp(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP ApplyCursor() override;
};
// {97378D4C-1FA7-4F10-9626-426A879BB01A}
DEFINE_GUID(CLSID_TextEditorTool,
    0x97378d4c, 0x1fa7, 0x4f10, 0x96, 0x26, 0x42, 0x6a, 0x87, 0x9b, 0xb0, 0x1a);


STDMETHODIMP CTextEditorTool::GetSite(void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;

    *ppvSite = _pvSite;
    return S_OK;
}

STDMETHODIMP CTextEditorTool::SetSite(void *pUnkSite)
{
    if (!pUnkSite)
        return E_POINTER;

    _pvSite = pUnkSite;
    return S_OK;
}

//
// CTextEditorExtension
//

class CTextEditorExtension : public IScreenshotEditorExtension
{
    ULONG _uRefCount;

public:
    //@Begin IUnknown
    STDMETHODIMP QueryInterface(const IID &riid, void **ppvOut) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        InterlockedIncrement(&_uRefCount);
        return _uRefCount;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        InterlockedDecrement(&_uRefCount);
        if (_uRefCount <= 0)
            delete this;
        return _uRefCount;
    }
    //@End IUnknown

    //@Begin IScreenshotEditorExtension
    STDMETHODIMP QueryInterface(REFIID riid, OUT void **ppvOut) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP GetName(OUT const TCHAR **pszOut) override;
    STDMETHODIMP GetVersionString(OUT const TCHAR **pszOut) override;
    STDMETHODIMP GetAuthor(OUT const TCHAR **pszOut) override;

    STDMETHODIMP GetToolSet(const CLSID **prgiidTools, int *piNumTools) override;
    //@End IScreenshotEditorExtension
};

STDMETHODIMP CTextEditorExtension::QueryInterface(const IID &riid, void **ppvOut)
{
    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IScreenshotEditorExtension))
    {
        *ppvOut = static_cast<IScreenshotEditorExtension *>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

inline HRESULT GiveStaticString(const TCHAR *pszStatic, size_t cbStr, OUT const TCHAR **pszOut)
{
    if (!pszOut)
        return E_POINTER;

    *pszOut = (const TCHAR *)CoTaskMemAlloc(cbStr);
    if (!pszOut)
        return E_OUTOFMEMORY;

    _tcscpy((TCHAR *)pszOut, pszStatic);
    return S_OK;
}

STDMETHODIMP CTextEditorExtension::GetName(OUT const TCHAR **pszOut)
{
    static const TCHAR szString[] = TEXT("Text Tool");
    return GiveStaticString(szString, sizeof(szString), pszOut);
}

STDMETHODIMP CTextEditorExtension::GetVersionString(OUT const TCHAR **pszOut)
{
    static const TCHAR szString[] = TEXT("1.0");
    return GiveStaticString(szString, sizeof(szString), pszOut);
}

STDMETHODIMP CTextEditorExtension::GetAuthor(OUT const TCHAR **pszOut)
{
    static const TCHAR szString[] = TEXT("ClaireLostHerPhone");
    return GiveStaticString(szString, sizeof(szString), pszOut);
}

STDMETHODIMP CTextEditorExtension::GetToolSet(const CLSID **prgiidTools, int *piNumTools)
{
    if (!prgiidTools || !piNumTools)
        return E_POINTER;

    static const const CLSID rgiidTool[] = {
        CLSID_TextEditorTool,
        { 0 },
    };

    *prgiidTools = &rgiidTool[0];
    *piNumTools = 1;
    return S_OK;
}

//
// CExtensionClassFactory
//

class CExtensionClassFactory : public IClassFactory
{
    ULONG _uRefCount;

public:
    //@Begin IUnknown
    STDMETHODIMP QueryInterface(const IID &riid, void **ppvOut) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        InterlockedIncrement(&_uRefCount);
        return _uRefCount;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        InterlockedDecrement(&_uRefCount);
        if (_uRefCount <= 0)
            delete this;
        return _uRefCount;
    }
    //@End IUnknown

    //@Begin IClassFactory
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, const IID &riid, void **ppvOut) override;
    STDMETHODIMP LockServer(BOOL fLock) override;
    //@End IClassFactory
};

STDMETHODIMP CExtensionClassFactory::QueryInterface(const IID &riid, void **ppvOut)
{
    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IClassFactory))
    {
        *ppvOut = static_cast<IClassFactory *>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CExtensionClassFactory::CreateInstance(IUnknown *pUnkOuter, const IID &riid, void **ppvOut)
{


    return E_NOTIMPL;
}

STDMETHODIMP CExtensionClassFactory::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}

//
// DllGetClassObject & DllMain
//

__declspec(dllexport) extern "C" HRESULT DllGetClassObject(CLSID &rclsid, IID &riid, void **ppv)
{
    if (nullptr == &rclsid || nullptr == &riid || !ppv)
        return E_POINTER;

    if (IsEqualGUID(rclsid, CLSID_ScreenshotEditorExtension))
    {
        CExtensionClassFactory *pFac = new (std::nothrow) CExtensionClassFactory();
        if (!pFac)
            return E_OUTOFMEMORY;
        HRESULT hr = pFac->QueryInterface(riid, ppv);
        pFac->Release();
        return SUCCEEDED(hr);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

_declspec(dllexport) extern "C" HRESULT DllCanUnloadNow(void)
{
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, void *lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}