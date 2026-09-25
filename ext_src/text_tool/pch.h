#pragma once

#include <new>
#define NOMINMAX
#include <windows.h>
#include <tchar.h>
#include "cunk.h"
#include "screenkirk.h"

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

#define ID_HOTKEY_SCREENSHOT (20)

extern HINSTANCE g_hinst;

template <size_t N>
inline HRESULT StaticCoTaskMemStringAlloc(const TCHAR (&sz)[N], TCHAR **pszOut)
{
    size_t cb = N * sizeof(TCHAR);
    *pszOut = (TCHAR *)CoTaskMemAlloc(cb);
    if (*pszOut)
    {
        _tcscpy_s(*(TCHAR **)pszOut, N, sz);
        return S_OK;
    }
    return E_OUTOFMEMORY;
};

#define IMPLEMENT_IUNKNOWN                                                               \
protected:                                                                               \
    UINT _uRefCount;                                                                     \
public:                                                                                  \
    STDMETHODIMP QueryInterface(const IID &riid, void **ppvOut) override;                \
    STDMETHODIMP_(ULONG) AddRef() override                                               \
    {                                                                                    \
        InterlockedIncrement(&_uRefCount);                                               \
        return _uRefCount;                                                               \
    }                                                                                    \
    STDMETHODIMP_(ULONG) Release() override                                              \
    {                                                                                    \
        InterlockedDecrement(&_uRefCount);                                               \
        if (_uRefCount == 0)                                                             \
        {                                                                                \
            delete this;                                                                 \
            return 0;                                                                    \
        }                                                                                \
        return _uRefCount;                                                               \
    }
