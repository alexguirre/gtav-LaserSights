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

struct VS_LaserBeam_Input
{
	float3 position : POSITION;
    float3 color : COLOR;
	float2 texcoord : TEXCOORD; // x=[0.0, BeamRange], y=[0.0, 1.0]
};

struct VS_LaserBeam_Output
{
    float4 position : SV_Position;
    float3 color : COLOR;
	float2 texcoord : TEXCOORD;
};

VS_LaserBeam_Output VS_LaserBeam(VS_LaserBeam_Input i)
{
    VS_LaserBeam_Output o = (VS_LaserBeam_Output)0;
    
    o.position = mul(float4(i.position, 1.0), gWorldViewProj);
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
    float2 uv = input.texcoord;
    uv.y = uv.y - 0.5;
    float t = abs(sin(gTime+uv.x))*0.15+0.9;
    float v = smoothstep(0.85*t, 0.0, abs(uv.y));
    float a = (my_noise(float3(gTime*0.1*uv*0.5, gTime+uv.x))+1.0)*0.25+0.005;
    float3 temp = input.color.rgb * v * a;


    float2 noiseUV = mul(gViewInverse, float4(input.position.xyz, 1.0)).xy * 0.005;
    float noise = tex2D(LaserNoise, noiseUV).a;

    // return float4(temp, v*0.0125*noise);

    // float2 noiseUV = mul(gViewInverse, float4(input.position.xyz, 1.0)).xy * 0.015;
    
    float4 timers = float4(gTime, gTime, gTime, gTime);
    float4 r0, r1;
    float4 rtdim = float4(0.00052, 0.00093, 0.0, 0.0);
    r0.x = rtdim.x / rtdim.y;
    r1.yz = rtdim.xy * input.position.xy;
    r1.x = r1.y / r0.x;
    r0.xyzw = (gTime * 0.25) * float4(0.140000001,0.0299999993,0.0799999982,-0.0900000036) + r1.xzxz;
    r0.zw = float2(0.200000003,0.200000003) + r0.zw;
    float dustNoise = tex2D(LaserNoise, r0.zw*0.15).a;
    r0.x = tex2D(LaserNoise, r0.xy*1.15).x;
    r0.y = tex2D(LaserNoise, r0.zw*1.15).x;
    r0.x = r0.x * r0.y;
    r0.y = 0.5;// saturate(0.5 * 0.0299999993 + 1);
    r0.x = r0.y * r0.x;
    r0.x = r0.x * 15 + 0.75;
    r0.x *= dustNoise;
    return float4(temp, v*0.0125*noise*r0.x);
}

technique LaserBeam
{
    pass
    {
        VertexShader = VS_LaserBeam;
        PixelShader = PS_LaserBeam;
    }
}
