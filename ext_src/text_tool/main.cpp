#include "pch.h"
#include "extension.h"

//
// CExtensionClassFactory
//

class CExtensionClassFactory : public IClassFactory
{
public:
    IMPLEMENT_IUNKNOWN;

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
    if (&riid == nullptr || !ppvOut)
        return E_POINTER;

    CTextEditorExtension *pExt = new (std::nothrow) CTextEditorExtension();
    if (pExt)
    {
        HRESULT hr = pExt->QueryInterface(riid, ppvOut);

        if (SUCCEEDED(hr))
        {
            return S_OK;
        }
        else
        {
            delete pExt;
            return hr;
        }
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CExtensionClassFactory::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}

//
// DllGetClassObject & DllMain
//

__declspec(dllexport) extern "C" HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
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

__declspec(dllexport) extern "C" HRESULT DllCanUnloadNow(void)
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