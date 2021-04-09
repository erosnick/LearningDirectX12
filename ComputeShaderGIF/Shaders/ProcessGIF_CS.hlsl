cbuffer GIFFrameParam : register(b0)
{
	float4	backgroundColor;		
	uint2	leftTop;
	uint2	size;
	uint	frame;
	uint	disposal;
};

// 帧纹理，尺寸就是当前帧画面的大小，注意这个往往不是整个GIF的大小
Texture2D			gifFrame : register(t0);

// 最终代表整个GIF画板纹理，大小就是整个GIF的大小
RWTexture2D<float4> paint	: register(u0);

[numthreads(1, 1, 1)]
void processGIF( uint3 threadID : SV_DispatchThreadID)
{
	if ( 0 == frame )
	{// 第一帧时用背景色整个绘制一下先，相当于一个Clear操作
		paint[threadID.xy] = backgroundColor;
	}

	// 注意画板的像素坐标需要偏移，画板坐标是：leftTop.xy + threadID.xy
	// leftTop.xy就是当前帧相对于整个画板左上角的偏移值
	// threadID.xy就是当前帧中对应要绘制的像素点坐标

	if ( 0 == disposal )
	{	//DM_NONE 不清理背景
		paint[leftTop.xy + threadID.xy]
			= gifFrame[threadID.xy].w ? gifFrame[threadID.xy] : paint[leftTop.xy + threadID.xy];
	}
	else if (1 == disposal)
	{	//DM_UNDEFINED 直接在原来画面基础上进行Alpha混色绘制
		// 注意gifFrame[threadID.xy].w只是简单的0或1值，所以没必要进行真正的Alpha混色计算
		// 但这里也没有使用IF Else判断，而是用了开销更小的?:三元表达式来进行计算
		paint[ leftTop.xy + threadID.xy ] 
			= gifFrame[threadID.xy].w ? gifFrame[threadID.xy] : paint[leftTop.xy + threadID.xy];
		// paint[ leftTop.xy + threadID.xy ]  =  gifFrame[threadID.xy];// : paint[leftTop.xy + threadID.xy];
	}
	else if ( 2 == disposal )
	{// DM_BACKGROUND 用背景色填充画板，然后绘制，相当于用背景色进行Alpha混色，过程同上
		paint[leftTop.xy + threadID.xy] = gifFrame[threadID.xy].w ? gifFrame[threadID.xy] : backgroundColor;
		//paint[leftTop.xy + threadID.xy] = backgroundColor;
	}
	else if( 3 == disposal )
	{// DM_PREVIOUS 保留前一帧
		paint[ leftTop.xy + threadID.xy ] = paint[threadID.xy + leftTop.xy];
	}
	else
	{// Disposal 是其它任何值时，都采用与背景Alpha混合的操作
		paint[leftTop.xy + threadID.xy]
			= gifFrame[threadID.xy].w ? gifFrame[threadID.xy] : paint[leftTop.xy + threadID.xy];
	}
}