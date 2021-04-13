//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 worldViewProjection; 
};

struct VertexIn
{
	float4 position  : POSITION;
    float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct VertexOut
{
	float4 position  : SV_POSITION;
    float4 color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D texture : register(t1);
SamplerState textureSampler : register(s0);

VertexOut VS(VertexIn input)
{
	VertexOut output;
	
	// Transform to homogeneous clip space.
	output.position = mul(input.position, worldViewProjection);
	// output.position = input.position;
	
	// Just pass vertex color into the pixel shader.
	// output.position = input.position;
    output.color = input.color;
	output.uv = input.uv;
    
    return output;
}

float4 PS(VertexOut input) : SV_Target
{
    return texture.Sample(textureSampler, input.uv);
	// return input.color;
}


