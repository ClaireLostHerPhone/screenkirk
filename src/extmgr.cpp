#include "pch.h"
#include "extmgr.h"
#include "util.h"

using DllGetClassObject_t = decltype(&DllGetClassObject);

CExtensionManager *g_pExtMgrInst = nullptr;

HRESULT CExtensionContext::UnloadExtension()
{
    if (_pszName)
        CoTaskMemFree((void *)_pszName);
    if (_pszVersionStr)
        CoTaskMemFree((void *)_pszVersionStr);
    if (_pszAuthor)
        CoTaskMemFree((void *)_pszAuthor);

    if (_pExt)
        _pExt->Release();

    return S_OK;
}

HRESULT CExtensionContext::GetExtension(OUT IScreenshotEditorExtension **ppExt)
{
    if (!ppExt)
        return E_POINTER;

    if (IsExtensionAvailable())
    {
        *ppExt = _pExt;
        return S_OK;
    }

    return E_FAIL;
}

// static
HRESULT CExtensionManager::CreateInstance()
{
    g_pExtMgrInst = new CExtensionManager();
    DBGPRINT(TEXT("Initialized extension manager."));
    return S_OK;
}

CExtensionManager *CExtensionManager::GetInstance()
{
    return g_pExtMgrInst;
}

HRESULT CExtensionManager::LoadAllExtensionsFromFolder(const TCHAR *pszFolder)
{
    TCHAR szFolderMatch[MAX_PATH] = { 0 };
    _tcscpy_s(szFolderMatch, pszFolder);
    _tcscat_s(szFolderMatch, TEXT("\\*.dll"));

    DBGPRINT(TEXT("Loading all extensions from: %s"), szFolderMatch);

    WIN32_FIND_DATA fd;
    HANDLE hFile = FindFirstFile(szFolderMatch, &fd);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        // This means that we don't have a file matching the pattern, which we don't want to report
        // as an error.
        return S_OK;
    }

    HRESULT hrLoad = E_FAIL;

    do
    {
        if (fd.dwFileAttributes & ~(FILE_ATTRIBUTE_DIRECTORY))
        {
            TCHAR szFilePathAbs[MAX_PATH] = { 0 };
            _tcscpy_s(szFilePathAbs, pszFolder);
            _tcscat_s(szFilePathAbs, TEXT("\\"));
            _tcscat_s(szFilePathAbs, fd.cFileName);

            DBGPRINT(TEXT("Going to load extension: %s"), szFilePathAbs);
            hrLoad = LoadExtension(szFilePathAbs);

            DBGPRINT(SUCCEEDED(hrLoad)
                ? TEXT("Loaded extension \"%s\" successfully.")
                : TEXT("Failed to load extension \"%s\" (%X)."), fd.cFileName, hrLoad);
        }
    }
    while (FindNextFile(hFile, &fd) != FALSE);

    FindClose(hFile);
    return FAILED(hrLoad) ? S_FALSE : S_OK;
}

HRESULT CExtensionManager::LoadExtension(const TCHAR *pszPath)
{
    CExtensionContext le;

    DBGPRINT(TEXT("Loading \"%s\""), pszPath);

    HMODULE hmod = LoadLibrary(pszPath);
    HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
    if (hmod)
    {
        le._hmod = hmod;
        GetModuleFileName(hmod, le._szDllPath, ARRAYSIZE(le._szDllPath));

        le._pszDllName = PathFindFileName(le._szDllPath);

        DllGetClassObject_t pfnDllGetClassObject = (DllGetClassObject_t)GetProcAddress(hmod, "DllGetClassObject");
        if (!pfnDllGetClassObject)
        {
            DBGPRINT(TEXT("Failed to get DllGetClassObject."));
            FreeLibrary(hmod);
            hr = E_FAIL;
        }
        else
        {
            IClassFactory *pFac = nullptr;
            hr = pfnDllGetClassObject(CLSID_ScreenshotEditorExtension, IID_PPV_ARGS(&pFac));
            if (SUCCEEDED(hr))
            {
                IScreenshotEditorExtension *pExt = nullptr;
                hr = pFac->CreateInstance(nullptr, IID_PPV_ARGS(&pExt));
                if (SUCCEEDED(hr))
                {
                    le._pExt = pExt;

                    if (FAILED(pExt->GetName(&le._pszName)))
                        le._pszName = nullptr;
                    if (FAILED(pExt->GetVersionString(&le._pszVersionStr)))
                        le._pszVersionStr = nullptr;
                    if (FAILED(pExt->GetAuthor(&le._pszAuthor)))
                        le._pszAuthor = nullptr;

                    hr = S_OK;
                }
                else
                {
                    DBGPRINT(TEXT("Failed to create ScreenshotEditorExtension instance."));
                }
                pFac->Release();
            }
            else
            {
                DBGPRINT(TEXT("Failed to create class factory instance."));
            }
        }
    }
    else
    {
        DBGPRINT(TEXT("Failed to load library."));
    }

    le._hr = hr;
    _vLoadedExts.Push(std::move(le));
    return hr;
}

HRESULT CExtensionManager::IterateExtensions(OUT CExtensionIterator **ppLoadedExtension)
{
    if (!ppLoadedExtension)
        return E_POINTER;

    *ppLoadedExtension = new (std::nothrow) CExtensionIterator(this);
    return *ppLoadedExtension ? S_OK : E_OUTOFMEMORY;
}

//
// CExtensionIterator
//

HRESULT CExtensionIterator::Get(OUT CExtensionContext **ppExt)
{
    if (!ppExt)
        return E_POINTER;

    if (_uPos < _pExtMgr->_vLoadedExts.GetSize())
    {
        *ppExt = &_pExtMgr->_vLoadedExts[_uPos];
    }
    else
    {
        return E_BOUNDS;
    }

    return S_OK;
}

HRESULT CExtensionIterator::GetNext(OUT CExtensionContext **ppExt)
{
    if (!ppExt)
        return E_POINTER;

    if (_uPos + 1 < _pExtMgr->_vLoadedExts.GetSize())
    {
        _uPos++;
        *ppExt = &_pExtMgr->_vLoadedExts[_uPos];
    }
    else
    {
        return E_BOUNDS;
    }

    return S_OK;
}
