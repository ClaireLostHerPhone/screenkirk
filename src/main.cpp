#include "pch.h"
#include "screenshot_manager.h"
#include "notify.h"

HINSTANCE g_hinst;

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PTSTR pCmdLine, int nCmdShow)
{
    g_hinst = hInstance;

    CNotifyWindow *pNotifyWindow = CNotifyWindow::Create();

    // TODO: Support loading a hotkey from user configuration.
    RegisterHotKey(nullptr, ID_HOTKEY_SCREENSHOT, MOD_SHIFT | MOD_WIN, 'S');

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.hwnd == nullptr && msg.message == WM_HOTKEY && msg.wParam == ID_HOTKEY_SCREENSHOT)
        {
            OnScreenshotKeyPressed();
        }
        else if (!TranslateAccelerator(nullptr, nullptr, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Destroy the notification tray icon:
    DestroyWindow(pNotifyWindow->GetHWND());

    return 0;
}