#pragma once
#include "pch.h"

class CTextEditorExtension : public IScreenshotEditorExtension
{
public:
    IMPLEMENT_IUNKNOWN;

    //@Begin IScreenshotEditorExtension
    STDMETHODIMP GetName(OUT const TCHAR **pszOut) override;
    STDMETHODIMP GetVersionString(OUT const TCHAR **pszOut) override;
    STDMETHODIMP GetAuthor(OUT const TCHAR **pszOut) override;

    STDMETHODIMP GetToolSet(const CLSID **prgiidTools, int *piNumTools) override;
    STDMETHODIMP CreateTool(REFCLSID rclsidTool, IScreenshotEditorTool **ppToolOut) override;
    //@End IScreenshotEditorExtension
};