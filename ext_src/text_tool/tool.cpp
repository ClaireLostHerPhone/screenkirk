#include "pch.h"
#include "tool.h"

STDMETHODIMP CTextEditorTool::QueryInterface(const IID &riid, void **ppvOut)
{
    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IScreenshotEditorTool))
    {
        *ppvOut = static_cast<IScreenshotEditorTool *>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CTextEditorTool::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;

    return _pUnkSite->QueryInterface(riid, ppvSite);
}

STDMETHODIMP CTextEditorTool::SetSite(IUnknown *pUnkSite)
{
    _pUnkSite = pUnkSite;
    HRESULT hr = E_FAIL;

    if (_pEditor)
    {
        _pEditor->Release();
        _pEditor = nullptr;
    }

    if (pUnkSite)
    {
        hr = pUnkSite->QueryInterface(IID_PPV_ARGS(&_pEditor));
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}


STDMETHODIMP CTextEditorTool::SelectTool()
{
    return S_OK;
}

STDMETHODIMP_(HICON) CTextEditorTool::GetToolIcon()
{
    return nullptr;
}

STDMETHODIMP CTextEditorTool::GetToolName(OUT const TCHAR **pszOut)
{
    if (!pszOut)
        return E_POINTER;
    return StaticCoTaskMemStringAlloc(TEXT("Text"), (TCHAR **)pszOut);
}

STDMETHODIMP CTextEditorTool::OnKeyDown(int iVirtualKey, LPARAM lParam)
{
    return S_OK;
}

STDMETHODIMP CTextEditorTool::OnKeyUp(int iVirtualKey, LPARAM lParam)
{
    return S_OK;
}

STDMETHODIMP CTextEditorTool::OnMouseMove(int x, int y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CTextEditorTool::OnMouseLButtonDown(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CTextEditorTool::OnMouseLButtonUp(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CTextEditorTool::OnMouseRButtonDown(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CTextEditorTool::OnMouseRButtonUp(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CTextEditorTool::ApplyCursor()
{
    SetCursor(LoadCursor(nullptr, IDC_IBEAM));
    return S_OK;
}
