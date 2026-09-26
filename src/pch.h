#pragma once

#define NOMINMAX
#include <windows.h>
#include <tchar.h>
#include "screenkirk.h"

#ifdef _DEBUG
#define DBGPRINT(...)                                                                    \
    (                                                                                    \
        (_tprintf(TEXT("[") __FUNCTION__ TEXT("] "))),                                   \
        (_tprintf(__VA_ARGS__)),                                                         \
        (_tprintf(TEXT("\n")))                                                           \
    )
#else
#define DBGPRINT(...)
#endif

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#define ID_HOTKEY_SCREENSHOT (20)

extern HINSTANCE g_hinst;