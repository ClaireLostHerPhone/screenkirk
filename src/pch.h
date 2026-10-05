#pragma once

#define NOMINMAX
#include <windows.h>
#include <unknwn.h>
#include <objbase.h>
#include <tchar.h>
#include "screenkirk.h"
#include "portable.h"

#define QS_APP_NAME         TEXT("codename screenkirk")
#define QS_VER_STRING       TEXT("ver. alpha 1.0")
#define QS_APP_FULL_BRAND   QS_APP_NAME TEXT(" ") QS_VER_STRING

#define TOK_CAT_INNER(a, b) a##b
#define TOK_CAT(a, b) TOK_CAT_INNER(a, b)

#if defined(_DEBUG) || defined(SCREENKIRK_SHOW_CONSOLE)
#define DBGPRINT(...)                                                                    \
    (                                                                                    \
        (_tprintf(TEXT("[") TEXT(__FUNCTION__) TEXT("] "))),                                   \
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
#define ASSERT_KEEP(expr) assert(expr)
#else
#define ASSERT_KEEP(expr) (expr)
#endif

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#define DEFINE_WINDOW_CLASS(cls) public: static const TCHAR *GetWindowClass() { return TEXT(cls); }

#define ID_HOTKEY_SCREENSHOT (20)

#define IMPLEMENT_IUNKNOWN                                                               \
protected:                                                                               \
    ULONG _uRefCount;                                                                    \
public:                                                                                  \
    STDMETHODIMP QueryInterface(const IID &riid, void **ppvOut) override;                \
    STDMETHODIMP_(ULONG) AddRef() override                                               \
    {                                                                                    \
        _uRefCount++;                                                                    \
        return _uRefCount;                                                               \
    }                                                                                    \
    STDMETHODIMP_(ULONG) Release() override                                              \
    {                                                                                    \
        _uRefCount--;                                                                    \
        if (_uRefCount == 0)                                                             \
        {                                                                                \
            delete this;                                                                 \
            return 0;                                                                    \
        }                                                                                \
        return _uRefCount;                                                               \
    }

extern HINSTANCE g_hinst;
