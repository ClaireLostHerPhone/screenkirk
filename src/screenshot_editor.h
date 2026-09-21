#pragma once
#include "pch.h"
#include "window.h"
#include "screenshot_manager.h"
#include "dynarray.h"

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

//
// Editor floating toolbar (used in fullscreen mode)
//
const TCHAR c_szEditorFloatingToolbarClassName[] = TEXT("screenkirk_EditorFloatingToolbar");
class CEditorFloatingToolbar : public CWindow<CEditorFloatingToolbar, c_szEditorFloatingToolbarClassName>
{
    HWND _hwndEditor;
    HWND _hwndToolbar;

protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    HRESULT _OnCreate();

public:
    enum Command
    {
        IDM_DISCARD = 100,
        IDM_COPY,
        IDM_SAVE,

        IDM_TOOLFIRST = 200,
    };

    static HRESULT RegisterWindowClass();

    /**
     * 
     */
    static CEditorFloatingToolbar *Create(HWND hwndEditor);
};

class CRenderObject
{
public:
    IScreenshotEditorObject *_pObj;
    IScreenshotEditorObjectRenderer *_pRenderer;
    IScreenshotEditorObjectRendererGDI *_pRendererGdi = nullptr;

    inline bool HasGdiRenderer()
    {
        return _pRendererGdi != nullptr;
    }
};

class CScreenshotEditorRendererGDI
{
    CScreenshotContext *_pScreenshotCtx;
    HWND _hwndRenderTarget;
    HPEN _hpenSelect = nullptr;
    HBITMAP _hbmScreenshotLight = nullptr;
    HBITMAP _hbmScreenshotDimmed = nullptr;
    HBITMAP *_hbmMipmaps = nullptr;
    RECT _rcSelection = { 0 };
    CDynamicArray<CRenderObject> _vRenderObjs;
    int _cMipmaps = 0;
    int _iSelMarqueeFrame = 0;
    float _iZoom = 1.0;

    struct Bitmap
    {
        bool fHasAnySelectionMade : 1;
        bool fAnyObjectDirty : 1;
        bool fDrawMarqueeSelection : 1;
        bool fCopiedScreenshot : 1;
        bool fSelectionBorderAnimDirty : 1;
        bool fSelectionDirty : 1;
        bool fSelectionDirtyNorth : 1;
        bool fSelectionDirtyEast : 1;
        bool fSelectionDirtySouth : 1;
        bool fSelectionDirtyWest : 1;
        bool fEntireFrameDirty : 1;
        bool fSelThickNorth : 1;
        bool fSelThickEast : 1;
        bool fSelThickSouth : 1;
        bool fSelThickWest : 1;
    } _bmp = { 0 };

    HRESULT _PaintSelectionRectangle(HDC hdc, RECT *prc, bool fUseMarquee);
    void _UpdateMarquee();
    void _ClearDragModeVisualFlags();
    HRESULT _StartSelectionMarqueeTimer();
    HRESULT _EndSelectionMarqueeTimer();
    HRESULT _DrawMarqueeDottedRectangle(HDC hdc, RECT *prc);
    HRESULT _MakeDimmedScreenshot();

public:
    static constexpr int c_idTimerMarquee = 101;

    CScreenshotEditorRendererGDI(CScreenshotContext *pCtx, HWND hwndRenderTarget)
        : _pScreenshotCtx(pCtx)
        , _hwndRenderTarget(hwndRenderTarget)
        , _hbmScreenshotLight(pCtx->_hbmScreenshot)
    {
    }

    ~CScreenshotEditorRendererGDI();

    HRESULT Initialize();
    HRESULT Paint(HDC hdc, RECT *prcPaint = nullptr);
    HRESULT UpdateSelection(RECT *prcNew);
    HRESULT UpdateDragMode(DragMode dm);
    HRESULT SetMarqueeSelection(bool fMarquee);
    HRESULT HandleWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

//
// Editor main window. 
//
const TCHAR c_szScreenshotEditorWindowClassName[] = TEXT("screenkirk_ScreenshotEditorWindow");
class CScreenshotEditorWindow : public CWindow<CScreenshotEditorWindow, c_szScreenshotEditorWindowClassName>
{
    CScreenshotContext *_pScreenshotCtx;
    CScreenshotEditorRendererGDI *_pRenderer = nullptr;
    CEditorFloatingToolbar *_pFloatingToolbar = nullptr;
    POINT _ptSelectionOrigin;
    RECT _rcSelection;
    RECT _rcDragBegin;
    ScreenshotEditorTool _tool = SSET_SELECT;
    IScreenshotEditorTool *_pExtTool = nullptr; // The current extension tool, if any.
    int _iToolMode = 0;
    bool _fEnumeratedWindows = false;
    bool _fIsSelectingRegion = false;
    bool _fHasAnySelectionMade = false;

protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    LRESULT _OnDestroy();
    LRESULT _OnKeyDown(WPARAM virtualKey, LPARAM lParam);
    LRESULT _OnMouseMove(int x, int y, WPARAM flags);
    LRESULT _OnMouseLButtonDown(int x, int y, WPARAM flags);
    LRESULT _OnMouseLButtonUp(int x, int y, WPARAM flags);
    LRESULT _OnMouseRButtonDown(int x, int y, WPARAM flags);
    LRESULT _OnMouseRButtonUp(int x, int y, WPARAM flags);

    HRESULT _ChangeTool(ScreenshotEditorTool newTool);
    void _ShowFloatingToolbar();
    void _HideFloatingToolbar();
    void _UpdateCursor();
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
        WM_SSE_GETWINDOWPOSITIONS = WM_APP + 1,
        WM_SSE_CHANGETOOL,
        WM_SSE_COPYTOCLIPBOARD,
    };

    HRESULT CopyToClipboardAndAccept();

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