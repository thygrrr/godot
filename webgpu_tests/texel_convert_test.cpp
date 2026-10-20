#include "pixel_formats_webgpu.h"

#include <cassert>
#include <cstdio>

static double round_trip(WGPUTextureFormat p_format, double p_value) {
	uint32_t channels = 0;
	WGTexelType type = WG_TEXEL_FLOAT32;
	assert(wgpu_format_texel_info(p_format, channels, type));
	uint8_t bytes[4] = {};
	wg_texel_encode(type, p_value, bytes);
	return wg_texel_decode(type, bytes);
}

int main() {
	// Signed 8-bit formats keep their sign (R8 storage promotions go through these).
	assert(round_trip(WGPUTextureFormat_R8Sint, -5) == -5);
	assert(round_trip(WGPUTextureFormat_R8Snorm, -1.0) == -1.0);
	assert(round_trip(WGPUTextureFormat_R8Uint, 300) == 255);
	assert(round_trip(WGPUTextureFormat_R8Unorm, 1.0) == 1.0);

	// 16-bit normalized formats stored as half floats without texture-formats-tier1.
	uint8_t unorm16[2] = { 0xFF, 0xFF };
	assert(wg_texel_decode(WG_TEXEL_UNORM16, unorm16) == 1.0);
	uint8_t half[2] = {};
	wg_texel_encode(WG_TEXEL_FLOAT16, wg_texel_decode(WG_TEXEL_UNORM16, unorm16), half);
	assert(wg_texel_decode(WG_TEXEL_FLOAT16, half) == 1.0);
	assert(round_trip(WGPUTextureFormat_R16Snorm, -0.5) > -0.5001 && round_trip(WGPUTextureFormat_R16Snorm, -0.5) < -0.4999);

	// Channel counts follow the format, not the byte size (RG16Float is 4 bytes, 2 channels).
	uint32_t channels = 0;
	WGTexelType type = WG_TEXEL_FLOAT32;
	assert(wgpu_format_texel_info(WGPUTextureFormat_RG16Float, channels, type) && channels == 2 && type == WG_TEXEL_FLOAT16);
	assert(!wgpu_format_texel_info(WGPUTextureFormat_RGB10A2Unorm, channels, type));
	assert(wgpu_format_pixel_size(WGPUTextureFormat_RGBA16Unorm) == 8);

	std::puts("PASS: texel encode/decode keeps sign, range and channel layout");
}
