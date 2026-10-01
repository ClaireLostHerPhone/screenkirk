#include "pch.h"
#include "dynarray.h"

class CExtensionContext
{
public:
    HMODULE _hmod;
    IScreenshotEditorExtension *_pExt;
    TCHAR _szDllPath[MAX_PATH];
    const TCHAR *_pszDllName;
    const TCHAR *_pszName;
    const TCHAR *_pszVersionStr;
    const TCHAR *_pszAuthor;
    HRESULT _hr;

    CExtensionContext()
        : _hmod(nullptr)
        , _pExt(nullptr)
        , _pszDllName(nullptr)
        , _pszVersionStr(nullptr)
        , _pszAuthor(nullptr)
        , _hr(S_OK)
    {
        ZeroMemory(_szDllPath, sizeof(_szDllPath));
    }

    CExtensionContext(const CExtensionContext &rOther)
        : _hmod(rOther._hmod)
        , _pExt(rOther._pExt)
        , _pszName(rOther._pszName)
        , _pszVersionStr(rOther._pszVersionStr)
        , _pszAuthor(rOther._pszAuthor)
        , _hr(rOther._hr)
    {
        _tcscpy_s(_szDllPath, rOther._szDllPath);
        _pszDllName = &_szDllPath[0] + (rOther._pszDllName - &rOther._szDllPath[0]);
    }

    HRESULT UnloadExtension();

    inline bool IsExtensionAvailable()
    {
        return SUCCEEDED(_hr);
    }

    HRESULT GetExtension(OUT IScreenshotEditorExtension **ppExt);
};

class CExtensionIterator
{
    class CExtensionManager *_pExtMgr;
    UINT _uPos;

public:
    CExtensionIterator(CExtensionManager *pMgr)
        : _pExtMgr(pMgr)
        , _uPos(0)
    {
    }

    inline void Close()
    {
        delete this;
    }

    HRESULT Get(OUT CExtensionContext **ppExt);
    HRESULT GetNext(OUT CExtensionContext **ppExt);
};

class CExtensionManager
{
    CDynamicArray<CExtensionContext> _vLoadedExts;

public:
    static HRESULT CreateInstance();
    static CExtensionManager *GetInstance();

    HRESULT LoadAllExtensionsFromFolder(const TCHAR *pszFolder);
    HRESULT LoadExtension(const TCHAR *pszPath);
    HRESULT IterateExtensions(OUT CExtensionIterator **ppLoadedExtension);

    friend class CExtensionIterator;
};