cbuffer cbPerFrame : register(b0)
{
    float4x4 gWorldViewProj;
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4 gCamPos;
};

struct DirLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Direction;
    float Pad;
};

cbuffer cbLightsBuffer : register(b1)
{
    DirLight gDirLight;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 PosW : POSITION;
    float3 NormalW : NORMAL;
    float4 Color : COLOR;
};

struct Material
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular; // w = SpecPower
};


void ComputeDirectionalLight(Material mat,
float3 normal, float3 toEye,
out float4 ambient,
out float4 diffuse,
out float4 spec)
{
    // Initialize outputs.
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -gDirLight.Direction;
    // Add ambient term.
    ambient = mat.Ambient * gDirLight.Ambient;
    // Add diffuse and specular term, provided the surface is in
    // the line of site of the light.
    float diffuseFactor = dot(lightVec, normal);
    // Flatten to avoid dynamic branching.

    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), mat.Specular.w);
        diffuse = diffuseFactor * mat.Diffuse * gDirLight.Diffuse;
        spec = specFactor * mat.Specular * gDirLight.Specular;
    }
}

float4 main(VertexOut pin) : SV_Target
{
    float4 ambient, diffuse, spec;
    
    float3 toEye = (float3) gCamPos - (float3) pin.PosW;
    float distToEye = length(toEye);
    toEye /= distToEye;

    Material mat;
    mat.Ambient = float4(0.5f, 0.5f, 0.5f, 1.0f),
    mat.Diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f),
    mat.Specular = float4(0.5f, 0.5f, 0.5f, 3.0f);
    
    ComputeDirectionalLight(mat, normalize(pin.NormalW),
        toEye, ambient, diffuse, spec
    );

    float4 litColor = ambient + diffuse + spec;
    litColor.a = 1.0f;
    
    return litColor; //pin.Color;
}