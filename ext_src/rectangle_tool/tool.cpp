#include "pch.h"
#include "tool.h"

#include "rectobj.h"

STDMETHODIMP CRectangleTool::QueryInterface(const IID &riid, void **ppvOut)
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

STDMETHODIMP CRectangleTool::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;

    return _pUnkSite->QueryInterface(riid, ppvSite);
}

STDMETHODIMP CRectangleTool::SetSite(IUnknown *pUnkSite)
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

STDMETHODIMP_(const TCHAR *) CRectangleTool::GetClassName()
{
    return TEXT("rectangle_tool.CRectangleTool");
}

STDMETHODIMP CRectangleTool::OnDestroyed()
{
    return S_OK;
}

STDMETHODIMP CRectangleTool::ToolSelectionChanged(BOOL fSelected)
{
    return S_OK;
}

STDMETHODIMP_(ULONG) CRectangleTool::GetFlags()
{
    return SSETF_DRAWSELECTION;
}

STDMETHODIMP_(HICON) CRectangleTool::GetToolIcon()
{
    return nullptr;
}

STDMETHODIMP CRectangleTool::GetToolName(OUT const TCHAR **pszOut)
{
    if (!pszOut)
        return E_POINTER;
    if (_type == TYPE_HIGHLIGHTER)
        return StaticCoTaskMemStringAlloc(TEXT("Highlighter"), (TCHAR **)pszOut);
    else
        return StaticCoTaskMemStringAlloc(TEXT("Rectangle"), (TCHAR **)pszOut);
}

STDMETHODIMP CRectangleTool::OnKeyDown(int iVirtualKey, LPARAM lParam)
{
    return S_FALSE;
}

STDMETHODIMP CRectangleTool::OnKeyUp(int iVirtualKey, LPARAM lParam)
{
    return S_FALSE;
}

STDMETHODIMP CRectangleTool::OnMouseMove(int x, int y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CRectangleTool::OnMouseLButtonDown(LONG x, LONG y, WPARAM flags)
{
    if (!_pObjCur)
    {
        CRectangleObject *pRectObj = new (std::nothrow) CRectangleObject(
            _type == TYPE_HIGHLIGHTER
                ? CRectangleObject::TYPE_HIGHLIGHTER
                : CRectangleObject::TYPE_SOLID
        );
        _pObjCur = pRectObj;
        _pEditor->InsertObject(pRectObj);
    }

    return S_OK;
}

STDMETHODIMP CRectangleTool::OnMouseLButtonUp(LONG x, LONG y, WPARAM flags)
{
    if (_pObjCur)
    {
        _pObjCur = nullptr;
    }

    return S_OK;
}

STDMETHODIMP CRectangleTool::OnMouseRButtonDown(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CRectangleTool::OnMouseRButtonUp(LONG x, LONG y, WPARAM flags)
{
    return S_OK;
}

STDMETHODIMP CRectangleTool::ApplyCursor()
{
    SetCursor(LoadCursor(nullptr, IDC_CROSS));
    return S_OK;
}

STDMETHODIMP CRectangleTool::OnSelectionChange(RECT *prcNew)
{
    if (_pObjCur)
    {
        CRectangleObject *pRectObj = (CRectangleObject *)_pObjCur;
        return pRectObj->UpdatePosition(prcNew);
    }

    return S_FALSE;
}
