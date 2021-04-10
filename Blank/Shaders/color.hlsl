//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

// cbuffer cbPerObject : register(b0)
// {
// 	float4x4 gWorldViewProj; 
// };

struct VertexIn
{
	float4 position  : POSITION;
    float4 color : COLOR;
	// float2 uv : TEXCOORD;
};

struct VertexOut
{
	float4 position  : SV_POSITION;
    float4 color : COLOR;
	// float2 uv : TEXCOORD;
};

// Texture2D texture : register(t1);
// SamplerState textureSampler : register(s0);

VertexOut VS(VertexIn input)
{
	VertexOut output;
	
	// Transform to homogeneous clip space.
	// vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
	output.position = input.position;
    output.color = input.color;
	// vout.uv = vin.uv;
    
    return output;
}

float4 PS(VertexOut input) : SV_Target
{
    // return texture.Sample(textureSampler, pin.uv);
	return input.color;
}


