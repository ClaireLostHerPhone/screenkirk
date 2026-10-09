#include "pch.h"
#include "rectobj.h"

STDMETHODIMP CRectangleObject::QueryInterface(const IID &riid, void **ppvOut)
{
    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IScreenshotEditorObject))
    {
        *ppvOut = static_cast<IScreenshotEditorObject *>(this);
        AddRef();
        return S_OK;
    }
    else if (IsEqualGUID(riid, IID_IScreenshotEditorObjectRenderer)
        || IsEqualGUID(riid, IID_IScreenshotEditorObjectRendererGDI))
    {
        *ppvOut = static_cast<IScreenshotEditorObjectRendererGDI *>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CRectangleObject::GetSite(REFIID riid, void **ppvSite)
{
    if (!ppvSite)
        return E_POINTER;
    if (!_pEditor)
        return E_NOINTERFACE;

    return _pEditor->QueryInterface(riid, ppvSite);
}

STDMETHODIMP CRectangleObject::SetSite(IUnknown *pUnkSite)
{
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

STDMETHODIMP_(const TCHAR *) CRectangleObject::GetClassName()
{
    return TEXT("rectangle_tool.CRectangleObject");
}

STDMETHODIMP_(ULONG __stdcall) CRectangleObject::GetFlags()
{
    return 0;
}

STDMETHODIMP CRectangleObject::InsertedIntoDocument()
{
    return S_OK;
}

STDMETHODIMP CRectangleObject::GetLogicalRect(IN RECT *prc)
{
    if (!prc)
        return E_POINTER;
    *prc = _rcPosLogical;
    return S_OK;
}

STDMETHODIMP CRectangleObject::GetVisualRect(IN RECT *prc)
{
    if (!prc)
        return E_POINTER;
    *prc = _rcPosVisual;
    return S_OK;
}

STDMETHODIMP_(BOOL) CRectangleObject::IsVisualDirty()
{
    return _fDirty;
}

STDMETHODIMP CRectangleObject::Move(RECT *prcNew)
{
    return UpdatePosition(prcNew);
}

STDMETHODIMP CRectangleObject::CreateRenderer(const REFIID riid, OUT IScreenshotEditorObjectRenderer **ppRendererOut)
{
    if (IsEqualGUID(riid, IID_IScreenshotEditorObjectRendererGDI))
    {
        return QueryInterface(IID_IScreenshotEditorObjectRendererGDI, (void **)ppRendererOut);
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CRectangleObject::Paint()
{
    // Test color (pure red)
    // (Yeah, the highlighter isn't transparent yet which kinda defeats the point)
    HBRUSH hbr = _type == TYPE_HIGHLIGHTER
        ? CreateSolidBrush(RGB(255, 255, 0))
        : CreateSolidBrush(RGB(255, 0, 0));

    HGDIOBJ hOldBrush = SelectObject(_hdc, hbr);

    Rectangle(_hdc, 0, 0, RECTWIDTH(_rcPosLogical), RECTHEIGHT(_rcPosLogical));

    SelectObject(_hdc, hOldBrush);
    DeleteObject(hbr);
    _fDirty = false;
    return S_OK;
}

STDMETHODIMP CRectangleObject::SetGdiParameters(HDC hdc, RECT *prcPaint)
{
    _hdc = hdc;
    return S_OK;
}

HRESULT CRectangleObject::UpdatePosition(RECT *prc)
{
    _fDirty = true;
    _rcPosLogical = *prc;

    _rcPosVisual.left = 0;
    _rcPosVisual.top = 0;
    _rcPosVisual.right = RECTWIDTH(_rcPosLogical);
    _rcPosVisual.bottom = RECTHEIGHT(_rcPosLogical);

    if (_pEditor)
        _pEditor->InvalidateObject(this);
    return S_OK;
}
