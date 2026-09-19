#pragma once
#include "pch.h"

struct WindowPositionRecord
{
    HWND hwnd;
    HWND hwndParent;
    DWORD dwStyle;
    DWORD dwExStyle;
    TCHAR szWindowClass[MAX_PATH];
    RECT rc;
};

struct WindowPositionList
{
    WindowPositionList *pNext;
    WindowPositionRecord rgRecords[16];
};

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

class CScreenshotContext : public IScreenshotContext
{
    HRESULT _EnsureModificationBuffer();

public:
    HBITMAP _hbmScreenshot;
    HBITMAP _hbmCursorColor; // The color channel of the cursor bitmap.
    HBITMAP _hbmCursorMask;  // The mask channel of the cursor bitmap.
    HBITMAP _hbmModified; // The buffer for all modifications to the original bitmap.
    POINT _ptVirtualScreen;
    SIZE _sizeDesktop;
    POINT _ptCursor;
    WindowPositionList *_pWndPosList; // A list of all windows, from highest to lowest z-index.
    bool _fCursorVisible;

    ~CScreenshotContext()
    {
        DeleteObject(_hbmScreenshot);

        if (_pWndPosList)
        {
            WindowPositionList *pAllocated = _pWndPosList;
            do
            {
                WindowPositionList *pToDelete = pAllocated;
                pAllocated = pAllocated->pNext;
                delete pToDelete;
            }
            while (pAllocated->pNext);
        }
    }

    inline bool WindowPositionsRetrieved()
    {
        return _pWndPosList != nullptr;
    }

    typedef HRESULT (*OnGetWindowPositionsCB)();

    /**
     * Gets all window positions on the desktop.
     */
    HRESULT GetWindowPositions(OnGetWindowPositionsCB cb);

    HRESULT Crop(RECT *prcCrop);
    HRESULT CopyToClipboard();

    //
    // Interface method implementations:
    //

    STDMETHODIMP_(HBITMAP) GetScreenshotBitmap()
    {
        return _hbmScreenshot;
    }

    STDMETHODIMP_(HBITMAP) GetCursorBitmapColorChannel()
    {
        return _hbmCursorColor;
    }

    STDMETHODIMP_(HBITMAP) GetCursorBitmapMaskChannel()
    {
        return _hbmCursorColor;
    }

    STDMETHODIMP_(POINT) GetCursorPosition()
    {
        return _ptCursor;
    }

    STDMETHODIMP_(BOOL) GetCursorVisible()
    {
        return _fCursorVisible;
    }

    STDMETHODIMP_(POINT) GetVirtualScreenOrigin()
    {
        return _ptVirtualScreen;
    }

    STDMETHODIMP_(SIZE) GetVirtualScreenSize()
    {
        return _sizeDesktop;
    }
};

void OnScreenshotKeyPressed();
HRESULT TakeScreenshot(OUT CScreenshotContext **ppContextOut);