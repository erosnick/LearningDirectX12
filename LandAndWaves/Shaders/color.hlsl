//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer constantBufferPerObject : register(b0)
{
	float4x4 world; 
};

cbuffer constantBufferPerPass : register(b1)
{
	float4x4 viewProjection; 
	float time;
};

cbuffer constantBufferMaterial : register(b2)
{
	uint materialIndex;
}

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 color : COLOR;
	float2 uv : TEXCOORD;
};

Texture2D textures[] : register(t0);
SamplerState textureSampler : register(s0);

VertexOut VS(VertexIn input)
{
	VertexOut output;

	// float4 posW = mul(float4(vin.PosL, 1.0f), world);

	// // Transform to homogeneous clip space.
	// vout.PosH = mul(posW, viewProjection);

	// float4x4 worldViewProjection = mul(viewProjection, world);
	// vout.PosH = mul(float4(vin.PosL, 1.0f), worldViewProjection);

	matrix newWorld = {
		{ 1.0f, 0.0f, 0.0f, 0.0f},
		{ 0.0f, 1.0f, 0.0f, 0.0f}, 
		{ 0.0f, 0.0f, 1.0f, 0.0f},
	    {-2.0f, 0.0f, 0.0f, 1.0f}
	};

	// newWorld = transpose(newWorld);

	matrix view = {
		{1.0f, 0.0f, 0.001192f, 0.0f},
		{-0.000843f, 0.707107f, 0.707107f, 0.0f},
		{-0.000843f, -0.707107f, 0.707107f, 0.0f},
		{0.000036f, 0.0f, 5.0f, 1.0f}
	};

	// view = transpose(view);

	matrix projection = {
		{1.35799515f, 0.0f, 0.0f, 0.0f},
		{0.0f, 2.41421232f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 1.001f},
		{0.0f, 0.0f, -1.001f, 0.0f}
	};

	// projection = transpose(projection);

	// float4 posW = mul(newWorld, float4(vin.PosL, 1.0f));
	// float4 posV = mul(view, posW);
	// vout.PosH = mul(projection, posV);
	// float4 posW = mul(float4(vin.PosL, 1.0f), newWorld);
	// float4 posV = mul(posW, view);
	// vout.PosH = mul(posV, projection);
	// float4 posV = mul(view, float4(vin.PosL, 1.0f));
	// float4.PosH = mul(projection, posV);
	float4 posW = mul(float4(input.PosL, 1.0f), world);
	output.PosH = mul(posW, viewProjection);
	
	// Just pass vertex color into the pixel shader.
    output.color = input.color;
	output.uv = input.uv;
    
    return output;
}

float4 PS(VertexOut input) : SV_Target
{
    // return textures[0].Sample(textureSampler, pin.uv);
	return input.color;
}


