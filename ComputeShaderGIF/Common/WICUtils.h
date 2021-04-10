#pragma once

#include <dxgi1_6.h>
#include <wincodec.h>
#include "d3dUtil.h"
#include "lodepng.h"

struct WICTranslate
{
	GUID wic;
	DXGI_FORMAT format;
};
 
static WICTranslate g_WICFormats[] =
{//WIC格式与DXGI像素格式的对应表，该表中的格式为被支持的格式
	{ GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },
 
	{ GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
	{ GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },
 
	{ GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
	{ GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
	{ GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1
 
	{ GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
	{ GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },
 
	{ GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
	{ GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },
 
	{ GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
	{ GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
	{ GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
	{ GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },
 
	{ GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },
};
 
// WIC 像素格式转换表.
struct WICConvert
{
	GUID source;
	GUID target;
};
 
static WICConvert g_WICConvert[] =
{
	// 目标格式一定是最接近的被支持的格式
	{ GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM
 
	{ GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
 
	{ GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM
	{ GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM
 
	{ GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT
	{ GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT
 
	{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM
 
	{ GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM
 
	{ GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
 
	{ GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
 
	{ GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
 
	{ GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
 
	{ GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
 
	{ GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
};
 
static bool GetTargetPixelFormat(const GUID* pSourceFormat, GUID* pTargetFormat)
{//查表确定兼容的最接近格式是哪个
	*pTargetFormat = *pSourceFormat;
	for (size_t i = 0; i < _countof(g_WICConvert); ++i)
	{
		if (InlineIsEqualGUID(g_WICConvert[i].source, *pSourceFormat))
		{
			*pTargetFormat = g_WICConvert[i].target;
			return true;
		}
	}
	return false;
}
 
static DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID* pPixelFormat)
{//查表确定最终对应的DXGI格式是哪一个
	for (size_t i = 0; i < _countof(g_WICFormats); ++i)
	{
		if (InlineIsEqualGUID(g_WICFormats[i].wic, *pPixelFormat))
		{
			return g_WICFormats[i].format;
		}
	}
	return DXGI_FORMAT_UNKNOWN;
}

// get the dxgi format equivilent of a wic format
static DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
    if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

    else return DXGI_FORMAT_UNKNOWN;
}

// get a dxgi compatible wic format from another wic format
static WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID) {
    if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

    else return GUID_WICPixelFormatDontCare;
}

static // get the number of bits per pixel for a dxgi format
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat) {
    if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

    else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
    else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
	return 0;
}

enum DisposalMethod
{
	DM_UNDEFINED = 0,
	DM_NONE = 1,
	DM_BACKGROUND = 2,
	DM_PREVIOUS = 3
};

struct GIF {
	ComPtr<IWICBitmapDecoder> decoder;
	WICColor backgroundColor;
	uint32_t width;
	uint32_t height;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t totalLoopCount;
	uint32_t frameDelay;
	uint32_t frameCount;
	uint32_t currentFrame;
	uint32_t loopNumber;
	bool hasLoop;
};

struct RectFloat {
	float left;
	float top;
	float right;
	float bottom;
};

struct GIFFrame {
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICBitmapSource> 	  bmp;
	DXGI_FORMAT					  textureFormat;
	uint32_t					  disposal;
	uint32_t 					  delay;
	uint32_t					  leftTop[2];
	uint32_t					  size[2];
};

bool loadGIF(wchar_t* fileName, IWICImagingFactory* factory, GIF& gif, const WICColor& defaultBackgroundColor = 0u) {
	PROPVARIANT propValue = {};
	unsigned char backgroundColorIndex = 0;
	WICColor paletteColors[256] = {};
	uint32_t colorsCopied = 0;
	IWICPalette* palette = nullptr;
	ComPtr<IWICMetadataQueryReader> metaData = nullptr;

	PropVariantInit(&propValue);

    ThrowIfFailed(factory->CreateDecoderFromFilename(
        fileName,		                 // 文件名
        nullptr,                         // 不指定解码器, 使用默认
        GENERIC_READ,                    // 访问权限
        WICDecodeMetadataCacheOnDemand,  // 若需要就缓冲数据
        &gif.decoder                     	 // 解码器对象
        ));

	gif.decoder->GetFrameCount(&gif.frameCount);

	ThrowIfFailed(gif.decoder->GetMetadataQueryReader(metaData.GetAddressOf()));

	if (SUCCEEDED(metaData->GetMetadataByName(L"/logscrdesc/GlobalColorTableFlag", &propValue)) 
	&& VT_BOOL == propValue.vt 
	&& propValue.boolVal) {
		// If there is a background color, read the background color
		PropVariantClear(&propValue);

		if (SUCCEEDED(metaData->GetMetadataByName(L"/logscrdesc/BackgroundColorIndex", &propValue))) {
			if (VT_UI1 == propValue.vt) {
				backgroundColorIndex = propValue.bVal;
			}

			ThrowIfFailed(factory->CreatePalette(&palette));
			ThrowIfFailed(gif.decoder->CopyPalette(palette));
			ThrowIfFailed(palette->GetColors(_countof(paletteColors), paletteColors, &colorsCopied));

			// Check whether background color is outside range
			if (backgroundColorIndex <= colorsCopied) {
				gif.backgroundColor = paletteColors[backgroundColorIndex];
			}
		}
	}
	else {
		gif.backgroundColor = defaultBackgroundColor;
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metaData->GetMetadataByName(L"/logscrdesc/Width", &propValue))
		&& VT_UI2 == propValue.vt) {
			gif.width = propValue.uintVal;
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metaData->GetMetadataByName(L"/logscrdesc/Height", &propValue))
		&& VT_UI2 == propValue.vt) {
			gif.height = propValue.uintVal;
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metaData->GetMetadataByName(L"/logscrdesc/PixelAspectRatio", &propValue))
	&& VT_UI1 == propValue.vt) {
		uint32_t uintPixelAspectRatio = propValue.bVal;

		if (uintPixelAspectRatio != 0) {
			float pixelAspectRatio = (uintPixelAspectRatio + 15.0f) / 64.0f;

			if (pixelAspectRatio > 1.0f) {
				gif.pixelWidth = gif.width;
				gif.pixelHeight = static_cast<uint32_t>(gif.height / pixelAspectRatio);
			}
			else {
				gif.pixelWidth = static_cast<uint32_t>(gif.width * pixelAspectRatio);
				gif.pixelHeight = gif.height;
			}
		}
		else {
			gif.pixelWidth = gif.width;
			gif.pixelHeight = gif.height;
		}
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metaData->GetMetadataByName(L"/appext/application", &propValue))
		&& propValue.vt == (VT_UI1 | VT_VECTOR)
		&& propValue.caub.cElems == 11
		&& (!memcmp(propValue.caub.pElems, "NETSCAPE2.0", propValue.caub.cElems)
			||
			!memcmp(propValue.caub.pElems, "ANIMEXTS1.0", propValue.caub.cElems))) {

		PropVariantClear(&propValue);

		if (SUCCEEDED(metaData->GetMetadataByName(L"/appext/data", &propValue))
			&& propValue.vt == (VT_UI1 | VT_VECTOR)
			&& propValue.caub.cElems >=4
			&& propValue.caub.pElems[0] > 0
			&& propValue.caub.pElems[1] == 1) {
			
			gif.totalLoopCount = MAKEWORD(propValue.caub.pElems[2], propValue.caub.pElems[3]);

			if (gif.totalLoopCount != 0) {
				gif.hasLoop = true;
			}
			else {
				gif.hasLoop = false;
			}
		}
	}

	return true;
}

static bool loadGIFFrame(IWICImagingFactory* factory, GIF& gif, GIFFrame& gifFrame) {
	WICPixelFormatGUID  guidRawFormat = {};
	WICPixelFormatGUID	guidTargetFormat = {};
	ComPtr<IWICFormatConverter>			converter;
	ComPtr<IWICMetadataQueryReader>		metadata;

	gifFrame.textureFormat = DXGI_FORMAT_UNKNOWN;

	// // 解码指定的帧
	ThrowIfFailed(gif.decoder->GetFrame(gif.currentFrame, gifFrame.frame.GetAddressOf()));

	// 获取WIC图片格式
	ThrowIfFailed(gifFrame.frame->GetPixelFormat(&guidRawFormat));

	gifFrame.textureFormat = GetDXGIFormatFromPixelFormat(&guidRawFormat);

	bool imageConverted = false;

	if (gifFrame.textureFormat == DXGI_FORMAT_UNKNOWN) {
		guidTargetFormat = GetConvertToWICFormat(guidRawFormat);

		if (guidTargetFormat == GUID_WICPixelFormatDontCare) {
			ThrowIfFailed(E_FAIL);
		}

		gifFrame.textureFormat = GetDXGIFormatFromWICFormat(guidTargetFormat);

		ThrowIfFailed(factory->CreateFormatConverter(converter.GetAddressOf()));

		BOOL canConvert = FALSE;
		ThrowIfFailed(converter->CanConvert(guidRawFormat, guidTargetFormat, &canConvert));

		ThrowIfFailed(converter->Initialize(gifFrame.frame.Get(), guidTargetFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom));

		imageConverted = true;
	}

	if (imageConverted) {
		ThrowIfFailed(converter.As(&gifFrame.bmp));
	}
	else {
		ThrowIfFailed(gifFrame.frame.As(&gifFrame.bmp));
	}

	ThrowIfFailed(gifFrame.frame->GetMetadataQueryReader(metadata.GetAddressOf()));

	PROPVARIANT propValue;

	PropVariantClear(&propValue);

	// Get the frame deplay time
	if (SUCCEEDED(metadata->GetMetadataByName(L"/grctlext/Delay", &propValue))) {
		if (VT_UI2 == propValue.vt) {
			// Convert the delay retrieved in 10 ms uints to a delay in 1 ms units
			ThrowIfFailed(UIntMult(propValue.uiVal, 10, &gif.frameDelay));
			if (gifFrame.delay == 0) {
				gifFrame.delay = 100;
			}
		}
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metadata->GetMetadataByName(L"/imgdesc/Left", &propValue))) {
		if (VT_UI2 == propValue.vt) {
			gifFrame.leftTop[0] = static_cast<uint32_t>(propValue.uiVal);
		}
	}

	PropVariantClear(&propValue);
	
	if (SUCCEEDED(metadata->GetMetadataByName(L"/imgdesc/Top", &propValue))) {
		if (VT_UI2 == propValue.vt) {
			gifFrame.leftTop[1] = static_cast<uint32_t>(propValue.uiVal);
		}
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metadata->GetMetadataByName(L"/imgdesc/Width", &propValue))) {
		if (VT_UI2 == propValue.vt) {
			gifFrame.size[0] = static_cast<uint32_t>(propValue.uiVal);
		}
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metadata->GetMetadataByName(L"/imgdesc/Height", &propValue))) {
		if (VT_UI2 == propValue.vt) {
			gifFrame.size[1] = static_cast<uint32_t>(propValue.uiVal);
		}
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metadata->GetMetadataByName(L"/grctlext/Disposal", &propValue))) {
		if (VT_UI1 == propValue.vt) {
			gifFrame.disposal = propValue.uiVal;
		}
	}
	else {
		gifFrame.disposal = DM_UNDEFINED;
	}
	
	return true;
}

static bool uploadGIFFrame(ID3D12Device4* device, ID3D12GraphicsCommandList* commandList, IWICImagingFactory* factory, GIFFrame& gifFrame, ID3D12Resource** gifFrameResource, ID3D12Resource** gifFrameUpload) {
	uint32_t textureWidth = 0;
	uint32_t textureHeight = 0;
	uint32_t bpp = 0;

	void* uploadBufferData = nullptr;

	ThrowIfFailed(gifFrame.bmp->GetSize(&textureWidth, &textureHeight));

	// 从属性解析出的帧画面大小与转换后的画面大小不一致
	if (textureWidth != gifFrame.size[0] || textureHeight != gifFrame.size[1]) {
		ThrowIfFailed(E_FAIL);
	}

	// 获取图片像素的位大小BPP(Bits Per Pixel)信息，用以计算图片行数据的真实大小(单位：字节)
	ComPtr<IWICComponentInfo> componentInfo;
	WICPixelFormatGUID guidTargetFormat = {};

	gifFrame.bmp->GetPixelFormat(&guidTargetFormat);
	ThrowIfFailed(factory->CreateComponentInfo(guidTargetFormat, componentInfo.GetAddressOf()));

	WICComponentType type;
	ThrowIfFailed(componentInfo->GetComponentType(&type));

	if (type != WICPixelFormat) {
		ThrowIfFailed(E_FAIL);
	}

	ComPtr<IWICPixelFormatInfo> pixelInfo;
	ThrowIfFailed(componentInfo.As(&pixelInfo));

	ThrowIfFailed(pixelInfo->GetBitsPerPixel(&bpp));

	uint32_t textureRowPitch = (textureWidth * bpp + 7) / 8;

	// 根据图片信息，填充2D纹理资源的信息结构体
	D3D12_RESOURCE_DESC textureDesc = {};

	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.MipLevels = 1;
	textureDesc.Format = gifFrame.textureFormat;
	textureDesc.Width = textureWidth;
	textureDesc.Height = textureHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;

	D3D12_HEAP_PROPERTIES heapProperties = {D3D12_HEAP_TYPE_DEFAULT};

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(gifFrameResource)));

	textureDesc = (*gifFrameResource)->GetDesc();

	// 这个方法是很有用的一个方法，从Buffer复制数据到纹理，
	// 或者从纹理复制数据到Buffer，这个方法都会告诉我们很多
	// 相关的辅助的信息其实主要的信息就是对于纹理来说行的大小，对齐方式等信息
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayouts;
	uint64_t textureRowSize = 0;
	uint64_t uploadBufferSize = 0;
	uint32_t textureRowNum = 0;
	device->GetCopyableFootprints(&textureDesc,
		0,
		1,
		0,
		&textureLayouts,
		&textureRowNum,
		&textureRowSize,
		&uploadBufferSize);

	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC uploadBufferDesc = {};

	uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// 资源维度为Buffer(1D)
	uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// 相当于将二维数组平摊成一维数组
	uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	uploadBufferDesc.Width = uploadBufferSize;
	uploadBufferDesc.Height = 1;
	uploadBufferDesc.DepthOrArraySize = 1;
	uploadBufferDesc.MipLevels = 1;
	uploadBufferDesc.SampleDesc.Count = 1;
	uploadBufferDesc.SampleDesc.Quality = 0;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(gifFrameUpload)));

	// 从当前进程的堆上分配内存
	void* gifFrameData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, uploadBufferSize);
	if (nullptr == gifFrameData) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	// 从图片中读取数据
	ThrowIfFailed(gifFrame.bmp->CopyPixels(nullptr,
		textureRowPitch,
		static_cast<uint32_t>(textureRowPitch * textureHeight),		//注意这里才是图片数据真实的大小，这个值通常小于缓冲的大小
		reinterpret_cast<byte*>(gifFrameData)));

	//--------------------------------------------------------------------------------------------------------------
	// 第一个Copy命令，从内存复制到上传堆（共享内存）中
	//因为上传堆实际就是CPU传递数据到GPU的中介
	//所以我们可以使用熟悉的Map方法将它先映射到CPU内存地址中
	//然后我们按行将数据复制到上传堆中
	//需要注意的是之所以按行拷贝是因为GPU资源的行大小
	//与实际图片的行大小是有差异的,二者的内存边界对齐要求是不一样的
	ThrowIfFailed((*gifFrameUpload)->Map(0, nullptr, reinterpret_cast<void**>(&uploadBufferData)));

	byte* destSlice = reinterpret_cast<byte*>(uploadBufferData) + textureLayouts.Offset;
	const byte* SrcSlice = reinterpret_cast<byte*>(gifFrameData);

	for (uint32_t row = 0; row < textureRowNum; row++) {
		memcpy_s(destSlice + static_cast<size_t>(textureLayouts.Footprint.RowPitch) * row, textureRowPitch,
				 SrcSlice + static_cast<size_t>(textureRowPitch) * row, textureRowPitch);
	}

	(*gifFrameUpload)->Unmap(0, nullptr);

	::HeapFree(::GetProcessHeap(), 0, gifFrameData);

	//--------------------------------------------------------------------------------------------------------------

	//--------------------------------------------------------------------------------------------------------------
	// 第二个Copy命令，由GPU的Copy Engine完成，从上传堆（共享内存）复制纹理数据到默认堆（显存）中
	D3D12_TEXTURE_COPY_LOCATION dest = {};
	dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dest.pResource = *gifFrameResource;
	dest.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src = {};
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.pResource = *gifFrameUpload;
	src.PlacedFootprint = textureLayouts;

	//第二个Copy动作由GPU来完成，将Texture数据从上传堆（共享内存）复制到默认堆（显存）中
	commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);
	//--------------------------------------------------------------------------------------------------------------

	// 设置一个资源屏障，默认堆上的纹理数据复制完成后，将纹理的状态从复制目标转换为Shader资源
	// 这里需要注意的是在这个例子中，状态转换后的纹理只用于Pixel Shader访问，其他的Shader没法访问
	D3D12_RESOURCE_BARRIER resourceBarrier = {};

	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceBarrier.Transition.pResource = *gifFrameResource;
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &resourceBarrier);

	return true;
}
	
static  std::vector<ComPtr<IWICBitmapSource>> WICLoadImage(const std::wstring& fileName, uint32_t& textureWidth, uint32_t& textureHeight, uint32_t& bpp, uint32_t& rowPitch, DXGI_FORMAT& textureFormat) {

	ComPtr<IWICImagingFactory>		factory = nullptr;
	ComPtr<IWICBitmapDecoder>		decoder = nullptr;
	ComPtr<IWICBitmapFrameDecode>	frame = nullptr;

	std::vector<ComPtr<IWICBitmapSource>> bmps;

	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf())));

    ThrowIfFailed(factory->CreateDecoderFromFilename(
        fileName.c_str(),                // 文件名
        nullptr,                         // 不指定解码器, 使用默认
        GENERIC_READ,                    // 访问权限
        WICDecodeMetadataCacheOnDemand,  // 若需要就缓冲数据
        &decoder                     	 // 解码器对象
        ));
	
	uint32_t frameCount = 0;

	decoder->GetFrameCount(&frameCount);

	for (uint32_t i = 0; i < frameCount; i++) {
		// 获取第一幅图片(因为GIF等格式文件可能会有多帧图像, 其他格式一般只有一帧图片)
		// 实际解析出来的往往是位图格式数据
		ThrowIfFailed(decoder->GetFrame(i, frame.GetAddressOf()));

		WICPixelFormatGUID pixelFormat = {};

		// 获取WIC图片格式
		ThrowIfFailed(frame->GetPixelFormat(&pixelFormat));

		ThrowIfFailed(frame->GetSize(&textureWidth, &textureHeight));

		textureFormat = GetDXGIFormatFromPixelFormat(&pixelFormat);

		bool imageConverted = false;

		ComPtr<IWICFormatConverter> converter;

		if (textureFormat == DXGI_FORMAT_UNKNOWN) {
			WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

			if (convertToPixelFormat == GUID_WICPixelFormatDontCare) {
				ThrowIfFailed(E_FAIL);
			}

			textureFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

			ThrowIfFailed(factory->CreateFormatConverter(converter.GetAddressOf()));

			BOOL canConvert = FALSE;
			ThrowIfFailed(converter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert));

			ThrowIfFailed(converter->Initialize(frame.Get(), convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom));

			imageConverted = true;
		}

		bpp = GetDXGIFormatBitsPerPixel(textureFormat);

		// GUID format = {};

		// // 通过第一道转换之后获取DXGI的等价格式
		// if (GetTargetPixelFormat(&pixelFormat, &format)) {
		// 	textureFormat = GetDXGIFormatFromPixelFormat(&format);
		// }

		// if (DXGI_FORMAT_UNKNOWN == textureFormat) {
		// 	// 不支持的图片格式
		// }

		// // 定义一个位图格式的图片数据对象接口
		ComPtr<IWICBitmapSource> bmp = nullptr;

		if (imageConverted) {
			ThrowIfFailed(converter.As(&bmp));
		}
		else {
			ThrowIfFailed(frame.As(&bmp));
		}

		// if (!InlineIsEqualGUID(pixelFormat, format)) {
		// 	// 这个判断很重要, 如果原WIC格式不是直接能转换为DXGI格式的图片时
		// 	// 我们需要做的就是转换图片格式为能够直接对应DXGI格式的形状
		// 	// 创建图片格式转换器
		// 	ComPtr<IWICFormatConverter> converter;
		// 	ThrowIfFailed(factory->CreateFormatConverter(&converter));

		// 	// 初始化一个图片转换器, 实际也就是将图片数据进行了转换
		// 	ThrowIfFailed(converter->Initialize(
		// 		frame.Get(),                 // 输入图片数据
		// 		format,                      // 指定待转换的目标格式
		// 		WICBitmapDitherTypeNone,     // 指定位图是否有调色板,现代都是真彩位图,不用调色板,所以为None
		// 		nullptr,                     // 指定调色板指针
		// 		0.0f,                        // 指定Alpha阈值
		// 		WICBitmapPaletteTypeCustom   // 调色板类型,实际没有使用,所以指定为Custom
		// 	));

		// 	// 调用QueryInterface方法获得对象的位图数据源接口
		// 	ThrowIfFailed(converter.As(&bmp));
		// }
		// else {
		// 	// 图片数据不需要转换, 直接获取其位图数据源接口
		// 	ThrowIfFailed(frame.As(&bmp));
		// }

		// // 获取图片像素的位大小的BPP(Bits Per Pixel)信息,用于计算图片行数据的真实大小(单位: 字节)
		// ComPtr<IWICComponentInfo> info;
		// ThrowIfFailed(factory->CreateComponentInfo(format, &info));

		// WICComponentType type;
		// ThrowIfFailed(info->GetComponentType(&type));

		// if (type != WICPixelFormat) {
		// 	ThrowIfFailed(E_FAIL);
		// }

		// ComPtr<IWICPixelFormatInfo> pixelInfo;
		// ThrowIfFailed(info.As(&pixelInfo));

		// // 到这里终于可以得到BPP了
		// ThrowIfFailed(pixelInfo->GetBitsPerPixel(&bpp));

		// 计算图片实际的行大小, 这里使用了一个上取整除法即(A + B - 1) / B
		// 这个算法保证了最后的结果是按8字节对齐, 也就是能够被8整除
		rowPitch = (uint64_t(textureWidth) * uint64_t(bpp) + 7) / 8;

		bmps.push_back(bmp);
	}
    
    return bmps;
}

static void saveImage(const std::string& path, byte* pixels, int width, int height)
{
	std::vector<unsigned char> pixelBuffer;

	int pixelCount = width * height;

	byte* pixelPtr = pixels;

	for (int i = 0; i < pixelCount; i++) {
		// pixelBuffer.push_back((unsigned char)pixelPtr[i * 4]);
		// pixelBuffer.push_back((unsigned char)pixelPtr[i * 4 + 1]);
		// pixelBuffer.push_back((unsigned char)pixelPtr[i * 4 + 2]);
		// pixelBuffer.push_back((unsigned char)pixelPtr[i * 4 + 3]);
		pixelBuffer.push_back((unsigned char)pixelPtr[0]);
		pixelBuffer.push_back((unsigned char)pixelPtr[1]);
		pixelBuffer.push_back((unsigned char)pixelPtr[2]);
		pixelBuffer.push_back((unsigned char)pixelPtr[3]);
		pixelPtr += 4;
	}

	//Encode the image
	unsigned error = lodepng::encode(path, pixelBuffer, width, height);

	pixelBuffer.clear();
}