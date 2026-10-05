#include "pch.h"
#include "screenshot_manager.h"
#include "extmgr.h"
#include "cfgmgr.h"
#include "notify.h"
#include "util.h"

HINSTANCE g_hinst;

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PTSTR pCmdLine, int nCmdShow)
{
    g_hinst = hInstance;

    HANDLE hInstMutex = CreateMutex(nullptr, FALSE, TEXT("Local\\screenkirk_InstanceMutex"));

    if (!hInstMutex)
    {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox(nullptr, TEXT("An instance of screenkirk is already running. A new instance will not be opened."),
            TEXT("screenkirk"), MB_OK | MB_ICONERROR);

        CloseHandle(hInstMutex);
        return 0;
    }

#if defined(_DEBUG) || defined(SCREENKIRK_SHOW_CONSOLE)
    AllocConsole();

    FILE *fpOut;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);

    FILE *fpErr;
    freopen_s(&fpErr, "CONOUT$", "w", stderr);

    FILE *fpIn;
    freopen_s(&fpIn, "CONIN$", "r", stdin);

    _tprintf(TEXT("Welcome to ") QS_APP_FULL_BRAND TEXT("!\n"));

    OSVersion *posv = GetOSVersion();
    _tprintf(
        TEXT("Running on %s %d.%d (build %d)\n"),
        (posv->flags & OSVF_WINE) ? TEXT("Wine") : (posv->flags & OSVF_WINNT) ? TEXT("Windows NT") : TEXT("Windows"),
        posv->dwMajorVersion, posv->dwMinorVersion, posv->dwBuildNumber
    );
#endif

    ASSERT_KEEP(SUCCEEDED(CConfigManager::CreateInstance()));

    ASSERT_KEEP(SUCCEEDED(CExtensionManager::CreateInstance()));
    TCHAR szFolderRoot[MAX_PATH];
    GetModuleFileName(g_hinst, szFolderRoot, ARRAYSIZE(szFolderRoot));
    PathPopFileName(szFolderRoot);
    PathAppend(szFolderRoot, TEXT("extensions"));
    ASSERT_KEEP(SUCCEEDED(CExtensionManager::GetInstance()->LoadAllExtensionsFromFolder(szFolderRoot)));

    CNotifyWindow *pNotifyWindow = CNotifyWindow::Create();

    if (GetOSVersion()->flags & OSVF_WINE)
    {
        MessageBox(
            nullptr,
            TEXT("Codename screenkirk cannot automatically register the screenshot shortcut on your platform.\n\n")
            TEXT("For Linux, I use 'xdotool key --window $(xdotool search class \"screenkirk_*\" | head - n 1) shift+Scroll_Lock'. ")
            TEXT("However, your mileage may vary."),
            TEXT("screenkirk"),
            MB_OK | MB_ICONWARNING
        );
    }

    if (!(GetOSVersion()->flags & OSVF_WINE))
    {
        // This does not work on some platforms for some reason.
        RegisterScreenshotShortcut();
    }
    else
    {
#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif
        // This works for now.
        RegisterHotKey(nullptr, ID_HOTKEY_SCREENSHOT, MOD_SHIFT | MOD_NOREPEAT, VK_SCROLL);
    }

    MSG msg = { 0 };
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
    if (pNotifyWindow)
        DestroyWindow(pNotifyWindow->GetHWND());

    CloseHandle(hInstMutex);
    return 0;
}