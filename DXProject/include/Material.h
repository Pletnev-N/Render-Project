#pragma once

#include <RenderDefs.h>
#include <d3d11.h>

class Material
{
public:
	Material();
	~Material();

	HRESULT LoadTextures(ComPtr<ID3D11Device> device, std::wstring colorMapFile, std::wstring normalMapFile);
	void AttachToShaders(ComPtr<ID3D11DeviceContext> deviceContext);

private:
	HRESULT LoadTGATexture(ComPtr<ID3D11Device> device, std::wstring file, ID3D11ShaderResourceView** textureView);

	DirectX::XMFLOAT4 mAmbient;
	DirectX::XMFLOAT4 mDiffuse;
	DirectX::XMFLOAT4 mSpecular;

    ComPtr<ID3D11Texture2D> mColorMap;
	ComPtr<ID3D11Texture2D> mNormalMap;
	ComPtr<ID3D11ShaderResourceView> mColorMapSRV;
	ComPtr<ID3D11ShaderResourceView> mNormalMapSRV;
};
