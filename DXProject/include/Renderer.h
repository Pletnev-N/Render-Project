#pragma once

#include <d3d11.h>
#include <string>
#include <RenderDefs.h>

#include "Utils.h"
#include "GameTimer.h"

using namespace DirectX;

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
    void CreateCubeMesh();
    void CreateConstantBuffers();
    void SetupLights();

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

    ID3D11InputLayout* mInputLayout;
    ID3D11Buffer* mPerFrameCbuffer;
    ID3D11Buffer* mDirectionalLightBuffer;

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

    UINT mIndexCount;
};