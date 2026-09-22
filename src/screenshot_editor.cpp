#include "pch.h"
#include "screenshot_editor.h"
#include "resource.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include <assert.h>

//
// CEditorFloatingToolbar
//

LRESULT CEditorFloatingToolbar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            if (FAILED(_OnCreate()))
            {
                MessageBox(nullptr, TEXT("Failed floating toolbar creation routine."), TEXT("Error"), MB_OK | MB_ICONERROR);
                return -1;
            }
            break;
        }

        case WM_CLOSE:
        {
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }

        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *pwp = (WINDOWPOS *)lParam;

            // If the window is activated, then it can be brought in front.
            // Otherwise, we'll keep it in front of the screenshot editor.
            pwp->hwndInsertAfter = GetWindow(_hwndEditor, GW_HWNDPREV);
            pwp->flags &= ~SWP_NOZORDER;

            return 0;
        }

        // Key input should pass through to the editor if the floating toolbar is
        // activated.
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            PostMessage(_hwndEditor, uMsg, wParam, lParam);
            // [[fallthrough]]
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) >= IDM_TOOLFIRST)
            {
                SendMessage(_hwndEditor, CScreenshotEditorWindow::WM_SSE_CHANGETOOL, LOWORD(wParam) - IDM_TOOLFIRST, 0);
            }
            else switch (LOWORD(wParam))
            {
                case IDM_DISCARD:
                {
                    DestroyWindow(_hwndEditor);
                    break;
                }

                case IDM_COPY:
                {
                    SendMessage(_hwndEditor, CScreenshotEditorWindow::WM_SSE_COPYTOCLIPBOARD, 0, 0);
                    break;
                }

                case IDM_SAVE:
                {
                    MessageBox(_hwndEditor, TEXT("This operation has yet to be implemented."), TEXT("Unimplemented!"), MB_OK | MB_ICONERROR);
                    break;
                }
            }

            break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HRESULT CEditorFloatingToolbar::_OnCreate()
{
    RECT rcClient;
    GetClientRect(_hwnd, &rcClient);

    _hwndToolbar = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        nullptr,
        WS_VISIBLE | WS_CHILD | CCS_NORESIZE | TBSTYLE_WRAPABLE,
        0, 0,
        RECTWIDTH(rcClient), RECTHEIGHT(rcClient),
        _hwnd,
        nullptr,
        g_hinst,
        nullptr
    );

    if (!_hwndToolbar)
    {
        return E_FAIL;
    }

    TBBUTTON rgtbButtons[5] = { 0 };

    rgtbButtons[0].idCommand = IDM_TOOLFIRST + SSET_SELECT;
    rgtbButtons[0].iString = (INT_PTR)TEXT("Select");
    rgtbButtons[0].fsState = TBSTATE_ENABLED;
    rgtbButtons[0].fsStyle = BTNS_BUTTON;

    rgtbButtons[1].idCommand = IDM_TOOLFIRST + SSET_DRAG;
    rgtbButtons[1].iString = (INT_PTR)TEXT("Move");
    rgtbButtons[1].fsState = TBSTATE_ENABLED;
    rgtbButtons[1].fsStyle = BTNS_BUTTON;

    rgtbButtons[2].idCommand = IDM_COPY;
    rgtbButtons[2].iString = (INT_PTR)TEXT("Copy");
    rgtbButtons[2].fsState = TBSTATE_ENABLED | TBSTATE_WRAP;
    rgtbButtons[2].fsStyle = BTNS_BUTTON;

    rgtbButtons[3].idCommand = IDM_SAVE;
    rgtbButtons[3].iString = (INT_PTR)TEXT("Save");
    rgtbButtons[3].fsState = TBSTATE_ENABLED;
    rgtbButtons[3].fsStyle = BTNS_BUTTON;

    rgtbButtons[4].idCommand = IDM_DISCARD;
    rgtbButtons[4].iString = (INT_PTR)TEXT("Discard");
    rgtbButtons[4].fsState = TBSTATE_ENABLED;
    rgtbButtons[4].fsStyle = BTNS_BUTTON;

    SendMessage(_hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(_hwndToolbar, TB_ADDBUTTONS, 5, (LPARAM)&rgtbButtons);

    return S_OK;
}

// static
HRESULT CEditorFloatingToolbar::RegisterWindowClass()
{
    WNDCLASS cls = {};
    cls.hInstance = g_hinst;
    cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
    cls.hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_APP));

    return CWindow::RegisterWindowClass(&cls);
}

// static
CEditorFloatingToolbar *CEditorFloatingToolbar::Create(HWND hwndEditor)
{
    if (FAILED(RegisterWindowClass()))
    {
        return nullptr;
    }

    CEditorFloatingToolbar *pWnd = CWindow::Create(
        WS_EX_PALETTEWINDOW,
        TEXT("Screenshot Editor Tools"),
        WS_CAPTION | WS_SYSMENU,
        0, 0,
        300, 600,
        nullptr,
        nullptr,
        g_hinst,
        nullptr
    );
    pWnd->_hwndEditor = hwndEditor;

    return pWnd;
}

//
// CScreenshotEditorRendererGDI
//

CScreenshotEditorRendererGDI::~CScreenshotEditorRendererGDI()
{
    DeleteObject(_hpenSelect);
    DeleteObject(_hbmScreenshotDimmed);

    if (_bmp.fCopiedScreenshot)
    {
        DeleteObject(_hbmScreenshotLight);
    }
}

HRESULT CScreenshotEditorRendererGDI::Initialize()
{
    HRESULT hr = _MakeDimmedScreenshot();
    _hpenSelect = CreatePen(PS_DOT, 1, RGB(128, 128, 128));
    return SUCCEEDED(hr) ? S_OK : hr;
}

HRESULT CScreenshotEditorRendererGDI::Paint(HDC hdc, RECT *prcPaint)
{
    // There is currently no good condition to avoid using the backbuffer under.
    bool fUseBackbuffer = true;

    HDC hdcScreenshot = CreateCompatibleDC(hdc);
    HGDIOBJ hObjOldSS = (HGDIOBJ)SelectObject(hdcScreenshot, _hbmScreenshotDimmed);

    HDC hdcBackbuffer = nullptr;
    HBITMAP hbmBackbuffer = nullptr;
    HGDIOBJ hObjOldBB = nullptr;
    if (fUseBackbuffer)
    {
        hdcBackbuffer = CreateCompatibleDC(hdc);
        hbmBackbuffer = CreateCompatibleBitmap(hdc, RECTWIDTH(*prcPaint), RECTHEIGHT(*prcPaint));
        hObjOldBB = (HGDIOBJ)SelectObject(hdcBackbuffer, hbmBackbuffer);

        BitBlt(
            hdcBackbuffer,
            0, 0,
            RECTWIDTH(*prcPaint), RECTHEIGHT(*prcPaint),
            hdcScreenshot,
            prcPaint->left, prcPaint->top,
            SRCCOPY
        );
        POINT ptOriginOld;
        SetViewportOrgEx(hdcBackbuffer, -prcPaint->left, -prcPaint->top, &ptOriginOld);
    }
    else
    {
        hdcBackbuffer = hdcScreenshot;
    }

    if (_bmp.fHasAnySelectionMade)
    {
        HDC hdcSelection = fUseBackbuffer
            ? hdcBackbuffer
            : hdc;

        // Highlight the selected area of the screenshot:
        SelectObject(hdcScreenshot, _hbmScreenshotLight);
        BitBlt(
            hdcSelection,
            _rcSelection.left, _rcSelection.top,
            RECTWIDTH(_rcSelection), RECTHEIGHT(_rcSelection),
            hdcScreenshot,
            _rcSelection.left, _rcSelection.top,
            SRCCOPY
        );

        _PaintSelectionRectangle(hdcSelection, &_rcSelection, _bmp.fDrawMarqueeSelection);

        SelectObject(hdcScreenshot, hObjOldSS);
    }

    if (_bmp.fAnyObjectDirty)
    {
        HDC hdcSelection = fUseBackbuffer
            ? hdcBackbuffer
            : hdc;

        // Paint all objects from back to front.
        // WARNING!! This is currently unoptimized to all hell. There is no regional consideration
        // or anything. All objects will be repainted no matter what.
        for (int i = 0; i < _vRenderObjs.GetSize(); i++)
        {
            CRenderObject *pRenderObject = &_vRenderObjs[i];

            // TODO: Pull this logic out into a new function, something like _UpdateVisualObject,
            // and improve error checking for GDI objects...
            if (pRenderObject->HasGdiRenderer())
            {
                HDC hdcLayer = CreateCompatibleDC(hdc);
                if (!pRenderObject->_hbmLayer)
                {
                    RECT rcVisual;
                    pRenderObject->_pObj->GetVisualRect(&rcVisual);
                    pRenderObject->_hbmLayer = CreateCompatibleBitmap(hdc, RECTWIDTH(rcVisual), RECTHEIGHT(rcVisual));
                }
                HGDIOBJ hObjOld = SelectObject(hdcLayer, pRenderObject->_hbmLayer);

                pRenderObject->_pRendererGdi->SetGdiParameters(hdcLayer, prcPaint);
                pRenderObject->_pRendererGdi->Paint();

                SelectObject(hdcLayer, hObjOld);
                DeleteDC(hdcLayer);

                // Then AlphaBlt the object's visual layer into the current framebuffer...
            }
            else
            {
                // Unimplemented.
                assert(0);
            }
        }
    }

    BitBlt(
        hdc,
        prcPaint->left, prcPaint->top,
        RECTWIDTH(*prcPaint), RECTHEIGHT(*prcPaint),
        hdcBackbuffer,
        prcPaint->left, prcPaint->top,
        SRCCOPY
    );

    if (fUseBackbuffer)
    {
        SelectObject(hdcBackbuffer, hObjOldBB);
        DeleteObject(hbmBackbuffer);
        DeleteDC(hdcBackbuffer);
    }

    SelectObject(hdcScreenshot, hObjOldSS);
    DeleteDC(hdcScreenshot);

    // Clear all applicable dirty flags:
    _bmp.fSelectionDirty = false;
    _bmp.fSelectionDirtyNorth = false;
    _bmp.fSelectionDirtySouth = false;
    _bmp.fSelectionDirtyEast = false;
    _bmp.fSelectionDirtyWest = false;

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::UpdateSelection(RECT *prcNew)
{
    RECT rcSelectionOld = _rcSelection;
    _rcSelection = *prcNew;

    if (RECTWIDTH(_rcSelection) != RECTWIDTH(rcSelectionOld)
        || RECTHEIGHT(_rcSelection) != RECTHEIGHT(rcSelectionOld))
    {
        _bmp.fSelectionDirty = true;
    }

    if (_rcSelection.left != rcSelectionOld.left)
    {
        _bmp.fSelectionDirtyWest = true;
    }
    if (_rcSelection.top != rcSelectionOld.top)
    {
        _bmp.fSelectionDirtyNorth = true;
    }
    if (_rcSelection.right != rcSelectionOld.right)
    {
        _bmp.fSelectionDirtyEast = true;
    }
    if (_rcSelection.bottom != rcSelectionOld.bottom)
    {
        _bmp.fSelectionDirtySouth = true;
    }
    
    RECT rcUnion;
    UnionRect(&rcUnion, &rcSelectionOld, &_rcSelection);
    InvalidateRect(_hwndRenderTarget, &rcUnion, FALSE);

    bool fOldSelectionState = _bmp.fHasAnySelectionMade;
    _bmp.fHasAnySelectionMade = RECTWIDTH(_rcSelection) > 0 || RECTHEIGHT(_rcSelection) > 0;

    if (fOldSelectionState != _bmp.fHasAnySelectionMade)
    {
        _bmp.fHasAnySelectionMade
            ? _StartSelectionMarqueeTimer()
            : _EndSelectionMarqueeTimer();
    }

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::UpdateDragMode(DragMode dm)
{
    _ClearDragModeVisualFlags();

    if (dm != DRAGM_DRAG)
    {
        if (dm & DRAGM_SIZEN)
            _bmp.fSelThickNorth = true;
        else if (dm & DRAGM_SIZES)
            _bmp.fSelThickSouth = true;

        if (dm & DRAGM_SIZEW)
            _bmp.fSelThickWest = true;
        else if (dm & DRAGM_SIZEE)
            _bmp.fSelThickEast = true;
    }

    InvalidateRect(_hwndRenderTarget, &_rcSelection, FALSE);
    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::SetMarqueeSelection(bool fMarquee)
{
    _bmp.fDrawMarqueeSelection = fMarquee;
    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::HandleWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(_hwndRenderTarget, &ps);
            Paint(hdc, &ps.rcPaint);
            EndPaint(_hwndRenderTarget, &ps);
            return S_OK;
        }

        case WM_TIMER:
        {
            _UpdateMarquee();
            return S_OK;
        }
    }

    return S_FALSE;
}

HRESULT CScreenshotEditorRendererGDI::CreateRenderObject(IScreenshotEditorObject *pObj)
{
    CRenderObject ro = { 0 };

    ro._pObj = pObj;
    pObj->AddRef();
    HRESULT hr = pObj->QueryInterface(IID_PPV_ARGS(&ro._pRenderer));
    if (SUCCEEDED(hr))
    {
        IScreenshotEditorObjectRendererGDI *pGdiRenderer = nullptr;
        if (SUCCEEDED(pObj->QueryInterface(IID_PPV_ARGS(&pGdiRenderer))))
        {
            ro._pRendererGdi = pGdiRenderer;
        }

        _vRenderObjs.Push(ro);
        return S_OK;
    }

    _bmp.fAnyObjectDirty = true;
    return hr;
}

HRESULT CScreenshotEditorRendererGDI::RemoveRenderObject(IScreenshotEditorObject *pObj)
{
    CRenderObject *pro;
    int idxRenderObj;
    if (SUCCEEDED(_FindRenderObjectFromInterfaceObject(pObj, &pro, &idxRenderObj)))
    {
        RECT rcVisual = { 0 };
        assert(SUCCEEDED(pObj->GetVisualRect(&rcVisual)));

        if (pro->_pRendererGdi)
            pro->_pRendererGdi->Release();
        if (pro->_pRenderer)
            pro->_pRenderer->Release();
        if (pro->_pObj)
            pro->_pObj->Release();

        _bmp.fAnyObjectDirty = true;
        InvalidateRect(_hwndRenderTarget, &rcVisual, FALSE);
    }

    return E_NOT_SET;
}

HRESULT CScreenshotEditorRendererGDI::InvalidateRenderObject(IScreenshotEditorObject *pObj)
{
    RECT rcVisual = { 0 };
    if (SUCCEEDED(pObj->GetVisualRect(&rcVisual)))
    {
        _bmp.fAnyObjectDirty = true;
        InvalidateRect(_hwndRenderTarget, &rcVisual, FALSE);
        return S_OK;
    }
    else
    {
        assert(0);
        return E_FAIL;
    }
}

HRESULT CScreenshotEditorRendererGDI::_PaintSelectionRectangle(HDC hdc, RECT *prc, bool fUseMarquee)
{
    HGDIOBJ hObjOldBB = SelectObject(hdc, _hpenSelect);
    HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    int iOldBkMode = SetBkMode(hdc, TRANSPARENT);
    int iOldRop = SetROP2(hdc, R2_XORPEN);

    if (!_bmp.fDrawMarqueeSelection || S_FALSE == _DrawMarqueeDottedRectangle(hdc, prc))
    {
        Rectangle(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }

    // I don't know how much I like this. I might resort to a more standard solution and
    // add in rectangle guidelines around the corners and center points.
    if (_bmp.fSelThickNorth || _bmp.fSelThickEast || _bmp.fSelThickSouth || _bmp.fSelThickWest)
    {
        HPEN hpenThick = CreatePen(PS_SOLID, 3, RGB(235, 69, 0));
        HGDIOBJ hOldPen = SelectObject(hdc, hpenThick);

        if (_bmp.fSelThickNorth)
        {
            Rectangle(hdc, prc->left + 1, prc->top + 1, prc->right - 1, prc->top + 3);
        }
        else if (_bmp.fSelThickSouth)
        {
            Rectangle(hdc, prc->left + 1, prc->bottom - 3, prc->right - 1, prc->bottom - 1);
        }

        if (_bmp.fSelThickWest)
        {
            Rectangle(hdc, prc->left + 1, prc->top + 1, prc->left + 3, prc->bottom - 1);
        }
        else if (_bmp.fSelThickEast)
        {
            Rectangle(hdc, prc->right - 3, prc->top + 1, prc->right - 1, prc->bottom - 1);
        }

        SelectObject(hdc, hOldPen);
        DeleteObject(hpenThick);
    }

    SetROP2(hdc, iOldRop);
    SetBkMode(hdc, iOldBkMode);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hObjOldBB);

    return S_OK;
}

void CScreenshotEditorRendererGDI::_UpdateMarquee()
{
    _iSelMarqueeFrame++;
    if (_iSelMarqueeFrame >= 6)
        _iSelMarqueeFrame = 0;
    _bmp.fSelectionBorderAnimDirty = true;
    InvalidateRect(_hwndRenderTarget, &_rcSelection, FALSE);
}

void CScreenshotEditorRendererGDI::_ClearDragModeVisualFlags()
{
    _bmp.fSelThickNorth = false;
    _bmp.fSelThickSouth = false;
    _bmp.fSelThickEast = false;
    _bmp.fSelThickWest = false;
}

HRESULT CScreenshotEditorRendererGDI::_StartSelectionMarqueeTimer()
{
    SetTimer(_hwndRenderTarget, c_idTimerMarquee, 60, nullptr);
    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::_EndSelectionMarqueeTimer()
{
    KillTimer(_hwndRenderTarget, c_idTimerMarquee);
    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::_DrawMarqueeDottedRectangle(HDC hdc, RECT *prc)
{
    // If the rect is particularly thin, then we'll avoid drawing certain parts
    // or at all to avoid artifacting/flashing lights. The caller can then choose
    // to draw a better way for that case.
    if (RECTWIDTH(*prc) <= 6 || RECTHEIGHT(*prc) <= 6)
    {
        // Nothing to draw.
        return S_FALSE;
    }

    // Top border of selection outline:
    MoveToEx(hdc, prc->left + _iSelMarqueeFrame, prc->top, nullptr);
    LineTo(hdc, prc->right - 1, prc->top);

    // Right border of selection outline:
    MoveToEx(hdc, prc->right - 1, prc->top + _iSelMarqueeFrame + 1, nullptr);
    LineTo(hdc, prc->right - 1, prc->bottom);

    // Bottom border of selection outline:
    MoveToEx(hdc, prc->right - 1 - _iSelMarqueeFrame, prc->bottom - 1, nullptr);
    LineTo(hdc, prc->left + 1, prc->bottom - 1);

    // Left border of selection outline:
    MoveToEx(hdc, prc->left, prc->bottom - 1 - _iSelMarqueeFrame, nullptr);
    LineTo(hdc, prc->left, prc->top);

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::_MakeDimmedScreenshot()
{
    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    if (hdcDesktop)
    {
        HDC hdcDimmed = CreateCompatibleDC(hdcDesktop);
        if (hdcDimmed)
        {
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(bmi);
            bmi.bmiHeader.biWidth = _pScreenshotCtx->_sizeDesktop.cx;
            bmi.bmiHeader.biHeight = _pScreenshotCtx->_sizeDesktop.cy;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage = 0;

            void *pvPixels = nullptr;
            _hbmScreenshotDimmed = CreateDIBSection(hdcDimmed, &bmi, DIB_RGB_COLORS, &pvPixels, nullptr, 0);
            if (_hbmScreenshotDimmed)
            {
                HGDIOBJ hObjOld = SelectObject(hdcDimmed, _hbmScreenshotDimmed);

                HDC hdcOrig = CreateCompatibleDC(hdcDesktop);
                HGDIOBJ hObjOld2 = SelectObject(hdcOrig, _pScreenshotCtx->_hbmScreenshot);

                BitBlt(hdcDimmed, 0, 0, _pScreenshotCtx->_sizeDesktop.cx, _pScreenshotCtx->_sizeDesktop.cy, hdcOrig, 0, 0, SRCCOPY);

                SelectObject(hdcOrig, hObjOld2);
                DeleteDC(hdcOrig);

                ULONG *pulSrc = (ULONG *)pvPixels;
                constexpr static int c_iDimAmount = 0xFF * 0.75;
                int cLength = _pScreenshotCtx->_sizeDesktop.cx * _pScreenshotCtx->_sizeDesktop.cy;

                for (int i = cLength - 1; i >= 0; i--)
                {
                    ULONG ulR = GetRValue(*pulSrc);
                    ULONG ulG = GetGValue(*pulSrc);
                    ULONG ulB = GetBValue(*pulSrc);
                    ULONG ulDim = (0xFF - c_iDimAmount);
                    ulR = (ulR * c_iDimAmount + ulDim) >> 8;
                    ulG = (ulG * c_iDimAmount + ulDim) >> 8;
                    ulB = (ulB * c_iDimAmount + ulDim) >> 8;
                    *pulSrc = (*pulSrc & 0xFF000000) | RGB(ulR, ulG, ulB);
                    pulSrc++;
                }

                SelectObject(hdcDimmed, hObjOld);
            }

            DeleteDC(hdcDimmed);
        }

        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::_FindRenderObjectFromInterfaceObject(
    IScreenshotEditorObject *pIfaceObj, OUT CRenderObject **ppRenderObjOut, OUT int *pIdxOut = nullptr
)
{
    if (!pIfaceObj || !ppRenderObjOut)
        return E_POINTER;

    for (int i = 0; i < _vRenderObjs.GetSize(); i++)
    {
        if (_vRenderObjs[i]._pObj == pIfaceObj)
        {
            *ppRenderObjOut = &_vRenderObjs[i];
            if (pIdxOut)
                *pIdxOut = i;
            return S_OK;
        }
    }

    return E_NOT_SET;
}

//
// CScreenshotEditorWindow
//

LRESULT CScreenshotEditorWindow::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (_pRenderer)
    {
        HRESULT hr = _pRenderer->HandleWindowMessage(hwnd, uMsg, wParam, lParam);
        if (SUCCEEDED(hr) && hr != S_FALSE)
        {
            return hr;
        }
    }

    switch (uMsg)
    {
        case WM_CREATE:
        {
            _pScreenshotCtx = (CScreenshotContext *)(((CREATESTRUCT *)lParam)->lpCreateParams);
            _pRenderer = new CScreenshotEditorRendererGDI(_pScreenshotCtx, hwnd);
            if (FAILED(_pRenderer->Initialize()))
            {
                MessageBox(nullptr, TEXT("Failed to create renderer."), TEXT("Error"), MB_OK | MB_ICONERROR);
                return -1;
            }
            _ChangeTool(SSET_SELECT);
            SetWindowPos(_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
            break;
        }

        case WM_DESTROY:
        {
            return _OnDestroy();
        }

        case WM_KEYDOWN:
        {
            return _OnKeyDown(wParam, lParam);
        }

        case WM_KEYUP:
        {
            HRESULT hrExtensionTool = S_FALSE;
            if (_tool == SSET_EXTENSION && _pExtTool)
            {
                hrExtensionTool = _pExtTool->OnKeyDown(wParam, lParam);

                if (SUCCEEDED(hrExtensionTool) && hrExtensionTool != S_FALSE)
                {
                    return hrExtensionTool;
                }
                else if (FAILED(hrExtensionTool))
                {
                    // What to do in this case?
                    assert(0);
                }
            }

            break;
        }

        case WM_SETCURSOR:
        {
            _UpdateCursor();
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            return _OnMouseMove(x, y, wParam);
        }

        case WM_LBUTTONDOWN:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            return _OnMouseLButtonDown(x, y, wParam);
        }

        case WM_LBUTTONUP:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            return _OnMouseLButtonUp(x, y, wParam);
        }

        case WM_RBUTTONDOWN:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            return _OnMouseRButtonDown(x, y, wParam);
        }

        case WM_RBUTTONUP:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            return _OnMouseRButtonUp(x, y, wParam);
        }

        case WM_SSE_GETWINDOWPOSITIONS:
        {
            // Update the cursor now that enumeration is complete.
            _UpdateCursor();
            return 0;
        }

        case WM_SSE_CHANGETOOL:
        {
            // Clamp the tool to valid options:
            if (wParam < 0 || wParam > SSET_ILLEGAL)
            {
                wParam = SSET_ILLEGAL;
            }

            _ChangeTool((ScreenshotEditorTool)wParam);
            return 0;
        }

        case WM_SSE_COPYTOCLIPBOARD:
        {
            CopyToClipboardAndAccept();
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CScreenshotEditorWindow::_OnDestroy()
{
    for (IScreenshotEditorObject *&pObj : _vObjs)
    {
        _RemoveObject(pObj);
    }

    if (_pRenderer)
        delete _pRenderer;
    if (_pScreenshotCtx)
        delete _pScreenshotCtx;
    if (_pFloatingToolbar)
        DestroyWindow(_pFloatingToolbar->GetHWND());
    return 0;
}

LRESULT CScreenshotEditorWindow::_OnKeyDown(WPARAM virtualKey, LPARAM lParam)
{
    HRESULT hrExtensionTool = S_FALSE;
    if (_tool == SSET_EXTENSION && _pExtTool)
    {
        hrExtensionTool = _pExtTool->OnKeyDown(virtualKey, lParam);

        if (SUCCEEDED(hrExtensionTool) && hrExtensionTool != S_FALSE)
        {
            return hrExtensionTool;
        }
        else if (FAILED(hrExtensionTool))
        {
            // What to do in this case?
        }
    }

    switch (virtualKey)
    {
        case VK_ESCAPE:
        {
            if (_fHasAnySelectionMade)
            {
                _CancelSelection();
            }
            else
            {
                DestroyWindow(_hwnd);
            }
            break;
        }

        case VK_RETURN:
        {
            CopyToClipboardAndAccept();
            break;
        }

        case 'T':
        {
            _ShowFloatingToolbar();
            break;
        }

        // TEMP: Set the tool to "select"
        case '1':
        {
            _ChangeTool(SSET_SELECT);
            break;
        }

        // TEMP: Set the tool to "drag"
        case '2':
        {
            _ChangeTool(SSET_DRAG);
            break;
        }
    }

    return 0;
}

LRESULT CScreenshotEditorWindow::_OnMouseMove(int x, int y, WPARAM flags)
{
    if (_fIsSelectingRegion)
    {
        if (_tool == SSET_SELECT)
        {
            _fHasAnySelectionMade = true;

            _rcSelection.left = x < _ptSelectionOrigin.x
                ? x
                : _ptSelectionOrigin.x;
            _rcSelection.top = y < _ptSelectionOrigin.y
                ? y
                : _ptSelectionOrigin.y;
            _rcSelection.right = x >= _ptSelectionOrigin.x
                ? x
                : _ptSelectionOrigin.x;
            _rcSelection.bottom = y >= _ptSelectionOrigin.y
                ? y
                : _ptSelectionOrigin.y;

            _pRenderer->UpdateSelection(&_rcSelection);
        }
        else if (_tool == SSET_DRAG)
        {
            if (_iToolMode == DRAGM_DRAG)
            {
                // This is a bit of a lazy implementation, but I decided to go with it because
                // I am bad at math.
                RECT rcOldSelection = _rcSelection;
                _rcSelection = _rcDragBegin;

                int iX = (x - _rcDragBegin.left - (_ptSelectionOrigin.x - _rcDragBegin.left));
                int iY = (y - _rcDragBegin.top - (_ptSelectionOrigin.y - _rcDragBegin.top));

                OffsetRect(&_rcSelection, iX, iY);

                _pRenderer->UpdateSelection(&_rcSelection);
            }
            else // Sizing modes:
            {
                if (_iToolMode & DRAGM_SIZEW)
                {
                    _rcSelection.left = x;
                }
                else if (_iToolMode & DRAGM_SIZEE)
                {
                    _rcSelection.right = x;
                }

                if (_iToolMode & DRAGM_SIZEN)
                {
                    _rcSelection.top = y;
                }
                else if (_iToolMode & DRAGM_SIZES)
                {
                    _rcSelection.bottom = y;
                }

                _pRenderer->UpdateSelection(&_rcSelection);
            }
        }
    }
    else if (_tool == SSET_DRAG)
    {
        // Set drag tool mode.
        constexpr static int c_iGrabRadius = 4; // On both sides.
        int dm = DRAGM_DRAG;

        if (y >= (_rcSelection.top - c_iGrabRadius) && y <= (_rcSelection.top + c_iGrabRadius)
            && x >= (_rcSelection.left - c_iGrabRadius) && x <= (_rcSelection.right + c_iGrabRadius))
        {
            dm |= DRAGM_SIZEN;
        }
        else if (y >= (_rcSelection.bottom - c_iGrabRadius) && y <= (_rcSelection.bottom + c_iGrabRadius)
            && x >= (_rcSelection.left - c_iGrabRadius) && x <= (_rcSelection.right + c_iGrabRadius))
        {
            dm |= DRAGM_SIZES;
        }

        if (x >= (_rcSelection.left - c_iGrabRadius) && x <= (_rcSelection.left + c_iGrabRadius)
            && y >= (_rcSelection.top - c_iGrabRadius) && y <= (_rcSelection.bottom + c_iGrabRadius))
        {
            dm |= DRAGM_SIZEW;
        }
        else if (x >= (_rcSelection.right - c_iGrabRadius) && x <= (_rcSelection.right + c_iGrabRadius)
            && y >= (_rcSelection.top - c_iGrabRadius) && y <= (_rcSelection.bottom + c_iGrabRadius))
        {
            dm |= DRAGM_SIZEE;
        }

        if (dm != _iToolMode)
        {
            _iToolMode = (DragMode)dm;
            _UpdateCursor();
            _pRenderer->UpdateDragMode((DragMode)dm);
        }
    }
    else if (_tool == SSET_EXTENSION && _pExtTool)
    {
        HRESULT hr = _pExtTool->OnMouseMove(x, y, flags);
        if (hr == S_FALSE)
        {
            return 0;
        }
    }

    return 0;
}

LRESULT CScreenshotEditorWindow::_OnMouseLButtonDown(int x, int y, WPARAM flags)
{
    SetCapture(_hwnd);
    _fIsSelectingRegion = true;

    GetCursorPos(&_ptSelectionOrigin);
    _ptSelectionOrigin.x -= _pScreenshotCtx->_ptVirtualScreen.x; // Otherwise the offsets are fucked up...
    _ptSelectionOrigin.y -= _pScreenshotCtx->_ptVirtualScreen.y;

    if (_tool == SSET_DRAG)
    {
        _rcDragBegin = _rcSelection;
    }
    else if (_tool == SSET_EXTENSION && _pExtTool)
    {
        HRESULT hr = _pExtTool->OnMouseLButtonDown(x, y, flags);
        if (hr == S_FALSE)
        {
            return 0;
        }
    }

    _HideFloatingToolbar();

    return 0;
}

LRESULT CScreenshotEditorWindow::_OnMouseLButtonUp(int x, int y, WPARAM flags)
{
    ReleaseCapture();

    // If the user clicks in a static location without drawing a new outline
    // with the drag tool selected, then we will clear what was previously
    // drawn. This reflects the user experience of graphics editors like
    // Photoshop or Paint.NET.
    if (_tool == SSET_SELECT && (x == _ptSelectionOrigin.x && y == _ptSelectionOrigin.y))
    {
        _CancelSelection();
    }

    _fIsSelectingRegion = false;
    _ptSelectionOrigin = { 0 };

    if (_tool == SSET_EXTENSION && _pExtTool)
    {
        HRESULT hr = _pExtTool->OnMouseLButtonUp(x, y, flags);
        if (hr == S_FALSE)
        {
            return 0;
        }
    }

    _ShowFloatingToolbar();

    return 0;
}

LRESULT CScreenshotEditorWindow::_OnMouseRButtonDown(int x, int y, WPARAM flags)
{
    // If the user right clicks while selecting a region, then cancel the selection.
    // This is similar to ShareX.
    if (_tool == SSET_SELECT && _fIsSelectingRegion)
    {
        ReleaseCapture();
        _CancelSelection();
        _fIsSelectingRegion = false;
        _ptSelectionOrigin = { 0, 0 };
    }
    else if (_tool == SSET_EXTENSION && _pExtTool)
    {
        HRESULT hr = _pExtTool->OnMouseRButtonDown(x, y, flags);
        if (hr == S_FALSE)
        {
            return 0;
        }
    }

    return 0;
}

LRESULT CScreenshotEditorWindow::_OnMouseRButtonUp(int x, int y, WPARAM flags)
{
    if (_tool == SSET_EXTENSION && _pExtTool)
    {
        HRESULT hr = _pExtTool->OnMouseRButtonUp(x, y, flags);
        if (hr == S_FALSE)
        {
            return 0;
        }
    }

    return 0;
}

HRESULT CScreenshotEditorWindow::_ChangeTool(ScreenshotEditorTool newTool)
{
    if (newTool == SSET_SELECT)
    {
        _tool = SSET_SELECT;
        _pRenderer->SetMarqueeSelection(true);
    }
    else if (newTool == SSET_DRAG)
    {
        _tool = SSET_DRAG;
        _pRenderer->SetMarqueeSelection(true);
    }
    else if (newTool == SSET_EXTENSION)
    {
        // Do whatever to get _pExtTool, then call this to allow the extension to reject
        // switching:
        /*if (FAILED(_pExtTool->SelectTool()))
        {
            return E_ABORT;
        }*/

        // TODO: Extension tools will be able to report this as they need to.
        _pRenderer->SetMarqueeSelection(false);
    }
    else
    {
        _pRenderer->SetMarqueeSelection(false);
    }

    _UpdateCursor();

    // We'll update the drag mode since it has implications for rendering.
    // If the tool isn't the drag tool, then we'll report DRAGM_DRAG since that is
    // the same as anything else.
    _pRenderer->UpdateDragMode(_tool == SSET_DRAG ? (DragMode)_iToolMode : DRAGM_DRAG);

    return S_OK;
}

void CScreenshotEditorWindow::_ShowFloatingToolbar()
{
    POINT ptShow;
    ptShow.x = (_rcSelection.right + _pScreenshotCtx->_ptVirtualScreen.x) + 10; // Maybe ask the renderer instead?
    ptShow.y = (_rcSelection.top + _pScreenshotCtx->_ptVirtualScreen.y);

    // If we don't have a toolbar yet, then we will create one:
    if (!_pFloatingToolbar)
    {
        _pFloatingToolbar = CEditorFloatingToolbar::Create(_hwnd);
    }

    if (_pFloatingToolbar)
    {
        SetWindowPos(_pFloatingToolbar->GetHWND(), nullptr, ptShow.x, ptShow.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    }
}

void CScreenshotEditorWindow::_HideFloatingToolbar()
{
    if (_pFloatingToolbar)
    {
        ShowWindow(_pFloatingToolbar->GetHWND(), SW_HIDE);
    }
}

void CScreenshotEditorWindow::_UpdateCursor()
{
    TCHAR *idc = IDC_ARROW;

#if 0 // Not yet implemented
    if (!_fEnumeratedWindows)
    {
        // We show the wait cursor while windows are still being enumerated,
        // since automatic selections will not work until then. Manual selections
        // can still be made.
        idc = IDC_WAIT;
    }
    else
#endif
    {
        switch (_tool)
        {
            case SSET_SELECT:
                idc = IDC_CROSS;
                break;
            case SSET_DRAG:
                switch (_iToolMode)
                {
                    case DRAGM_SIZEN:
                    case DRAGM_SIZES:
                        idc = IDC_SIZENS;
                        break;
                    case DRAGM_SIZEW:
                    case DRAGM_SIZEE:
                        idc = IDC_SIZEWE;
                        break;
                    case DRAGM_SIZENW:
                    case DRAGM_SIZESE:
                        idc = IDC_SIZENWSE;
                        break;
                    case DRAGM_SIZENE:
                    case DRAGM_SIZESW:
                        idc = IDC_SIZENESW;
                        break;
                    default:
                        idc = IDC_SIZEALL;
                        break;
                }
                break;
            case SSET_EXTENSION:
            {
                if (_pExtTool && SUCCEEDED(_pExtTool->ApplyCursor()))
                {
                    return;
                }
                break;
            }
            case SSET_ILLEGAL:
                idc = IDC_NO;
                break;
        }
    }

    HCURSOR hcur = LoadCursor(NULL, idc);
    SetCursor(hcur);
}

void CScreenshotEditorWindow::_CancelSelection()
{
    RECT rcSelectionOld = _rcSelection;
    _rcSelection = { 0, 0 };
    _fHasAnySelectionMade = false;
    _pRenderer->UpdateSelection(&_rcSelection);
    _HideFloatingToolbar();
}

HRESULT CScreenshotEditorWindow::_RemoveObject(IScreenshotEditorObject *pObj)
{
    // Search an object by its reference and remove it.
    for (int i = 0; i < _vObjs.GetSize(); i++)
    {
        if (_vObjs[i] == pObj)
        {
            _pRenderer->RemoveRenderObject(pObj);

            pObj->SetSite(nullptr);
            pObj->Release();
            _vObjs.Remove(i);
            return S_OK;
        }
    }

    // The requested object does not exist in the array.
    assert(0);
    return E_NOT_SET;
}

HRESULT CScreenshotEditorWindow::_OnGetWindowPositions()
{
    _fEnumeratedWindows = true;
    PostMessage(_hwnd, WM_SSE_GETWINDOWPOSITIONS, 0, 0);
    return S_OK;
}

STDMETHODIMP CScreenshotEditorWindow::QueryInterface(const REFIID riid, void **ppvOut)
{
    if (!&riid || !ppvOut)
        return E_POINTER;

    if (IsEqualGUID(riid, IID_IScreenshotEditor)
        || IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvOut = static_cast<IScreenshotEditor *>(this);
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP CScreenshotEditorWindow::InsertObject(IScreenshotEditorObject *pObj)
{
    if (!pObj)
        return E_POINTER;

    for (IScreenshotEditorObject *&pExisting : _vObjs)
    {
        if (pObj == pExisting)
        {
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
    }

    pObj->AddRef();
    pObj->SetSite(this);

    _vObjs.Push(pObj);
    HRESULT hr = _pRenderer->CreateRenderObject(pObj);

    if (SUCCEEDED(hr))
    {
        hr = pObj->InsertedIntoDocument();
        if (FAILED(hr) && hr != E_NOTIMPL)
        {
            _RemoveObject(pObj);
        }

        hr = S_OK;
    }
    else if (SUCCEEDED(_RemoveObject(pObj)))
    {
        return E_FAIL;
    }
    else
    {
        assert(0);
        return E_NOT_VALID_STATE;
    }

    return hr;
}

STDMETHODIMP CScreenshotEditorWindow::InvalidateObject(IScreenshotEditorObject *pObj)
{
    // Apart from invalidating the render object, what should this method do?
    _pRenderer->InvalidateRenderObject(pObj);
    return E_NOTIMPL;
}

STDMETHODIMP CScreenshotEditorWindow::GetScreenshotContext(OUT IScreenshotContext **ppContext)
{
    if (!ppContext)
        return E_POINTER;
    *ppContext = _pScreenshotCtx;
    return S_OK;
}

HRESULT CScreenshotEditorWindow::CopyToClipboardAndAccept()
{
    if (!_fHasAnySelectionMade)
    {
        _rcSelection = { 0, 0, _pScreenshotCtx->_sizeDesktop.cx, _pScreenshotCtx->_sizeDesktop.cy };
    }

    if (SUCCEEDED(_pScreenshotCtx->Crop(&_rcSelection)))
    {
        if (SUCCEEDED(_pScreenshotCtx->CopyToClipboard()))
        {
            DestroyWindow(_hwnd);
        }
        else
        {
            MessageBox(_hwnd, TEXT("Failed to copy image to clipboard."), TEXT("Error"), MB_OK | MB_ICONERROR);
            return E_FAIL;
        }
    }
    else
    {
        MessageBox(_hwnd, TEXT("Failed to crop image."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return E_FAIL;
    }

    return S_OK;
}

// static
HRESULT CScreenshotEditorWindow::RegisterWindowClass()
{
    WNDCLASS cls = {};
    cls.hInstance = g_hinst;
    cls.hbrBackground = nullptr;
    cls.hCursor = nullptr;
    cls.hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_APP));

    return CWindow::RegisterWindowClass(&cls);
}

// static
CScreenshotEditorWindow *CScreenshotEditorWindow::CreateAndShow(CScreenshotContext *pScreenshotCtx)
{
    if (FAILED(RegisterWindowClass()))
    {
        return nullptr;
    }

    CScreenshotEditorWindow *pWnd = CWindow::Create(
        // We apply topmost temporarily during window creation to prevent anything
        // from being captured under it. Topmost status is cleared pretty quickly
        // since topmost fullscreen overlay windows are incredibly annoying.
        WS_EX_TOPMOST,
        TEXT("Screenshot Editor - screenkirk"),
        WS_POPUP,
        pScreenshotCtx->_ptVirtualScreen.x, pScreenshotCtx->_ptVirtualScreen.y,
        pScreenshotCtx->_sizeDesktop.cx, pScreenshotCtx->_sizeDesktop.cy,
        nullptr,
        nullptr,
        g_hinst,
        pScreenshotCtx
    );

    ShowWindow(pWnd->GetHWND(), SW_SHOW);
    UpdateWindow(pWnd->GetHWND());

    return pWnd;
}

//
// CFloatingScreenshotEditorWindow
//

LRESULT CFloatingScreenshotEditorWindow::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// static
HRESULT CFloatingScreenshotEditorWindow::RegisterWindowClass()
{
    WNDCLASS cls = {};
    cls.hInstance = g_hinst;
    cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
    cls.hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_APP));

    return CWindow::RegisterWindowClass(&cls);
}

// static
CFloatingScreenshotEditorWindow *CFloatingScreenshotEditorWindow::CreateAndShow(CScreenshotEditorWindow *pEditor)
{
    // TODO: Implement!
    return nullptr;
}
