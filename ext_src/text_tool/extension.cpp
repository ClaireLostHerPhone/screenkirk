#include "pch.h"
#include "extension.h"
#include "tool.h"

STDMETHODIMP CTextEditorExtension::QueryInterface(const IID &riid, void **ppvOut)
{
    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IScreenshotEditorExtension))
    {
        *ppvOut = static_cast<IScreenshotEditorExtension *>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CTextEditorExtension::GetName(OUT const TCHAR **pszOut)
{
    return StaticCoTaskMemStringAlloc(TEXT("Text Tool"), (TCHAR **)pszOut);
}

STDMETHODIMP CTextEditorExtension::GetVersionString(OUT const TCHAR **pszOut)
{
    return StaticCoTaskMemStringAlloc(TEXT("1.0"), (TCHAR **)pszOut);
}

STDMETHODIMP CTextEditorExtension::GetAuthor(OUT const TCHAR **pszOut)
{
    return StaticCoTaskMemStringAlloc(TEXT("ClaireLostHerPhone"), (TCHAR **)pszOut);
}

STDMETHODIMP CTextEditorExtension::GetToolSet(const CLSID **prgclsidTools, int *piNumTools)
{
    if (!prgclsidTools || !piNumTools)
        return E_POINTER;

    static const const CLSID rgclsidTool[] = {
        CLSID_TextEditorTool,
        { 0 },
    };

    *prgclsidTools = &rgclsidTool[0];
    *piNumTools = ARRAYSIZE(rgclsidTool) - 1;
    return S_OK;
}

STDMETHODIMP CTextEditorExtension::CreateTool(REFCLSID rclsidTool, IScreenshotEditorTool **ppToolOut)
{
    if (IsEqualGUID(rclsidTool, CLSID_TextEditorTool))
    {
        CTextEditorTool *pTool = new (std::nothrow) CTextEditorTool();
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