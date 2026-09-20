#pragma once
#include <windows.h>

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

/**
 * Interface for extension tools.
 */
DECLARE_INTERFACE(IScreenshotEditorTool)
{
    STDMETHOD_(HICON, GetToolIcon)() PURE;
    #ifdef _UNICODE
    STDMETHOD(GetToolName)(const WCHAR *) PURE;
    #else
    STDMETHOD(GetToolName)(const CHAR *) PURE;
    #endif
    STDMETHOD(OnMouseMove)(int x, int y, WPARAM flags) PURE;
    STDMETHOD(OnMouseLButtonDown)(LONG x, LONG y, WPARAM flags) PURE;
    STDMETHOD(OnMouseLButtonUp)(LONG x, LONG y, WPARAM flags) PURE;
    STDMETHOD(OnMouseRButtonDown)(LONG x, LONG y, WPARAM flags) PURE;
    STDMETHOD(OnMouseRButtonUp)(LONG x, LONG y, WPARAM flags) PURE;
    STDMETHOD(ApplyCursor)() PURE;
};
