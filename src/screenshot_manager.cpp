#include "pch.h"
#include "screenshot_manager.h"
#include "screenshot_editor.h"

void OnScreenshotKeyPressed()
{
    CScreenshotContext *pScreenshotCtx = nullptr;

    HRESULT hr = TakeScreenshot(&pScreenshotCtx);
    if (SUCCEEDED(hr))
    {
        CScreenshotEditorWindow *pScreenshotWnd = CScreenshotEditorWindow::CreateAndShow(pScreenshotCtx);
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
        HRESULT hr = E_FAIL;

        HDC hdcDesktop = GetDC(HWND_DESKTOP);
        if (hdcDesktop)
        {
            HDC hdcOrig = CreateCompatibleDC(hdcDesktop);
            HDC hdcCopy = CreateCompatibleDC(hdcDesktop);
            if (hdcOrig && hdcCopy)
            {
                _hbmModified = CreateCompatibleBitmap(hdcDesktop, _sizeDesktop.cx, _sizeDesktop.cy);
                if (_hbmModified)
                {
                    HGDIOBJ hObjOldOrig = SelectObject(hdcOrig, _hbmScreenshot);
                    HGDIOBJ hObjOld = SelectObject(hdcCopy, _hbmModified);

                    BitBlt(hdcCopy, 0, 0, _sizeDesktop.cx, _sizeDesktop.cy, hdcOrig, 0, 0, SRCCOPY);

                    SelectObject(hdcCopy, hObjOld);
                    SelectObject(hdcOrig, hObjOldOrig);

                    hr = S_OK;
                }
            }
            if (hdcOrig)
                DeleteDC(hdcOrig);
            if (hdcCopy)
                DeleteDC(hdcCopy);
            ReleaseDC(HWND_DESKTOP, hdcDesktop);
        }

        if (FAILED(hr) && _hbmModified)
        {
            DeleteObject(_hbmModified);
            _hbmModified = nullptr;
        }

        return hr;
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

    // This is just written from the top of my head, and it probably sucks. Whatever.
    hr = E_FAIL;

    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    HBITMAP hbmNew = nullptr;
    if (hdcDesktop)
    {
        HDC hdcOrig = CreateCompatibleDC(hdcDesktop);
        HDC hdcCopy = CreateCompatibleDC(hdcDesktop);
        if (hdcOrig && hdcCopy)
        {
            hbmNew = CreateCompatibleBitmap(hdcDesktop, RECTWIDTH(*prcCrop), RECTHEIGHT(*prcCrop));
            if (hbmNew)
            {
                HGDIOBJ hObjOldOrig = SelectObject(hdcOrig, _hbmModified);
                HGDIOBJ hObjOld = SelectObject(hdcCopy, hbmNew);

                BitBlt(hdcCopy, 0, 0, RECTWIDTH(*prcCrop), RECTHEIGHT(*prcCrop), hdcOrig, prcCrop->left, prcCrop->top, SRCCOPY);

                SelectObject(hdcCopy, hObjOld);
                SelectObject(hdcOrig, hObjOldOrig);

                hr = S_OK;
            }
        }
        if (hdcOrig)
            DeleteDC(hdcOrig);
        if (hdcCopy)
            DeleteDC(hdcCopy);
        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

    if (hbmNew)
    {
        if (SUCCEEDED(hr))
        {
            DeleteObject(_hbmModified);
            _hbmModified = hbmNew;
        }
        else
        {
            DeleteObject(hbmNew);
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CScreenshotContext::CopyToClipboard()
{
    OpenClipboard(nullptr);
    EmptyClipboard();

    SetClipboardData(CF_BITMAP, _hbmModified);
    
    CloseClipboard();
    return S_OK;
}
