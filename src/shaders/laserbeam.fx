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
    float3 color : COLOR;
	float2 texcoord : TEXCOORD; // x=[0.0, BeamRange], y=[0.0, 1.0]
};

struct VS_LaserBeam_Output
{
    float4 position : SV_Position;
    float4 view_position : TEXCOORD0;
    float3 color : COLOR;
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

float4 PS_LaserBeam(VS_LaserBeam_Output input) : SV_Target
{
    float4 timers = float4(gTime, gTime, gTime, gTime);
    float4 r0, r1;
    float4 rtdim = float4(0.00052, 0.00093, 0.0, 0.0);
    r0.x = rtdim.x / rtdim.y;
    r1.yz = rtdim.xy * input.position.xy;
    r1.x = r1.y / r0.x;
    
    float2 uv = input.texcoord;
    uv.y = uv.y - 0.5;
    float t = abs(sin(gTime+r1.x))*0.15+0.9;
    float v = smoothstep(0.8*t, 0.0, abs(uv.y));
    float a = saturate(tex2D(LaserNoise, gTime*0.1/**r1.xz*5*/).g)+0.05;//(my_noise(float3(gTime*0.1*uv*0.5, gTime+uv.x))+1.0)*0.25+0.005;
    // return float4(a, 0, 0, 1);
    float3 temp = input.color.rgb * v;


    float4 worldPos = mul(gViewInverse, input.view_position);
    float2 worldNoiseUV = gTime * 0.001 * float2((worldPos.x + worldPos.y + worldPos.z) * 0.25, (worldPos.x - worldPos.y - worldPos.z) * 0.25);
    float worldNoise = tex2D(LaserNoise, worldNoiseUV).g;

    // return float4(temp, v*0.0125*noise);

    // float2 noiseUV = mul(gViewInverse, float4(input.position.xyz, 1.0)).xy * 0.015;
    
    float particleSpeed = 0.45;// + (((gTime * 0.00001) % 1.0) - 0.25);// worldNoiseUV.y * worldNoiseUV.x;

    r0.xyzw = (gTime * particleSpeed) * float4(0.140000001,0.0299999993,0.0799999982,-0.0900000036) + r1.xzxz;
    r0.zw = float2(0.200000003,0.200000003) + r0.zw;
    float4 r0_reversed = (-gTime * particleSpeed) * float4(0.140000001,0.0299999993,0.0799999982,-0.0900000036) + r1.xzxz;
    r0_reversed.zw = float2(0.200000003,0.200000003) + r0_reversed.zw;
    float dustNoise = tex2D(LaserNoise, r0.zw*0.15).g;
    
    float mult = 1.5;
    float2 noiseUV1 = r0.xy*mult, noiseUV2 = r0.zw*mult, noiseUV3 = r0_reversed.xy*mult, noiseUV4 = r0_reversed.zw*mult;

    float noise1 = tex2D(LaserNoise, noiseUV1).r;
    float noise2 = tex2D(LaserNoise, noiseUV2).r;
    float noise3 = tex2D(LaserNoise, noiseUV3).r;
    float noise4 = tex2D(LaserNoise, noiseUV4).r;

    r0.x = 8 * noise1 * noise2 * noise3 * noise4;
    r0.y = 0.5;// saturate(0.5 * 0.0299999993 + 1);
    r0.x = r0.y * r0.x;
    r0.x = r0.x * 15 + 0.75;
    r0.x *= dustNoise;
    float4 output = float4(temp, 100.0*v*0.0125*worldNoise*r0.x);

    float laserDepth = input.position.z;
    float depthPreAlpha = tex2D(DepthBufferPreAlpha, input.position.xy / float2(1920.0f, 1080.0f)).x; // TODO: unhardcode screen res
    bool depthTest = (depthPreAlpha <= laserDepth); // TODO: doesn't seem to work with water?

    return output * depthTest;
}

technique LaserBeam
{
    pass
    {
        VertexShader = VS_LaserBeam;
        PixelShader = PS_LaserBeam;
    }
}
