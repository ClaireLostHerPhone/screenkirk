#include "pch.h"
#include "extension.h"
#include "tool.h"

STDMETHODIMP CRectangleToolExtension::QueryInterface(const IID &riid, void **ppvOut)
{
    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IScreenshotEditorExtension))
    {
        *ppvOut = static_cast<IScreenshotEditorExtension *>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CRectangleToolExtension::GetExtensionFlags()
{
    ULONG uFlags = 0;
#ifdef _UNICODE
    uFlags |= SSEEF_UNICODE;
#endif
    return uFlags;
}

STDMETHODIMP CRectangleToolExtension::GetName(OUT const TCHAR **pszOut)
{
    return StaticCoTaskMemStringAlloc(TEXT("Rectangle and Highlighter Tool"), (TCHAR **)pszOut);
}

STDMETHODIMP CRectangleToolExtension::GetVersionString(OUT const TCHAR **pszOut)
{
    return StaticCoTaskMemStringAlloc(TEXT("1.0"), (TCHAR **)pszOut);
}

STDMETHODIMP CRectangleToolExtension::GetAuthor(OUT const TCHAR **pszOut)
{
    return StaticCoTaskMemStringAlloc(TEXT("ClaireLostHerPhone"), (TCHAR **)pszOut);
}

STDMETHODIMP CRectangleToolExtension::GetToolSet(const CLSID **prgclsidTools, int *piNumTools)
{
    if (!prgclsidTools || !piNumTools)
        return E_POINTER;

    static const const CLSID rgclsidTool[] = {
        CLSID_RectangleTool,
        CLSID_RectangleHighlighterTool,
        { 0 },
    };

    *prgclsidTools = &rgclsidTool[0];
    *piNumTools = ARRAYSIZE(rgclsidTool) - 1;
    return S_OK;
}

STDMETHODIMP CRectangleToolExtension::CreateTool(REFCLSID rclsidTool, IScreenshotEditorTool **ppToolOut)
{
    if (IsEqualGUID(rclsidTool, CLSID_RectangleTool))
    {
        CRectangleTool *pTool = new (std::nothrow) CRectangleTool(CRectangleTool::TYPE_SOLID);
        if (pTool)
        {
            *ppToolOut = pTool;
            pTool->AddRef();
            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else if (IsEqualGUID(rclsidTool, CLSID_RectangleHighlighterTool))
    {
        CRectangleTool *pTool = new (std::nothrow) CRectangleTool(CRectangleTool::TYPE_HIGHLIGHTER);
        if (pTool)
        {
            *ppToolOut = pTool;
            pTool->AddRef();
            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}