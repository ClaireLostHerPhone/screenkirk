#include "pch.h"
#include "tool.h"

STDMETHODIMP CObjectDebugTool::QueryInterface(const IID &riid, void **ppvOut)
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

STDMETHODIMP CObjectDebugTool::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;

    return _pUnkSite->QueryInterface(riid, ppvSite);
}

STDMETHODIMP CObjectDebugTool::SetSite(IUnknown *pUnkSite)
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


STDMETHODIMP_(const TCHAR *) CObjectDebugTool::GetClassName()
{
    return TEXT("objdebug_tool.CObjectDebugTool");
}

STDMETHODIMP CObjectDebugTool::OnDestroyed()
{
    // We technically don't need to do this; the window is destroyed when the document is destroyed
    // since the closing the document editor exits the entire host thread.
    if (_pWnd)
    {
        DestroyWindow(_pWnd->GetHWND());
    }

    return S_OK;
}

STDMETHODIMP CObjectDebugTool::ToolSelectionChanged(BOOL fSelected)
{
    if (!_pWnd)
    {
        _pWnd = CObjectDebugWindow::Create(_pEditor, 0, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, nullptr);
        if (!_pWnd)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    ShowWindow(_pWnd->GetHWND(), fSelected ? SW_SHOW : SW_HIDE);
    return S_OK;
}

STDMETHODIMP_(ULONG) CObjectDebugTool::GetFlags()
{
    return 0;
}

STDMETHODIMP_(HICON) CObjectDebugTool::GetToolIcon(SIZE size)
{
    return nullptr;
}

STDMETHODIMP CObjectDebugTool::GetToolName(OUT const TCHAR **pszOut)
{
    if (!pszOut)
        return E_POINTER;
    return StaticCoTaskMemStringAlloc(TEXT("Object Debug Tool"), (TCHAR **)pszOut);
}

STDMETHODIMP CObjectDebugTool::OnKeyDown(int iVirtualKey, LPARAM lParam)
{
    return S_FALSE;
}

STDMETHODIMP CObjectDebugTool::OnKeyUp(int iVirtualKey, LPARAM lParam)
{
    return S_FALSE;
}

STDMETHODIMP CObjectDebugTool::OnMouseMove(int x, int y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CObjectDebugTool::OnMouseLButtonDown(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CObjectDebugTool::OnMouseLButtonUp(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CObjectDebugTool::OnMouseRButtonDown(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CObjectDebugTool::OnMouseRButtonUp(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CObjectDebugTool::ApplyCursor()
{
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
    return S_OK;
}

STDMETHODIMP CObjectDebugTool::OnSelectionChange(RECT *prcNew)
{
    return E_NOTIMPL;
}
