#include "pch.h"
#include "screenshot_editor.h"
#include "resource.h"
#include <windowsx.h>

//
// CEditorFloatingToolbar
//

LRESULT CEditorFloatingToolbar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
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
CEditorFloatingToolbar *CEditorFloatingToolbar::CreateAndShow()
{
    // TODO: Implement!
    return nullptr;
}

//
// CScreenshotEditorRendererGDI
//

CScreenshotEditorRendererGDI::~CScreenshotEditorRendererGDI()
{
    DeleteObject(_hbmScreenshotDimmed);

    if (_bmp.fCopiedScreenshot)
    {
        DeleteObject(_hbmScreenshotLight);
    }
}

HRESULT CScreenshotEditorRendererGDI::Initialize()
{
    HRESULT hr = _MakeDimmedScreenshot();
    return SUCCEEDED(hr) ? S_OK : hr;
}

HRESULT CScreenshotEditorRendererGDI::Paint(HDC hdc, RECT *prcPaint)
{
    // Whether or not to use a separate backbuffer. We avoid creating these
    // GDI objects unless required for rendering.
    bool fUseBackbuffer = (_bmp.fHasAnySelectionMade);

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
        SelectObject(hdcScreenshot, hObjOldSS);
        hObjOldSS = SelectObject(hdcScreenshot, _hbmScreenshotLight);
        BitBlt(
            hdcSelection,
            _rcSelection.left, _rcSelection.top,
            RECTWIDTH(_rcSelection), RECTHEIGHT(_rcSelection),
            hdcScreenshot,
            _rcSelection.left, _rcSelection.top,
            SRCCOPY
        );

        // Draw the selection outline:
        HPEN hDotPen = CreatePen(PS_DOT, 1, RGB(128, 128, 128));
        hObjOldBB = SelectObject(hdcSelection, hDotPen);
        HGDIOBJ hOldBrush = SelectObject(hdcSelection, GetStockObject(HOLLOW_BRUSH));
        int iOldBkMode = SetBkMode(hdcSelection, TRANSPARENT);
        int iOldRop = SetROP2(hdcSelection, R2_XORPEN);

        _DrawMarqueeDottedRectangle(hdcSelection, &_rcSelection);

        SetROP2(hdcSelection, iOldRop);
        SetBkMode(hdcSelection, iOldBkMode);
        SelectObject(hdcSelection, hOldBrush);
        SelectObject(hdcSelection, hObjOldBB);
        DeleteObject(hDotPen);

        SelectObject(hdcScreenshot, hObjOldSS);
    }

    HGDIOBJ hObjOld = (HGDIOBJ)SelectObject(hdc, _hbmScreenshotLight);
    POINT ptOriginOld;
    BitBlt(
        hdc,
        prcPaint->left, prcPaint->top,
        RECTWIDTH(*prcPaint), RECTHEIGHT(*prcPaint),
        hdcBackbuffer,
        prcPaint->left, prcPaint->top,
        SRCCOPY
    );
    SelectObject(hdc, hObjOld);

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

    if (RECTWIDTH(_rcSelection) > RECTWIDTH(rcSelectionOld) && RECTHEIGHT(_rcSelection) > RECTHEIGHT(rcSelectionOld))
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
        SendMessage(
            _hwndRenderTarget, 
            _bmp.fHasAnySelectionMade
                ? CScreenshotEditorWindow::WM_SCREENSHOTEDITOR_BEGINMARQUEETIMER
                : CScreenshotEditorWindow::WM_SCREENSHOTEDITOR_ENDMARQUEETIMER,
            0, 0
        );
    }

    return S_OK;
}

void CScreenshotEditorRendererGDI::UpdateMarquee()
{
    _iSelMarqueeFrame++;
    if (_iSelMarqueeFrame >= 6)
        _iSelMarqueeFrame = 0;
    _bmp.fSelectionBorderAnimDirty = true;

    ///RECT rcUpdate = _rcSelection;
    //InflateRect(&rcUpdate, 2, 2);
    InvalidateRect(_hwndRenderTarget, &_rcSelection, FALSE);
}

HRESULT CScreenshotEditorRendererGDI::_DrawMarqueeDottedRectangle(HDC hdc, RECT *prc)
{
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

//
// CScreenshotEditorWindow
//

LRESULT CScreenshotEditorWindow::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
            break;
        }

        case WM_DESTROY:
        {
            return _OnDestroy();
        }

        case WM_PAINT:
        {
            return _OnPaint();
        }

        case WM_KEYDOWN:
        {
            return _OnKeyDown(wParam, lParam);
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

        case WM_TIMER:
        {
            if (wParam == c_idTimerMarquee)
            {
                _pRenderer->UpdateMarquee();
            }

            return 0;
        }

        case WM_SCREENSHOTEDITOR_GETWINDOWPOSITIONS:
        {
            // Update the cursor now that enumeration is complete.
            _UpdateCursor();
            return 0;
        }

        case WM_SCREENSHOTEDITOR_BEGINMARQUEETIMER:
        {
            SetTimer(_hwnd, c_idTimerMarquee, 60, nullptr);
            return 0;
        }

        case WM_SCREENSHOTEDITOR_ENDMARQUEETIMER:
        {
            KillTimer(_hwnd, c_idTimerMarquee);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CScreenshotEditorWindow::_OnDestroy()
{
    delete _pRenderer;
    delete _pScreenshotCtx;
    return 0;
}

LRESULT CScreenshotEditorWindow::_OnPaint()
{
    if (!_pScreenshotCtx)
    {
        // If we don't have a screenshot context yet, then don't even try
        // to paint. I don't know this condition to occur, but just in case.
        return ERROR_NOT_READY;
    }

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(_hwnd, &ps);

    _pRenderer->Paint(hdc, &ps.rcPaint);
    
    EndPaint(_hwnd, &ps);
    return 0;
}

LRESULT CScreenshotEditorWindow::_OnKeyDown(WPARAM virtualKey, LPARAM lParam)
{
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
            if (SUCCEEDED(_pScreenshotCtx->Crop(&_rcSelection))
                && SUCCEEDED(_pScreenshotCtx->CopyToClipboard()))
            {
                DestroyWindow(_hwnd);
            }
            else
            {
                MessageBox(_hwnd, TEXT("Failed to copy image to clipboard!"), TEXT("Error"), MB_OK | MB_ICONERROR);
            }
            
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
    }
    else if (newTool == SSET_DRAG)
    {
        _tool = SSET_DRAG;
    }

    _UpdateCursor();
    return S_OK;
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
}

HRESULT CScreenshotEditorWindow::_OnGetWindowPositions()
{
    _fEnumeratedWindows = true;
    PostMessage(_hwnd, WM_SCREENSHOTEDITOR_GETWINDOWPOSITIONS, 0, 0);
    return S_OK;
}

// static
HRESULT CScreenshotEditorWindow::RegisterWindowClass()
{
    WNDCLASS cls = {};
    cls.hInstance = g_hinst;
    cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
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
