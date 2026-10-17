/**************************************************************************/
/*  tint_descriptor_arrays_test.cpp                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
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

#include "spirv-tools/libspirv.h"
#include "spirv_preprocess.h"
#include "tint_wrapper.h"

#include "drivers/webgpu/tests/check.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

static size_t count_occurrences(const char *p_text, const char *p_pattern) {
	size_t count = 0;
	for (const char *cursor = p_text; (cursor = std::strstr(cursor, p_pattern)); cursor++) {
		count++;
	}
	return count;
}

int main() {
	// Adjacent arrays read and write the element a dynamic index selects. The upstream variable-index pass left
	// the original image write outside its switch.
	const char *source = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %invocation
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %invocation BuiltIn GlobalInvocationId
OpDecorate %dest DescriptorSet 0
OpDecorate %dest Binding 0
OpDecorate %dest NonReadable
OpDecorate %src DescriptorSet 0
OpDecorate %src Binding 1
OpDecorate %src NonWritable
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%int = OpTypeInt 32 1
%u3 = OpTypeVector %uint 3
%u4 = OpTypeVector %uint 4
%i2 = OpTypeVector %int 2
%zero = OpConstant %int 0
%two = OpConstant %uint 2
%coords = OpConstantComposite %i2 %zero %zero
%img = OpTypeImage %uint 2D 0 0 0 2 R32ui
%array = OpTypeArray %img %two
%parray = OpTypePointer UniformConstant %array
%pimg = OpTypePointer UniformConstant %img
%pinput = OpTypePointer Input %u3
%invocation = OpVariable %pinput Input
%dest = OpVariable %parray UniformConstant
%src = OpVariable %parray UniformConstant
%fn = OpTypeFunction %void
%main = OpFunction %void None %fn
%entry = OpLabel
%xyz = OpLoad %u3 %invocation
%x = OpCompositeExtract %uint %xyz 0
%index = OpUMod %uint %x %two
%sptr = OpAccessChain %pimg %src %index
%simg = OpLoad %img %sptr
%pixel = OpImageRead %u4 %simg %coords
%dptr = OpAccessChain %pimg %dest %index
%dimg = OpLoad %img %dptr
OpImageWrite %dimg %coords %pixel
OpReturn
OpFunctionEnd
)";
	auto context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
	spv_binary binary = nullptr;
	spv_diagnostic diagnostic = nullptr;
	const spv_result_t assembled = spvTextToBinary(context, source, std::strlen(source), &binary, &diagnostic);
	CHECK(assembled == SPV_SUCCESS);
	Vector<uint8_t> bytes;
	bytes.resize(binary->wordCount * 4);
	std::memcpy(bytes.ptrw(), binary->code, bytes.size());

	// Like the driver, give each array a range no other binding uses: dest takes 2-3 and src 4-5.
	HashMap<uint32_t, uint32_t> ranges;
	ranges.insert(0, 2);
	ranges.insert(1, 4);
	bytes = spirv_preprocess::remap_bindings(bytes, ranges);

	const Vector<uint8_t> scalar = spirv_preprocess::scalarize_binding_arrays(bytes);
	spv_text assembly = nullptr;
	const spv_result_t disassembled = spvBinaryToText(context, reinterpret_cast<const uint32_t *>(scalar.ptr()), scalar.size() / 4, 0, &assembly, &diagnostic);
	CHECK(disassembled == SPV_SUCCESS);
	CHECK(count_occurrences(assembly->str, "OpImageWrite") == 2); // One per switch case, without the original write.
	CHECK(count_occurrences(assembly->str, "OpTypeArray") == 0);

	bytes = spirv_preprocess::run_pipeline(bytes);
	tint_wrapper_initialize();
	char *error = nullptr;
	char *wgsl = tint_wrapper_spirv_to_wgsl(reinterpret_cast<const uint32_t *>(bytes.ptr()), bytes.size() / 4, &error);
	if (error) {
		std::fprintf(stderr, "%s\n", error);
	}
	CHECK(wgsl);
	// The split doubles every binding.
	for (const char *binding : { "4", "6", "8", "10" }) {
		char expected[64];
		std::snprintf(expected, sizeof(expected), "//RESOURCE_USED:0,%s\n", binding);
		CHECK(std::strstr(wgsl, expected));
	}
	CHECK(!std::strstr(wgsl, "binding_array"));
	std::free(wgsl);
	std::free(error);
	spvTextDestroy(assembly);
	spvBinaryDestroy(binary);
	spvDiagnosticDestroy(diagnostic);
	spvContextDestroy(context);
	std::puts("PASS: dynamically indexed image arrays keep every element and execute one store");
}
