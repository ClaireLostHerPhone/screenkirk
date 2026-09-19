#pragma once
#include "pch.h"
#include "window.h"
#include "screenshot_manager.h"

enum ScreenshotEditorTool
{
    SSET_SELECT,
    SSET_DRAG,
    SSET_EXTENSION, // Used for extensions. Not really currently implemented.
    SSET_ILLEGAL, // Used as a fallback if an extension tool fails.
};

/**
 * Modes for the drag tool.
 */
enum DragMode
{
    DRAGM_DRAG = 0,
    DRAGM_SIZEN = 1,
    DRAGM_SIZES = 2,
    DRAGM_SIZEW = 1 << 2,
    DRAGM_SIZEE = 2 << 2,
    DRAGM_SIZENW = DRAGM_SIZEN | DRAGM_SIZEW, // Top-left corner
    DRAGM_SIZENE = DRAGM_SIZEN | DRAGM_SIZEE, // Top-right corner
    DRAGM_SIZESW = DRAGM_SIZES | DRAGM_SIZEW, // Bottom-left corner
    DRAGM_SIZESE = DRAGM_SIZES | DRAGM_SIZEE, // Bottom-right corner
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

//
// Editor floating toolbar (used in fullscreen mode)
//
const TCHAR c_szEditorFloatingToolbarClassName[] = TEXT("screenkirk_EditorFloatingToolbar");
class CEditorFloatingToolbar : public CWindow<CEditorFloatingToolbar, c_szEditorFloatingToolbarClassName>
{
protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

public:
    static HRESULT RegisterWindowClass();

    /**
     * 
     */
    static CEditorFloatingToolbar *CreateAndShow();
};

//
// Editor main window. 
//
const TCHAR c_szScreenshotEditorWindowClassName[] = TEXT("screenkirk_ScreenshotEditorWindow");
class CScreenshotEditorWindow : public CWindow<CScreenshotEditorWindow, c_szScreenshotEditorWindowClassName>
{
    CScreenshotContext *_pScreenshotCtx;
    HBITMAP _hbmScreenshotDimmed = nullptr;
    HBITMAP *_hbmMipmaps = nullptr;
    POINT _ptSelectionOrigin;
    RECT _rcSelection;
    RECT _rcDragBegin;
    ScreenshotEditorTool _tool = SSET_SELECT;
    DragMode _dragMode = DRAGM_DRAG;
    IScreenshotEditorTool *_pExtTool = nullptr; // The current extension tool, if any.
    int _cMipmaps = 0;
    float _iZoom = 1.0;
    bool _fEnumeratedWindows = false;
    bool _fIsSelectingRegion = false;
    bool _fHasAnySelectionMade = false;

protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    LRESULT _OnDestroy();
    LRESULT _OnPaint();
    LRESULT _OnKeyDown(WPARAM virtualKey, LPARAM lParam);
    LRESULT _OnMouseMove(int x, int y, WPARAM flags);
    LRESULT _OnMouseLButtonDown(int x, int y, WPARAM flags);
    LRESULT _OnMouseLButtonUp(int x, int y, WPARAM flags);
    LRESULT _OnMouseRButtonDown(int x, int y, WPARAM flags);
    LRESULT _OnMouseRButtonUp(int x, int y, WPARAM flags);

    HRESULT _ChangeTool(ScreenshotEditorTool newTool);
    void _UpdateCursor();
    HRESULT _MakeDimmedScreenshot();
    void _CancelSelection();

    /**
     * Event callback from the window enumeration thread from the screenshot
     * manager.
     * 
     * This runs in another thread, and posts a message to the UI message for
     * most functionality.
     */
    HRESULT _OnGetWindowPositions();

public:
    enum WM
    {
        WM_SCREENSHOTEDITOR_GETWINDOWPOSITIONS = WM_APP + 1,
    };

    static HRESULT RegisterWindowClass();

    /**
     * Creates the main screenshot editor viewport window.
     * 
     * @param _pScreenshotCtx
     *      Taken ownership of.
     */
    static CScreenshotEditorWindow *CreateAndShow(CScreenshotContext *pScreenshotCtx);
};

//
// Host window for floating (non-fullscreen) screenshot editors.
//
const TCHAR c_szFloatingScreenshotEditorWindowClassName[] = TEXT("screenkirk_FloatingScreenshotEditorWindow");
class CFloatingScreenshotEditorWindow : public CWindow<CFloatingScreenshotEditorWindow, c_szFloatingScreenshotEditorWindowClassName>
{
    CScreenshotEditorWindow *_pEditor;
    HWND _hwndToolbar;

protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

public:
    static HRESULT RegisterWindowClass();

    /**
     *
     */
    static CFloatingScreenshotEditorWindow *CreateAndShow(CScreenshotEditorWindow *pEditor);
};