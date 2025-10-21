#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>

#include "Utils.h"
#include "GameTimer.h"

using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct VS_CONSTANT_BUFFER
{
    XMFLOAT4X4 mWorldViewProj;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Init(HWND mhMainWnd);
    bool IsInitialized() const { return mbInitialized; }
    void UpdateScene(float dt);
    void DrawScene();
    void CalculateFrameStats(const GameTimer& timer, std::wstring& mMainWndCaption, HWND mhMainWnd);

    void OnResize(int width, int height);

    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y);

private:
    bool InitDirect3D(HWND mhMainWnd);
    void CreateShaders();
    void CreateMesh();
    void CreateConstantBuffer();

    float AspectRatio() const;

    bool mbInitialized;

    D3D_DRIVER_TYPE md3dDriverType;
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