#pragma once
#include "pch.h"

class CTextEditorTool : public IScreenshotEditorTool
{
    ULONG _uRefCount;
    IUnknown *_pUnkSite;

    IScreenshotEditor *_pEditor;

public:
    IMPLEMENT_IUNKNOWN;

    //@Begin IObjectWithSite
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;
    STDMETHODIMP SetSite(IUnknown *pUnkSite) override;
    //@End IObjectWithSite

    //@Begin IScreenshotEditorTool
    STDMETHODIMP SelectTool() override;
    STDMETHODIMP_(HICON) GetToolIcon() override;
    STDMETHODIMP GetToolName(OUT const TCHAR **pszOut) override;
    STDMETHODIMP OnKeyDown(int iVirtualKey, LPARAM lParam) override;
    STDMETHODIMP OnKeyUp(int iVirtualKey, LPARAM lParam) override;
    STDMETHODIMP OnMouseMove(int x, int y, WPARAM flags) override;
    STDMETHODIMP OnMouseLButtonDown(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseLButtonUp(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseRButtonDown(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseRButtonUp(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP ApplyCursor() override;
    //@End IScreenshotEditorTool
};

// {97378D4C-1FA7-4F10-9626-426A879BB01A}
DEFINE_GUID(CLSID_TextEditorTool,
    0x97378d4c, 0x1fa7, 0x4f10, 0x96, 0x26, 0x42, 0x6a, 0x87, 0x9b, 0xb0, 0x1a);