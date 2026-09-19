#pragma once
#include "pch.h"

template <typename CImpl, const TCHAR *c_szClassName>
class CWindow
{
protected:
    HWND _hwnd;

private:
    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CWindow *pWnd = (CWindow *)GetWindowLongPtr(hwnd, 0);

        if (uMsg == WM_CREATE)
        {
            if (!pWnd)
            {
                pWnd = new CImpl();
                pWnd->_hwnd = hwnd;
                SetWindowLongPtr(hwnd, 0, (LONG_PTR)pWnd);
            }
        }
        else if (uMsg == WM_DESTROY)
        {
            if (pWnd)
            {
                LRESULT lr = pWnd->v_WndProc(hwnd, uMsg, wParam, lParam);
                SetWindowLongPtr(hwnd, 0, (LONG_PTR)nullptr);
                delete (CImpl *)pWnd;
                return lr;
            }
        }

        if (pWnd)
        {
            return pWnd->v_WndProc(hwnd, uMsg, wParam, lParam);
        }
        else
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

protected:
    static HRESULT RegisterWindowClass(WNDCLASS *pWndClass)
    {
        pWndClass->lpszClassName = c_szClassName;
        pWndClass->lpfnWndProc = s_WndProc;
        pWndClass->cbWndExtra = sizeof(CImpl *);

        if (!RegisterClass(pWndClass))
        {
            int lastError = GetLastError();
            if (lastError == ERROR_CLASS_ALREADY_EXISTS)
            {
                SetLastError(ERROR_SUCCESS);
                return S_OK;
            }
            else
            {
                return HRESULT_FROM_WIN32(lastError);
            }
        }
        else
        {
            return S_OK;
        }
    }

    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    static CImpl *Create(
        DWORD dwExStyle, const TCHAR *lpWindowName, DWORD dwStyle, int x, int y, int cx, int cy,
        HWND hwndParent, HMENU hMenu, HINSTANCE hInstance, void *pParam)
    {
        HWND hwnd = CreateWindowEx(dwExStyle, c_szClassName, lpWindowName, dwStyle, x, y, cx, cy,
            hwndParent, hMenu, hInstance, pParam);
        return hwnd
            ? (CImpl *)GetWindowLongPtr(hwnd, 0)
            : nullptr;
    }

public:
    CWindow()
        : _hwnd(nullptr)
    {
    }

    inline HWND GetHWND()
    {
        return _hwnd;
    }
};