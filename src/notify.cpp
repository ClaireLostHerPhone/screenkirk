#include "pch.h"
#include "notify.h"
#include "resource.h"
#include <shellapi.h>

LRESULT CNotifyWindow::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            _hmenu = CreatePopupMenu();

            MENUITEMINFO mii = { sizeof(mii) };
            mii.fMask = MIIM_ID | MIIM_STRING;
            mii.wID = 100;
            mii.dwTypeData = (TCHAR *)TEXT("E&xit");
            mii.cch = _tcslen(mii.dwTypeData);

            InsertMenuItem(_hmenu, 0, FALSE, &mii);

            break;
        }

        case WM_DESTROY:
        {
            NOTIFYICONDATA nid = { sizeof(nid) };
            nid.hWnd = _hwnd;
            nid.uID = 1;
            Shell_NotifyIcon(NIM_DELETE, &nid);
            break;
        }

        case WM_NOTIFYICON:
        {
            if (lParam == WM_RBUTTONUP)
            {
                POINT pt;
                GetCursorPos(&pt);

                if (!TrackPopupMenu(
                    _hmenu,
                    TPM_LEFTALIGN,
                    pt.x, pt.y,
                    0,
                    _hwnd,
                    nullptr
                ))
                {
#ifdef _DEBUG
                    MessageBox(nullptr, TEXT("Failed TPM"), TEXT("Error"), MB_OK | MB_ICONERROR);
#endif
                }
            }

            return 0;
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == 100)
            {
                PostQuitMessage(0);
            }

            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HRESULT CNotifyWindow::_CreateNotifyIcon()
{
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.hWnd = _hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_NOTIFYICON;
    nid.hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_APP));
    lstrcpy(nid.szTip, TEXT("screenkirk"));

    Shell_NotifyIcon(NIM_ADD, &nid);

    return S_OK;
}

// static
HRESULT CNotifyWindow::RegisterWindowClass()
{
    WNDCLASS cls = {};
    cls.hInstance = g_hinst;
    cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
    cls.hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_APP));

    return CWindow::RegisterWindowClass(&cls);
}

// static
CNotifyWindow *CNotifyWindow::Create()
{
    RegisterWindowClass();

    CNotifyWindow *pWnd = CWindow::Create(
        0,
        nullptr,
        0,
        0, 0, 0, 0,
        nullptr,
        nullptr,
        g_hinst,
        nullptr
    );

    if (FAILED(pWnd->_CreateNotifyIcon()))
    {
        DestroyWindow(pWnd->GetHWND());
        return nullptr;
    }

    return pWnd;
}