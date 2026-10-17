#include "spirv-tools/libspirv.h"
#include "tint_wrapper.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

int main() {
	// Both reads and writes must select the actual array element. The upstream
	// variable-index pass left the original image write outside its switch.
	const char *source = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %invocation
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %invocation BuiltIn GlobalInvocationId
OpDecorate %dest DescriptorSet 0
OpDecorate %dest Binding 128
OpDecorate %dest NonReadable
OpDecorate %src DescriptorSet 0
OpDecorate %src Binding 130
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
	assert(spvTextToBinary(context, source, std::strlen(source), &binary, &diagnostic) == SPV_SUCCESS);
	tint_wrapper_initialize();
	char *error = nullptr;
	size_t count = 0;
	auto *expanded = tint_wrapper_scalarize_descriptors(binary->code, binary->wordCount, &count, &error);
	if (error) {
		std::fprintf(stderr, "%s\n", error);
	}
	assert(expanded);
	spv_text assembly = nullptr;
	assert(spvBinaryToText(context, expanded, count, 0, &assembly, &diagnostic) == SPV_SUCCESS);
	size_t writes = 0;
	const char *cursor = assembly->str;
	while ((cursor = std::strstr(cursor, "OpImageWrite"))) {
		++writes;
		++cursor;
	}
	assert(writes == 2); // One write per case, with no extra original write.
	char *wgsl = tint_wrapper_spirv_to_wgsl(expanded, count, &error);
	if (error) {
		std::fprintf(stderr, "%s\n", error);
	}
	assert(wgsl);
	for (auto binding : { "128", "129", "130", "131" }) {
		char expected[64];
		std::snprintf(expected, sizeof(expected), "//RESOURCE_USED:0,%s\n", binding);
		assert(std::strstr(wgsl, expected));
	}
	assert(!std::strstr(wgsl, "binding_array"));
	std::free(wgsl);
	std::free(expanded);
	std::free(error);
	spvTextDestroy(assembly);
	spvBinaryDestroy(binary);
	spvDiagnosticDestroy(diagnostic);
	spvContextDestroy(context);
	std::puts("PASS: dynamic descriptor-array reads and writes preserve every resource and execute one store");
}
