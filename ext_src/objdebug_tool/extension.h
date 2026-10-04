#pragma once
#include "pch.h"

class CObjectDebugExtension : public IScreenshotEditorExtension
{
public:
    IMPLEMENT_IUNKNOWN;

    //@Begin IScreenshotEditorExtension
    STDMETHODIMP_(ULONG) GetExtensionFlags() override;
    STDMETHODIMP GetName(OUT const TCHAR **pszOut) override;
    STDMETHODIMP GetVersionString(OUT const TCHAR **pszOut) override;
    STDMETHODIMP GetAuthor(OUT const TCHAR **pszOut) override;

    STDMETHODIMP GetToolSet(const CLSID **prgclsidTools, int *piNumTools) override;
    STDMETHODIMP CreateTool(REFCLSID rclsidTool, IScreenshotEditorTool **ppToolOut) override;
    //@End IScreenshotEditorExtension

    CObjectDebugExtension()
        : _uRefCount(0)
    {
    }
};