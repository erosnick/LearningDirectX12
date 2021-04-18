//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 world; 
};

cbuffer cbPerObject : register(b1)
{
	float4x4 viewProjection; 
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
	float2 uv : TEXCOORD;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D texture : register(t0);
SamplerState textureSampler : register(s0);

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	float4 worldPosition = mul(float4(vin.PosL, 1.0f), world);
	vout.PosH = mul(worldPosition, viewProjection);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
	vout.uv = vin.uv;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return texture.Sample(textureSampler, pin.uv);
	// return pin.Color;
}


