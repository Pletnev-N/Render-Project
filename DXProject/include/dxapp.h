#pragma once

#include <windows.h>
#include <assert.h>
#include <system_error>
#include <string>

#include "Utils.h"
#include "Renderer.h"


class DXApp
{
public:
    DXApp(HINSTANCE hInstance);
    ~DXApp();

    HINSTANCE AppInst() const;
    HWND      MainWnd() const;

    int Run();

    // Framework methods.  Derived client class overrides these methods to 
    // implement specific application requirements.

    virtual bool Init();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y);
    virtual void OnMouseUp(WPARAM btnState, int x, int y);
    virtual void OnMouseMove(WPARAM btnState, int x, int y);

protected:
    bool InitMainWindow();

    HINSTANCE mhAppInst; // application instance handle
    HWND      mhMainWnd; // main window handle
    bool      mAppPaused; // is the application paused?
    bool      mMinimized; // is the application minimized?
    bool      mMaximized; // is the application maximized?
    bool      mResizing; // are the resize bars being dragged?
    int       mClientWidth;
    int       mClientHeight;

    GameTimer mTimer;
    Renderer mRenderer;

    std::wstring mMainWndCaption;
};

