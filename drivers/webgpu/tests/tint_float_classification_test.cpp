/**************************************************************************/
/*  tint_float_classification_test.cpp                                    */
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

int main() {
	// Vector OpIsNan/OpIsInf, which WGSL lacks (thirdparty/tint/patches/0010).
	const char *source = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %input DescriptorSet 0
OpDecorate %input Binding 0
OpDecorate %output DescriptorSet 0
OpDecorate %output Binding 1
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpMemberDecorate %block 1 Offset 16
%void = OpTypeVoid
%bool = OpTypeBool
%uint = OpTypeInt 32 0
%float = OpTypeFloat 32
%u4 = OpTypeVector %uint 4
%f4 = OpTypeVector %float 4
%b4 = OpTypeVector %bool 4
%block = OpTypeStruct %u4 %u4
%pblock = OpTypePointer StorageBuffer %block
%pu4 = OpTypePointer StorageBuffer %u4
%input = OpVariable %pblock StorageBuffer
%output = OpVariable %pblock StorageBuffer
%zero = OpConstant %uint 0
%one = OpConstant %uint 1
%zeros = OpConstantComposite %u4 %zero %zero %zero %zero
%ones = OpConstantComposite %u4 %one %one %one %one
%fn = OpTypeFunction %void
%main = OpFunction %void None %fn
%entry = OpLabel
%iptr = OpAccessChain %pu4 %input %zero
%bits = OpLoad %u4 %iptr
%values = OpBitcast %f4 %bits
%nan = OpIsNan %b4 %values
%inf = OpIsInf %b4 %values
%nan_flags = OpSelect %u4 %nan %ones %zeros
%inf_flags = OpSelect %u4 %inf %ones %zeros
%optr0 = OpAccessChain %pu4 %output %zero
%optr1 = OpAccessChain %pu4 %output %one
OpStore %optr0 %nan_flags
OpStore %optr1 %inf_flags
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
	bytes = spirv_preprocess::run_pipeline(bytes);
	tint_wrapper_initialize();
	char *error = nullptr;
	char *wgsl = tint_wrapper_spirv_to_wgsl(reinterpret_cast<const uint32_t *>(bytes.ptr()), bytes.size() / 4, &error);
	if (error) {
		std::fprintf(stderr, "%s\n", error);
	}
	CHECK(wgsl);
	// |bits| > infinity's bits is NaN, == is infinity; a float comparison could be folded away.
	CHECK(std::strstr(wgsl, "bitcast<vec4<u32>>"));
	CHECK(std::strstr(wgsl, "2147483647u"));
	CHECK(std::strstr(wgsl, "2139095040u"));
	CHECK(std::strstr(wgsl, " > "));
	CHECK(std::strstr(wgsl, " == "));
	std::free(wgsl);
	std::free(error);
	spvBinaryDestroy(binary);
	spvDiagnosticDestroy(diagnostic);
	spvContextDestroy(context);
	std::puts("PASS: NaN and infinity classification lower to sign-independent IEEE-754 bit tests");
}
