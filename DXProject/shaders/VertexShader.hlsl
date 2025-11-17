cbuffer cbPerFrame : register(b0)
{
    float4x4 gWorldViewProj;
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4 gCamPos;
};

struct VertexIn
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 TexUV : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexUV : TEXCOORD;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    //vout.PosH = float4(vin.Pos, 1.0f);
    vout.PosH = mul(float4(vin.Pos, 1.0f), gWorldViewProj);
    vout.PosW = mul(float4(vin.Pos, 1.0f), gWorld);
    vout.NormalW = mul(vin.Normal, (float3x3) gWorldInvTranspose);
    vout.TexUV = vin.TexUV;
    return vout;
}