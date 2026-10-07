#include "pch.h"
#include "notify.h"
#include "resource.h"
#include "util.h"
#include "screenshot_manager.h"
#include <shellapi.h>

LRESULT CNotifyWindow::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            _hmenu = CreatePopupMenu();

            int idxMenuItem = 0;
            {
                MENUITEMINFO miiVer = { sizeof(miiVer) };
                miiVer.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
                miiVer.fState = MFS_DISABLED;
                miiVer.wID = IDM_BRAND;
                miiVer.dwTypeData = (TCHAR *)QS_APP_FULL_BRAND;
                miiVer.cch = _tcslen(miiVer.dwTypeData);
                InsertMenuItem(_hmenu, idxMenuItem++, FALSE, &miiVer);
            }

            {
                MENUITEMINFO miiSep = { sizeof(miiSep) };
                miiSep.fMask = MIIM_TYPE;
                miiSep.fType = MFT_SEPARATOR;
                InsertMenuItem(_hmenu, idxMenuItem++, FALSE, &miiSep);
            }

            {
                MENUITEMINFO mii = { sizeof(mii) };
                mii.fMask = MIIM_ID | MIIM_STRING;
                mii.wID = IDM_TAKESCREENSHOT;
                mii.dwTypeData = (TCHAR *)TEXT("Take &screenshot");
                mii.cch = _tcslen(mii.dwTypeData);
                InsertMenuItem(_hmenu, idxMenuItem++, FALSE, &mii);
            }

            {
                MENUITEMINFO miiSep = { sizeof(miiSep) };
                miiSep.fMask = MIIM_TYPE;
                miiSep.fType = MFT_SEPARATOR;
                InsertMenuItem(_hmenu, idxMenuItem++, FALSE, &miiSep);
            }

            {
                MENUITEMINFO mii = { sizeof(mii) };
                mii.fMask = MIIM_ID | MIIM_STRING;
                mii.wID = IDM_CLOSEAPP;
                mii.dwTypeData = (TCHAR *)TEXT("E&xit");
                mii.cch = _tcslen(mii.dwTypeData);
                InsertMenuItem(_hmenu, idxMenuItem++, FALSE, &mii);
            }

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

                if (!_fIsMenuOpen)
                {
                    SetForegroundWindow(_hwnd);
                    if (!TrackPopupMenu(
                        _hmenu,
                        TPM_LEFTALIGN,
                        pt.x, pt.y,
                        0,
                        _hwnd,
                        nullptr
                    ))
                    {
                        MessageBox(
                            nullptr, TEXT("Failed to open the notification item's context menu."),
                            TEXT("Error"), MB_OK | MB_ICONERROR
                        );
                    }
                }
            }

            return 0;
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDM_CLOSEAPP)
            {
                PostQuitMessage(0);
            }
            else if (LOWORD(wParam) == IDM_TAKESCREENSHOT)
            {
                // TODO: This should wait until the menu fades out. Otherwise, it is bound to be captured in
                // the screenshot, which is annoying. I have looked into this a little bit, but it seems to
                // be annoyingly difficult. It looks like commands are processed after the menu is closing
                // and the fade is registered, and the fade itself is an internal GDI sprite rather than a
                // window, which means there is no interface to manage it.
                OnScreenshotKeyPressed();
            }

            return 0;
        }

        case WM_ENTERMENULOOP:
        {
            _fIsMenuOpen = true;
            break;
        }

        case WM_EXITMENULOOP:
        {
            _fIsMenuOpen = false;
            break;
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
    WNDCLASS cls = { 0 };
    cls.hInstance = g_hinst;
    cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
    cls.hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_APP));

    return CWindow::RegisterWindowClass(&cls);
}

// static
CNotifyWindow *CNotifyWindow::Create()
{
    // Windows 3.x-series operating systems don't have a taskbar or support notification
    // tray items.
    if (GetOSVersion()->dwMajorVersion <= 3)
    {
        return nullptr;
    }

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