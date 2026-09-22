#pragma once
#include "pch.h"
#include "window.h"

const TCHAR c_szNotifyWindowClassName[] = TEXT("screenkirk_NotifyWindow");
class CNotifyWindow : public CWindow<CNotifyWindow, c_szNotifyWindowClassName>
{
    HMENU _hmenu;
    bool _fIsMenuOpen = false;

protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    HRESULT _CreateNotifyIcon();

public:
    enum WM
    {
        WM_NOTIFYICON = WM_APP + 1,
    };

    static HRESULT RegisterWindowClass();
    static CNotifyWindow *Create();
};
