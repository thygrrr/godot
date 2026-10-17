#include "spirv-tools/libspirv.h"
#include "tint_wrapper.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main() {
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
	assert(spvTextToBinary(context, source, std::strlen(source), &binary, &diagnostic) == SPV_SUCCESS);
	tint_wrapper_initialize();
	char *error = nullptr;
	char *wgsl = tint_wrapper_spirv_to_wgsl(binary->code, binary->wordCount, &error);
	if (error) {
		std::fprintf(stderr, "%s\n", error);
	}
	assert(wgsl);
	assert(std::strstr(wgsl, "bitcast<vec4<u32>>"));
	assert(std::strstr(wgsl, "2147483647u"));
	assert(std::strstr(wgsl, "2139095040u"));
	assert(std::strstr(wgsl, " > "));
	assert(std::strstr(wgsl, " == "));
	std::free(wgsl);
	std::free(error);
	spvBinaryDestroy(binary);
	spvDiagnosticDestroy(diagnostic);
	spvContextDestroy(context);
	std::puts("PASS: NaN and infinity classification lower to sign-independent IEEE-754 bit tests");
}
