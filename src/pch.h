#pragma once

#define NOMINMAX
#include <windows.h>
#include <unknwn.h>
#include <objbase.h>
#include <tchar.h>
#include "screenkirk.h"

#ifndef E_BOUNDS
#define E_BOUNDS ((HRESULT)0x8000000BL)
#endif

#define TOK_CAT_INNER(a, b) a##b
#define TOK_CAT(a, b) TOK_CAT_INNER(a, b)

#ifdef _UNICODE
#define _AW(x) x##W
#define _AWSTR "W"
#else
#define _AW(x) x##A
#define _AWSTR "A"
#endif

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
#define ASSERT_EXPR(expr) assert(expr)
#else
#define ASSERT_EXPR(expr) (expr)
#endif

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#ifdef _WIN16
#define CoTaskMemAlloc(x)   (malloc(x))
#define CoTaskMemFree(x)    (free(x))
#define CoTaskMemRealloc(x) (realloc(x))
#endif

#define DEFINE_WINDOW_CLASS(cls) public: static const TCHAR *GetWindowClass() { return TEXT(cls); }

#define FOR_EACH(decl, array)                                                            \
    for (int i = 0, __size = COUNTOF(array); i < __size; ++i)                            \
        if (bool __run = true)                                                           \
            for (decl = (array)[i]; __run; __run = false)

#define FOR_EACH_DYNARR(decl, array)                                                     \
    for (int i = 0, __size = (array).GetSize(); i < __size; ++i)                         \
        if (bool __run = true)                                                           \
            for (decl = (array)[i]; __run; __run = false)

#if defined(_MSVC_LANG) && _MSVC_LANG < 201103L || !defined(_MSVC_LANG) && __cplusplus < 201103L
    #define nullptr NULL
    #define constexpr const
#endif

#define ID_HOTKEY_SCREENSHOT (20)

extern HINSTANCE g_hinst;

// TODO: Move to a file like portable.h
#ifndef IID_PPV_ARGS
    //  IID_PPV_ARGS(ppType)
    //      ppType is the variable of type IType that will be filled
    //
    //      RESULTS in:  IID_IType, ppvType
    //      will create a compiler error if wrong level of indirection is used.
    //
    extern "C++"
    {
        template<typename T> void** IID_PPV_ARGS_Helper(T** pp) 
        {
            static_cast<IUnknown*>(*pp);    // make sure everyone derives from IUnknown
            return reinterpret_cast<void**>(pp);
        }
    }

    #define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)

    #define E_NOT_SET                HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
    #define E_NOT_VALID_STATE        HRESULT_FROM_WIN32(ERROR_INVALID_STATE)
    #define E_NOT_SUFFICIENT_BUFFER  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
    #define E_TIME_SENSITIVE_THREAD  HRESULT_FROM_WIN32(ERROR_TIME_SENSITIVE_THREAD)
    #define E_NO_TASK_QUEUE          HRESULT_FROM_WIN32(ERROR_NO_TASK_QUEUE)
#endif

#ifndef TBSTYLE_EX_DOUBLEBUFFER
#define TBSTYLE_EX_DOUBLEBUFFER             0x00000080 // Double Buffer the toolbar
#endif
