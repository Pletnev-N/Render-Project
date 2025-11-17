#include <Material.h>
#include <DirectXTex\DirectXTex.h>

Material::Material()
    : mAmbient(0.0f, 0.0f, 0.0f, 0.0f),
    mDiffuse(0.0f, 0.0f, 0.0f, 0.0f),
    mSpecular(0.0f, 0.0f, 0.0f, 0.0f),
    mColorMap(nullptr),
    mNormalMap(nullptr),
    mColorMapSRV(nullptr),
    mNormalMapSRV(nullptr)
{
}

Material::~Material()
{
}

HRESULT Material::LoadTextures(ComPtr<ID3D11Device> device, std::wstring colorMapFile, std::wstring normalMapFile)
{
    HRESULT hr = LoadTGATexture(device, colorMapFile, mColorMapSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = LoadTGATexture(device, normalMapFile, mNormalMapSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;
    
    return S_OK;
}

void Material::AttachToShaders(ComPtr<ID3D11DeviceContext> deviceContext)
{
    deviceContext->PSSetShaderResources(0, 1, mColorMapSRV.GetAddressOf());
    deviceContext->PSSetShaderResources(1, 1, mNormalMapSRV.GetAddressOf());
}

HRESULT Material::LoadTGATexture(ComPtr<ID3D11Device> device, std::wstring file, ID3D11ShaderResourceView** textureView)
{
    DirectX::ScratchImage image;
    HRESULT hr = LoadFromTGAFile(file.c_str(), nullptr, image);
    if (FAILED(hr))
        return hr;

    const DirectX::Image* img = image.GetImage(0, 0, 0);
    if (!img)
        return E_FAIL;

    ID3D11Resource* texResource;
    hr = CreateTexture(device.Get(), image.GetImages(), image.GetImageCount(), image.GetMetadata(), &texResource);
    if (FAILED(hr))
        return hr;

    hr = CreateShaderResourceView(device.Get(), img, 1, image.GetMetadata(), textureView);
    if (FAILED(hr))
        return hr;

    return S_OK;
    
}