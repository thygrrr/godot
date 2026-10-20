#include "spirv_preprocess.h"
#include "tint_wrapper.h"
#include "spirv-tools/libspirv.h"

#include "check.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main() {
    // A nested helper takes opaque pointers. One sampler is deliberately unused.
    const char *source = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %color
OpExecutionMode %main OriginUpperLeft
OpDecorate %color Location 0
OpDecorate %image DescriptorSet 0
OpDecorate %image Binding 0
OpDecorate %sampler DescriptorSet 0
OpDecorate %sampler Binding 1
OpDecorate %unused DescriptorSet 0
OpDecorate %unused Binding 2
%void = OpTypeVoid
%float = OpTypeFloat 32
%v2 = OpTypeVector %float 2
%v4 = OpTypeVector %float 4
%img = OpTypeImage %float 2D 0 0 0 1 Unknown
%samp = OpTypeSampler
%sampled = OpTypeSampledImage %img
%pimg = OpTypePointer UniformConstant %img
%psamp = OpTypePointer UniformConstant %samp
%pout = OpTypePointer Output %v4
%image = OpVariable %pimg UniformConstant
%sampler = OpVariable %psamp UniformConstant
%unused = OpVariable %psamp UniformConstant
%color = OpVariable %pout Output
%half = OpConstant %float 0.5
%uv = OpConstantComposite %v2 %half %half
%helper_type = OpTypeFunction %v4 %pimg %psamp
%main_type = OpTypeFunction %void
%inner = OpFunction %v4 None %helper_type
%inner_image = OpFunctionParameter %pimg
%inner_sampler = OpFunctionParameter %psamp
%inner_label = OpLabel
%loaded_image = OpLoad %img %inner_image
%loaded_sampler = OpLoad %samp %inner_sampler
%combined = OpSampledImage %sampled %loaded_image %loaded_sampler
%pixel = OpImageSampleImplicitLod %v4 %combined %uv
OpReturnValue %pixel
OpFunctionEnd
%outer = OpFunction %v4 None %helper_type
%outer_image = OpFunctionParameter %pimg
%outer_sampler = OpFunctionParameter %psamp
%outer_label = OpLabel
%nested = OpFunctionCall %v4 %inner %outer_image %outer_sampler
OpReturnValue %nested
OpFunctionEnd
%main = OpFunction %void None %main_type
%main_label = OpLabel
%result = OpFunctionCall %v4 %outer %image %sampler
OpStore %color %result
OpReturn
OpFunctionEnd
)";
    auto context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
    spv_binary binary = nullptr;
    spv_diagnostic diagnostic = nullptr;
    const spv_result_t assembled = spvTextToBinary(context, source, std::strlen(source), &binary, &diagnostic);
    CHECK(assembled == SPV_SUCCESS);
    // The driver's pipeline inlines the opaque helpers and doubles the bindings (0, 1, 2 -> 0, 2, 4).
    Vector<uint8_t> bytes;
    bytes.resize(binary->wordCount * 4);
    std::memcpy(bytes.ptrw(), binary->code, bytes.size());
    bytes = spirv_preprocess::run_pipeline(bytes);
    tint_wrapper_initialize();
    char *error = nullptr;
    char *wgsl = tint_wrapper_spirv_to_wgsl(reinterpret_cast<const uint32_t *>(bytes.ptr()), bytes.size() / 4, &error);
    if (error) std::fprintf(stderr, "%s\n", error);
    CHECK(wgsl);
    CHECK(std::strstr(wgsl, "//RESOURCE_USAGE\n"));
    CHECK(std::strstr(wgsl, "//RESOURCE_USED:0,0\n"));
    CHECK(std::strstr(wgsl, "//RESOURCE_USED:0,2\n"));
    CHECK(!std::strstr(wgsl, "//RESOURCE_USED:0,4\n"));
    CHECK(std::strstr(wgsl, "textureSample("));
    std::free(wgsl);
    std::free(error);
    spvBinaryDestroy(binary);
    spvDiagnosticDestroy(diagnostic);
    spvContextDestroy(context);
    std::puts("PASS: opaque helper translation and reachable resource reflection");
}
