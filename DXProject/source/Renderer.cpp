#include "Renderer.h"

#include <vector>
#include <fstream>
#include <DirectXColors.h>
#include <d3d11shader.h>
#include <sstream>
#include <FbxReader.h>

Renderer::Renderer()
    : md3dDriverType(D3D_DRIVER_TYPE_HARDWARE),
	mbInitialized(false),
    mClientWidth(800),
    mClientHeight(600),
    mEnable4xMsaa(true),
    m4xMsaaQuality(0),


    md3dDevice(nullptr),
    md3dImmediateContext(nullptr),
    mSwapChain(0),
    mDepthStencilBuffer(0),
    mRenderTargetView(0),
    mDepthStencilView(0),
    mVertexShader(nullptr),
    mPixelShader(nullptr),
    mInputLayout(nullptr),
	mPerFrameCbuffer(nullptr),
    mDirectionalLightBuffer(nullptr),

    mTheta(0),
    mPhi(0.5f * XM_PI),
    mRadius(5.0f),

	mIndexCount(0)
{
    ZeroMemory(&mScreenViewport, sizeof(D3D11_VIEWPORT));

    mLastMousePos.x = 0;
    mLastMousePos.y = 0;

    XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&mWorld, I);
    XMStoreFloat4x4(&mView, I);
    XMStoreFloat4x4(&mProj, I);
}

Renderer::~Renderer()
{
    // Restore all default settings.
    if (md3dImmediateContext)
        md3dImmediateContext->ClearState();
}

bool Renderer::Init(HWND mhMainWnd)
{
	if (!InitDirect3D(mhMainWnd)) return false;

	CreateShaders();
	CreateMesh();
	CreateConstantBuffers();
	CreateMaterial();

    SetupLights();

	mbInitialized = true;
    return true;
}

void Renderer::UpdateScene(float dt)
{
    // Get camera position in Cartesian coordinates
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);
	
	// Build the view matrix
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = XMMatrixTranspose(world * view * proj);
	XMMATRIX worldInvTrans = XMMatrixTranspose(XMMatrixInverse(nullptr, world));

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	md3dImmediateContext->Map(mPerFrameCbuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	PER_FRAME_CBUFFER* data = static_cast<PER_FRAME_CBUFFER*>(mappedResource.pData);
	XMStoreFloat4x4(&data->mWorldViewProj, worldViewProj);
	XMStoreFloat4x4(&data->mWorldInvTrans, worldInvTrans);
	data->mWorld = mWorld;
	data->CamPos = XMFLOAT4(x, y, z, 1.0f);
	md3dImmediateContext->Unmap(mPerFrameCbuffer.Get(), 0);
}

void Renderer::DrawScene()
{
	const FLOAT blue[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

	assert(md3dImmediateContext);
	assert(mSwapChain);

	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView.Get(), blue);
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	md3dImmediateContext->IASetInputLayout(mInputLayout.Get());

	md3dImmediateContext->DrawIndexed(mIndexCount, 0, 0);

	HR(mSwapChain->Present(0, 0));
}

bool Renderer::InitDirect3D(HWND mhMainWnd)
{
	// Create the device and device context.

	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		0,                 // default adapter
		md3dDriverType,
		0,                 // no software device
		createDeviceFlags,
		0, 0,              // default feature level array
		D3D11_SDK_VERSION,
		&md3dDevice,
		&featureLevel,
		&md3dImmediateContext);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	if (featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	HR(md3dDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality));
	assert(m4xMsaaQuality > 0);

	// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Use 4X MSAA? 
	if (mEnable4xMsaa)
	{
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = m4xMsaaQuality - 1;
	}
	// No MSAA
	else
	{
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	// To correctly create the swap chain, we must use the IDXGIFactory that was
	// used to create the device.  If we tried to use a different IDXGIFactory instance
	// (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
	// This function is being called with a device from a different IDXGIFactory."

	IDXGIDevice* dxgiDevice = 0;
	HR(md3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

	IDXGIAdapter* dxgiAdapter = 0;
	HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	IDXGIFactory* dxgiFactory = 0;
	HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

	HR(dxgiFactory->CreateSwapChain(md3dDevice.Get(), &sd, &mSwapChain));

	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);

	// The remaining steps that need to be carried out for d3d creation
	// also need to be executed every time the window is resized.  So
	// just call the OnResize method here to avoid code duplication.

	OnResize(mClientWidth, mClientHeight);

	return true;
}

void Renderer::CalculateFrameStats(const GameTimer& timer, std::wstring& mMainWndCaption, HWND mhMainWnd)
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wostringstream outs;
		outs.precision(6);
		outs << mMainWndCaption << L"    "
			<< L"FPS: " << fps << L"    "
			<< L"Frame Time: " << mspf << L" (ms)";
		SetWindowText(mhMainWnd, outs.str().c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void Renderer::CreateShaders()
{
	std::ifstream shaderFile;

	shaderFile.open("ShadersBin\\VertexShader.cso", std::ios::binary);
	std::vector<char> fileData((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());

	HR(md3dDevice->CreateVertexShader(fileData.data(), fileData.size(), nullptr, &mVertexShader));
	md3dImmediateContext->VSSetShader(mVertexShader.Get(), nullptr, 0);

	D3D11_INPUT_ELEMENT_DESC desc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HR(md3dDevice->CreateInputLayout(desc, 3, fileData.data(), fileData.size(), &mInputLayout));

	shaderFile.close();
	shaderFile.open("ShadersBin\\PixelShader.cso", std::ios::binary);
	fileData.clear();
	fileData = { (std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>() };

	HR(md3dDevice->CreatePixelShader(fileData.data(), fileData.size(), nullptr, &mPixelShader));
	md3dImmediateContext->PSSetShader(mPixelShader.Get(), nullptr, 0);
}

void Renderer::CreateMesh()
{
	std::vector<VertexTextured> vertices;
	std::vector<UINT> indices;

	FBXReader fbxReader;
	//fbxReader.LoadFbxFile("C:\\repositories\\DXProject\\models\\coca-cola\\coca-cola.fbx");
	fbxReader.LoadFbxFile("C:\\repositories\\DXProject\\models\\coca-cola-2\\Coke_Can_Final.fbx");
	//fbxReader.LoadFbxFile("C:\\repositories\\DXProject\\models\\eyeball\\eyeball.fbx");
	fbxReader.GetVertices(vertices, indices);
	mIndexCount = indices.size();

	D3D11_BUFFER_DESC vertexBufDescr;
	vertexBufDescr.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufDescr.ByteWidth = sizeof(VertexTextured) * static_cast<UINT>(vertices.size());
	vertexBufDescr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufDescr.CPUAccessFlags = 0;
	vertexBufDescr.MiscFlags = 0;
	vertexBufDescr.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices.data();

	ID3D11Buffer* vertexBuffer;
	HR(md3dDevice->CreateBuffer(&vertexBufDescr, &vertexData, &vertexBuffer));

	UINT stride = sizeof(VertexTextured);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);


	D3D11_BUFFER_DESC indexBufDescr;
	indexBufDescr.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufDescr.ByteWidth = sizeof(UINT) * static_cast<UINT>(indices.size());
	indexBufDescr.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufDescr.CPUAccessFlags = 0;
	indexBufDescr.MiscFlags = 0;
	indexBufDescr.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices.data();

	ID3D11Buffer* indexBuffer;
	HR(md3dDevice->CreateBuffer(&indexBufDescr, &indexData, &indexBuffer));

	md3dImmediateContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void Renderer::CreateCubeMesh()
{
	CubeVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4((const float*)&Colors::White) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4((const float*)&Colors::Black) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4((const float*)&Colors::Red) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4((const float*)&Colors::Green) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4((const float*)&Colors::Blue) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4((const float*)&Colors::Yellow) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4((const float*)&Colors::Cyan) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4((const float*)&Colors::Magenta) }
	};

	D3D11_BUFFER_DESC vertexBufDescr;
	vertexBufDescr.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufDescr.ByteWidth = sizeof(CubeVertex) * 8;
	vertexBufDescr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufDescr.CPUAccessFlags = 0;
	vertexBufDescr.MiscFlags = 0;
	vertexBufDescr.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices;

	ID3D11Buffer* vertexBuffer;
	HR(md3dDevice->CreateBuffer(&vertexBufDescr, &vertexData, &vertexBuffer));

	UINT stride = sizeof(CubeVertex);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);


	UINT indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	D3D11_BUFFER_DESC indexBufDescr;
	indexBufDescr.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufDescr.ByteWidth = sizeof(UINT) * 36;
	indexBufDescr.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufDescr.CPUAccessFlags = 0;
	indexBufDescr.MiscFlags = 0;
	indexBufDescr.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;

	ID3D11Buffer* indexBuffer;
	HR(md3dDevice->CreateBuffer(&indexBufDescr, &indexData, &indexBuffer));

	md3dImmediateContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void Renderer::CreateConstantBuffers()
{
	D3D11_BUFFER_DESC cbDesc;
	D3D11_SUBRESOURCE_DATA InitData;

    //------------ PER_FRAME_CBUFFER ----------------

	PER_FRAME_CBUFFER PerFrameConstData;
	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&PerFrameConstData.mWorldViewProj, I);
    PerFrameConstData.CamPos = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	cbDesc.ByteWidth = sizeof(PER_FRAME_CBUFFER);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	InitData.pSysMem = &PerFrameConstData;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	HR(md3dDevice->CreateBuffer(&cbDesc, &InitData, mPerFrameCbuffer.GetAddressOf()));
	md3dImmediateContext->VSSetConstantBuffers(0, 1, mPerFrameCbuffer.GetAddressOf());
	md3dImmediateContext->PSSetConstantBuffers(0, 1, mPerFrameCbuffer.GetAddressOf());

	//-------------- LIGHTS_CBUFFER ---------------

	cbDesc.ByteWidth = sizeof(LIGHTS_CBUFFER);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	HR(md3dDevice->CreateBuffer(&cbDesc, nullptr, mDirectionalLightBuffer.GetAddressOf()));
	md3dImmediateContext->PSSetConstantBuffers(1, 1, mDirectionalLightBuffer.GetAddressOf());
}

void Renderer::CreateMaterial()
{
    
    mMaterial.LoadTextures(md3dDevice,
		//L"C:\\repositories\\DXProject\\\models\\coca-cola-2\\Coke_Clean\\test.tga",
		L"C:\\repositories\\DXProject\\\models\\coca-cola-2\\Coke_Clean\\Coke_Can_VRayMtl1_Reflection.tga",
		L"C:\\repositories\\DXProject\\\models\\coca-cola-2\\Coke_Clean\\Coke_Can_VRayMtl1_Normal.tga"
	);
    mMaterial.AttachToShaders(md3dImmediateContext);
}

void Renderer::SetupLights()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	md3dImmediateContext->Map(mDirectionalLightBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	LIGHTS_CBUFFER* lights = static_cast<LIGHTS_CBUFFER*>(mappedResource.pData);

    lights->DirLight.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    lights->DirLight.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    lights->DirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 2.0f);

    XMVECTOR dir = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    dir = XMVector3Normalize(dir);
    XMStoreFloat3(&lights->DirLight.Direction, dir);

	md3dImmediateContext->Unmap(mDirectionalLightBuffer.Get(), 0);
}

void Renderer::OnResize(int width, int height)
{
	mClientWidth = width;
    mClientHeight = height;

	assert(md3dImmediateContext);
	assert(md3dDevice);
	assert(mSwapChain);

	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.

	mRenderTargetView.Reset();
	mDepthStencilView.Reset();
	mDepthStencilBuffer.Reset();


	// Resize the swap chain and recreate the render target view.

	HR(mSwapChain->ResizeBuffers(1, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ID3D11Texture2D* backBuffer;
	HR(mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	HR(md3dDevice->CreateRenderTargetView(backBuffer, 0, mRenderTargetView.GetAddressOf()));
	ReleaseCOM(backBuffer);

	// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if (mEnable4xMsaa)
	{
		depthStencilDesc.SampleDesc.Count = 4;
		depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
	}
	// No MSAA
	else
	{
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}

	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	HR(md3dDevice->CreateTexture2D(&depthStencilDesc, 0, mDepthStencilBuffer.GetAddressOf()));
	HR(md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), 0, mDepthStencilView.GetAddressOf()));


	// Bind the render target view and depth/stencil view to the pipeline.

	md3dImmediateContext->OMSetRenderTargets(1, mRenderTargetView.GetAddressOf(), mDepthStencilView.Get());


	// Set the viewport transform.

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	md3dImmediateContext->RSSetViewports(1, &mScreenViewport);

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void Renderer::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Renderer::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = -XMConvertToRadians(
			0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = -XMConvertToRadians(
			0.25f * static_cast<float>(y - mLastMousePos.y));
		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;
		// Restrict the angle mPhi.
		mPhi = Clamp(mPhi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = -0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = -0.005f * static_cast<float>(y - mLastMousePos.y);
		// Update the camera radius based on input.
		mRadius += dx - dy;
		// Restrict the radius.
		mRadius = Clamp(mRadius, 3.0f, 15.0f);
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

float Renderer::AspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}