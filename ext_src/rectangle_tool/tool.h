#pragma once
#include "pch.h"

class CRectangleTool : public IScreenshotEditorTool
{
public:
    enum RectangleToolType
    {
        TYPE_SOLID,
        TYPE_HIGHLIGHTER,
    };

private:
    IUnknown *_pUnkSite;
    IScreenshotEditor *_pEditor;
    IScreenshotEditorObject *_pObjCur;
    RectangleToolType _type;

public:
    IMPLEMENT_IUNKNOWN;

    //@Begin IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite) override;
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override;
    //@End IObjectWithSite

    //@Begin IScreenshotEditorTool
    STDMETHODIMP_(const TCHAR *) GetClassName() override;
    STDMETHODIMP OnDestroyed() override;
    STDMETHODIMP ToolSelectionChanged(BOOL fSelected) override;
    STDMETHODIMP_(ULONG) GetFlags() override;
    STDMETHODIMP_(HICON) GetToolIcon(SIZE size) override;
    STDMETHODIMP GetToolName(OUT const TCHAR **pszOut) override;
    STDMETHODIMP OnKeyDown(int iVirtualKey, LPARAM lParam) override;
    STDMETHODIMP OnKeyUp(int iVirtualKey, LPARAM lParam) override;
    STDMETHODIMP OnMouseMove(int x, int y, WPARAM flags) override;
    STDMETHODIMP OnMouseLButtonDown(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseLButtonUp(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseRButtonDown(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP OnMouseRButtonUp(LONG x, LONG y, WPARAM flags) override;
    STDMETHODIMP ApplyCursor() override;
    STDMETHODIMP OnSelectionChange(RECT *prcNew) override;
    //@End IScreenshotEditorTool

    CRectangleTool(RectangleToolType type)
        : _uRefCount(0)
        , _pUnkSite(nullptr)
        , _pEditor(nullptr)
        , _pObjCur(nullptr)
        , _type(type)
    {
    }
};

// {FAA1F25A-7882-4061-A8FD-E3CB2AE83400}
DEFINE_GUID(CLSID_RectangleTool,
    0xfaa1f25a, 0x7882, 0x4061, 0xa8, 0xfd, 0xe3, 0xcb, 0x2a, 0xe8, 0x34, 0x0);

// {FAA1F25A-7882-4061-A8FD-E3CB2AE83401}
DEFINE_GUID(CLSID_RectangleHighlighterTool,
    0xfaa1f25a, 0x7882, 0x4061, 0xa8, 0xfd, 0xe3, 0xcb, 0x2a, 0xe8, 0x34, 0x1);
