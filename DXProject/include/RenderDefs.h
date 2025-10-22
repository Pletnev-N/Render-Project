#pragma once

#include <DirectXMath.h>
#include <windows.h>

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

struct VS_CONSTANT_BUFFER
{
    DirectX::XMFLOAT4X4 mWorldViewProj;
};