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

#define PRINT_GUID_PATTERN TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}")
#define PRINT_GUID_PARAMS(guid)                                                          \
    (guid).Data1, (guid).Data2, (guid).Data3,                                            \
    (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3],                  \
    (guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]                   \

#ifdef _DEBUG
#define ASSERT_EXPR(expr) assert(expr)
#else
#define ASSERT_EXPR(expr) (expr)
#endif

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#define ID_HOTKEY_SCREENSHOT (20)

extern HINSTANCE g_hinst;