#pragma once
#include <windows.h>
#include <initguid.h>
#include <comdef.h>

#ifndef DECLARE_INTERFACE_IID_
#define DECLARE_INTERFACE_IID_(iface, baseiface, iid)   interface DECLSPEC_UUID(iid) DECLSPEC_NOVTABLE iface : public baseiface
#endif

#ifdef _UNICODE
#define QS_TCHAR WCHAR
#else
#define QS_TCHAR CHAR
#endif

// I don't think this would benefit from being a COM class.
#undef INTERFACE
#define INTERFACE IScreenshotContext
DECLARE_INTERFACE(IScreenshotContext)
{
    STDMETHOD_(THIS_ HBITMAP, GetScreenshotBitmap)() PURE;
    STDMETHOD_(THIS_ HBITMAP, GetCursorBitmapColorChannel)() PURE;
    STDMETHOD_(THIS_ HBITMAP, GetCursorBitmapMaskChannel)() PURE;
    STDMETHOD_(THIS_ POINT, GetCursorPosition)() PURE;
    STDMETHOD_(THIS_ BOOL, GetCursorVisible)() PURE;
    STDMETHOD_(THIS_ POINT, GetVirtualScreenOrigin)() PURE;
    STDMETHOD_(THIS_ SIZE, GetVirtualScreenSize)() PURE;
};

//
// As design speculation, I would just say that it is probably best to implement an object
// and at least its primary renderer in the same class. They will usually benefit from
// accessing the same state.
//

#undef INTERFACE
#define INTERFACE IScreenshotEditorObjectRenderer
DECLARE_INTERFACE_IID_(IScreenshotEditorObjectRenderer, IUnknown, "{56E23ECC-23B2-4033-BB8D-50FFDA763D10}")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    
    STDMETHOD(Paint)(THIS) PURE;
};
// {56E23ECC-23B2-4033-BB8D-50FFDA763D10}
DEFINE_GUID(IID_IScreenshotEditorObjectRenderer,
    0x56e23ecc, 0x23b2, 0x4033, 0xbb, 0x8d, 0x50, 0xff, 0xda, 0x76, 0x3d, 0x10);

#undef INTERFACE
#define INTERFACE IScreenshotEditorObjectRendererGDI
DECLARE_INTERFACE_IID_(IScreenshotEditorObjectRendererGDI, IScreenshotEditorObjectRenderer, "{56E23ECC-23B2-4033-BB8D-50FFDA763D11}")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD(Paint)(THIS) PURE;

    STDMETHOD(SetGdiParameters)(THIS_ HDC hdc, RECT *prcPaint) PURE;
};
// {56E23ECC-23B2-4033-BB8D-50FFDA763D11}
DEFINE_GUID(IID_IScreenshotEditorObjectRendererGDI,
    0x56e23ecc, 0x23b2, 0x4033, 0xbb, 0x8d, 0x50, 0xff, 0xda, 0x76, 0x3d, 0x11);

//
// Editor objects and tools have the screenshot editor as their site.
//

//@Begin IScreenshotEditorObject::GetFlags() flags
#define SSEOF_NODRAG (1 << 0)
#define SSEOF_NOBACKBUFFER (1 << 1)
#define SSEOF_MAYFLATTEN (1 << 2)
//@End IScreenshotEditorObject::GetFlags() flags

/**
 * Represents a screenshot document object.
 * 
 * An object is a visual layer with metadata for manipulation with tools.
 * 
 * The site of an object is the hosting screenshot editor.
 */
#undef INTERFACE
#define INTERFACE IScreenshotEditorObject
DECLARE_INTERFACE_IID_(IScreenshotEditorObject, IObjectWithSite, "{841C133A-9960-44C3-9E95-C1349C731401}")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(SetSite)(THIS_ IUnknown *pUnkSite) PURE;
    STDMETHOD(GetSite)(THIS_ REFIID riid, void **ppvSite) PURE;

    /**
     * Gets various flags used to control the interaction of the object.
     */
    STDMETHOD_(ULONG, GetFlags)(THIS) PURE;

    /**
     * Called when the object is inserted into the document.
     */
    STDMETHOD(InsertedIntoDocument)(THIS) PURE;

    /**
     * Gets the logical bounding rectangle for the object.
     * 
     * An object's logical rectangle is relative to the document.
     */
    STDMETHOD(GetLogicalRect)(THIS_ IN RECT *prc) PURE;

    /**
     * Gets the visual bounding rectangle for the object.
     * 
     * An object's visual rectangle is relative to its logical rectangle. It may have negative
     * coordinates.
     */
    STDMETHOD(GetVisualRect)(THIS_ IN RECT *prc) PURE;

    /**
     * Returns whether or not the visual is dirty.
     * 
     * An object with a dirty visual is prompted to be redrawn. The last clean visual is cached
     * by the renderer, so the object will not be requested to redraw unless it is dirty.
     */
    STDMETHOD_(BOOL, IsVisualDirty)(THIS) PURE;

    /**
     * Changes the position and/or size of the object.
     */
    STDMETHOD(Move)(THIS_ RECT *prcNew) PURE;

    /**
     * Creates a renderer for this object.
     */
    STDMETHOD(CreateRenderer)(THIS_ REFIID riid, OUT IScreenshotEditorObjectRenderer **ppRendererOut) PURE;
};
// {841C133A-9960-44C3-9E95-C1349C731401}
DEFINE_GUID(IID_IScreenshotEditorObject,
    0x841c133a, 0x9960, 0x44c3, 0x9e, 0x95, 0xc1, 0x34, 0x9c, 0x73, 0x14, 0x1);

#undef INTERFACE
#define INTERFACE IScreenshotEditor
DECLARE_INTERFACE_IID_(IScreenshotEditor, IUnknown, "{0A573BD7-2C24-4602-A842-ACC82B12E1F1}")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /**
     * Inserts an object into the screenshot document.
     */
    STDMETHOD(InsertObject)(THIS_ IScreenshotEditorObject *pObj) PURE;

    /**
     * Invalidates an object's visual.
     */
    STDMETHOD(InvalidateObject)(THIS_ IScreenshotEditorObject *pObj) PURE;

    /**
     * Gets the context of the screenshot document.
     */
    STDMETHOD(GetScreenshotContext)(THIS_ OUT IScreenshotContext **ppContextOut) PURE;

    /**
     * Gets the position of the mouse cursor relative to the document.
     */
    STDMETHOD(GetCursorPosition)(THIS_ OUT POINT *pptCursor) PURE;

    /**
     * Gets the handle to the window containing the document editor.
     */
    STDMETHOD_(HWND, GetEditorHWND)(THIS) PURE;
};
// {0A573BD7-2C24-4602-A842-ACC82B12E1F1}
DEFINE_GUID(IID_IScreenshotEditor,
    0xa573bd7, 0x2c24, 0x4602, 0xa8, 0x42, 0xac, 0xc8, 0x2b, 0x12, 0xe1, 0xf1);

//@Begin IScreenshotEditorTool::GetFlags() flags
#define SSETF_DRAWSELECTION (1 << 0)
//@End IScreenshotEditorTool::GetFlags() flags

/**
 * Represents a tool that can be used in the screenshot editor.
 * 
 * Extensions can add new tools.
 */
#undef INTERFACE
#define INTERFACE IScreenshotEditorTool
DECLARE_INTERFACE_IID_(IScreenshotEditorTool, IObjectWithSite, "{1C093E9E-696B-42CA-B4C3-67B793AF2D52}")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(SetSite)(THIS_ IUnknown *pUnkSite) PURE;
    STDMETHOD(GetSite)(THIS_ REFIID riid, void **ppvSite) PURE;

    /**
     * Called when the tool is selected.
     */
    STDMETHOD(ToolSelectionChanged)(THIS_ BOOL fSelected) PURE;

    /**
     * Gets various flags used to control the interaction of the tool.
     */
    STDMETHOD_(ULONG, GetFlags)(THIS) PURE;

    /**
     * Gets the tool icon to be displayed in the toolbox.
     */
    STDMETHOD_(HICON, GetToolIcon)(THIS) PURE;

    /**
     * Gets the name of the tool to be displayed in a tooltip when hovering the tool
     * in the toolbox.
     */
    STDMETHOD(GetToolName)(THIS_ OUT const QS_TCHAR **pszOut) PURE;

    /**
     * Called when any keyboard key is pressed down in the document editor while the tool is selected.
     */
    STDMETHOD(OnKeyDown)(THIS_ int iVirtualKey, LPARAM lParam) PURE;

    /**
     * Called when any keyboard key is released in the document editor while the tool is selected.
     */
    STDMETHOD(OnKeyUp)(THIS_ int iVirtualKey, LPARAM lParam) PURE;

    /**
     * Called when the mouse is moved within the document editor while the tool is selected.
     */
    STDMETHOD(OnMouseMove)(THIS_ int x, int y, WPARAM flags) PURE;

    /**
     * Called when the left mouse button is pressed down in the document editor while the tool is selected.
     */
    STDMETHOD(OnMouseLButtonDown)(THIS_ LONG x, LONG y, WPARAM flags) PURE;

    /**
     * Called when the left mouse button is released in the document editor while the tool is selected.
     */
    STDMETHOD(OnMouseLButtonUp)(THIS_ LONG x, LONG y, WPARAM flags) PURE;

    /**
     * Called when the right mouse button is pressed down in the document editor while the tool is selected.
     */
    STDMETHOD(OnMouseRButtonDown)(THIS_ LONG x, LONG y, WPARAM flags) PURE;

    /**
     * Called when the right mouse button is released in the document editor while the tool is selected.
     */
    STDMETHOD(OnMouseRButtonUp)(THIS_ LONG x, LONG y, WPARAM flags) PURE;

    /**
     * Called to allow the tool to apply a new cursor.
     */
    STDMETHOD(ApplyCursor)(THIS) PURE;

    /**
     * If the SSETF_DRAWSELECTION flag is set, called when the drawn selection is changed.
     */
    STDMETHOD(OnSelectionChange)(THIS_ RECT *prcNew) PURE;
};
// {1C093E9E-696B-42CA-B4C3-67B793AF2D52}
DEFINE_GUID(IID_IScreenshotEditorTool,
    0x1c093e9e, 0x696b, 0x42ca, 0xb4, 0xc3, 0x67, 0xb7, 0x93, 0xaf, 0x2d, 0x52);


//@Begin IScreenshotEditorExtension::GetExtensionFlags() flags
#define SSEEF_UNICODE (1 << 0)
//@End IScreenshotEditorExtension::GetExtensionFlags() flags

#undef INTERFACE
#define INTERFACE IScreenshotEditorExtension
DECLARE_INTERFACE_IID_(IScreenshotEditorExtension, IUnknown, "{42D1141D-1455-47EA-A610-089D87173C8E}")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, OUT void **ppvOut) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD_(ULONG, GetExtensionFlags)(THIS) PURE;

    /**
     * Gets the name of the extension.
     */
    STDMETHOD(GetName)(THIS_ OUT const QS_TCHAR **pszOut) PURE;

    /**
     * Gets the version of the extension as a string.
     */
    STDMETHOD(GetVersionString)(THIS_ OUT const QS_TCHAR **pszOut) PURE;

    /**
     * Gets the name of the author of the extension.
     */
    STDMETHOD(GetAuthor)(THIS_ OUT const QS_TCHAR **pszOut) PURE;

    /**
     * Gets a set of tools provided by the extension.
     * 
     * This function puts out an array of CLSIDs for each tool. Tools must be constructed
     * with a call to CreateTool on the same extension.
     */
    STDMETHOD(GetToolSet)(THIS_ OUT const CLSID **prgclsidTools, OUT int *piNumTools) PURE;

    /**
     * Creates an instance of a tool which is provided by this extension.
     */
    STDMETHOD(CreateTool)(THIS_ REFCLSID rclsidTool, OUT IScreenshotEditorTool **ppToolOut) PURE;
};
// {42D1141D-1455-47EA-A610-089D87173C8E}
DEFINE_GUID(IID_IScreenshotEditorExtension,
    0x42d1141d, 0x1455, 0x47ea, 0xa6, 0x10, 0x8, 0x9d, 0x87, 0x17, 0x3c, 0x8e);

// {2FE63844-CA9B-4D9A-84F9-1ACF4AD39C7E}
DEFINE_GUID(CLSID_ScreenshotEditorExtension,
    0x2fe63844, 0xca9b, 0x4d9a, 0x84, 0xf9, 0x1a, 0xcf, 0x4a, 0xd3, 0x9c, 0x7e);