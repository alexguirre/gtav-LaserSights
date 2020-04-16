#include <rage_shared.fx>

cbuffer LaserParam : register(b10)
{
	float gTime;
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
	float v = smoothstep(0.5*t, 0.0, abs(uv.y));
    float a = (my_noise(float3(gTime*0.1*uv*0.5, gTime+uv.x))+1.0)*0.25+0.005;
    return float4(input.color.r*v*a, input.color.g*v*a, input.color.b*v*a, 0.0);
}

technique LaserBeam
{
    pass
    {
        VertexShader = VS_LaserBeam;
        PixelShader = PS_LaserBeam;
    }
}
