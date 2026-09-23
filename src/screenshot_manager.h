#pragma once
#include "pch.h"

struct WindowPositionRecord
{
    HWND hwnd;
    HWND hwndParent;
    DWORD dwStyle;
    DWORD dwExStyle;
    TCHAR szWindowClass[MAX_PATH];
    RECT rcNonclient;
    RECT rcClient;
};

struct WindowPositionList
{
    WindowPositionList *pNext;
    WindowPositionRecord rgRecords[16];
};

class CScreenshotContext : public IScreenshotContext
{
    HRESULT _EnsureModificationBuffer();

public:
    HBITMAP _hbmScreenshot; // The original screenshot.
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
        DeleteObject(_hbmCursorColor);
        DeleteObject(_hbmCursorMask);

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

    //@Begin IScreenshotContext
    STDMETHODIMP_(HBITMAP) GetScreenshotBitmap() override
    {
        return _hbmScreenshot;
    }

    STDMETHODIMP_(HBITMAP) GetCursorBitmapColorChannel() override
    {
        return _hbmCursorColor;
    }

    STDMETHODIMP_(HBITMAP) GetCursorBitmapMaskChannel() override
    {
        return _hbmCursorColor;
    }

    STDMETHODIMP_(POINT) GetCursorPosition() override
    {
        return _ptCursor;
    }

    STDMETHODIMP_(BOOL) GetCursorVisible() override
    {
        return _fCursorVisible;
    }

    STDMETHODIMP_(POINT) GetVirtualScreenOrigin() override
    {
        return _ptVirtualScreen;
    }

    STDMETHODIMP_(SIZE) GetVirtualScreenSize() override
    {
        return _sizeDesktop;
    }
    //@End IScreenshotContext
};

void OnScreenshotKeyPressed();
HRESULT TakeScreenshot(OUT CScreenshotContext **ppContextOut);