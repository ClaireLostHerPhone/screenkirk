#include "pch.h"
#include "screenshot_manager.h"
#include "extmgr.h"
#include "notify.h"
#include "util.h"

HINSTANCE g_hinst;

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PTSTR pCmdLine, int nCmdShow)
{
    g_hinst = hInstance;

#ifdef _DEBUG
    AllocConsole();

    FILE *fpOut;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);

    FILE *fpErr;
    freopen_s(&fpErr, "CONOUT$", "w", stderr);

    FILE *fpIn;
    freopen_s(&fpIn, "CONIN$", "r", stdin);

    _tprintf(TEXT("Welcome to codename screenkirk ver. alpha 1.0!\n"));
#endif

    assert(SUCCEEDED(CExtensionManager::CreateInstance()));
    TCHAR szFolderRoot[MAX_PATH];
    GetModuleFileName(g_hinst, szFolderRoot, ARRAYSIZE(szFolderRoot));
    PathPopFileName(szFolderRoot);
    PathAppend(szFolderRoot, TEXT("extensions"));
    assert(SUCCEEDED(CExtensionManager::GetInstance()->LoadAllExtensionsFromFolder(szFolderRoot)));

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