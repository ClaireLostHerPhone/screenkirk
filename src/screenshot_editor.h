#pragma once
#include "pch.h"
#include "window.h"
#include "screenshot_manager.h"
#include "dynarray.h"

enum ScreenshotEditorTool
{
    SSET_SELECT,
    SSET_DRAG,
    SSET_ILLEGAL, // Used as a fallback if an extension tool fails.

    SSET_EXTENSIONFIRST, // Used for extensions. Not really currently implemented.
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

struct ExtensionToolInfo
{
    IScreenshotEditorTool *pTool;
    UINT idTool;
    const TCHAR *pszToolName;
};

//
// Editor floating toolbar (used in fullscreen mode)
//
class CEditorFloatingToolbar : public CWindow<CEditorFloatingToolbar>
{
    DEFINE_WINDOW_CLASS("screenkirk_EditorFloatingToolbar");

private:
    class CScreenshotEditorWindow *_pEditor;
    HWND _hwndToolbarTools;
    HWND _hwndToolbarActions;

protected:
    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    HRESULT _OnCreate();
    LRESULT _OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT _OnNotify(NMHDR *pnmh, WPARAM wParam, bool *pfHandled);

    HRESULT _UnselectTool();

public:
    enum Command
    {
        IDM_DISCARD = 100,
        IDM_COPY,
        IDM_SAVE,
        IDM_OPENINEXTERNALEDITOR,

        IDM_TOOLFIRST = 200,
    };

    static HRESULT RegisterWindowClass();

    /**
     * Creates the floating toolbar.
     */
    static CEditorFloatingToolbar *Create(class CScreenshotEditorWindow *pEditor);

    HRESULT SelectOrdinalTool(int idx);
    HRESULT OnToolChanged(ScreenshotEditorTool toolNew);
};

class CRenderObject
{
public:
    IScreenshotEditorObject *_pObj;
    IScreenshotEditorObjectRenderer *_pRenderer;
    IScreenshotEditorObjectRendererGDI *_pRendererGdi;
    HBITMAP _hbmLayer;

    inline bool HasGdiRenderer()
    {
        return _pRendererGdi != nullptr;
    }
};

class CScreenshotEditorRendererGDI
{
    CScreenshotContext *_pScreenshotCtx;
    HWND _hwndRenderTarget;
    HPEN _hpenSelect;
    HPEN _hpenSizingHelpers;
    HBITMAP _hbmScreenshotLight;
    HBITMAP _hbmScreenshotDimmed;
    HBITMAP *_hbmMipmaps;
    RECT _rcSelection;
    RECT _rcSelectionVisual;
    CDynamicArray<CRenderObject> _vRenderObjs;
    int _cMipmaps;
    int _iSelMarqueeFrame;
    float _iZoom;

    struct Bitmap
    {
        bool fHasAnySelectionMade : 1;
        bool fAnyObjectDirty : 1;
        bool fDrawSelectionSizeHelpers : 1;
        bool fDrawMarqueeSelection : 1;
        bool fCopiedScreenshot : 1;
        bool fSelectionBorderAnimDirty : 1;
        bool fSelectionDirty : 1;
        bool fSelectionDirtyNorth : 1;
        bool fSelectionDirtyEast : 1;
        bool fSelectionDirtySouth : 1;
        bool fSelectionDirtyWest : 1;
        bool fEntireFrameDirty : 1;
    } _bmp = { 0 };

    HRESULT _PaintSelectionRectangle(HDC hdc, RECT *prc, bool fUseMarquee);
    HRESULT _PaintSizingHelpers(HDC hdc, RECT *prc);
    HRESULT _PaintRenderObjectVisualBuffer(HDC hdcRenderTarget, RECT *prcPaint, CRenderObject *pRenderObject);
    void _UpdateMarquee();
    HRESULT _StartSelectionMarqueeTimer();
    HRESULT _EndSelectionMarqueeTimer();
    HRESULT _DrawMarqueeDottedRectangle(HDC hdc, RECT *prc);
    HRESULT _MakeDimmedScreenshot();
    HRESULT _FindRenderObjectFromInterfaceObject(
        IScreenshotEditorObject *pIfaceObj, OUT CRenderObject **ppRenderObjOut, OUT int *pIdxOut = nullptr);

public:
    static constexpr int c_iRadiusSelHelper = 9;
    static constexpr int c_idTimerMarquee = 101;

    CScreenshotEditorRendererGDI(CScreenshotContext *pCtx, HWND hwndRenderTarget)
        : _pScreenshotCtx(pCtx)
        , _hwndRenderTarget(hwndRenderTarget)
        , _hbmScreenshotLight(pCtx->_hbmScreenshot)
        , _iZoom(1.0)
    {
    }

    ~CScreenshotEditorRendererGDI();

    HRESULT Initialize();
    HRESULT Paint(HDC hdc, RECT *prcPaint = nullptr);
    HRESULT UpdateSelection(RECT *prcNew);
    HRESULT UpdateSizingHelpersVisibility(bool fVisible);
    HRESULT SetMarqueeSelection(bool fMarquee);
    HRESULT HandleWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT CreateRenderObject(IScreenshotEditorObject *pObj);
    HRESULT RemoveRenderObject(IScreenshotEditorObject *pObj);
    HRESULT InvalidateRenderObject(IScreenshotEditorObject *pObj);
};

//
// Editor main window. 
//
class CScreenshotEditorWindow 
    : public CWindow<CScreenshotEditorWindow>
    , public IScreenshotEditor
{
    DEFINE_WINDOW_CLASS("screenkirk_ScreenshotEditorWindow");

private:
    CScreenshotContext *_pScreenshotCtx;
    CScreenshotEditorRendererGDI *_pRenderer;
    CEditorFloatingToolbar *_pFloatingToolbar;
    CDynamicArray<IScreenshotEditorObject *> _vObjs;
    CDynamicArray<ExtensionToolInfo> _vExtToolInfo;
    POINT _ptSelectionOrigin;
    RECT _rcSelection;
    RECT _rcDragBegin;
    ScreenshotEditorTool _tool;
    IScreenshotEditorTool *_pExtTool; // The current extension tool, if any.
    int _iToolMode;
    bool _fEnumeratedWindows;
    bool _fIsSelectingRegion;
    bool _fHasAnySelectionMade;

protected:
    inline bool _IsExtensionTool()
    {
        return _tool >= SSET_EXTENSIONFIRST;
    }

    LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    LRESULT _OnCreate(CREATESTRUCT *pCs);
    LRESULT _OnDestroy();
    LRESULT _OnKeyDown(WPARAM virtualKey, LPARAM lParam);
    LRESULT _OnMouseMove(int x, int y, WPARAM flags);
    LRESULT _OnMouseLButtonDown(int x, int y, WPARAM flags);
    LRESULT _OnMouseLButtonUp(int x, int y, WPARAM flags);
    LRESULT _OnMouseRButtonDown(int x, int y, WPARAM flags);
    LRESULT _OnMouseRButtonUp(int x, int y, WPARAM flags);

    HRESULT _ApplyCrop();

    HRESULT _LoadExtensionTools();

    HRESULT _ChangeTool(ScreenshotEditorTool newTool);
    void _ShowFloatingToolbar();
    void _HideFloatingToolbar();
    void _UpdateCursor();
    void _CancelSelection();
    HRESULT _RemoveObject(IScreenshotEditorObject *pObj);

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
        WM_SSE_SAVEIMAGE,
    };

    //@Begin IUnknown
    STDMETHODIMP QueryInterface(const REFIID riid, void **ppvOut) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        // We don't use COM reference counting since the lifetime of this object
        // is bound to the window. Unfortunately, we just must risk crashing if
        // a bad user doesn't respond to our tear-down requests.
        return 1;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        return 1;
    }
    //@End IUnknown

    //@Begin IScreenshotEditor
    STDMETHODIMP InsertObject(IScreenshotEditorObject *pObj);
    STDMETHODIMP InvalidateObject(IScreenshotEditorObject *pObj);
    STDMETHODIMP GetScreenshotContext(OUT IScreenshotContext **ppContext);
    //@End IScreenshotEditor

    HRESULT CopyToClipboardAndAccept();
    HRESULT SaveImageToFileAndAccept();
    int GetExtensionToolCount();
    HRESULT GetExtensionToolInfo(int idx, OUT ExtensionToolInfo *pExtToolInfo);

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
class CFloatingScreenshotEditorWindow : public CWindow<CFloatingScreenshotEditorWindow>
{
    DEFINE_WINDOW_CLASS("screenkirk_FloatingScreenshotEditorWindow");

private:
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