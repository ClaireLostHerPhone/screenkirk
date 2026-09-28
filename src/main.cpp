#include "pch.h"
#include "screenshot_manager.h"
#include "extmgr.h"
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

#ifdef _DEBUG
    AllocConsole();

    FILE *fpOut;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);

    FILE *fpErr;
    freopen_s(&fpErr, "CONOUT$", "w", stderr);

    FILE *fpIn;
    freopen_s(&fpIn, "CONIN$", "r", stdin);

    _tprintf(TEXT("Welcome to codename screenkirk ver. alpha 1.0!\n"));

    OSVersion *posv = GetOSVersion();
    _tprintf(
        TEXT("Running on %s %d.%d (build %d)\n"),
        posv->IsWindowsNT() ? TEXT("Windows NT") : TEXT("Windows"),
        posv->dwMajorVersion, posv->dwMinorVersion, posv->dwBuildNumber
    );
#endif

    ASSERT_EXPR(SUCCEEDED(CExtensionManager::CreateInstance()));
    TCHAR szFolderRoot[MAX_PATH];
    GetModuleFileName(g_hinst, szFolderRoot, ARRAYSIZE(szFolderRoot));
    PathPopFileName(szFolderRoot);
    PathAppend(szFolderRoot, TEXT("extensions"));
    ASSERT_EXPR(SUCCEEDED(CExtensionManager::GetInstance()->LoadAllExtensionsFromFolder(szFolderRoot)));

#ifndef _WIN16
    CNotifyWindow *pNotifyWindow = CNotifyWindow::Create();
#endif

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

#ifndef _WIN16
    // Destroy the notification tray icon:
    if (pNotifyWindow)
        DestroyWindow(pNotifyWindow->GetHWND());
#endif

    CloseHandle(hInstMutex);
    return 0;
}