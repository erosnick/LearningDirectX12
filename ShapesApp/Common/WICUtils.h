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

struct GIF {
	WICColor backgroundColor;
	uint32_t width;
	uint32_t height;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t totalLoopCount;
	uint32_t frameDelay;
	bool hasLoop;
};

static  std::vector<ComPtr<IWICBitmapSource>> WICLoadImage(const std::wstring& fileName, uint32_t& textureWidth, uint32_t& textureHeight, uint32_t& bpp, uint32_t& rowPitch, DXGI_FORMAT& textureFormat) {

	ComPtr<IWICImagingFactory>		factory = nullptr;
	ComPtr<IWICBitmapDecoder>		decoder = nullptr;
	ComPtr<IWICBitmapFrameDecode>	frame = nullptr;
	ComPtr<IWICMetadataQueryReader> metaDataReader = nullptr;
	PROPVARIANT propValue = {};
	unsigned char backgroundColorIndex = 0;
	WICColor paletteColors[256] = {};
	uint32_t colorsCopied = 0;
	IWICPalette* palette = nullptr;
	GIF gif;

	PropVariantInit(&propValue);

    ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));

    ThrowIfFailed(factory->CreateDecoderFromFilename(
        fileName.c_str(),                // 文件名
        nullptr,                         // 不指定解码器, 使用默认
        GENERIC_READ,                    // 访问权限
        WICDecodeMetadataCacheOnDemand,  // 若需要就缓冲数据
        &decoder                     	 // 解码器对象
        ));

	uint32_t frameCount = 0;

	decoder->GetFrameCount(&frameCount);

	ThrowIfFailed(decoder->GetMetadataQueryReader(metaDataReader.GetAddressOf()));

	if (SUCCEEDED(metaDataReader->GetMetadataByName(L"/logscrdesc/GlobalColorTableFlag", &propValue)) 
	&& VT_BOOL == propValue.vt 
	&& propValue.boolVal) {
		// If there is a background color, read the background color
		PropVariantClear(&propValue);

		if (SUCCEEDED(metaDataReader->GetMetadataByName(L"/logscrdesc/BackgroundColorIndex", &propValue))) {
			if (VT_UI1 == propValue.vt) {
				backgroundColorIndex = propValue.bVal;
			}

			ThrowIfFailed(factory->CreatePalette(&palette));
			ThrowIfFailed(decoder->CopyPalette(palette));
			ThrowIfFailed(palette->GetColors(_countof(paletteColors), paletteColors, &colorsCopied));

			// Check whether background color is outside range
			if (backgroundColorIndex <= colorsCopied) {
				gif.backgroundColor = paletteColors[backgroundColorIndex];
			}
		}
	}
	else {
		gif.backgroundColor = 0xffffffff;
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metaDataReader->GetMetadataByName(L"/logscrdesc/Width", &propValue))
		&& VT_UI2 == propValue.vt) {
			gif.width = propValue.uintVal;
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metaDataReader->GetMetadataByName(L"/logscrdesc/Height", &propValue))
		&& VT_UI2 == propValue.vt) {
			gif.height = propValue.uintVal;
	}

	PropVariantClear(&propValue);

	if (SUCCEEDED(metaDataReader->GetMetadataByName(L"/logscrdesc/PixelAspectRatio", &propValue))
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

	if (SUCCEEDED(metaDataReader->GetMetadataByName(L"/appext/application", &propValue))
		&& propValue.vt == (VT_UI1 | VT_VECTOR)
		&& propValue.caub.cElems == 11
		&& (!memcmp(propValue.caub.pElems, "NETSCAPE2.0", propValue.caub.cElems)
			||
			!memcmp(propValue.caub.pElems, "ANIMEXTS1.0", propValue.caub.cElems))) {

		PropVariantClear(&propValue);

		if (SUCCEEDED(metaDataReader->GetMetadataByName(L"/appext/data", &propValue))
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

		PropVariantClear(&propValue);

		// Get the frame deplay time
		if (SUCCEEDED(metaDataReader->GetMetadataByName(L"/grctlext/Delay", &propValue))) {
			if (VT_UI2 == propValue.vt) {
				// Convert the delay retrieved in 10 ms uints to a delay in 1 ms units
				ThrowIfFailed(UIntMult(propValue.uiVal, 10, &gif.frameDelay));
			}
		}
	}

	std::vector<ComPtr<IWICBitmapSource>> bmps;

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
				ThrowIfFailed(S_FALSE);
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
		// 	ThrowIfFailed(S_FALSE);
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