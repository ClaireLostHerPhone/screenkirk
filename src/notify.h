#pragma once
#include "pch.h"
#include "window.h"

class CNotifyWindow : public CWindow<CNotifyWindow>
{
    DEFINE_WINDOW_CLASS("screenkirk_NotifyWindow");

private:
    HMENU _hmenu;
    bool _fIsMenuOpen;

protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    HRESULT _CreateNotifyIcon();

public:
    enum WM
    {
        WM_NOTIFYICON = WM_APP + 1,
    };

    CNotifyWindow()
        : _hmenu(nullptr)
        , _fIsMenuOpen(false)
    {
    }

    static HRESULT RegisterWindowClass();
    static CNotifyWindow *Create();
};
