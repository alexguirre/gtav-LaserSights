#include <rage_shared.fx>

// Ported from Max Payne 3's laserbeam.fxc

cbuffer LaserParam : register(b10)
{
	float2 LaserVisibilityMinMax;
};

sampler2D LaserSampler : register(s10) : register(t10)
{
    AddressU = WRAP;
    AddressV = WRAP;
};

struct VS_LaserBeam_Input
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float4 texCoord0 : TEXCOORD0;
	float4 texCoord1 : TEXCOORD1;
	float4 texCoord2 : TEXCOORD2;
};

struct VS_LaserBeam_Output
{
    float4 position : SV_Position;
    float3 texCoord0 : TEXCOORD0;
    float3 texCoord1 : TEXCOORD1;
    float2 texCoord2 : TEXCOORD2;
    float4 texCoord3 : TEXCOORD3;
    float4 texCoord4 : TEXCOORD4;
};

VS_LaserBeam_Output VS_LaserBeam(VS_LaserBeam_Input i)
{
    VS_LaserBeam_Output o = (VS_LaserBeam_Output)0;
    float4 r0;
    
    r0 = i.position.yyyy * gWorldViewProj[1].xyzw;
    r0 = i.position.xxxx * gWorldViewProj[0].xyzw + r0.xyzw;
    r0 = i.position.zzzz * gWorldViewProj[2].xyzw + r0.xyzw;
    r0 = r0.xyzw + gWorldViewProj[3].xyzw;
    o.position = r0;
    o.texCoord3.w = r0.z;
    o.texCoord0.xyz = i.position.xyz + -gViewInverse[3].xyz;
    o.texCoord1.xyz = i.normal.xyz;
    o.texCoord2.xy = i.texCoord0.xy;
    r0.x = i.position.y * gWorldView[1].z;
    r0.x = i.position.x * gWorldView[0].z + r0.x;
    r0.x = i.position.z * gWorldView[2].z + r0.x;
    r0.x = r0.x + gWorldView[3].z;
    o.texCoord3.z = -r0.x;
    o.texCoord3.xy = i.texCoord2.xy;
    o.texCoord4.xyzw = i.texCoord1.xyzw;
	return o;
}

float4 PS_LaserBeam(VS_LaserBeam_Output input) : SV_Target
{
    float4 r0, r1;
    
    r0.x = max(LaserVisibilityMinMax.x, 0.0f);
    r0.x = min(r0.x, LaserVisibilityMinMax.y);
    r0.y = -abs(input.texCoord3.y) * abs(input.texCoord3.y) + 1.0f; 
    r1 = r0.y * input.texCoord4;
    r0 = r0.x * r1;
    r1.x = dot(r0.xyz, r0.www);
    r0.xyz = r1.xxx * float3(0.5f,0.5f,0.5f) + r0.xyz;
    r1.x = saturate(input.texCoord3.w * 5.0f);
	return (r0.xyzw * r1.xxxx);
}

technique LaserBeam
{
    pass
    {
        VertexShader = VS_LaserBeam;
        PixelShader = PS_LaserBeam;

        // Rasterizer State
        FillMode = SOLID;
        CullMode = NONE;

        // Depth-Stencil State
        DepthEnable = TRUE;
        DepthWriteMask = ALL;
        DepthFunc = GREATER_EQUAL;
        StencilEnable = FALSE;
        StencilWriteMask = 0xFF;
        StencilReadMask = 0xFF;
        FrontFaceStencilFail = KEEP;
        FrontFaceStencilDepthFail = KEEP;
        FrontFaceStencilPass = REPLACE;
        FrontFaceStencilFunc = ALWAYS;

        // Blend State
        AlphaToCoverageEnable = FALSE;
        BlendEnable0 = TRUE;
        SrcBlend0 = ONE;
        DestBlend0 = INV_SRC_ALPHA;
        BlendOp0 = ADD;
        RenderTargetWriteMask0 = 0xF; 
    }
}
