#include "pch.h"
#include "screenshot_manager.h"
#include "screenshot_editor.h"
#include "util.h"

void OnScreenshotKeyPressed()
{
    CScreenshotContext *pScreenshotCtx = nullptr;

    HRESULT hr = TakeScreenshot(&pScreenshotCtx);
    if (SUCCEEDED(hr))
    {
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
