//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
struct VertexIn
{
	float4 position  : POSITION;
    float4 color : COLOR;
};

struct VertexOut
{
	float4 position  : SV_POSITION;
    float4 color : COLOR;
};

Texture2D texture : register(t1);
SamplerState textureSampler : register(s0);

VertexOut VS(VertexIn input)
{
	VertexOut output;
	
	output.position = input.position;
	
	// Just pass vertex color into the pixel shader.
    output.color = input.color;
    
    return output;
}

float4 PS(VertexOut input) : SV_Target
{
	return input.color;
}


