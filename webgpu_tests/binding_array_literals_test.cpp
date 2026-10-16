#include "spirv_preprocess.h"
#include "spirv-tools/libspirv.h"

#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    // Array type ID 112 deliberately collides with offsets, bindings and constants.
    const char *source = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %color
OpExecutionMode %main OriginUpperLeft
OpDecorate %color Location 0
OpDecorate %resources DescriptorSet 0
OpDecorate %resources Binding 112
OpDecorate %block Block
OpMemberDecorate %block 0 Offset 0
OpMemberDecorate %block 1 Offset 112
OpDecorate %data DescriptorSet 0
OpDecorate %data Binding 3
%void = OpTypeVoid
%float = OpTypeFloat 32
%uint = OpTypeInt 32 0
%v4 = OpTypeVector %float 4
%zero = OpConstant %uint 0
%one = OpConstant %uint 1
%two = OpConstant %uint 2
%literal = OpConstant %uint 112
%sampler = OpTypeSampler
%112 = OpTypeArray %sampler %two
%parray = OpTypePointer UniformConstant %112
%psampler = OpTypePointer UniformConstant %sampler
%resources = OpVariable %parray UniformConstant
%block = OpTypeStruct %v4 %v4
%pblock = OpTypePointer Uniform %block
%pvec = OpTypePointer Uniform %v4
%data = OpVariable %pblock Uniform
%pout = OpTypePointer Output %v4
%color = OpVariable %pout Output
%main_type = OpTypeFunction %void
%main = OpFunction %void None %main_type
%label = OpLabel
%element = OpAccessChain %psampler %resources %zero
%loaded = OpLoad %sampler %element
%field = OpAccessChain %pvec %data %one
%value = OpLoad %v4 %field
OpStore %color %value
OpReturn
OpFunctionEnd
)";
    auto context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
    spv_binary binary = nullptr;
    spv_diagnostic diagnostic = nullptr;
    assert(spvTextToBinaryWithOptions(context, source, std::strlen(source), SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS, &binary, &diagnostic) == SPV_SUCCESS);
    Vector<uint8_t> bytes;
    bytes.resize(binary->wordCount * 4);
    std::memcpy(bytes.ptrw(), binary->code, bytes.size());
    auto result = spirv_preprocess::flatten_binding_arrays(bytes);
    const auto *words = reinterpret_cast<const uint32_t *>(result.ptr());
    bool offset = false, binding = false, constant = false;
    for (size_t pos = 5; pos < size_t(result.size() / 4); pos += words[pos] >> 16) {
        uint32_t opcode = words[pos] & 0xffff;
        if (opcode == 72 && words[pos + 3] == 35 && words[pos + 2] == 1) {
            assert(words[pos + 4] == 112);
            offset = true;
        }
        if (opcode == 71 && words[pos + 2] == 33 && words[pos + 3] == 112) binding = true;
        if (opcode == 43 && words[pos + 3] == 112) constant = true;
        assert(opcode != 28); // Handle array type removed.
    }
    assert(offset && binding && constant);
    assert(spvValidateBinary(context, words, result.size() / 4, &diagnostic) == SPV_SUCCESS);
    spvBinaryDestroy(binary);
    spvDiagnosticDestroy(diagnostic);
    spvContextDestroy(context);
    std::puts("PASS: handle array flattening preserves colliding literal offsets, bindings and constants");
}
