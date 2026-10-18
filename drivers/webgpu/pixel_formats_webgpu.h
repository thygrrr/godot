/**************************************************************************/
/*  pixel_formats_webgpu.h                                                */
/**************************************************************************/
/*                       This file is part of:                            */
/*                           GODOT ENGINE                                 */
/*                      https://godotengine.org                           */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef PIXEL_FORMATS_WEBGPU_H
#define PIXEL_FORMATS_WEBGPU_H

#include "servers/rendering/rendering_device_commons.h"
#include <webgpu/webgpu.h>

// Formats omitted here are unsupported by WebGPU.
static inline WGPUTextureFormat data_format_to_wgpu(RenderingDeviceCommons::DataFormat p_format) {
	using DF = RenderingDeviceCommons::DataFormat;
	switch (p_format) {
		case DF::DATA_FORMAT_R8_UNORM: return WGPUTextureFormat_R8Unorm;
		case DF::DATA_FORMAT_R8_SNORM: return WGPUTextureFormat_R8Snorm;
		case DF::DATA_FORMAT_R8_UINT: return WGPUTextureFormat_R8Uint;
		case DF::DATA_FORMAT_R8_SINT: return WGPUTextureFormat_R8Sint;
		case DF::DATA_FORMAT_R8G8_UNORM: return WGPUTextureFormat_RG8Unorm;
		case DF::DATA_FORMAT_R8G8_SNORM: return WGPUTextureFormat_RG8Snorm;
		case DF::DATA_FORMAT_R8G8_UINT: return WGPUTextureFormat_RG8Uint;
		case DF::DATA_FORMAT_R8G8_SINT: return WGPUTextureFormat_RG8Sint;
		case DF::DATA_FORMAT_R8G8B8A8_UNORM: return WGPUTextureFormat_RGBA8Unorm;
		case DF::DATA_FORMAT_R8G8B8A8_SNORM: return WGPUTextureFormat_RGBA8Snorm;
		case DF::DATA_FORMAT_R8G8B8A8_UINT: return WGPUTextureFormat_RGBA8Uint;
		case DF::DATA_FORMAT_R8G8B8A8_SINT: return WGPUTextureFormat_RGBA8Sint;
		case DF::DATA_FORMAT_R8G8B8A8_SRGB: return WGPUTextureFormat_RGBA8UnormSrgb;
		case DF::DATA_FORMAT_B8G8R8A8_UNORM: return WGPUTextureFormat_BGRA8Unorm;
		case DF::DATA_FORMAT_B8G8R8A8_SRGB: return WGPUTextureFormat_BGRA8UnormSrgb;
		case DF::DATA_FORMAT_A8B8G8R8_UNORM_PACK32: return WGPUTextureFormat_RGBA8Unorm;
		case DF::DATA_FORMAT_A8B8G8R8_SNORM_PACK32: return WGPUTextureFormat_RGBA8Snorm;
		case DF::DATA_FORMAT_A8B8G8R8_UINT_PACK32: return WGPUTextureFormat_RGBA8Uint;
		case DF::DATA_FORMAT_A8B8G8R8_SINT_PACK32: return WGPUTextureFormat_RGBA8Sint;
		case DF::DATA_FORMAT_A8B8G8R8_SRGB_PACK32: return WGPUTextureFormat_RGBA8UnormSrgb;
		case DF::DATA_FORMAT_A2B10G10R10_UNORM_PACK32: return WGPUTextureFormat_RGB10A2Unorm;
		case DF::DATA_FORMAT_A2B10G10R10_UINT_PACK32: return WGPUTextureFormat_RGB10A2Uint;
		case DF::DATA_FORMAT_R16_UINT: return WGPUTextureFormat_R16Uint;
		case DF::DATA_FORMAT_R16_SINT: return WGPUTextureFormat_R16Sint;
		case DF::DATA_FORMAT_R16_SFLOAT: return WGPUTextureFormat_R16Float;
		case DF::DATA_FORMAT_R16G16_UINT: return WGPUTextureFormat_RG16Uint;
		case DF::DATA_FORMAT_R16G16_SINT: return WGPUTextureFormat_RG16Sint;
		case DF::DATA_FORMAT_R16G16_SFLOAT: return WGPUTextureFormat_RG16Float;
		case DF::DATA_FORMAT_R16G16B16A16_UINT: return WGPUTextureFormat_RGBA16Uint;
		case DF::DATA_FORMAT_R16G16B16A16_SINT: return WGPUTextureFormat_RGBA16Sint;
		case DF::DATA_FORMAT_R16G16B16A16_SFLOAT: return WGPUTextureFormat_RGBA16Float;
		case DF::DATA_FORMAT_R32_UINT: return WGPUTextureFormat_R32Uint;
		case DF::DATA_FORMAT_R32_SINT: return WGPUTextureFormat_R32Sint;
		case DF::DATA_FORMAT_R32_SFLOAT: return WGPUTextureFormat_R32Float;
		case DF::DATA_FORMAT_R32G32_UINT: return WGPUTextureFormat_RG32Uint;
		case DF::DATA_FORMAT_R32G32_SINT: return WGPUTextureFormat_RG32Sint;
		case DF::DATA_FORMAT_R32G32_SFLOAT: return WGPUTextureFormat_RG32Float;
		case DF::DATA_FORMAT_R32G32B32A32_UINT: return WGPUTextureFormat_RGBA32Uint;
		case DF::DATA_FORMAT_R32G32B32A32_SINT: return WGPUTextureFormat_RGBA32Sint;
		case DF::DATA_FORMAT_R32G32B32A32_SFLOAT: return WGPUTextureFormat_RGBA32Float;
		case DF::DATA_FORMAT_B10G11R11_UFLOAT_PACK32: return WGPUTextureFormat_RG11B10Ufloat;
		case DF::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32: return WGPUTextureFormat_RGB9E5Ufloat;
		case DF::DATA_FORMAT_D16_UNORM: return WGPUTextureFormat_Depth16Unorm;
		case DF::DATA_FORMAT_X8_D24_UNORM_PACK32: return WGPUTextureFormat_Depth24Plus;
		case DF::DATA_FORMAT_D32_SFLOAT: return WGPUTextureFormat_Depth32Float;
		case DF::DATA_FORMAT_S8_UINT: return WGPUTextureFormat_Stencil8;
		case DF::DATA_FORMAT_D24_UNORM_S8_UINT: return WGPUTextureFormat_Depth24PlusStencil8;
		case DF::DATA_FORMAT_D32_SFLOAT_S8_UINT: return WGPUTextureFormat_Depth32FloatStencil8;
		case DF::DATA_FORMAT_BC1_RGB_UNORM_BLOCK: return WGPUTextureFormat_BC1RGBAUnorm;
		case DF::DATA_FORMAT_BC1_RGB_SRGB_BLOCK: return WGPUTextureFormat_BC1RGBAUnormSrgb;
		case DF::DATA_FORMAT_BC1_RGBA_UNORM_BLOCK: return WGPUTextureFormat_BC1RGBAUnorm;
		case DF::DATA_FORMAT_BC1_RGBA_SRGB_BLOCK: return WGPUTextureFormat_BC1RGBAUnormSrgb;
		case DF::DATA_FORMAT_BC2_UNORM_BLOCK: return WGPUTextureFormat_BC2RGBAUnorm;
		case DF::DATA_FORMAT_BC2_SRGB_BLOCK: return WGPUTextureFormat_BC2RGBAUnormSrgb;
		case DF::DATA_FORMAT_BC3_UNORM_BLOCK: return WGPUTextureFormat_BC3RGBAUnorm;
		case DF::DATA_FORMAT_BC3_SRGB_BLOCK: return WGPUTextureFormat_BC3RGBAUnormSrgb;
		case DF::DATA_FORMAT_BC4_UNORM_BLOCK: return WGPUTextureFormat_BC4RUnorm;
		case DF::DATA_FORMAT_BC4_SNORM_BLOCK: return WGPUTextureFormat_BC4RSnorm;
		case DF::DATA_FORMAT_BC5_UNORM_BLOCK: return WGPUTextureFormat_BC5RGUnorm;
		case DF::DATA_FORMAT_BC5_SNORM_BLOCK: return WGPUTextureFormat_BC5RGSnorm;
		case DF::DATA_FORMAT_BC6H_UFLOAT_BLOCK: return WGPUTextureFormat_BC6HRGBUfloat;
		case DF::DATA_FORMAT_BC6H_SFLOAT_BLOCK: return WGPUTextureFormat_BC6HRGBFloat;
		case DF::DATA_FORMAT_BC7_UNORM_BLOCK: return WGPUTextureFormat_BC7RGBAUnorm;
		case DF::DATA_FORMAT_BC7_SRGB_BLOCK: return WGPUTextureFormat_BC7RGBAUnormSrgb;
		case DF::DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return WGPUTextureFormat_ETC2RGB8Unorm;
		case DF::DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return WGPUTextureFormat_ETC2RGB8UnormSrgb;
		case DF::DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return WGPUTextureFormat_ETC2RGB8A1Unorm;
		case DF::DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return WGPUTextureFormat_ETC2RGB8A1UnormSrgb;
		case DF::DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return WGPUTextureFormat_ETC2RGBA8Unorm;
		case DF::DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return WGPUTextureFormat_ETC2RGBA8UnormSrgb;
		case DF::DATA_FORMAT_EAC_R11_UNORM_BLOCK: return WGPUTextureFormat_EACR11Unorm;
		case DF::DATA_FORMAT_EAC_R11_SNORM_BLOCK: return WGPUTextureFormat_EACR11Snorm;
		case DF::DATA_FORMAT_EAC_R11G11_UNORM_BLOCK: return WGPUTextureFormat_EACRG11Unorm;
		case DF::DATA_FORMAT_EAC_R11G11_SNORM_BLOCK: return WGPUTextureFormat_EACRG11Snorm;
		case DF::DATA_FORMAT_ASTC_4x4_UNORM_BLOCK: return WGPUTextureFormat_ASTC4x4Unorm;
		case DF::DATA_FORMAT_ASTC_4x4_SRGB_BLOCK: return WGPUTextureFormat_ASTC4x4UnormSrgb;
		case DF::DATA_FORMAT_ASTC_5x4_UNORM_BLOCK: return WGPUTextureFormat_ASTC5x4Unorm;
		case DF::DATA_FORMAT_ASTC_5x4_SRGB_BLOCK: return WGPUTextureFormat_ASTC5x4UnormSrgb;
		case DF::DATA_FORMAT_ASTC_5x5_UNORM_BLOCK: return WGPUTextureFormat_ASTC5x5Unorm;
		case DF::DATA_FORMAT_ASTC_5x5_SRGB_BLOCK: return WGPUTextureFormat_ASTC5x5UnormSrgb;
		case DF::DATA_FORMAT_ASTC_6x5_UNORM_BLOCK: return WGPUTextureFormat_ASTC6x5Unorm;
		case DF::DATA_FORMAT_ASTC_6x5_SRGB_BLOCK: return WGPUTextureFormat_ASTC6x5UnormSrgb;
		case DF::DATA_FORMAT_ASTC_6x6_UNORM_BLOCK: return WGPUTextureFormat_ASTC6x6Unorm;
		case DF::DATA_FORMAT_ASTC_6x6_SRGB_BLOCK: return WGPUTextureFormat_ASTC6x6UnormSrgb;
		case DF::DATA_FORMAT_ASTC_8x5_UNORM_BLOCK: return WGPUTextureFormat_ASTC8x5Unorm;
		case DF::DATA_FORMAT_ASTC_8x5_SRGB_BLOCK: return WGPUTextureFormat_ASTC8x5UnormSrgb;
		case DF::DATA_FORMAT_ASTC_8x6_UNORM_BLOCK: return WGPUTextureFormat_ASTC8x6Unorm;
		case DF::DATA_FORMAT_ASTC_8x6_SRGB_BLOCK: return WGPUTextureFormat_ASTC8x6UnormSrgb;
		case DF::DATA_FORMAT_ASTC_8x8_UNORM_BLOCK: return WGPUTextureFormat_ASTC8x8Unorm;
		case DF::DATA_FORMAT_ASTC_8x8_SRGB_BLOCK: return WGPUTextureFormat_ASTC8x8UnormSrgb;
		case DF::DATA_FORMAT_ASTC_10x5_UNORM_BLOCK: return WGPUTextureFormat_ASTC10x5Unorm;
		case DF::DATA_FORMAT_ASTC_10x5_SRGB_BLOCK: return WGPUTextureFormat_ASTC10x5UnormSrgb;
		case DF::DATA_FORMAT_ASTC_10x6_UNORM_BLOCK: return WGPUTextureFormat_ASTC10x6Unorm;
		case DF::DATA_FORMAT_ASTC_10x6_SRGB_BLOCK: return WGPUTextureFormat_ASTC10x6UnormSrgb;
		case DF::DATA_FORMAT_ASTC_10x8_UNORM_BLOCK: return WGPUTextureFormat_ASTC10x8Unorm;
		case DF::DATA_FORMAT_ASTC_10x8_SRGB_BLOCK: return WGPUTextureFormat_ASTC10x8UnormSrgb;
		case DF::DATA_FORMAT_ASTC_10x10_UNORM_BLOCK: return WGPUTextureFormat_ASTC10x10Unorm;
		case DF::DATA_FORMAT_ASTC_10x10_SRGB_BLOCK: return WGPUTextureFormat_ASTC10x10UnormSrgb;
		case DF::DATA_FORMAT_ASTC_12x10_UNORM_BLOCK: return WGPUTextureFormat_ASTC12x10Unorm;
		case DF::DATA_FORMAT_ASTC_12x10_SRGB_BLOCK: return WGPUTextureFormat_ASTC12x10UnormSrgb;
		case DF::DATA_FORMAT_ASTC_12x12_UNORM_BLOCK: return WGPUTextureFormat_ASTC12x12Unorm;
		case DF::DATA_FORMAT_ASTC_12x12_SRGB_BLOCK: return WGPUTextureFormat_ASTC12x12UnormSrgb;
		default: return WGPUTextureFormat_Undefined;
	}
}

// ============================================================================
// DataFormat → WGPUVertexFormat mapping (for vertex buffers)
// ============================================================================
//
// WebGPU vertex formats are more limited than texture formats but support
// some formats that textures don't (e.g., Float32x3 for RGB32F vertices).
//
// This mapping is used in vertex_format_create() to build WGPUVertexAttribute
// descriptors from Godot VertexAttribute descriptions.
// ============================================================================

static inline WGPUVertexFormat data_format_to_wgpu_vertex(RenderingDeviceCommons::DataFormat p_format) {
	using DF = RenderingDeviceCommons::DataFormat;
	switch (p_format) {
		// 8-bit
		case DF::DATA_FORMAT_R8_UINT: return WGPUVertexFormat_Uint8;
		case DF::DATA_FORMAT_R8_SINT: return WGPUVertexFormat_Sint8;
		case DF::DATA_FORMAT_R8G8_UINT: return WGPUVertexFormat_Uint8x2;
		case DF::DATA_FORMAT_R8G8_SINT: return WGPUVertexFormat_Sint8x2;
		case DF::DATA_FORMAT_R8G8B8A8_UINT: return WGPUVertexFormat_Uint8x4;
		case DF::DATA_FORMAT_R8G8B8A8_SINT: return WGPUVertexFormat_Sint8x4;
		case DF::DATA_FORMAT_R8G8_UNORM: return WGPUVertexFormat_Unorm8x2;
		case DF::DATA_FORMAT_R8G8_SNORM: return WGPUVertexFormat_Snorm8x2;
		case DF::DATA_FORMAT_R8G8B8A8_UNORM: return WGPUVertexFormat_Unorm8x4;
		case DF::DATA_FORMAT_R8G8B8A8_SNORM: return WGPUVertexFormat_Snorm8x4;

		// 16-bit
		case DF::DATA_FORMAT_R16_UINT: return WGPUVertexFormat_Uint16;
		case DF::DATA_FORMAT_R16_SINT: return WGPUVertexFormat_Sint16;
		case DF::DATA_FORMAT_R16G16_UINT: return WGPUVertexFormat_Uint16x2;
		case DF::DATA_FORMAT_R16G16_SINT: return WGPUVertexFormat_Sint16x2;
		case DF::DATA_FORMAT_R16G16B16A16_UINT: return WGPUVertexFormat_Uint16x4;
		case DF::DATA_FORMAT_R16G16B16A16_SINT: return WGPUVertexFormat_Sint16x4;
		case DF::DATA_FORMAT_R16G16_UNORM: return WGPUVertexFormat_Unorm16x2;
		case DF::DATA_FORMAT_R16G16_SNORM: return WGPUVertexFormat_Snorm16x2;
		case DF::DATA_FORMAT_R16G16B16A16_UNORM: return WGPUVertexFormat_Unorm16x4;
		case DF::DATA_FORMAT_R16G16B16A16_SNORM: return WGPUVertexFormat_Snorm16x4;
		case DF::DATA_FORMAT_R16_SFLOAT: return WGPUVertexFormat_Float16;
		case DF::DATA_FORMAT_R16G16_SFLOAT: return WGPUVertexFormat_Float16x2;
		case DF::DATA_FORMAT_R16G16B16A16_SFLOAT: return WGPUVertexFormat_Float16x4;

		// 32-bit float
		case DF::DATA_FORMAT_R32_SFLOAT: return WGPUVertexFormat_Float32;
		case DF::DATA_FORMAT_R32G32_SFLOAT: return WGPUVertexFormat_Float32x2;
		case DF::DATA_FORMAT_R32G32B32_SFLOAT: return WGPUVertexFormat_Float32x3; // 3-comp OK for vertex!
		case DF::DATA_FORMAT_R32G32B32A32_SFLOAT: return WGPUVertexFormat_Float32x4;

		// 32-bit integer
		case DF::DATA_FORMAT_R32_UINT: return WGPUVertexFormat_Uint32;
		case DF::DATA_FORMAT_R32G32_UINT: return WGPUVertexFormat_Uint32x2;
		case DF::DATA_FORMAT_R32G32B32_UINT: return WGPUVertexFormat_Uint32x3; // 3-comp OK for vertex!
		case DF::DATA_FORMAT_R32G32B32A32_UINT: return WGPUVertexFormat_Uint32x4;
		case DF::DATA_FORMAT_R32_SINT: return WGPUVertexFormat_Sint32;
		case DF::DATA_FORMAT_R32G32_SINT: return WGPUVertexFormat_Sint32x2;
		case DF::DATA_FORMAT_R32G32B32_SINT: return WGPUVertexFormat_Sint32x3; // 3-comp OK for vertex!
		case DF::DATA_FORMAT_R32G32B32A32_SINT: return WGPUVertexFormat_Sint32x4;

		// 10-10-10-2 packed (A2B10G10R10)
		case DF::DATA_FORMAT_A2B10G10R10_UNORM_PACK32: return WGPUVertexFormat_Unorm10_10_10_2;

		default:
			return (WGPUVertexFormat)0; // WGPUVertexFormat_Undefined was removed in Dawn
	}
}

// ============================================================================
// Depth format helper: is this a depth or depth-stencil format?
// ============================================================================

static inline bool is_depth_format_wgpu(WGPUTextureFormat p_format) {
	switch (p_format) {
		case WGPUTextureFormat_Depth16Unorm:
		case WGPUTextureFormat_Depth24Plus:
		case WGPUTextureFormat_Depth24PlusStencil8:
		case WGPUTextureFormat_Depth32Float:
		case WGPUTextureFormat_Depth32FloatStencil8:
			return true;
		default:
			return false;
	}
}

static inline bool has_stencil_wgpu(WGPUTextureFormat p_format) {
	switch (p_format) {
		case WGPUTextureFormat_Stencil8:
		case WGPUTextureFormat_Depth24PlusStencil8:
		case WGPUTextureFormat_Depth32FloatStencil8:
			return true;
		default:
			return false;
	}
}

// ============================================================================
// Texture aspect mapping
// ============================================================================

static inline WGPUTextureAspect texture_aspect_to_wgpu(
		RenderingDeviceDriver::TextureAspect p_aspect) {
	using TA = RenderingDeviceDriver::TextureAspect;
	switch (p_aspect) {
		case TA::TEXTURE_ASPECT_COLOR:
			return WGPUTextureAspect_All;
		case TA::TEXTURE_ASPECT_DEPTH:
			return WGPUTextureAspect_DepthOnly;
		case TA::TEXTURE_ASPECT_STENCIL:
			return WGPUTextureAspect_StencilOnly;
		default:
			return WGPUTextureAspect_All;
	}
}

// ============================================================================
// Bytes per pixel for supported formats (used for buffer<->texture copies)
// ============================================================================

static inline uint32_t wgpu_format_pixel_size(WGPUTextureFormat p_format) {
	switch (p_format) {
		// 1 byte
		case WGPUTextureFormat_R8Unorm:
		case WGPUTextureFormat_R8Snorm:
		case WGPUTextureFormat_R8Uint:
		case WGPUTextureFormat_R8Sint:
		case WGPUTextureFormat_Stencil8:
			return 1;

		// 2 bytes
		case WGPUTextureFormat_R16Uint:
		case WGPUTextureFormat_R16Sint:
		case WGPUTextureFormat_R16Float:
		// R16Unorm/R16Snorm not in base WebGPU spec (emdawnwebgpu 4.0.10)
		case WGPUTextureFormat_RG8Unorm:
		case WGPUTextureFormat_RG8Snorm:
		case WGPUTextureFormat_RG8Uint:
		case WGPUTextureFormat_RG8Sint:
		case WGPUTextureFormat_Depth16Unorm:
			return 2;

		// 4 bytes
		case WGPUTextureFormat_R32Uint:
		case WGPUTextureFormat_R32Sint:
		case WGPUTextureFormat_R32Float:
		case WGPUTextureFormat_RG16Uint:
		case WGPUTextureFormat_RG16Sint:
		case WGPUTextureFormat_RG16Float:
		// RG16Unorm/RG16Snorm not in base WebGPU spec (emdawnwebgpu 4.0.10)
		case WGPUTextureFormat_RGBA8Unorm:
		case WGPUTextureFormat_RGBA8UnormSrgb:
		case WGPUTextureFormat_RGBA8Snorm:
		case WGPUTextureFormat_RGBA8Uint:
		case WGPUTextureFormat_RGBA8Sint:
		case WGPUTextureFormat_BGRA8Unorm:
		case WGPUTextureFormat_BGRA8UnormSrgb:
		case WGPUTextureFormat_RGB10A2Unorm:
		case WGPUTextureFormat_RGB10A2Uint:
		case WGPUTextureFormat_RG11B10Ufloat:
		case WGPUTextureFormat_RGB9E5Ufloat:
		case WGPUTextureFormat_Depth32Float:
		case WGPUTextureFormat_Depth24Plus:
			return 4;

		// 8 bytes
		case WGPUTextureFormat_RG32Uint:
		case WGPUTextureFormat_RG32Sint:
		case WGPUTextureFormat_RG32Float:
		case WGPUTextureFormat_RGBA16Uint:
		case WGPUTextureFormat_RGBA16Sint:
		case WGPUTextureFormat_RGBA16Float:
		// RGBA16Unorm/RGBA16Snorm not in base WebGPU spec (emdawnwebgpu 4.0.10)
		case WGPUTextureFormat_Depth24PlusStencil8:
		case WGPUTextureFormat_Depth32FloatStencil8:
			return 8;

		// 16 bytes
		case WGPUTextureFormat_RGBA32Uint:
		case WGPUTextureFormat_RGBA32Sint:
		case WGPUTextureFormat_RGBA32Float:
			return 16;

		default:
			return 0; // Compressed or unknown — use block size calculations instead
	}
}

#endif // PIXEL_FORMATS_WEBGPU_H
