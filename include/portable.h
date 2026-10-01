#pragma once

#ifdef _UNICODE
#define _AW(x) x##W
#define _AWSTR "W"
#else
#define _AW(x) x##A
#define _AWSTR "A"
#endif

#define FOR_EACH(decl, array)                                                            \
    for (int i = 0, __size = COUNTOF(array); i < __size; ++i)                            \
        if (bool __run = true)                                                           \
            for (decl = (array)[i]; __run; __run = false)

#define FOR_EACH_DYNARR(decl, array)                                                     \
    for (int i = 0, __size = (array).GetSize(); i < __size; ++i)                         \
        if (bool __run = true)                                                           \
            for (decl = (array)[i]; __run; __run = false)

#ifdef _WIN16
#define CoTaskMemAlloc(x)   (malloc(x))
#define CoTaskMemFree(x)    (free(x))
#define CoTaskMemRealloc(x) (realloc(x))
#endif

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

#if defined(_MSVC_LANG) && _MSVC_LANG < 201103L || !defined(_MSVC_LANG) && __cplusplus < 201103L
    #define nullptr NULL
    #define constexpr const
#endif

#ifndef E_BOUNDS
#define E_BOUNDS ((HRESULT)0x8000000BL)
#endif

#ifndef TBSTYLE_EX_DOUBLEBUFFER
#define TBSTYLE_EX_DOUBLEBUFFER             0x00000080 // Double Buffer the toolbar
#endif