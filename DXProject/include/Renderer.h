#pragma once

#include <d3d11.h>
#include <string>
#include <RenderDefs.h>
#include <Material.h>
#include <Utils.h>
#include <GameTimer.h>

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
    void CreateMaterial();
    void SetupLights();

    float AspectRatio() const;

    bool mbInitialized;

    D3D_DRIVER_TYPE md3dDriverType;
    ComPtr<ID3D11Device> md3dDevice;
    ComPtr<ID3D11DeviceContext> md3dImmediateContext;
    ComPtr<IDXGISwapChain> mSwapChain;
    ComPtr<ID3D11Texture2D> mDepthStencilBuffer;

    ComPtr<ID3D11RenderTargetView> mRenderTargetView;
    ComPtr<ID3D11DepthStencilView> mDepthStencilView;
    D3D11_VIEWPORT mScreenViewport;

    ComPtr<ID3D11VertexShader> mVertexShader;
    ComPtr<ID3D11PixelShader> mPixelShader;

    Material mMaterial;

    ComPtr<ID3D11InputLayout> mInputLayout;
    ComPtr<ID3D11Buffer> mPerFrameCbuffer;
    ComPtr<ID3D11Buffer> mDirectionalLightBuffer;

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