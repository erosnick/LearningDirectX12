cbuffer GIFFrameParam : register(b0)
{
	float4	backgroundColor;		
	uint2	leftTop;
	uint2	size;
	uint	frame;
	uint	disposal;
};

// ֡�����ߴ���ǵ�ǰ֡����Ĵ�С��ע�����������������GIF�Ĵ�С
Texture2D			gifFrame : register(t0);

// ���մ�������GIF����������С��������GIF�Ĵ�С
RWTexture2D<float4> paint	: register(u0);

[numthreads(1, 1, 1)]
void processGIF( uint3 threadID : SV_DispatchThreadID)
{
	if ( 0 == frame )
	{// ��һ֡ʱ�ñ���ɫ��������һ���ȣ��൱��һ��Clear����
		paint[threadID.xy] = backgroundColor;
	}

	// ע�⻭�������������Ҫƫ�ƣ����������ǣ�leftTop.xy + threadID.xy
	// leftTop.xy���ǵ�ǰ֡����������������Ͻǵ�ƫ��ֵ
	// threadID.xy���ǵ�ǰ֡�ж�ӦҪ���Ƶ����ص�����

	if ( 0 == disposal )
	{	//DM_NONE ��������
		paint[leftTop.xy + threadID.xy]
			= gifFrame[threadID.xy].w ? gifFrame[threadID.xy] : paint[leftTop.xy + threadID.xy];
	}
	else if (1 == disposal)
	{	//DM_UNDEFINED ֱ����ԭ����������Ͻ���Alpha��ɫ����
		// ע��gifFrame[threadID.xy].wֻ�Ǽ򵥵�0��1ֵ������û��Ҫ����������Alpha��ɫ����
		// ������Ҳû��ʹ��IF Else�жϣ��������˿�����С��?:��Ԫ���ʽ�����м���
		paint[ leftTop.xy + threadID.xy ] 
			= gifFrame[threadID.xy].w ? gifFrame[threadID.xy] : paint[leftTop.xy + threadID.xy];
		// paint[ leftTop.xy + threadID.xy ]  =  gifFrame[threadID.xy];// : paint[leftTop.xy + threadID.xy];
	}
	else if ( 2 == disposal )
	{// DM_BACKGROUND �ñ���ɫ��仭�壬Ȼ����ƣ��൱���ñ���ɫ����Alpha��ɫ������ͬ��
		paint[leftTop.xy + threadID.xy] = gifFrame[threadID.xy].w ? gifFrame[threadID.xy] : backgroundColor;
		//paint[leftTop.xy + threadID.xy] = backgroundColor;
	}
	else if( 3 == disposal )
	{// DM_PREVIOUS ����ǰһ֡
		paint[ leftTop.xy + threadID.xy ] = paint[threadID.xy + leftTop.xy];
	}
	else
	{// Disposal �������κ�ֵʱ���������뱳��Alpha��ϵĲ���
		paint[leftTop.xy + threadID.xy]
			= gifFrame[threadID.xy].w ? gifFrame[threadID.xy] : paint[leftTop.xy + threadID.xy];
	}
}