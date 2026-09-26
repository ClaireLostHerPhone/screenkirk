#include "pch.h"
#include "dynarray.h"

class CLoadedExtension
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

    ~CLoadedExtension();

    inline bool IsExtensionAvailable()
    {
        return SUCCEEDED(_hr);
    }

    HRESULT GetExtension(OUT IScreenshotEditorExtension **ppExt);
};

class CExtensionIterator
{
    class CExtensionManager *_pExtMgr;
    UINT _uPos = 0;

public:
    CExtensionIterator(CExtensionManager *pMgr)
        : _pExtMgr(pMgr)
    {
    }

    inline void Close()
    {
        delete this;
    }

    HRESULT Get(OUT CLoadedExtension **ppExt);
    HRESULT GetNext(OUT CLoadedExtension **ppExt);
};

class CExtensionManager
{
    CDynamicArray<CLoadedExtension> _vLoadedExts;

public:
    static HRESULT CreateInstance();
    static CExtensionManager *GetInstance();

    HRESULT LoadAllExtensionsFromFolder(const TCHAR *pszFolder);
    HRESULT LoadExtension(const TCHAR *pszPath);
    HRESULT IterateExtensions(OUT CExtensionIterator **ppLoadedExtension);

    friend class CExtensionIterator;
};