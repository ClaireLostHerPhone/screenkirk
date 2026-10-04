#pragma once
#include "pch.h"
#include "window.h"

#define DEFINE_WINDOW_CLASS(cls) public: static const TCHAR *GetWindowClass() { return TEXT(cls); }

class CObjectDebugWindow : public CWindow<CObjectDebugWindow>
{
    DEFINE_WINDOW_CLASS("screenkirk.objdebug.ObjectDebugWindow");

private:
    IScreenshotEditor *_pEditor;
    HWND _hwndToolbar;
    HWND _hwndListView;

protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    LRESULT _OnCreate(CREATESTRUCT *pcs);
    LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);
    HRESULT _UpdateLayout();
    HRESULT _EnumItems();

public:
    enum Command
    {
        IDM_REFRESH = 100,
        IDM_DELETE,
    };

    static HRESULT RegisterWindowClass();

    /**
    *
    */
    static CObjectDebugWindow *Create(IScreenshotEditor *pse, DWORD dwExStyle, DWORD dwStyle, int x, int y, int cx, int cy, HWND hwndParent);

    CObjectDebugWindow()
        : _pEditor(nullptr)
        , _hwndToolbar(nullptr)
        , _hwndListView(nullptr)
    {
    }
};