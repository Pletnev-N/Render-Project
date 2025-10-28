#pragma once

#include <DirectXMath.h>
#include <windows.h>
#include <LogWriter.h>

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT4 Color;
};

struct CubeVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

struct PER_FRAME_CBUFFER
{
    DirectX::XMFLOAT4X4 mWorldViewProj;
    DirectX::XMFLOAT4X4 mWorld;
    DirectX::XMFLOAT4X4 mWorldInvTrans;
    DirectX::XMFLOAT4 CamPos;
};

struct DIRECTIONAL_LIGHT
{
    DirectX::XMFLOAT4 Ambient;
    DirectX::XMFLOAT4 Diffuse;
    DirectX::XMFLOAT4 Specular;
    DirectX::XMFLOAT3 Direction;
    float Pad;
};

struct LIGHTS_CBUFFER
{
    DIRECTIONAL_LIGHT DirLight;
};
