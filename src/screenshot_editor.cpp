#include "pch.h"
#include "screenshot_editor.h"
#include "extmgr.h"
#include "resource.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include "util.h"
#include <assert.h>
#include "cursor.h"
#include "cfgmgr.h"

//
// CEditorToolbar
//

LRESULT CEditorToolbar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
            _pEditor = (CScreenshotEditorWindow *)pcs->lpCreateParams;
            return _OnCreate(pcs);
        }

        case WM_DESTROY:
        {
            return _OnDestroy();
        }

        case WM_SIZE:
        {
            int iWidth = LOWORD(lParam);
            int iHeight = HIWORD(lParam);

            SetWindowPos(_hwndToolbar, nullptr, 0, 0, iWidth, iHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

            break;
        }

        // Key input should pass through to the editor if the toolbar is
        // activated.
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            PostMessage(_pEditor->GetHWND(), uMsg, wParam, lParam);
            break;
        }

        case WM_COMMAND:
        {
            return _OnCommand(wParam, lParam);
        }

        case WM_NOTIFY:
        {
            NMHDR *pnmh = (NMHDR *)lParam;
            bool fHandled = false;
            LRESULT lr = _OnNotify(pnmh, wParam, &fHandled);
            if (fHandled)
                return lr;
        }

        default:
        {
            if (_hwndToolbar && uMsg >= WM_USER)
            {
                return SendMessage(_hwndToolbar, uMsg, wParam, lParam);
            }
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CEditorToolbar::_OnCreate(CREATESTRUCT *pcs)
{
    _hwndToolbar = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        nullptr,
        WS_VISIBLE | WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_TOP,
        0, 0,
        0, 0,
        _hwnd,
        nullptr,
        g_hinst,
        nullptr
    );

    if (!_hwndToolbar)
    {
        return -1;
    }

    SendMessage(_hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

    int cExtTools = _pEditor->GetExtensionToolCount();
    int cSuccessfulTools = 0;
    TBBUTTON *rgtbButtons = new TBBUTTON[3 + cExtTools];
    DBGPRINT(TEXT("Extension tool count: %d"), cExtTools);
    ZeroMemory(rgtbButtons, sizeof(TBBUTTON) * (3 + cExtTools));
    {
        int &i = cSuccessfulTools;

        int iSizeIcon = MulDiv(16, 96, _uDpi);
        HICON hiconSelect = (HICON)LoadImage(g_hinst, MAKEINTRESOURCE(IDI_TOOLSELECT), IMAGE_ICON, iSizeIcon, iSizeIcon, LR_DEFAULTCOLOR);
        HICON hiconMove = (HICON)LoadImage(g_hinst, MAKEINTRESOURCE(IDI_TOOLMOVE), IMAGE_ICON, iSizeIcon, iSizeIcon, LR_DEFAULTCOLOR);
        HICON hiconCursorShow = (HICON)LoadImage(g_hinst, MAKEINTRESOURCE(IDI_TOOLCURSORSHOW), IMAGE_ICON, iSizeIcon, iSizeIcon, LR_DEFAULTCOLOR);

        int idxIcon = 0;

        int idxIconSelect = 0;
        int idxIconMove = 0;
        int idxIconCursorShow = 0;

        {
            HBITMAP hbm = HICONToHBITMAP(hiconSelect);
            _vhbmIcons.Push(hbm);
            DestroyIcon(hiconSelect);

            TBADDBITMAP ab;
            ab.hInst = nullptr;
            ab.nID = (UINT_PTR)hbm;
            SendMessage(_hwndToolbar, TB_ADDBITMAP, 1, (LPARAM)&ab);
            idxIconSelect = idxIcon++;
        }

        {
            HBITMAP hbm = HICONToHBITMAP(hiconMove);
            _vhbmIcons.Push(hbm);
            DestroyIcon(hiconMove);

            TBADDBITMAP ab;
            ab.hInst = nullptr;
            ab.nID = (UINT_PTR)hbm;
            SendMessage(_hwndToolbar, TB_ADDBITMAP, 1, (LPARAM)&ab);
            idxIconMove = idxIcon++;
        }

        {
            HBITMAP hbm = HICONToHBITMAP(hiconCursorShow);
            _vhbmIcons.Push(hbm);
            DestroyIcon(hiconCursorShow);

            TBADDBITMAP ab;
            ab.hInst = nullptr;
            ab.nID = (UINT_PTR)hbm;
            SendMessage(_hwndToolbar, TB_ADDBITMAP, 1, (LPARAM)&ab);
            idxIconCursorShow = idxIcon++;
        }

        rgtbButtons[i].idCommand = SSET_SELECT;
        rgtbButtons[i].dwData = (INT_PTR)TEXT("Select");
        rgtbButtons[i].fsState = TBSTATE_ENABLED;
        rgtbButtons[i].fsStyle = BTNS_CHECK;
        rgtbButtons[i].iBitmap = idxIconSelect;

        i++;
        rgtbButtons[i].idCommand = SSET_DRAG;
        rgtbButtons[i].dwData = (INT_PTR)TEXT("Move");
        rgtbButtons[i].fsState = TBSTATE_ENABLED;
        rgtbButtons[i].fsStyle = BTNS_CHECK;
        rgtbButtons[i].iBitmap = idxIconMove;

        i++;
        rgtbButtons[i].idCommand = SSET_SHOWCURSOR;
        rgtbButtons[i].dwData = (INT_PTR)TEXT("Show cursor");
        rgtbButtons[i].fsState = TBSTATE_ENABLED;
        if (_pEditor->IsCursorShown())
            rgtbButtons[i].fsState |= TBSTATE_CHECKED;
        rgtbButtons[i].fsStyle = BTNS_CHECK;
        rgtbButtons[i].iBitmap = idxIconCursorShow;

        for (int j = 0; j < _pEditor->GetExtensionToolCount(); j++)
        {
            ExtensionToolInfo eti;
            if (SUCCEEDED(_pEditor->GetExtensionToolInfo(j, &eti)))
            {
                SIZE size;
                size.cx = size.cy = MulDiv(16, 96, _uDpi);
                HICON hiconExtTool = eti.pTool->GetToolIcon(size);
                int idxExtToolIcon = 0;
                if (hiconExtTool)
                {
                    HBITMAP hbm = HICONToHBITMAP(hiconExtTool);
                    _vhbmIcons.Push(hbm);
                    DestroyIcon(hiconExtTool);

                    TBADDBITMAP ab;
                    ab.hInst = nullptr;
                    ab.nID = (UINT_PTR)hbm;
                    SendMessage(_hwndToolbar, TB_ADDBITMAP, 1, (LPARAM)&ab);
                    idxExtToolIcon = idxIcon++;
                }
                else
                {
                    HBITMAP hbm = nullptr;
                    if (SUCCEEDED(_GenerateToolIcon(eti.pszToolName, &hbm)))
                    {
                        _vhbmIcons.Push(hbm);
                        TBADDBITMAP ab;
                        ab.hInst = nullptr;
                        ab.nID = (UINT_PTR)hbm;
                        SendMessage(_hwndToolbar, TB_ADDBITMAP, 1, (LPARAM)&ab);
                        idxExtToolIcon = idxIcon++;
                    }
                }

                i++;
                rgtbButtons[i].idCommand = eti.idTool;
                rgtbButtons[i].dwData = (INT_PTR)eti.pszToolName;
                rgtbButtons[i].fsState = TBSTATE_ENABLED;
                rgtbButtons[i].fsStyle = BTNS_CHECK;
                rgtbButtons[i].iBitmap = idxExtToolIcon;
            }
            else
            {
                DBGPRINT(TEXT("Failed to get extension tool info."));
            }
        }
    }

    SendMessage(_hwndToolbar, TB_ADDBUTTONS, cSuccessfulTools + 1, (LPARAM)rgtbButtons);

    delete[] rgtbButtons;

    return DefWindowProc(_hwnd, WM_CREATE, 0, (LPARAM)pcs);
}

LRESULT CEditorToolbar::_OnDestroy()
{
    FOR_EACH_DYNARR(HBITMAP hbm, _vhbmIcons)
    {
        DeleteObject(hbm);
    }

    return DefWindowProc(_hwnd, WM_DESTROY, 0, 0);
}

LRESULT CEditorToolbar::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    WPARAM toolRequested = LOWORD(wParam);

    if (toolRequested == SSET_SHOWCURSOR)
    {
        // This tool does not receive selection.
        if (!SendMessage(_hwndToolbar, TB_ISBUTTONCHECKED, SSET_SHOWCURSOR, 0))
        {
            SendMessage(_pEditor->GetHWND(), CScreenshotEditorWindow::WM_SSE_HIDECURSOR, 0, 0);
        }
        else
        {
            SendMessage(_pEditor->GetHWND(), CScreenshotEditorWindow::WM_SSE_SHOWCURSOR, 0, 0);
        }
    }
    else
    {
        SendMessage(_pEditor->GetHWND(), CScreenshotEditorWindow::WM_SSE_CHANGETOOL, toolRequested, 0);
    }

    return 0;
}

LRESULT CEditorToolbar::_OnNotify(NMHDR *pnmh, WPARAM wParam, bool *pfHandled)
{
    if (pnmh->code == TTN_GETDISPINFO)
    {
        NMTTDISPINFO *pDispInfo = (NMTTDISPINFO *)pnmh;

        TBBUTTONINFO tbbi = { sizeof(tbbi) };
        tbbi.dwMask = TBIF_LPARAM;
        if (SendMessage(_hwndToolbar, TB_GETBUTTONINFO, pDispInfo->hdr.idFrom, (LPARAM)&tbbi) > -1)
        {
            pDispInfo->lpszText = (TCHAR *)tbbi.lParam;
            return 0;
        }
    }

    return 0;
}

HRESULT CEditorToolbar::_GenerateToolIcon(const TCHAR *pszToolName, OUT HBITMAP *phbmOut)
{
    const int iSizeIcon = MulDiv(16, 96, _uDpi);

    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    HDC hdc = CreateCompatibleDC(hdcDesktop);
    HBITMAP hbm = CreateCompatibleBitmap(hdcDesktop, iSizeIcon, iSizeIcon);

    HGDIOBJ hBmpOld = SelectObject(hdc, hbm);
    HBRUSH hbr = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HGDIOBJ hBrushOld = SelectObject(hdc, hbr);

    HPEN hpen = (HPEN)GetStockObject(BLACK_PEN);
    HGDIOBJ hPenOld = SelectObject(hdc, hpen);

    Rectangle(hdc, 0, 0, iSizeIcon, iSizeIcon);

    NONCLIENTMETRICS ncm = { 0 };
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE);
    
    ncm.lfMessageFont.lfHeight = MulDiv(12, 96, _uDpi);
    HFONT hFont = CreateFontIndirect(&ncm.lfMessageFont);
    HGDIOBJ hFontOld = SelectObject(hdc, hFont);

    RECT rcText;
    rcText.left = rcText.top = 0;
    rcText.right = rcText.bottom = iSizeIcon;
    InflateRect(&rcText, -1, -1);
    DrawText(hdc, pszToolName, 1, &rcText, DT_CENTER | DT_VCENTER);

    *phbmOut = hbm;

    SelectObject(hdc, hPenOld);
    SelectObject(hdc, hBrushOld);
    SelectObject(hdc, hFontOld);
    DeleteObject(hFont);
    SelectObject(hdc, hBmpOld);
    DeleteDC(hdc);
    ReleaseDC(HWND_DESKTOP, hdcDesktop);
    return S_OK;
}

HRESULT CEditorToolbar::_UnselectTool()
{
    for (int i = 0, j = SendMessage(_hwndToolbar, TB_BUTTONCOUNT, 0, 0); i < j; i++)
    {
        TBBUTTON tbb = { 0 };
        SendMessage(_hwndToolbar, TB_GETBUTTON, i, (LPARAM)&tbb);

        if (tbb.fsState & TBSTATE_CHECKED && tbb.idCommand != SSET_SHOWCURSOR)
        {
            SendMessage(_hwndToolbar, TB_CHECKBUTTON, tbb.idCommand, FALSE);
        }
    }

    return S_OK;
}

// static
HRESULT CEditorToolbar::RegisterWindowClass()
{
    WNDCLASS cls = { 0 };
    cls.hInstance = g_hinst;
    cls.hbrBackground = nullptr;
    cls.hCursor = nullptr;
    cls.hIcon = nullptr;

    return CWindow::RegisterWindowClass(&cls);
}

// static
CEditorToolbar *CEditorToolbar::Create(CScreenshotEditorWindow *pEditor, DWORD dwExStyle, DWORD dwStyle, int x, int y, int cx, int cy, HWND hwndParent)
{
    if (FAILED(RegisterWindowClass()))
    {
        return nullptr;
    }

    CEditorToolbar *pWnd = CWindow::Create(
        dwExStyle,
        nullptr,
        dwStyle,
        x, y,
        cx, cy,
        hwndParent,
        nullptr,
        g_hinst,
        pEditor
    );

    return pWnd;
}

HRESULT CEditorToolbar::SelectOrdinalTool(int idx)
{
    int cButtons = SendMessage(_hwndToolbar, TB_BUTTONCOUNT, 0, 0);

    if (idx <= cButtons)
    {
        TBBUTTON tbb;
        SendMessage(_hwndToolbar, TB_GETBUTTON, idx - 1, (LPARAM)&tbb);

        SendMessage(_pEditor->GetHWND(), CScreenshotEditorWindow::WM_SSE_CHANGETOOL, tbb.idCommand, 0);
        return S_OK;
    }

    return E_BOUNDS;
}

HRESULT CEditorToolbar::OnToolChanged(ScreenshotEditorTool toolNew)
{
    ASSERT_KEEP(SUCCEEDED(_UnselectTool()));

    // If the requested tool has no toolbar item, then this will supposedly fail.
    ASSERT_KEEP(SendMessage(_hwndToolbar, TB_CHECKBUTTON, toolNew, TRUE));

    return S_OK;
}

//
// CEditorActionsStrip
//

LRESULT CEditorActionsStrip::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
            _pEditor = (CScreenshotEditorWindow *)pcs->lpCreateParams;
            return _OnCreate(pcs);
        }

        case WM_DESTROY:
        {
            return _OnDestroy();
        }

        case WM_SIZE:
        {
            int iWidth = LOWORD(lParam);
            int iHeight = HIWORD(lParam);

            SetWindowPos(_hwndActions, nullptr, 0, 0, iWidth, iHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

            break;
        }

        // Key input should pass through to the editor if the floating toolbar is
        // activated.
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            PostMessage(_pEditor->GetHWND(), uMsg, wParam, lParam);
            break;
        }

        case WM_COMMAND:
        {
            return _OnCommand(wParam, lParam);
        }

        case WM_NOTIFY:
        {
            NMHDR *pnmh = (NMHDR *)lParam;
            bool fHandled = false;
            LRESULT lr = _OnNotify(pnmh, wParam, &fHandled);
            if (fHandled)
                return lr;
        }

        default:
        {
            if (_hwndActions && uMsg >= WM_USER)
            {
                return SendMessage(_hwndActions, uMsg, wParam, lParam);
            }
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CEditorActionsStrip::_OnCreate(CREATESTRUCT *pcs)
{
    _hwndActions = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        nullptr,
        WS_VISIBLE | WS_CHILD | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NORESIZE,
        0, 0,
        0, 0,
        _hwnd,
        nullptr,
        g_hinst,
        nullptr
    );

    if (!_hwndActions)
    {
        return E_FAIL;
    }

    SendMessage(_hwndActions, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(_hwndActions, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_DRAWDDARROWS);

    _LoadIcons();

    TBBUTTON rgtbButTools[4] = { 0 };
    {
        TBADDBITMAP ab;
        ab.hInst = HINST_COMMCTRL;
        ab.nID = IDB_STD_SMALL_COLOR;
        SendMessage(_hwndActions, TB_ADDBITMAP, 15, (LPARAM)&ab);

        int iIconIdx = 15;

        const int iIconDiscard = iIconIdx++;
        {
            TBADDBITMAP ab2;
            ab2.hInst = nullptr;
            ab2.nID = (UINT_PTR)_hbmIconDiscard;
            SendMessage(_hwndActions, TB_ADDBITMAP, 1, (LPARAM)&ab2);
        }

        const int iIconCopy = iIconIdx++;
        {
            TBADDBITMAP ab2;
            ab2.hInst = nullptr;
            ab2.nID = (UINT_PTR)_hbmIconCopy;
            SendMessage(_hwndActions, TB_ADDBITMAP, 1, (LPARAM)&ab2);
        }

        const int iIconSave = iIconIdx++;
        {
            TBADDBITMAP ab2;
            ab2.hInst = nullptr;
            ab2.nID = (UINT_PTR)_hbmIconSave;
            SendMessage(_hwndActions, TB_ADDBITMAP, 1, (LPARAM)&ab2);
        }

        int i = 0;

        rgtbButTools[i].idCommand = IDM_OPENINEXTERNALEDITOR;
        rgtbButTools[i].iString = (INT_PTR)TEXT("Open in...");
        rgtbButTools[i].fsState = TBSTATE_ENABLED;
        rgtbButTools[i].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_DROPDOWN;
        rgtbButTools[i].iBitmap = STD_FILENEW; // Temporary icon.

        i++;
        rgtbButTools[i].idCommand = IDM_COPY;
        rgtbButTools[i].iString = (INT_PTR)TEXT("Copy");
        rgtbButTools[i].fsState = TBSTATE_ENABLED;
        rgtbButTools[i].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
        rgtbButTools[i].iBitmap = _hbmIconCopy ? iIconCopy : STD_COPY;

        i++;
        rgtbButTools[i].idCommand = IDM_SAVE;
        rgtbButTools[i].iString = (INT_PTR)TEXT("Save");
        rgtbButTools[i].fsState = TBSTATE_ENABLED;
        rgtbButTools[i].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
        rgtbButTools[i].iBitmap = _hbmIconSave ? iIconSave : STD_FILESAVE;

        i++;
        rgtbButTools[i].idCommand = IDM_DISCARD;
        rgtbButTools[i].iString = (INT_PTR)TEXT("Discard");
        rgtbButTools[i].fsState = TBSTATE_ENABLED;
        rgtbButTools[i].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
        rgtbButTools[i].iBitmap = _hbmIconDiscard ? iIconDiscard : STD_DELETE;
    }

    SendMessage(_hwndActions, TB_ADDBUTTONS, ARRAYSIZE(rgtbButTools), (LPARAM)&rgtbButTools);

    return DefWindowProc(_hwnd, WM_CREATE, 0, (LPARAM)pcs);
}

LRESULT CEditorActionsStrip::_OnDestroy()
{
    if (_hbmIconDiscard)
        DeleteObject(_hbmIconDiscard);
    if (_hbmIconCopy)
        DeleteObject(_hbmIconCopy);
    if (_hbmIconSave)
        DeleteObject(_hbmIconSave);

    return DefWindowProc(_hwnd, WM_DESTROY, 0, 0);
}

LRESULT CEditorActionsStrip::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = _pEditor->GetHWND();

    switch (LOWORD(wParam))
    {
        case IDM_DISCARD:
        {
            DestroyWindow(hwndEditor);
            break;
        }

        case IDM_COPY:
        {
            SendMessage(hwndEditor, CScreenshotEditorWindow::WM_SSE_COPYTOCLIPBOARD, 0, 0);
            break;
        }

        case IDM_SAVE:
        {
            SendMessage(hwndEditor, CScreenshotEditorWindow::WM_SSE_SAVEIMAGE, 0, 0);
            break;
        }

        case IDM_OPENINEXTERNALEDITOR:
        {
            MessageBox(hwndEditor, TEXT("This operation has yet to be implemented."), TEXT("Unimplemented!"), MB_OK | MB_ICONERROR);
            break;
        }
    }

    return 0;
}

LRESULT CEditorActionsStrip::_OnNotify(NMHDR *pnmh, WPARAM wParam, bool *pfHandled)
{
    if (pnmh->code == NM_CUSTOMDRAW && pnmh->hwndFrom == _hwndActions)
    {
        NMTBCUSTOMDRAW *ptbcd = (NMTBCUSTOMDRAW *)pnmh;

        switch (ptbcd->nmcd.dwDrawStage)
        {
            case CDDS_PREPAINT:
            {
                RECT rc;
                GetClientRect(pnmh->hwndFrom, &rc);
                FillRect(ptbcd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));
                *pfHandled = true;
                return CDRF_DODEFAULT;
            }
        }
    }

    return 0;
}

HRESULT CEditorActionsStrip::_LoadIcons()
{
    if (GetOSVersion()->dwMajorVersion >= 5 && !(GetOSVersion()->dwMajorVersion == 5 && GetOSVersion()->dwMinorVersion < 1))
    {
        HMODULE hmShell32 = LoadLibrary(TEXT("shell32.dll"));
        const int iSizeIcon = MulDiv(16, 96, _uDpi);

        HICON hiconDiscard = (HICON)LoadImage(hmShell32, MAKEINTRESOURCE(240), IMAGE_ICON, iSizeIcon, iSizeIcon, 0);
        if (hiconDiscard)
        {
            _hbmIconDiscard = HICONToHBITMAP(hiconDiscard);
            DestroyIcon(hiconDiscard);
        }

        HICON hiconCopy = (HICON)LoadImage(hmShell32, MAKEINTRESOURCE(243), IMAGE_ICON, iSizeIcon, iSizeIcon, 0);
        if (hiconCopy)
        {
            _hbmIconCopy = HICONToHBITMAP(hiconCopy);
            DestroyIcon(hiconCopy);
        }

        HICON hiconSave = (HICON)LoadImage(hmShell32, MAKEINTRESOURCE(16761), IMAGE_ICON, iSizeIcon, iSizeIcon, 0);
        if (hiconSave)
        {
            _hbmIconSave = HICONToHBITMAP(hiconSave);
            DestroyIcon(hiconSave);
        }
        else
        {
            // Windows XP does not have this icon, so we'll take it from the shell icons set
            // instead.
            // TODO: Implement!
        }

        FreeLibrary(hmShell32);
    }

    return S_OK;
}

// static
HRESULT CEditorActionsStrip::RegisterWindowClass()
{
    WNDCLASS cls = { 0 };
    cls.hInstance = g_hinst;
    cls.hbrBackground = nullptr;
    cls.hCursor = nullptr;
    cls.hIcon = nullptr;

    return CWindow::RegisterWindowClass(&cls);
}

// static
CEditorActionsStrip *CEditorActionsStrip::Create(CScreenshotEditorWindow *pEditor, DWORD dwExStyle, DWORD dwStyle, int x, int y, int cx, int cy, HWND hwndParent)
{
    if (FAILED(RegisterWindowClass()))
    {
        return nullptr;
    }

    CEditorActionsStrip *pWnd = CWindow::Create(
        dwExStyle,
        nullptr,
        dwStyle,
        x, y,
        cx, cy,
        hwndParent,
        nullptr,
        g_hinst,
        pEditor
    );

    return pWnd;
}

//
// CEditorFloatingToolbar
//

LRESULT CEditorFloatingToolbar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
            _pToolbarTools = (CEditorToolbar *)pcs->lpCreateParams;
            _pEditor = _pToolbarTools->GetOwnerEditor();

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
            pwp->hwndInsertAfter = GetWindow(_pEditor->GetHWND(), GW_HWNDPREV);
            pwp->flags &= ~SWP_NOZORDER;

            return 0;
        }

        // Key input should pass through to the editor if the floating toolbar is
        // activated.
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            PostMessage(_pEditor->GetHWND(), uMsg, wParam, lParam);
            break;
        }

        case WM_COMMAND:
        {
            return _OnCommand(wParam, lParam);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HRESULT CEditorFloatingToolbar::_OnCreate()
{
    RECT rcClient;
    GetClientRect(_hwnd, &rcClient);

    SetParent(_pToolbarTools->GetHWND(), _hwnd);
    SetWindowPos(_pToolbarTools->GetHWND(), nullptr, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER);
    _pActionStrip = CEditorActionsStrip::Create(_pEditor, 0, WS_CHILD, 0, 0, 0, 0, _hwnd);

    SIZE sizeActionsIdeal;
    SendMessage(_pToolbarTools->GetHWND(), TB_GETMAXSIZE, FALSE, (LPARAM)&sizeActionsIdeal);
    
    RECT rcActions;
    rcActions.left = 0;
    rcActions.top = 0;
    rcActions.right = rcActions.left;
    rcActions.bottom = rcActions.top + sizeActionsIdeal.cy;

    // Getting the maximum size of the toolbar doesn't actually work out so well, so we will just
    // calculate it manually...
    for (int i = 0, j = SendMessage(_pActionStrip->GetHWND(), TB_BUTTONCOUNT, 0, 0); i < j; i++)
    {
        RECT rcItem;
        SendMessage(_pActionStrip->GetHWND(), TB_GETITEMRECT, i, (LPARAM)&rcItem);

        if (RECTHEIGHT(rcActions) < RECTHEIGHT(rcItem))
        {
            rcActions.bottom = rcActions.top + RECTHEIGHT(rcItem);
        }

        rcActions.right += RECTWIDTH(rcItem);
    }

    // Cooperating with the autosize logic is incredibly annoying. We set the window size
    // intermediately here in order to make the TB_AUTOSIZE message for the next toolbar
    // scale to just about the right size. I cannot get it to scale to the right size,
    // and if I wanted to use CCS_NORESIZE, then I would need to recreate the behavior.
    // I might just do that anyway since I think it would get the best results.
    {
        DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
        DWORD dwExStyle = GetWindowLong(_hwnd, GWL_EXSTYLE);

        RECT rcAdjusted;
        rcAdjusted.left = rcAdjusted.top = 0;
        rcAdjusted.right = RECTWIDTH(rcActions);
        rcAdjusted.bottom = RECTHEIGHT(rcActions);
        AdjustWindowRectEx(&rcAdjusted, dwStyle, FALSE, dwExStyle);

        SetWindowPos(
            _pToolbarTools->GetHWND(),
            nullptr,
            0, 0,
            RECTWIDTH(rcActions), RECTHEIGHT(rcActions),
            SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED
        );
    }

    SendMessage(_pToolbarTools->GetHWND(), TB_AUTOSIZE, 0, 0);

    RECT rcTools;
    GetWindowRect(_pToolbarTools->GetHWND(), &rcTools);
    MapWindowPoints(HWND_DESKTOP, GetParent(_pToolbarTools->GetHWND()), (LPPOINT)&rcTools, 2);

    // Move the actions strip to appear below the toolbox:
    OffsetRect(&rcActions, 0, rcTools.bottom);

    // I feel that the actions strip looks a bit off with the height that common
    // controls gives, so I'll make it a little bigger manually:
    rcActions.bottom += 2;

    RECT rcUnion;
    UnionRect(&rcUnion, &rcTools, &rcActions);

    DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
    DWORD dwExStyle = GetWindowLong(_hwnd, GWL_EXSTYLE);

    RECT rcAdjusted;
    rcAdjusted.left = rcAdjusted.top = 0;
    rcAdjusted.right = RECTWIDTH(rcUnion);
    rcAdjusted.bottom = RECTHEIGHT(rcUnion);
    AdjustWindowRectEx(&rcAdjusted, dwStyle, FALSE, dwExStyle);

    SetWindowPos(
        _pActionStrip->GetHWND(),
        _pToolbarTools->GetHWND(),
        rcActions.left, rcActions.top,
        RECTWIDTH(rcAdjusted), RECTHEIGHT(rcActions),
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
    );
    SetWindowPos(
        _pToolbarTools->GetHWND(),
        nullptr,
        rcTools.left, rcTools.top,
        RECTWIDTH(rcAdjusted), RECTHEIGHT(rcTools),
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
    );

    SetWindowPos(
        _hwnd,
        nullptr,
        0, 0,
        RECTWIDTH(rcAdjusted), RECTHEIGHT(rcAdjusted),
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED
    );

    ShowWindow(_pToolbarTools->GetHWND(), SW_SHOW);
    ShowWindow(_pActionStrip->GetHWND(), SW_SHOW);

    return S_OK;
}

LRESULT CEditorFloatingToolbar::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (LOWORD(wParam) >= CEditorActionsStrip::IDM_TOOLFIRST)
    {
        SendMessage(_pToolbarTools->GetHWND(), WM_COMMAND, wParam - CEditorActionsStrip::IDM_TOOLFIRST, lParam);
    }
    else switch (LOWORD(wParam))
    {
        SendMessage(_pActionStrip->GetHWND(), WM_COMMAND, wParam, lParam);
    }

    return 0;
}

// static
HRESULT CEditorFloatingToolbar::RegisterWindowClass()
{
    WNDCLASS cls = { 0 };
    cls.hInstance = g_hinst;
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
    cls.hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_APP));

    return CWindow::RegisterWindowClass(&cls);
}

// static
CEditorFloatingToolbar *CEditorFloatingToolbar::Create(CEditorToolbar *pToolbar)
{
    if (FAILED(RegisterWindowClass()))
    {
        return nullptr;
    }

    DWORD dwExStyle = WS_EX_PALETTEWINDOW;

    // If running under Linux, the Windows palette window style results in the non-client area
    // using Wine's clone of the Windows classic theme rather than deferring to the Linux
    // window manager, which is ugly in my opinion.
    if (GetOSVersion()->flags & OSVF_WINE)
    {
        dwExStyle &= ~WS_EX_PALETTEWINDOW;
    }

    CEditorFloatingToolbar *pWnd = CWindow::Create(
        dwExStyle,
        TEXT("Tools"),
        WS_CAPTION | WS_SYSMENU,
        0, 0,
        0, 0,
        nullptr,
        nullptr,
        g_hinst,
        pToolbar
    );

    return pWnd;
}

//
// CScreenshotEditorRendererGDI
//

CScreenshotEditorRendererGDI::~CScreenshotEditorRendererGDI()
{
    DeleteObject(_hpenSelect);
    DeleteObject(_hpenSizingHelpers);
    DeleteObject(_hbmScreenshotDimmed);

    if (_bmp.fCopiedScreenshot)
    {
        DeleteObject(_hbmScreenshotLight);
    }
}

HRESULT CScreenshotEditorRendererGDI::Initialize()
{
    POINT ptCursor = _pScreenshotCtx->_ptCursor;
    ScreenToClient(_hwndRenderTarget, &ptCursor);
    CCursorRenderer cursorRenderer(
        _pScreenshotCtx->_hbmScreenshot,
        _pScreenshotCtx->_hcursor,
        _pScreenshotCtx->_hbmCursorColor,
        _pScreenshotCtx->_hbmCursorMask,
        ptCursor
    );
    cursorRenderer.RenderCursor(&_hbmCursor, &_rcCursor);

    HRESULT hr = _MakeDimmedScreenshot();
    _hpenSelect = CreatePen(PS_DOT, 1, RGB(128, 128, 128));
    _hpenSizingHelpers = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));

    ASSERT_KEEP(SUCCEEDED(_MakeHideCursorBuffers()));

    bool fShowCursor = true;
    if (FAILED(CConfigManager::GetInstance()->GetBool(TEXT("HideCursorByDefault"), &fShowCursor)))
    {
        fShowCursor = true;
    }
    else
    {
        fShowCursor = !fShowCursor;
    }

    if (fShowCursor)
    {
        ASSERT_KEEP(SUCCEEDED(ShowCursor()));
    }

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
        HGDIOBJ hOldBmp = SelectObject(hdcScreenshot, _hbmScreenshotLight);
        BitBlt(
            hdcSelection,
            _rcSelection.left, _rcSelection.top,
            RECTWIDTH(_rcSelection), RECTHEIGHT(_rcSelection),
            hdcScreenshot,
            _rcSelection.left, _rcSelection.top,
            SRCCOPY
        );

        // If we have an object selected, then we want that to be the frontmost selection outline.
        // In this case, we will render the guidelines in the back.
        if (_pRenderObjSel)
        {
            _PaintSelectionRectangle(hdcSelection, &_rcSelection, false);
        }

        SelectObject(hdcScreenshot, hOldBmp);
        SelectObject(hdcScreenshot, _hbmScreenshotDimmed);
    }

    // Paint objects:
    {
        HDC hdcSelection = fUseBackbuffer
            ? hdcBackbuffer
            : hdc;

        // Paint all objects from back to front.
        for (int i = 0, j = _vRenderObjs.GetSize(); i < j; i++)
        {
            CRenderObject *pRenderObject = &_vRenderObjs[i];

            RECT rcLogical;
            RECT rcVisual;
            if (SUCCEEDED(pRenderObject->_pObj->GetLogicalRect(&rcLogical))
                && SUCCEEDED(pRenderObject->_pObj->GetVisualRect(&rcVisual)))
            {
                OffsetRect(&rcVisual, rcLogical.left, rcLogical.top);

                RECT rcTemp;
                if (!IntersectRect(&rcTemp, &rcVisual, prcPaint))
                {
                    // Skip over objects that can't be reasonably drawn anyway.
                    continue;
                }

                ASSERT_KEEP(SUCCEEDED(_PaintRenderObjectVisualBuffer(hdcSelection, prcPaint, pRenderObject)));
            }
        }
    }
    
    // If we have a selection made and no object is selected, then we want to render the
    // selection outline for the screenshot selection in front.
    if (_bmp.fHasAnySelectionMade)
    {
        HDC hdcSelection = fUseBackbuffer
            ? hdcBackbuffer
            : hdc;

        if (!_pRenderObjSel)
        {
            _PaintSelectionRectangle(hdcSelection, &_rcSelection, _bmp.fDrawMarqueeSelection);
            if (_bmp.fDrawSelectionSizeHelpers)
            {
                _PaintSizingHelpers(hdcSelection, &_rcSelection);
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
    _bmp.fAnyObjectDirty = false;

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::UpdateSelection(RECT *prcNew)
{
    RECT rcSelectionOld = _rcSelectionVisual;
    _rcSelection = *prcNew;

    _rcSelectionVisual = *prcNew;
    if (_bmp.fDrawSelectionSizeHelpers)
    {
        InflateRect(&_rcSelectionVisual, c_iRadiusSelHelper / 2, c_iRadiusSelHelper / 2);
    }

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
    UnionRect(&rcUnion, &rcSelectionOld, &_rcSelectionVisual);
    //InflateRect(&rcUnion, 2, 2);
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

HRESULT CScreenshotEditorRendererGDI::UpdateSizingHelpersVisibility(bool fVisible)
{
    if (fVisible != _bmp.fDrawSelectionSizeHelpers)
    {
        _bmp.fDrawSelectionSizeHelpers = fVisible;

        RECT rcRefresh = _rcSelection;
        InflateRect(&rcRefresh, c_iRadiusSelHelper / 2, c_iRadiusSelHelper / 2);

        if (fVisible)
        {
            _rcSelectionVisual = rcRefresh;
        }

        InvalidateRect(_hwndRenderTarget, &rcRefresh, FALSE);
    }
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

HRESULT CScreenshotEditorRendererGDI::ShowCursor()
{
    return _ShowHideCursor(true);
}

HRESULT CScreenshotEditorRendererGDI::HideCursor()
{
    return _ShowHideCursor(false);
}

bool CScreenshotEditorRendererGDI::IsCursorShown()
{
    return _bmp.fCursorVisible;
}

HRESULT CScreenshotEditorRendererGDI::CreateRenderObject(IScreenshotEditorObject *pObj)
{
    CRenderObject ro;

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
        RECT rcLogical = { 0 };
        ASSERT_KEEP(SUCCEEDED(pObj->GetLogicalRect(&rcLogical)));

        RECT rcVisual = { 0 };
        ASSERT_KEEP(SUCCEEDED(pObj->GetVisualRect(&rcVisual)));
        OffsetRect(&rcVisual, rcLogical.left, rcLogical.top);

        if (pro->_pRendererGdi)
            pro->_pRendererGdi->Release();
        if (pro->_pRenderer)
            pro->_pRenderer->Release();
        if (pro->_pObj)
            pro->_pObj->Release();

        _vRenderObjs.Remove(idxRenderObj);
        _bmp.fAnyObjectDirty = true;
        InvalidateRect(_hwndRenderTarget, &rcVisual, FALSE);
    }

    return E_NOT_SET;
}

HRESULT CScreenshotEditorRendererGDI::InvalidateRenderObject(IScreenshotEditorObject *pObj)
{
    RECT rcLogical = { 0 };
    if (SUCCEEDED(pObj->GetLogicalRect(&rcLogical)))
    {
        RECT rcVisual = { 0 };
        if (SUCCEEDED(pObj->GetVisualRect(&rcVisual)))
        {
            OffsetRect(&rcVisual, rcLogical.left, rcLogical.top);

            CRenderObject *pro = nullptr;
            if (SUCCEEDED(_FindRenderObjectFromInterfaceObject(pObj, &pro, nullptr)))
            {
                RECT rcUnion;
                UnionRect(&rcUnion, &rcVisual, &pro->_rcLatestVisual);

                _bmp.fAnyObjectDirty = true;
                InvalidateRect(_hwndRenderTarget, &rcUnion, FALSE);
                return S_OK;
            }
        }
    }

    assert(0);
    return E_FAIL;
}

HRESULT CScreenshotEditorRendererGDI::_PaintSelectionRectangle(HDC hdc, RECT *prc, bool fUseMarquee)
{
    HGDIOBJ hObjOldBB = SelectObject(hdc, _hpenSelect);
    HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    int iOldBkMode = SetBkMode(hdc, TRANSPARENT);
    int iOldRop = SetROP2(hdc, R2_XORPEN);

    if (!fUseMarquee || S_FALSE == _DrawMarqueeDottedRectangle(hdc, prc))
    {
        Rectangle(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }

    SetROP2(hdc, iOldRop);
    SetBkMode(hdc, iOldBkMode);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hObjOldBB);

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::_PaintSizingHelpers(HDC hdc, RECT *prc)
{
    HGDIOBJ hObjOldBB = SelectObject(hdc, _hpenSizingHelpers);
    HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    int iOldBkMode = SetBkMode(hdc, TRANSPARENT);
    int iOldRop = SetROP2(hdc, R2_XORPEN);

    static constexpr int c_iHelper = c_iRadiusSelHelper;
    static constexpr int c_iRadiusHalfHelper = (c_iRadiusSelHelper / 2);

    // Top-left
    Rectangle(
        hdc,
        prc->left - c_iRadiusHalfHelper,
        prc->top - c_iRadiusHalfHelper,
        prc->left + c_iRadiusHalfHelper,
        prc->top + c_iRadiusHalfHelper
    );

    // Top-center
    Rectangle(
        hdc,
        ((prc->right + prc->left) / 2) - c_iRadiusHalfHelper,
        prc->top - c_iRadiusHalfHelper,
        ((prc->right + prc->left) / 2) + c_iRadiusHalfHelper,
        prc->top + c_iRadiusHalfHelper
    );

    // Top-right
    Rectangle(
        hdc,
        prc->right - c_iRadiusHalfHelper,
        prc->top - c_iRadiusHalfHelper,
        prc->right + c_iRadiusHalfHelper,
        prc->top + c_iRadiusHalfHelper
    );

    // Center-left
    Rectangle(
        hdc,
        prc->left - c_iRadiusHalfHelper,
        ((prc->bottom + prc->top) / 2) - c_iRadiusHalfHelper,
        prc->left + c_iRadiusHalfHelper,
        ((prc->bottom + prc->top) / 2) + c_iRadiusHalfHelper
    );

    // Center-right
    Rectangle(
        hdc,
        prc->right - c_iRadiusHalfHelper,
        ((prc->bottom + prc->top) / 2) - c_iRadiusHalfHelper,
        prc->right + c_iRadiusHalfHelper,
        ((prc->bottom + prc->top) / 2) + c_iRadiusHalfHelper
    );

    // Bottom-left
    Rectangle(
        hdc,
        prc->left - c_iRadiusHalfHelper,
        prc->bottom - c_iRadiusHalfHelper,
        prc->left + c_iRadiusHalfHelper,
        prc->bottom + c_iRadiusHalfHelper
    );

    // Bottom-center
    Rectangle(
        hdc,
        ((prc->right + prc->left) / 2) - c_iRadiusHalfHelper,
        prc->bottom - c_iRadiusHalfHelper,
        ((prc->right + prc->left) / 2) + c_iRadiusHalfHelper,
        prc->bottom + c_iRadiusHalfHelper
    );

    // Bottom-right
    Rectangle(
        hdc,
        prc->right - c_iRadiusHalfHelper,
        prc->bottom - c_iRadiusHalfHelper,
        prc->right + c_iRadiusHalfHelper,
        prc->bottom + c_iRadiusHalfHelper
    );

    SetROP2(hdc, iOldRop);
    SetBkMode(hdc, iOldBkMode);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hObjOldBB);

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::_PaintRenderObjectVisualBuffer(
    HDC hdcRenderTarget, RECT *prcPaint, CRenderObject *pRenderObject)
{
    if (pRenderObject->HasGdiRenderer())
    {
        HDC hdcLayer = hdcRenderTarget;
        
        RECT rcLogical;
        RECT rcVisual;
        if (SUCCEEDED(pRenderObject->_pObj->GetLogicalRect(&rcLogical))
            && SUCCEEDED(pRenderObject->_pObj->GetVisualRect(&rcVisual)))
        {
            OffsetRect(&rcVisual, rcLogical.left, rcLogical.top);
            pRenderObject->_rcLatestVisual = rcVisual;

            HGDIOBJ hObjOld = SelectObject(hdcLayer, pRenderObject->_hbmLayer);

            POINT ptViewportOld;
            // We need to translate the viewport by the paint rect or the positioning will be 
            // off when painting small regions.
            SetViewportOrgEx(hdcLayer, -prcPaint->left + rcVisual.left, -prcPaint->top + rcVisual.top, &ptViewportOld);

            ASSERT_KEEP(SUCCEEDED(pRenderObject->_pRendererGdi->SetGdiParameters(hdcLayer, prcPaint)));
            ASSERT_KEEP(SUCCEEDED(pRenderObject->_pRendererGdi->Paint()));

            SelectObject(hdcLayer, hObjOld);

            SetViewportOrgEx(hdcLayer, ptViewportOld.x, ptViewportOld.y, nullptr);
        }

        return S_OK;
    }
    else
    {
        // Unimplemented.
        assert(0);
    }

    return E_NOTIMPL;
}

void CScreenshotEditorRendererGDI::_UpdateMarquee()
{
    _iSelMarqueeFrame++;
    if (_iSelMarqueeFrame >= 6)
        _iSelMarqueeFrame = 0;
    _bmp.fSelectionBorderAnimDirty = true;
    InvalidateRect(_hwndRenderTarget, &_rcSelectionVisual, FALSE);
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
    BITMAP bm = { 0 };
    if (GetObject(_pScreenshotCtx->_hbmScreenshot, sizeof(bm), &bm))
    {
        if (bm.bmBitsPixel == 32)
        {
            return _DimScreenshot32BPP();
        }
        else if (bm.bmBitsPixel == 24)
        {
            return _DimScreenshot24BPP();
        }
        else
        {
            return _DitherScreenshot(&bm);
        }
    }

    // We shouldn't really get here.
    return E_FAIL;
}

HRESULT CScreenshotEditorRendererGDI::_DimScreenshot32BPP()
{
    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    if (hdcDesktop)
    {
        HDC hdcDimmed = CreateCompatibleDC(hdcDesktop);
        if (hdcDimmed)
        {
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
            bmi.bmiHeader.biWidth = _pScreenshotCtx->_sizeDesktop.cx;
            bmi.bmiHeader.biHeight = -_pScreenshotCtx->_sizeDesktop.cy;
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

                int cLength = _pScreenshotCtx->_sizeDesktop.cx * _pScreenshotCtx->_sizeDesktop.cy;
                ULONG *pulSrc = (ULONG *)pvPixels;
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

HRESULT CScreenshotEditorRendererGDI::_DimScreenshot24BPP()
{
    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    if (hdcDesktop)
    {
        HDC hdcDimmed = CreateCompatibleDC(hdcDesktop);
        if (hdcDimmed)
        {
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
            bmi.bmiHeader.biWidth = _pScreenshotCtx->_sizeDesktop.cx;
            bmi.bmiHeader.biHeight = -_pScreenshotCtx->_sizeDesktop.cy;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 24;
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

                int iWidth = _pScreenshotCtx->_sizeDesktop.cx;
                int iHeight = _pScreenshotCtx->_sizeDesktop.cy;

                // Pad the number of bytes to a 4-byte boundary.
                int iStride = ((iWidth * 24 + 31) & ~31) >> 3;

                BYTE *pBaseRow = (BYTE *)pvPixels;
                ULONG ulDim = (0xFF - c_iDimAmount);

                for (int y = 0; y < iHeight; y++)
                {
                    BYTE *pPixel = pBaseRow;

                    for (int x = 0; x < iWidth; x++)
                    {
                        ULONG ulB = pPixel[0];
                        ULONG ulG = pPixel[1];
                        ULONG ulR = pPixel[2];

                        ulR = (ulR * c_iDimAmount + ulDim) >> 8;
                        ulG = (ulG * c_iDimAmount + ulDim) >> 8;
                        ulB = (ulB * c_iDimAmount + ulDim) >> 8;

                        pPixel[0] = (BYTE)ulB;
                        pPixel[1] = (BYTE)ulG;
                        pPixel[2] = (BYTE)ulR;

                        pPixel += 3;
                    }

                    pBaseRow += iStride;
                }

                SelectObject(hdcDimmed, hObjOld);
            }

            DeleteDC(hdcDimmed);
        }

        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::_DitherScreenshot(BITMAP *pbm)
{
    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    if (hdcDesktop)
    {
        HDC hdcDimmed = CreateCompatibleDC(hdcDesktop);
        if (hdcDimmed)
        {
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
            bmi.bmiHeader.biWidth = _pScreenshotCtx->_sizeDesktop.cx;
            bmi.bmiHeader.biHeight = -_pScreenshotCtx->_sizeDesktop.cy;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = pbm->bmBitsPixel;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage = 0;

            int iWidth = _pScreenshotCtx->_sizeDesktop.cx;
            int iHeight = _pScreenshotCtx->_sizeDesktop.cy;

            void *pvPixels = nullptr;
            _hbmScreenshotDimmed = CreateDIBSection(hdcDimmed, &bmi, DIB_RGB_COLORS, &pvPixels, nullptr, 0);
            if (_hbmScreenshotDimmed)
            {
                HGDIOBJ hObjOld = SelectObject(hdcDimmed, _hbmScreenshotDimmed);

                HDC hdcOrig = CreateCompatibleDC(hdcDesktop);
                HGDIOBJ hObjOld2 = SelectObject(hdcOrig, _pScreenshotCtx->_hbmScreenshot);

                BitBlt(hdcDimmed, 0, 0, iWidth, iHeight, hdcOrig, 0, 0, SRCCOPY);

                static const WORD c_GrayBits[] = { 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA };
                HBITMAP hbmp = CreateBitmap(8, 8, 1, 1, c_GrayBits);
                HBRUSH hBrush = CreatePatternBrush(hbmp);

#define ROP_DPna 0x000A0329 // Not included in the Windows headers, but this operation is used for greyscale dithering.
                HGDIOBJ hBrushOld = SelectObject(hdcDimmed, hBrush);
                PatBlt(hdcDimmed, 0, 0, iWidth, iHeight, ROP_DPna);

                SelectObject(hdcDimmed, hBrushOld);
                SelectObject(hdcDimmed, hObjOld);
                DeleteObject(hBrush);
                DeleteObject(hbmp);

                SelectObject(hdcOrig, hObjOld2);
                DeleteDC(hdcOrig);
            }

            DeleteDC(hdcDimmed);
        }

        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

    return S_OK;
}

HRESULT CScreenshotEditorRendererGDI::_FindRenderObjectFromInterfaceObject(
    IScreenshotEditorObject *pIfaceObj, OUT CRenderObject **ppRenderObjOut, OUT int *pIdxOut
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

HBITMAP CScreenshotEditorRendererGDI::_MakeHideCursorBuffer(HBITMAP hbm)
{
    HDC hdcDesktop = GetDC(HWND_DESKTOP);
    HDC hdc = CreateCompatibleDC(hdcDesktop);
    HBITMAP hbmHidden = CreateCompatibleBitmap(hdcDesktop, RECTWIDTH(_rcCursor), RECTHEIGHT(_rcCursor));
    HGDIOBJ hBmpOld = SelectObject(hdc, hbmHidden);
    HDC hdcScreenshot = CreateCompatibleDC(hdcDesktop);
    HGDIOBJ hBmpOldScreenshot = SelectObject(hdcScreenshot, hbm);

    BitBlt(hdc, 0, 0, RECTWIDTH(_rcCursor), RECTHEIGHT(_rcCursor), hdcScreenshot, _rcCursor.left, _rcCursor.top, SRCCOPY);

    SelectObject(hdcScreenshot, hBmpOldScreenshot);
    DeleteDC(hdcScreenshot);
    SelectObject(hdc, hBmpOld);
    DeleteDC(hdc);
    ReleaseDC(HWND_DESKTOP, hdcDesktop);

    return hbmHidden;
}

HRESULT CScreenshotEditorRendererGDI::_MakeHideCursorBuffers()
{
    _hbmCursorNone = _MakeHideCursorBuffer(_hbmScreenshotLight);
    _hbmCursorDimmedNone = _MakeHideCursorBuffer(_hbmScreenshotDimmed);
    return (_hbmCursor && _hbmCursorDimmedNone) ? S_OK : E_OUTOFMEMORY;
}

HRESULT CScreenshotEditorRendererGDI::_ShowHideCursor(bool fVisible)
{
    {
        HDC hdcDesktop = GetDC(HWND_DESKTOP);
        HDC hdc = CreateCompatibleDC(hdcDesktop);
        HGDIOBJ hBmpOld = SelectObject(hdc, _hbmScreenshotLight);

        HDC hdcCursor = CreateCompatibleDC(hdcDesktop);
        HGDIOBJ hBmpOldCursor = SelectObject(hdcCursor, fVisible ? _hbmCursor : _hbmCursorNone);

        BitBlt(
            hdc,
            _rcCursor.left, _rcCursor.top,
            RECTWIDTH(_rcCursor), RECTHEIGHT(_rcCursor),
            hdcCursor,
            0, 0,
            SRCCOPY
        );

        SelectObject(hdcCursor, hBmpOldCursor);
        DeleteDC(hdcCursor);
        SelectObject(hdc, hBmpOld);
        DeleteDC(hdc);
        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

    if (!_hbmCursorDimmed)
    {
        // We have to refresh the dimmed screenshot to reflect the changes.
        if (_hbmScreenshotDimmed)
            DeleteObject(_hbmScreenshotDimmed);
        _MakeDimmedScreenshot();

        // Cache the dimmed cursor so we don't have to rebuild the entire dimmed framebuffer next time
        // we want to draw it:
        HDC hdcDesktop = GetDC(HWND_DESKTOP);
        HDC hdc = CreateCompatibleDC(hdcDesktop);
        _hbmCursorDimmed = CreateCompatibleBitmap(hdcDesktop, RECTWIDTH(_rcCursor), RECTHEIGHT(_rcCursor));
        HGDIOBJ hBmpOld = SelectObject(hdc, _hbmCursorDimmed);
        HDC hdcScreenshot = CreateCompatibleDC(hdcDesktop);
        HGDIOBJ hBmpOldScreenshot = SelectObject(hdcScreenshot, _hbmScreenshotDimmed);

        BitBlt(hdc, 0, 0, RECTWIDTH(_rcCursor), RECTHEIGHT(_rcCursor), hdcScreenshot, _rcCursor.left, _rcCursor.top, SRCCOPY);

        SelectObject(hdcScreenshot, hBmpOldScreenshot);
        DeleteDC(hdcScreenshot);
        SelectObject(hdc, hBmpOld);
        DeleteDC(hdc);
        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }
    else
    {
        HDC hdcDesktop = GetDC(HWND_DESKTOP);
        HDC hdc = CreateCompatibleDC(hdcDesktop);
        HGDIOBJ hBmpOld = SelectObject(hdc, _hbmScreenshotDimmed);

        HDC hdcCursor = CreateCompatibleDC(hdcDesktop);
        HGDIOBJ hBmpOldCursor = SelectObject(hdcCursor, fVisible ? _hbmCursorDimmed : _hbmCursorDimmedNone);

        BitBlt(
            hdc,
            _rcCursor.left, _rcCursor.top,
            RECTWIDTH(_rcCursor), RECTHEIGHT(_rcCursor),
            hdcCursor,
            0, 0,
            SRCCOPY
        );

        SelectObject(hdcCursor, hBmpOldCursor);
        DeleteDC(hdcCursor);
        SelectObject(hdc, hBmpOld);
        DeleteDC(hdc);
        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

    _bmp.fCursorVisible = fVisible;
    InvalidateRect(_hwndRenderTarget, &_rcCursor, FALSE);
    return S_OK;
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
            return _OnCreate((CREATESTRUCT *)lParam);
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
            if (_IsExtensionTool())
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
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            return _OnMouseMove(pt.x, pt.y, wParam);
        }

        case WM_LBUTTONDOWN:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            return _OnMouseLButtonDown(pt.x, pt.y, wParam);
        }

        case WM_LBUTTONUP:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            return _OnMouseLButtonUp(pt.x, pt.y, wParam);
        }

        case WM_RBUTTONDOWN:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            return _OnMouseRButtonDown(pt.x, pt.y, wParam);
        }

        case WM_RBUTTONUP:
        {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            return _OnMouseRButtonUp(pt.x, pt.y, wParam);
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
            if (wParam < 0 || wParam > SSET_EXTENSIONFIRST + GetExtensionToolCount())
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

        case WM_SSE_SAVEIMAGE:
        {
            SaveImageToFileAndAccept();
            return 0;
        }

        case WM_SSE_SHOWCURSOR:
        {
            _pRenderer->ShowCursor();
            return 0;
        }

        case WM_SSE_HIDECURSOR:
        {
            _pRenderer->HideCursor();
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CScreenshotEditorWindow::_OnCreate(CREATESTRUCT *pCs)
{
    ASSERT_KEEP(_pScreenshotCtx = (CScreenshotContext *)pCs->lpCreateParams);
    ASSERT_KEEP(_pRenderer = new CScreenshotEditorRendererGDI(_pScreenshotCtx, _hwnd));
    if (FAILED(_pRenderer->Initialize()))
    {
        MessageBox(nullptr, TEXT("Failed to create renderer."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return -1;
    }
    _ChangeTool(SSET_SELECT);
    SetWindowPos(_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    ASSERT_KEEP(SUCCEEDED(_LoadExtensionTools()));

    return DefWindowProc(_hwnd, WM_CREATE, 0, (LPARAM)pCs);
}

LRESULT CScreenshotEditorWindow::_OnDestroy()
{
    for (int i = _vObjs.GetSize() - 1; i >= 0; i--)
    {
        _RemoveObject(_vObjs[i]);
    }

    if (_pHistoryMgr)
        delete _pHistoryMgr;
    if (_pRenderer)
        delete _pRenderer;
    if (_pScreenshotCtx)
        delete _pScreenshotCtx;
    if (_pFloatingToolbar)
        DestroyWindow(_pFloatingToolbar->GetHWND());

    PostQuitMessage(0);
    return 0;
}

LRESULT CScreenshotEditorWindow::_OnKeyDown(WPARAM virtualKey, LPARAM lParam)
{
    HRESULT hrExtensionTool = S_FALSE;
    if (_IsExtensionTool())
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

        case 'Z':
        {
            if (GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_CONTROL) < 0)
            {
                _Redo();
            }
            else if (GetKeyState(VK_CONTROL) < 0)
            {
                _Undo();
            }
            break;
        }

        case 'Y':
        {
            if (GetKeyState(VK_CONTROL) < 0)
            {
                _Redo();
            }
            break;
        }

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            if (_pToolbar)
            {
                _pToolbar->SelectOrdinalTool(1 + virtualKey - '1');
            }
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
                RECT rcOld = _rcSelection;

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

                // If the resulting rectangle is "negative", then correct it
                // to make all coordinates positive, and flip the tool mode
                // accordingly.
                if (_rcSelection.right < rcOld.left)
                {
                    _rcSelection.left = _rcSelection.right;
                    _rcSelection.right = rcOld.left;

                    // Invert the current tool mode:
                    _iToolMode ^= (DRAGM_SIZEE | DRAGM_SIZEW);
                }
                if (_rcSelection.bottom < rcOld.top)
                {
                    _rcSelection.top = _rcSelection.bottom;
                    _rcSelection.bottom = rcOld.top;

                    // Invert the current tool mode:
                    _iToolMode ^= (DRAGM_SIZES | DRAGM_SIZEN);
                }

                _pRenderer->UpdateSelection(&_rcSelection);
            }
        }
        else if (_IsExtensionTool() && (_pExtTool->GetFlags() & SSETF_DRAWSELECTION))
        {
            _rcDragSelectCur.left = x < _ptSelectionOrigin.x
                ? x
                : _ptSelectionOrigin.x;
            _rcDragSelectCur.top = y < _ptSelectionOrigin.y
                ? y
                : _ptSelectionOrigin.y;
            _rcDragSelectCur.right = x >= _ptSelectionOrigin.x
                ? x
                : _ptSelectionOrigin.x;
            _rcDragSelectCur.bottom = y >= _ptSelectionOrigin.y
                ? y
                : _ptSelectionOrigin.y;

            HRESULT hr = _pExtTool->OnSelectionChange(&_rcDragSelectCur);
            if (hr == S_FALSE)
            {
                return 0;
            }
        }
    }
    else if (_tool == SSET_DRAG)
    {
        // Set drag tool mode.
        POINT ptCursor;
        ptCursor.x = x;
        ptCursor.y = y;
        int dm = _ComputeDragMode(ptCursor, &_rcSelection);

        if (dm != _iToolMode)
        {
            _iToolMode = (DragMode)dm;
            _UpdateCursor();
        }
    }
    
    // N.B. We perform a separate check for if the tool is an extension tool since clicking and
    // dragging can set _fIsSelectingRegion, which means that an "else if" would result in this
    // not executing in that case.
    if (_IsExtensionTool())
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
    ScreenToClient(_hwnd, &_ptSelectionOrigin);

    _rcDragSelectCur.left = _rcDragSelectCur.right = x;
    _rcDragSelectCur.top = _rcDragSelectCur.bottom = y;

    if (_tool == SSET_DRAG)
    {
        _rcDragBegin = _rcSelection;
    }
    else if (_IsExtensionTool())
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
    _ptSelectionOrigin.x = _ptSelectionOrigin.y = 0;

    if (_tool == SSET_SELECT || _tool == SSET_DRAG)
    {
        _pHistoryMgr->PushSelectRegion(&_rcSelection);
    }
    else if (_IsExtensionTool())
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
        _ptSelectionOrigin.x = 0;
        _ptSelectionOrigin.y = 0;
    }
    else if (_IsExtensionTool())
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
    if (_IsExtensionTool())
    {
        HRESULT hr = _pExtTool->OnMouseRButtonUp(x, y, flags);
        if (hr == S_FALSE)
        {
            return 0;
        }
    }

    return 0;
}

HRESULT CScreenshotEditorWindow::_ApplyCrop()
{
    if (!_fHasAnySelectionMade)
    {
        _rcSelection.left = _rcSelection.top = 0;
        _rcSelection.right = _pScreenshotCtx->_sizeDesktop.cx;
        _rcSelection.bottom = _pScreenshotCtx->_sizeDesktop.cy;
    }

    if (SUCCEEDED(_pScreenshotCtx->Crop(&_rcSelection)))
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

HRESULT CScreenshotEditorWindow::_LoadExtensionTools()
{
    CExtensionIterator *pExtIterator = nullptr;
    if (SUCCEEDED(CExtensionManager::GetInstance()->IterateExtensions(&pExtIterator)))
    {
        DBGPRINT(TEXT("Loading extension tools..."));
        CExtensionContext *pLoadedExt = nullptr;
        if (SUCCEEDED(pExtIterator->Get(&pLoadedExt)))
        {
            do
            {
                IScreenshotEditorExtension *pExt = nullptr;
                if (SUCCEEDED(pLoadedExt->GetExtension(&pExt)))
                {
                    const CLSID *rgclsid = nullptr;
                    int cclsid = 0;

                    if (SUCCEEDED(pExt->GetToolSet(&rgclsid, &cclsid)))
                    {
                        for (int i = 0; i < cclsid; i++)
                        {
                            IScreenshotEditorTool *pTool = nullptr;
                            if (SUCCEEDED(pExt->CreateTool(rgclsid[i], &pTool)))
                            {
                                ExtensionToolInfo eti = { 0 };
                                eti.idTool = SSET_EXTENSIONFIRST + _vExtToolInfo.GetSize(); // Last member's index + 1
                                ASSERT_KEEP(SUCCEEDED(pTool->GetToolName(&eti.pszToolName)));
                                eti.pTool = pTool;
                                eti.pTool->SetSite(this);
                                DBGPRINT(TEXT("Created tool \"%s\" (" PRINT_GUID_PATTERN TEXT(") from extension \"%s\"")),
                                    eti.pszToolName,
                                    PRINT_GUID_PARAMS(rgclsid[i]),
                                    pLoadedExt->_pszName ? pLoadedExt->_pszName : pLoadedExt->_pszDllName);
                                _vExtToolInfo.Push(eti);
                            }
                            else
                            {
                                DBGPRINT(TEXT("Failed to create tool " PRINT_GUID_PATTERN TEXT(" from extension \"%s\"")),
                                    PRINT_GUID_PARAMS(rgclsid[i]),
                                    pLoadedExt->_pszName ? pLoadedExt->_pszName : pLoadedExt->_pszDllName);
                            }
                        }
                    }
                    else
                    {
                        DBGPRINT(TEXT("Failed to get tool set from extension \"%s\""),
                            pLoadedExt->_pszName ? pLoadedExt->_pszName : pLoadedExt->_pszDllName);
                    }
                }
                else
                {
                    DBGPRINT(TEXT("Failed to get extension from CExtensionContext."));
                }
            }
            while (SUCCEEDED(pExtIterator->GetNext(&pLoadedExt)));
        }
    }
    else
    {
        DBGPRINT(TEXT("Failed to get CExtensionIterator. Will not load extension tools."));
    }

    return S_OK;
}

HRESULT CScreenshotEditorWindow::_ApplyHistoryState(CEditHistoryManager::EditHistoryData *pehd)
{
    _fIsManagingHistory = true;
    HRESULT hr = S_OK;

    if (pehd->idAction == CEditHistoryManager::EHID_SELECTREGION)
    {
        _rcSelection = pehd->data.selection.rcRegionNew;
        _pRenderer->UpdateSelection(&_rcSelection);
    }
    else if (pehd->idAction == CEditHistoryManager::EHID_OBJECTCREATE)
    {
        hr = InsertObject(pehd->data.pObject);
    }
    else if (pehd->idAction == CEditHistoryManager::EHID_OBJECTREMOVE)
    {
        hr = _RemoveObject(pehd->data.pObject);
    }
    // Not handled yet: OBJECTMOVE, OBJECT (unique)

    _fIsManagingHistory = false;
    return hr;
}

HRESULT CScreenshotEditorWindow::_Undo()
{
    CEditHistoryManager::EditHistoryData ehd;
    HRESULT hr = _pHistoryMgr->GetPrevious(&ehd);
    if (SUCCEEDED(hr))
    {
        CEditHistoryManager::EditHistoryData ehdCur;
        hr = _pHistoryMgr->GetCurrent(&ehdCur);
        if (SUCCEEDED(hr))
        {
            _fIsManagingHistory = true;

            // Invert the current case (if necessary):
            if (ehdCur.idAction == CEditHistoryManager::EHID_OBJECTCREATE)
            {
                // Remove the object if it was added.
                _RemoveObject(ehdCur.data.pObject);
            }
            else if (ehdCur.idAction == CEditHistoryManager::EHID_OBJECTREMOVE)
            {
                // Add the object if it was removed.
                InsertObject(ehdCur.data.pObject);
            }

            // Apply the previous case:
            hr = _ApplyHistoryState(&ehd); // Unsets _fIsManagingHistory for us.
            _pHistoryMgr->Rewind();
        }
    }

    return hr;
}

HRESULT CScreenshotEditorWindow::_Redo()
{
    CEditHistoryManager::EditHistoryData ehd;
    HRESULT hr = _pHistoryMgr->GetNext(&ehd);
    if (SUCCEEDED(hr))
    {
        hr = _ApplyHistoryState(&ehd);
        _pHistoryMgr->Progress();
    }

    return hr;
}

HRESULT CScreenshotEditorWindow::_ChangeTool(ScreenshotEditorTool newTool)
{
    ScreenshotEditorTool oldTool = _tool;
    IScreenshotEditorTool *pOldExtTool = _pExtTool;

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
    else if (newTool >= SSET_EXTENSIONFIRST)
    {
        IScreenshotEditorTool *pExtTool = nullptr;

        for (int i = 0; i < _vExtToolInfo.GetSize(); i++)
        {
            if (_vExtToolInfo[i].idTool == (UINT)newTool)
            {
                pExtTool = _vExtToolInfo[i].pTool;
                break;
            }
        }

        if (!pExtTool)
        {
            return E_ABORT;
        }

        if (FAILED(pExtTool->ToolSelectionChanged(TRUE)))
        {
            return E_ABORT;
        }

        _tool = newTool;
        _pExtTool = pExtTool;

        _pRenderer->SetMarqueeSelection(false);
    }
    else
    {
        _tool = SSET_ILLEGAL;
        _pRenderer->SetMarqueeSelection(false);
    }

    if (newTool != oldTool && oldTool >= SSET_EXTENSIONFIRST && pOldExtTool)
    {
        pOldExtTool->ToolSelectionChanged(FALSE);
    }

    _UpdateCursor();

    _pRenderer->UpdateSizingHelpersVisibility(_tool == SSET_DRAG);
    if (_pToolbar)
        _pToolbar->OnToolChanged(newTool);

    return S_OK;
}

HRESULT CScreenshotEditorWindow::_EnsureToolbar()
{
    if (!_pToolbar)
    {
        _pToolbar = CEditorToolbar::Create(this, 0, WS_CHILD, 0, 0, 0, 0, _hwnd);
    }

    return _pToolbar ? S_OK : E_FAIL;
}

void CScreenshotEditorWindow::_ShowFloatingToolbar()
{
    POINT ptShow;
    ptShow.x = (_rcSelection.right + _pScreenshotCtx->_ptVirtualScreen.x) + 10; // Maybe ask the renderer instead?
    ptShow.y = (_rcSelection.top + _pScreenshotCtx->_ptVirtualScreen.y);

    if (SUCCEEDED(_EnsureToolbar()))
    {
        // If we don't have a toolbar yet, then we will create one:
        if (!_pFloatingToolbar)
        {
            _pFloatingToolbar = CEditorFloatingToolbar::Create(_pToolbar);
            _pToolbar->OnToolChanged(_tool);
        }

        if (_pFloatingToolbar)
        {
            SetWindowPos(_pFloatingToolbar->GetHWND(), nullptr, ptShow.x, ptShow.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
        }
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
        if (_IsExtensionTool())
        {
            if (_pExtTool && SUCCEEDED(_pExtTool->ApplyCursor()))
            {
                return;
            }
        }

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
            case SSET_ILLEGAL:
                idc = IDC_NO;
                break;
            // Extension tool fallback:
            default:
                idc = IDC_ARROW;
                break;
        }
    }

    HCURSOR hcur = LoadCursor(NULL, idc);
    SetCursor(hcur);
}

void CScreenshotEditorWindow::_CancelSelection()
{
    RECT rcSelectionOld = _rcSelection;
    _rcSelection.left = _rcSelection.top = _rcSelection.right = _rcSelection.bottom = 0;
    _fHasAnySelectionMade = false;
    _pRenderer->UpdateSelection(&_rcSelection);
    _HideFloatingToolbar();
}

int CScreenshotEditorWindow::_ComputeDragMode(POINT ptCursor, RECT *prcDraggedObj)
{
    static constexpr int c_iGrabRadius = 4; // On all sides.
    int dm = DRAGM_DRAG;

    if (ptCursor.y >= (prcDraggedObj->top - c_iGrabRadius) && ptCursor.y <= (prcDraggedObj->top + c_iGrabRadius)
        && ptCursor.x >= (prcDraggedObj->left - c_iGrabRadius) && ptCursor.x <= (prcDraggedObj->right + c_iGrabRadius))
    {
        dm |= DRAGM_SIZEN;
    }
    else if (ptCursor.y >= (prcDraggedObj->bottom - c_iGrabRadius) && ptCursor.y <= (prcDraggedObj->bottom + c_iGrabRadius)
        && ptCursor.x >= (prcDraggedObj->left - c_iGrabRadius) && ptCursor.x <= (prcDraggedObj->right + c_iGrabRadius))
    {
        dm |= DRAGM_SIZES;
    }

    if (ptCursor.x >= (prcDraggedObj->left - c_iGrabRadius) && ptCursor.x <= (prcDraggedObj->left + c_iGrabRadius)
        && ptCursor.y >= (prcDraggedObj->top - c_iGrabRadius) && ptCursor.y <= (prcDraggedObj->bottom + c_iGrabRadius))
    {
        dm |= DRAGM_SIZEW;
    }
    else if (ptCursor.x >= (prcDraggedObj->right - c_iGrabRadius) && ptCursor.x <= (prcDraggedObj->right + c_iGrabRadius)
        && ptCursor.y >= (prcDraggedObj->top - c_iGrabRadius) && ptCursor.y <= (prcDraggedObj->bottom + c_iGrabRadius))
    {
        dm |= DRAGM_SIZEE;
    }

    return dm;
}

HRESULT CScreenshotEditorWindow::_RemoveObject(IScreenshotEditorObject *pObj)
{
    // Search an object by its reference and remove it.
    for (size_t i = 0; i < _vObjs.GetSize(); i++)
    {
        if (_vObjs[i] == pObj)
        {
            _pRenderer->RemoveRenderObject(pObj);

            pObj->SetSite(nullptr);

            // N.B. We want to call PushObjectRemove before releasing the object here.
            // The resulting history entry will add a reference to the object and hold
            // it until it is destroyed. If we did it the other way around, then we risk
            // freeing the object from memory (thus breaking redo functionality)
            if (!_fIsManagingHistory)
                _pHistoryMgr->PushObjectRemove(pObj);

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

    FOR_EACH_DYNARR(IScreenshotEditorObject *&pExisting, _vObjs)
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

        if (!_fIsManagingHistory)
            _pHistoryMgr->PushObjectCreate(pObj);

        InvalidateObject(pObj);
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

STDMETHODIMP CScreenshotEditorWindow::RemoveObject(IScreenshotEditorObject *pObj)
{
    return _RemoveObject(pObj);
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

STDMETHODIMP CScreenshotEditorWindow::GetCursorPosition(OUT POINT *pptCursor)
{
    if (!pptCursor)
        return E_POINTER;
    GetCursorPos(pptCursor);
    ScreenToClient(_hwnd, pptCursor);
    return S_OK;
}

STDMETHODIMP_(HWND) CScreenshotEditorWindow::GetEditorHWND()
{
    return GetHWND();
}

STDMETHODIMP CScreenshotEditorWindow::EnumObjects(OUT IEnumUnknown **ppEnumUnknown)
{
    if (!ppEnumUnknown)
        return E_POINTER;

    *ppEnumUnknown = new (std::nothrow) CEnumObjects(&_vObjs);
    if (*ppEnumUnknown)
    {
        (*ppEnumUnknown)->AddRef();
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP CScreenshotEditorWindow::GetSelectedRegion(RECT *prc)
{
    if (!prc)
        return E_POINTER;
    *prc = _rcSelection;
    return S_OK;
}

STDMETHODIMP CScreenshotEditorWindow::SetSelectedRegion(RECT *prc)
{
    if (!prc)
        return E_POINTER;
    _rcSelection = *prc;
    return S_OK;
}

STDMETHODIMP CScreenshotEditorWindow::SetSelectedObject(IScreenshotEditorObject *pObj)
{
    // TODO: Implement!
    return E_NOTIMPL;
}

bool CScreenshotEditorWindow::IsCursorShown()
{
    // The architecture is a bit messy here. I might have the editor window track this on its own instead of
    // asking the renderer.
    return _pRenderer->IsCursorShown();
}

HRESULT CScreenshotEditorWindow::CopyToClipboardAndAccept()
{
    if (SUCCEEDED(_ApplyCrop()))
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

HRESULT CScreenshotEditorWindow::SaveImageToFileAndAccept()
{
    if (SUCCEEDED(_ApplyCrop()))
    {
        HRESULT hr = _pScreenshotCtx->SaveToFile();
        if (SUCCEEDED(hr))
        {
            DestroyWindow(_hwnd);
        }
        else if (hr == E_ABORT)
        {
            return E_ABORT;
        }
        else
        {
            MessageBox(_hwnd, TEXT("Failed to save image to file."), TEXT("Error"), MB_OK | MB_ICONERROR);
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

int CScreenshotEditorWindow::GetExtensionToolCount()
{
    return _vExtToolInfo.GetSize();
}

HRESULT CScreenshotEditorWindow::GetExtensionToolInfo(int idx, OUT ExtensionToolInfo *pExtToolInfo)
{
    if (!pExtToolInfo)
        return E_POINTER;

    if (idx < _vExtToolInfo.GetSize())
    {
        *pExtToolInfo = _vExtToolInfo[idx];
        return S_OK;
    }

    return E_BOUNDS;
}

// static
HRESULT CScreenshotEditorWindow::RegisterWindowClass()
{
    WNDCLASS cls = { 0 };
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
// CScreenshotEditorWindow::CEnumObjects
//

STDMETHODIMP CScreenshotEditorWindow::CEnumObjects::QueryInterface(const IID &riid, void **ppvOut)
{
    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IEnumUnknown))
    {
        *ppvOut = static_cast<CScreenshotEditorWindow::CEnumObjects *>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CScreenshotEditorWindow::CEnumObjects::Next(ULONG celt, IUnknown **rgelt, ULONG *pceltFetched)
{
    if (celt == 0)
        return S_OK;
    if (!rgelt)
        return E_POINTER;
    if (celt > 1 && !pceltFetched) // The COM API demands this.
        return E_POINTER;

    size_t sizeObjArr = _pvObjs->GetSize();
    int iFetched = 0;

    for (int i = 0; i < celt && (_idx + i < sizeObjArr); i++)
    {
        rgelt[i] = _pvObjs->At(_idx + i);
        iFetched++;
        _idx++;
    }

    if (iFetched > 0)
    {
        for (int i = 0; i < celt; i++)
        {
            rgelt[i]->AddRef();
        }
    }

    if (pceltFetched)
        *pceltFetched = iFetched;

    return celt == iFetched ? S_OK : S_FALSE;
}

STDMETHODIMP CScreenshotEditorWindow::CEnumObjects::Skip(ULONG celt)
{
    _idx += celt;
    return S_OK;
}

STDMETHODIMP CScreenshotEditorWindow::CEnumObjects::Reset()
{
    _idx = 0;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) CScreenshotEditorWindow::CEnumObjects::Clone(IEnumUnknown **ppenum)
{
    if (!ppenum)
        return E_POINTER;

    *ppenum = new CScreenshotEditorWindow::CEnumObjects(_pvObjs);
    return S_OK;
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
    WNDCLASS cls = { 0 };
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

//
// CEditHistoryManager
//

HRESULT CEditHistoryManager::_Push(EditHistoryData *pData)
{
    if (_vData.GetSize() > 0 && _iPos != _vData.GetSize() - 1)
    {
        for (size_t i = _vData.GetSize() - 1; i > _iPos; i--)
        {
            _DestroyEntry(i);
            _vData.Remove(i);
        }
    }

    _vData.Push(*pData);
    Progress();
    return S_OK;
}

HRESULT CEditHistoryManager::_DestroyEntry(int idxEntry)
{
    EditHistoryData ehd = _vData[idxEntry];
    HRESULT hr = S_OK;

    IScreenshotEditorObject *pEditorObj = nullptr;

    if (ehd.idAction == EHID_OBJECTCREATE || ehd.idAction == EHID_OBJECTREMOVE)
    {
        if (ehd.data.pObject)
            pEditorObj = ehd.data.pObject;
    }
    else if (ehd.idAction == EHID_OBJECTMOVE)
    {
        if (ehd.data.objectMove.pObject)
            pEditorObj = ehd.data.objectMove.pObject;
    }
    else if (ehd.idAction == EHID_OBJECT)
    {
        if (ehd.data.objectUnique.pObject)
            pEditorObj = ehd.data.objectUnique.pObject;
    }

    if (pEditorObj)
    {
#ifdef _DEBUG
        {
            HWND hwndEditor = FindWindow(CScreenshotEditorWindow::GetWindowClass(), nullptr);
            CScreenshotEditorWindow *pEditor = (CScreenshotEditorWindow *)GetWindowLongPtr(hwndEditor, 0);

            FOR_EACH_DYNARR(IScreenshotEditorObject *&pObjInEditor, pEditor->_vObjs)
            {
                if (pObjInEditor == pEditorObj)
                {
                    assert("Destroying an entry for an object which exists in the document!" && 0);
                }
            }
        }
#endif

        hr = pEditorObj->Release();
    }

    return hr;
}

CEditHistoryManager::~CEditHistoryManager()
{
    for (int i = _vData.GetSize() - 1; i >= 0; i--)
    {
        _DestroyEntry(i);
    }
}

HRESULT CEditHistoryManager::PushSelectRegion(RECT *prcSelectionNew)
{
    EditHistoryData ehd;
    ehd.idAction = EHID_SELECTREGION;
    ehd.data.selection.rcRegionNew = *prcSelectionNew;

    return _Push(&ehd);
}

HRESULT CEditHistoryManager::PushObjectCreate(IScreenshotEditorObject *pObj)
{
    EditHistoryData ehd;
    ehd.idAction = EHID_OBJECTCREATE;
    ehd.data.pObject = pObj;
    pObj->AddRef();

    return _Push(&ehd);
}

HRESULT CEditHistoryManager::PushObjectRemove(IScreenshotEditorObject *pObj)
{
    EditHistoryData ehd;
    ehd.idAction = EHID_OBJECTREMOVE;
    ehd.data.pObject = pObj;
    pObj->AddRef();

    return _Push(&ehd);
}

HRESULT CEditHistoryManager::PushObjectMove(IScreenshotEditorObject *pObj, RECT *prcNew, RECT *prcOld)
{
    EditHistoryData ehd;
    ehd.idAction = EHID_OBJECTMOVE;
    ehd.data.objectMove.pObject = pObj;
    ehd.data.objectMove.rcNew = *prcNew;
    ehd.data.objectMove.rcOld = *prcOld;
    pObj->AddRef();

    return _Push(&ehd);
}

HRESULT CEditHistoryManager::PushObjectUnique(IScreenshotEditorObject *pObj, ULONG ulEventId)
{
    EditHistoryData ehd;
    ehd.idAction = EHID_OBJECT;
    ehd.data.objectUnique.pObject = pObj;
    ehd.data.objectUnique.ulEventId = ulEventId;
    pObj->AddRef();

    return _Push(&ehd);
}

HRESULT CEditHistoryManager::GetPrevious(EditHistoryData *pData)
{
    if (!pData)
        return E_POINTER;

    if ((_iPos - 1) < 0)
        return E_BOUNDS;

    *pData = _vData[_iPos - 1];
    return S_OK;
}

HRESULT CEditHistoryManager::GetNext(EditHistoryData *pData)
{
    if (!pData)
        return E_POINTER;
    
    if ((_iPos + 1) > _vData.GetSize() - 1)
        return E_BOUNDS;

    *pData = _vData[_iPos + 1];
    return S_OK;
}

HRESULT CEditHistoryManager::GetCurrent(EditHistoryData *pData)
{
    if (!pData)
        return E_POINTER;
    
    if (_vData.GetSize() < 1)
        return E_BOUNDS;

    *pData = _vData[_iPos];
    return S_OK;
}
