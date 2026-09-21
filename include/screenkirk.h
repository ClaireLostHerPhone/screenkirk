#pragma once
#include <windows.h>
#include <initguid.h>
#include <comdef.h>

// I don't think this would benefit from being a COM class.
DECLARE_INTERFACE(IScreenshotContext)
{
    STDMETHOD_(HBITMAP, GetScreenshotBitmap)() PURE;
    STDMETHOD_(HBITMAP, GetCursorBitmapColorChannel)() PURE;
    STDMETHOD_(HBITMAP, GetCursorBitmapMaskChannel)() PURE;
    STDMETHOD_(POINT, GetCursorPosition)() PURE;
    STDMETHOD_(BOOL, GetCursorVisible)() PURE;
    STDMETHOD_(POINT, GetVirtualScreenOrigin)() PURE;
    STDMETHOD_(SIZE, GetVirtualScreenSize)() PURE;
};

//
// As design speculation, I would just say that it is probably best to implement an object
// and at least its primary renderer in the same class. They will usually benefit from
// accessing the same state.
//

DECLARE_INTERFACE_IID_(IScreenshotEditorObjectRenderer, IUnknown, "{56E23ECC-23B2-4033-BB8D-50FFDA763D10}")
{
    STDMETHOD_(ULONG, QueryInterface)(const REFIID riid, OUT void *ppvOut);
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;

    STDMETHOD(Paint)() PURE;
};
// {56E23ECC-23B2-4033-BB8D-50FFDA763D10}
DEFINE_GUID(IID_IScreenshotEditorObjectRenderer,
    0x56e23ecc, 0x23b2, 0x4033, 0xbb, 0x8d, 0x50, 0xff, 0xda, 0x76, 0x3d, 0x10);

DECLARE_INTERFACE_IID_(IScreenshotEditorObjectRendererGDI, IScreenshotEditorObjectRenderer, "{56E23ECC-23B2-4033-BB8D-50FFDA763D11}")
{
    STDMETHOD_(ULONG, QueryInterface)(const REFIID riid, OUT void *ppvOut);
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;

    STDMETHOD(Paint)() PURE;

    STDMETHOD(SetGdiParameters)(HDC hdc, RECT *prcPaint) PURE;
};
// {56E23ECC-23B2-4033-BB8D-50FFDA763D11}
DEFINE_GUID(IID_IScreenshotEditorObjectRendererGDI,
    0x56e23ecc, 0x23b2, 0x4033, 0xbb, 0x8d, 0x50, 0xff, 0xda, 0x76, 0x3d, 0x11);

//
// Editor objects and tools have the screenshot editor as their site.
//

DECLARE_INTERFACE_IID_(IScreenshotEditorObject, IObjectWithSite, "{841C133A-9960-44C3-9E95-C1349C731401}")
{
    STDMETHOD_(ULONG, QueryInterface)(const REFIID riid, OUT void *ppvOut);
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;
    STDMETHOD(GetSite)(void *ppvSite) PURE;
    STDMETHOD(SetSite)(void *pUnkSite) PURE;

    STDMETHOD(GetManipulationToolMask)() PURE; // TODO: How to go about the parameters here?
    STDMETHOD(GetLogicalRect)() PURE;
    STDMETHOD(GetVisualRect)() PURE;
    STDMETHOD_(BOOL, IsVisualDirty)() PURE;
    STDMETHOD(CreateRenderer)(const REFIID riid, OUT IScreenshotEditorObjectRenderer *pRendererOut) PURE;
};
// {841C133A-9960-44C3-9E95-C1349C731401}
DEFINE_GUID(IID_IScreenshotEditorObject,
    0x841c133a, 0x9960, 0x44c3, 0x9e, 0x95, 0xc1, 0x34, 0x9c, 0x73, 0x14, 0x1);

/**
 * Interface for extension tools.
 */
DECLARE_INTERFACE_IID_(IScreenshotEditorTool, IObjectWithSite, "{1C093E9E-696B-42CA-B4C3-67B793AF2D52}")
{
    STDMETHOD_(ULONG, QueryInterface)(const REFIID riid, OUT void *ppvOut);
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;
    STDMETHOD(GetSite)(void *ppvSite) PURE;
    STDMETHOD(SetSite)(void *pUnkSite) PURE;

    STDMETHOD_(HICON, GetToolIcon)() PURE;
#ifdef _UNICODE
    STDMETHOD(GetToolName)(const WCHAR *) PURE;
#else
    STDMETHOD(GetToolName)(const CHAR *) PURE;
#endif
    STDMETHOD(OnKeyDown)(int iVirtualKey, LPARAM lParam) PURE;
    STDMETHOD(OnKeyUp)(int iVirtualKey, LPARAM lParam) PURE;
    STDMETHOD(OnMouseMove)(int x, int y, WPARAM flags) PURE;
    STDMETHOD(OnMouseLButtonDown)(LONG x, LONG y, WPARAM flags) PURE;
    STDMETHOD(OnMouseLButtonUp)(LONG x, LONG y, WPARAM flags) PURE;
    STDMETHOD(OnMouseRButtonDown)(LONG x, LONG y, WPARAM flags) PURE;
    STDMETHOD(OnMouseRButtonUp)(LONG x, LONG y, WPARAM flags) PURE;
    STDMETHOD(ApplyCursor)() PURE;
};
// {1C093E9E-696B-42CA-B4C3-67B793AF2D52}
DEFINE_GUID(IID_IScreenshotEditorTool,
    0x1c093e9e, 0x696b, 0x42ca, 0xb4, 0xc3, 0x67, 0xb7, 0x93, 0xaf, 0x2d, 0x52);
