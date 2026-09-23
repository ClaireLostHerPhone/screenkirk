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
    STDMETHOD(QueryInterface)(REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;

    STDMETHOD(Paint)() PURE;
};
// {56E23ECC-23B2-4033-BB8D-50FFDA763D10}
DEFINE_GUID(IID_IScreenshotEditorObjectRenderer,
    0x56e23ecc, 0x23b2, 0x4033, 0xbb, 0x8d, 0x50, 0xff, 0xda, 0x76, 0x3d, 0x10);

DECLARE_INTERFACE_IID_(IScreenshotEditorObjectRendererGDI, IScreenshotEditorObjectRenderer, "{56E23ECC-23B2-4033-BB8D-50FFDA763D11}")
{
    STDMETHOD(QueryInterface)(REFIID riid, OUT void **ppvOut) PURE;
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

/**
 * Represents a screenshot document object.
 * 
 * An object is a visual layer with metadata for manipulation with tools.
 * 
 * The site of an object is the hosting screenshot editor.
 */
DECLARE_INTERFACE_IID_(IScreenshotEditorObject, IObjectWithSite, "{841C133A-9960-44C3-9E95-C1349C731401}")
{
    STDMETHOD(QueryInterface)(REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;
    STDMETHOD(GetSite)(void *ppvSite) PURE;
    STDMETHOD(SetSite)(void *pUnkSite) PURE;

    STDMETHOD(InsertedIntoDocument)() PURE;
    STDMETHOD(GetManipulationToolMask)() PURE; // TODO: How to go about the parameters here?
    STDMETHOD(GetLogicalRect)(IN RECT *prc) PURE; // An object's logical rect is relative to the document.
    STDMETHOD(GetVisualRect)(IN RECT *prc) PURE; // An object's visual rect is relative to its logical rect.
    STDMETHOD_(BOOL, IsVisualDirty)() PURE;
    STDMETHOD(CreateRenderer)(const REFIID riid, OUT IScreenshotEditorObjectRenderer *pRendererOut) PURE;
};
// {841C133A-9960-44C3-9E95-C1349C731401}
DEFINE_GUID(IID_IScreenshotEditorObject,
    0x841c133a, 0x9960, 0x44c3, 0x9e, 0x95, 0xc1, 0x34, 0x9c, 0x73, 0x14, 0x1);

DECLARE_INTERFACE_IID_(IScreenshotEditor, IUnknown, "{0A573BD7-2C24-4602-A842-ACC82B12E1F1}")
{
    STDMETHOD(QueryInterface)(REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;

    /**
     * Inserts an object into the screenshot document.
     */
    STDMETHOD(InsertObject)(IScreenshotEditorObject *pObj) PURE;

    /**
     * Invalidates an object's visual.
     */
    STDMETHOD(InvalidateObject)(IScreenshotEditorObject *pObj) PURE;

    /**
     * Gets the context of the screenshot document.
     */
    STDMETHOD(GetScreenshotContext)(OUT IScreenshotContext **ppContextOut) PURE;
};
// {0A573BD7-2C24-4602-A842-ACC82B12E1F1}
DEFINE_GUID(IID_IScreenshotEditor,
    0xa573bd7, 0x2c24, 0x4602, 0xa8, 0x42, 0xac, 0xc8, 0x2b, 0x12, 0xe1, 0xf1);

/**
 * Represents a tool that can be used in the screenshot editor.
 * 
 * Extensions can add new tools.
 */
DECLARE_INTERFACE_IID_(IScreenshotEditorTool, IObjectWithSite, "{1C093E9E-696B-42CA-B4C3-67B793AF2D52}")
{
    STDMETHOD(QueryInterface)(REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;
    STDMETHOD(GetSite)(void **ppvSite) PURE;
    STDMETHOD(SetSite)(void *pUnkSite) PURE;

    STDMETHOD(SelectTool)() PURE;
    STDMETHOD_(HICON, GetToolIcon)() PURE;
#ifdef _UNICODE
    STDMETHOD(GetToolName)(OUT const WCHAR **pszOut) PURE;
#else
    STDMETHOD(GetToolName)(OUT const CHAR **pszOut) PURE;
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

DECLARE_INTERFACE_IID_(IScreenshotEditorExtension, IUnknown, "{42D1141D-1455-47EA-A610-089D87173C8E}")
{
    STDMETHOD(QueryInterface)(REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;

#ifdef _UNICODE
    STDMETHOD(GetName)(OUT const WCHAR **pszOut) PURE;
    STDMETHOD(GetVersionString)(OUT const WCHAR **pszOut) PURE;
    STDMETHOD(GetAuthor)(OUT const WCHAR **pszOut) PURE;
#else
    STDMETHOD(GetName)(OUT const CHAR **pszOut) PURE;
    STDMETHOD(GetVersionString)(OUT const CHAR **pszOut) PURE;
    STDMETHOD(GetAuthor)(OUT const CHAR **pszOut) PURE;
#endif

    STDMETHOD(GetToolSet)(const CLSID **prgiidTools, int *piNumTools) PURE;
};
// {42D1141D-1455-47EA-A610-089D87173C8E}
DEFINE_GUID(IID_IScreenshotEditorExtension,
    0x42d1141d, 0x1455, 0x47ea, 0xa6, 0x10, 0x8, 0x9d, 0x87, 0x17, 0x3c, 0x8e);

// {2FE63844-CA9B-4D9A-84F9-1ACF4AD39C7E}
DEFINE_GUID(CLSID_ScreenshotEditorExtension,
    0x2fe63844, 0xca9b, 0x4d9a, 0x84, 0xf9, 0x1a, 0xcf, 0x4a, 0xd3, 0x9c, 0x7e);