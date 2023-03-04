#include <rage_shared.fx>

cbuffer LaserParam : register(b10)
{
	float gTime;
};

sampler2D LaserNoise : register(s10) : register(t10)
{
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler2D DepthBuffer : register(s11) : register(t11)
{
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler2D DepthBufferPreAlpha : register(s12) : register(t12)
{
    AddressU = WRAP;
    AddressV = WRAP;
};

struct VS_LaserBeam_Input
{
	float3 position : POSITION;
    float4 color : COLOR;
	float2 texcoord : TEXCOORD; // x=[0.0, BeamRange], y=[0.0, 1.0]
};

struct VS_LaserBeam_Output
{
    float4 position : SV_Position;
    float4 view_position : TEXCOORD0;
    float4 color : COLOR;
	float2 texcoord : TEXCOORD1;
};

VS_LaserBeam_Output VS_LaserBeam(VS_LaserBeam_Input i)
{
    VS_LaserBeam_Output o = (VS_LaserBeam_Output)0;
    
    o.position = mul(float4(i.position, 1.0), gWorldViewProj);
    o.view_position = mul(float4(i.position, 1.0), gWorldView);
    o.color = i.color;
    o.texcoord = i.texcoord;
	return o;
}

// hash based 3d value noise
// function taken from https://www.shadertoy.com/view/XslGRr
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
float hash( float n )
{
    return frac(sin(n)*43758.5453);
}

float my_noise( float3 x )
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    float3 p = floor(x);
    float3 f = frac(x);

    f       = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;

    return lerp(lerp(lerp( hash(n+0.0), hash(n+1.0),f.x),
                   lerp( hash(n+57.0), hash(n+58.0),f.x),f.y),
               lerp(lerp( hash(n+113.0), hash(n+114.0),f.x),
                   lerp( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}


float4 ApplyDustParticles(VS_LaserBeam_Output input, float4 output, float4 r1, float v)
{
    float particleSpeed = 0.45;

    float4 r0 = (gTime * particleSpeed) * float4(0.140000001,0.0299999993,0.0799999982,-0.0900000036) + r1.xzxz;
    r0.zw = float2(0.200000003,0.200000003) + r0.zw;
    float4 r0_reversed = (-gTime * particleSpeed) * float4(0.140000001,0.0299999993,0.0799999982,-0.0900000036) + r1.xzxz;
    r0_reversed.zw = float2(0.200000003,0.200000003) + r0_reversed.zw;
    float dustNoise = tex2D(LaserNoise, r0.zw*0.15).g;

    float mult = 1.5;
    float2 noiseUV1 = r0.xy*mult,
           noiseUV2 = r0.zw*mult,
           noiseUV3 = r0_reversed.xy*mult,
           noiseUV4 = r0_reversed.zw*mult;

    float noise1 = tex2D(LaserNoise, noiseUV1).r;
    float noise2 = tex2D(LaserNoise, noiseUV2).r;
    float noise3 = tex2D(LaserNoise, noiseUV3).r;
    float noise4 = tex2D(LaserNoise, noiseUV4).r;

    float rx = 8 * noise1 * noise2 * noise3 * noise4;
    float ry = 0.5;// saturate(0.5 * 0.0299999993 + 1);
    rx = ry * rx;
    rx = rx * 15 + 0.75;
    rx *= dustNoise;
    return output * float4(1.0, 1.0, 1.0, /*100.0**/rx);
}

float4 ApplyDepthTest(VS_LaserBeam_Output input, float4 output)
{
    // TODO: use depth-stencil state
    float laserDepth = input.position.z;
    float depthPreAlpha = tex2D(DepthBufferPreAlpha, input.position.xy * globalScreenSize.zw).x;
    bool depthTest = (depthPreAlpha <= laserDepth); // TODO: doesn't seem to work with water?
    return output * depthTest;
}

float GetDistanceFromSource(VS_LaserBeam_Output input)
{
    return input.texcoord.x;
}

//static const float cBeamLength = 100.0f; // TODO: unhardcode beam length
//float GetDistanceFromSourceNormalized(VS_LaserBeam_Output input)
//{
//    return GetDistanceFromSource(input) / cBeamLength;
//}

float4 PS_LaserBeam(VS_LaserBeam_Output input) : SV_Target
{
    float4 r0, r1;
    float4 rtdim = float4(globalScreenSize.z, globalScreenSize.w, 0.0, 0.0);
    float aspectRatio = rtdim.x / rtdim.y;
    r1.yz = rtdim.xy * input.position.xy;
    r1.x = r1.y / aspectRatio;
    
    float2 uv = input.texcoord;
    uv.y = uv.y - 0.5;
    float t = abs(sin(gTime+r1.x))*0.15+0.9;
    float v = smoothstep(0.8*t, 0.0, abs(uv.y));
    float a = saturate(tex2D(LaserNoise, gTime*0.1/**r1.xz*5*/).g)+0.05;



    float4 worldPos = mul(gViewInverse, input.view_position);
    float2 worldNoiseUV = gTime * 0.001 * float2((worldPos.x + worldPos.y + worldPos.z) * 0.25, (worldPos.x - worldPos.y - worldPos.z) * 0.25);
    float worldNoise = tex2D(LaserNoise, worldNoiseUV).g;

    float4 output = float4(input.color.rgb * v, v * 0.0125 * worldNoise);

    output = ApplyDustParticles(input, output, r1, v);

    output = ApplyDepthTest(input, output);
    output *= input.color.a;
    return output;
}

technique LaserBeam
{
    pass
    {
        VertexShader = VS_LaserBeam;
        PixelShader = PS_LaserBeam;
    }
}
