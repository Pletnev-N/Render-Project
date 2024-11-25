#ifndef DXAPP_H
#define DXAPP_H


#include <d3d11.h>
#include <assert.h>
#include <system_error>
#include <string>
#include <DirectXMath.h>

#include "GameTimer.h"

using namespace DirectX;

//if (FAILED(hr)) \
//{ \
//    std::wstring message = std::system_category().message(hr); \
//    message = L"file: " + __FILE__ + L"\nline: " + __LINE__ + L"\nhr: " + std::to_string(hr) + L"\nmessage: " + message; \
//    MessageBox(0, message.c_str(), 0, 0); \
//} \

#if defined(DEBUG) | defined(_DEBUG)
    #ifndef HR
    #define HR(x) \
    { \
        HRESULT hr = (x); \
        assert(SUCCEEDED(hr)); \
    }
    #endif
#else
    #ifndef HR
    #define HR(x) (x)
    #endif
#endif

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }


template<typename T>
static T Clamp(const T& x, const T& low, const T& high)
{
    return x < low ? low : (x > high ? high : x);
}

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct VS_CONSTANT_BUFFER
{
    XMFLOAT4X4 mWorldViewProj;
};

class DXApp
{
public:
    DXApp(HINSTANCE hInstance);
    ~DXApp();

    HINSTANCE AppInst() const;
    HWND      MainWnd() const;
    float     AspectRatio() const;

    int Run();

    // Framework methods.  Derived client class overrides these methods to 
    // implement specific application requirements.

    virtual bool Init();
    virtual void OnResize();
    virtual void UpdateScene(float dt);
    virtual void DrawScene();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y);
    virtual void OnMouseUp(WPARAM btnState, int x, int y);
    virtual void OnMouseMove(WPARAM btnState, int x, int y);

protected:
    bool InitMainWindow();
    bool InitDirect3D();

    void CalculateFrameStats();

    bool CreateDevice();
    UINT Check4xMSAA();
    void FillSwapChainDescr();

    void CreateShaders();
    void CreateMesh();
    void CreateConstantBuffer();

    GameTimer mTimer;

    HINSTANCE mhAppInst; // application instance handle
    HWND      mhMainWnd; // main window handle
    bool      mAppPaused; // is the application paused?
    bool      mMinimized; // is the application minimized?
    bool      mMaximized; // is the application maximized?
    bool      mResizing; // are the resize bars being dragged?

    ID3D11Device* md3dDevice;
    ID3D11DeviceContext* md3dImmediateContext;
    IDXGISwapChain* mSwapChain;
    ID3D11Texture2D* mDepthStencilBuffer;

    ID3D11RenderTargetView* mRenderTargetView;
    ID3D11DepthStencilView* mDepthStencilView;
    D3D11_VIEWPORT mScreenViewport;

    ID3D11VertexShader* mVertexShader;
    ID3D11PixelShader* mPixelShader;

    ID3D11Buffer* mVsConstantBuffer;
    ID3D11InputLayout* mInputLayout;

    UINT m4xMsaaQuality;
    bool mEnable4xMsaa;

    std::wstring mMainWndCaption;
    D3D_DRIVER_TYPE md3dDriverType;
    int mClientWidth;
    int mClientHeight;

    XMFLOAT4X4 mWorld;
    XMFLOAT4X4 mView;
    XMFLOAT4X4 mProj;
    float mTheta;
    float mPhi;
    float mRadius;
    POINT mLastMousePos;
};



#endif