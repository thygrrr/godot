/**************************************************************************/
/*  tint_wrapper.cpp                                                      */
/**************************************************************************/
/*                       This file is part of:                            */
/*                           GODOT ENGINE                                 */
/*                      https://godotengine.org                           */
/**************************************************************************/
/* Compiled with C++20 in the Tint build environment.  Wraps Tint's       */
/* SPIR-V reader + WGSL writer behind a simple C-compatible interface     */
/* so that the main Godot driver code (C++17) never includes Tint headers.*/
/**************************************************************************/

#include "tint_wrapper.h"

#include "src/tint/api/tint.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/spirv/reader/reader.h"
#include "src/tint/lang/wgsl/inspector/inspector.h"
#include "src/tint/lang/wgsl/program/program.h"
#include "src/tint/lang/wgsl/writer/common/options.h"
#include "src/tint/lang/wgsl/writer/writer.h"
#include "spirv-tools/optimizer.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

void tint_wrapper_initialize() {
	tint::Initialize();
}

uint32_t *tint_wrapper_prepare_spirv(const uint32_t *p_words, size_t p_count, size_t *r_count, char **r_error) {
	// Tint's texture lowering does not accept Godot 4.7's opaque handle
	// parameters. Inline just image/sampler calls, retaining ordinary functions.
	spvtools::Optimizer optimizer(SPV_ENV_UNIVERSAL_1_6);
	std::string diagnostics;
	optimizer.SetMessageConsumer([&](spv_message_level_t, const char *, const spv_position_t &, const char *message) {
		diagnostics += message;
		diagnostics += '\n';
	});
	optimizer.RegisterPass(spvtools::CreateMergeReturnPass());
	optimizer.RegisterPass(spvtools::CreateInlineOpaquePass());
	optimizer.RegisterPass(spvtools::CreateEliminateDeadFunctionsPass());
	std::vector<uint32_t> inlined;
	if (!optimizer.Run(p_words, p_count, &inlined)) {
		if (r_error) {
			*r_error = static_cast<char *>(malloc(diagnostics.size() + 1));
			if (*r_error) {
				memcpy(*r_error, diagnostics.c_str(), diagnostics.size() + 1);
			}
		}
		return nullptr;
	}
	*r_count = inlined.size();
	auto *result = static_cast<uint32_t *>(malloc(inlined.size() * sizeof(uint32_t)));
	if (result) {
		memcpy(result, inlined.data(), inlined.size() * sizeof(uint32_t));
	}
	return result;
}

uint32_t *tint_wrapper_scalarize_descriptors(const uint32_t *p_words, size_t p_count, size_t *r_count, char **r_error) {
	spvtools::Optimizer optimizer(SPV_ENV_UNIVERSAL_1_6);
	std::string diagnostics;
	optimizer.SetMessageConsumer([&](spv_message_level_t, const char *, const spv_position_t &, const char *message) {
		diagnostics += message;
		diagnostics += '\n';
	});
	optimizer.RegisterPass(spvtools::CreateReplaceDescArrayAccessUsingVarIndexPass());
	optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());
	optimizer.RegisterPass(spvtools::CreateDescriptorArrayScalarReplacementPass());
	optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());
	optimizer.RegisterPass(spvtools::CreateDeadVariableEliminationPass());
	optimizer.RegisterPass(spvtools::CreateEliminateDeadConstantPass());
	std::vector<uint32_t> output;
	if (!optimizer.Run(p_words, p_count, &output)) {
		if (r_error) {
			*r_error = static_cast<char *>(malloc(diagnostics.size() + 1));
			if (*r_error) {
				memcpy(*r_error, diagnostics.c_str(), diagnostics.size() + 1);
			}
		}
		return nullptr;
	}
	*r_count = output.size();
	auto *result = static_cast<uint32_t *>(malloc(output.size() * sizeof(uint32_t)));
	if (result) {
		memcpy(result, output.data(), output.size() * sizeof(uint32_t));
	}
	return result;
}

char *tint_wrapper_spirv_to_wgsl(const uint32_t *p_spirv_words, size_t p_word_count, char **r_error) {
	std::vector<uint32_t> words(p_spirv_words, p_spirv_words + p_word_count);
	// Allow all WGSL extensions and language features so Tint can emit
	// constructs like readonly storage textures without validation errors.
	tint::wgsl::writer::Options wgsl_options;
	wgsl_options.allowed_features = tint::wgsl::AllowedFeatures::Everything();
	// Godot's GLSL shaders use textureSample/dpdx in non-uniform control flow
	// (valid in Vulkan, but WGSL requires uniform control flow for derivatives).
	// This inserts `diagnostic(off, derivative_uniformity)` in the output.
	wgsl_options.allow_non_uniform_derivatives = true;

	// Keep the resolved Program so layout visibility can follow Tint's call
	// graph, including bindings introduced by texture/sampler lowering.
	auto convert = [&]() -> tint::Result<std::string> {
		TINT_CHECK_RESULT_UNWRAP(ir, tint::spirv::reader::ReadIR(words));
		TINT_CHECK_RESULT_UNWRAP(program, tint::wgsl::writer::ProgramFromIR(ir, wgsl_options));
		tint::inspector::Inspector inspector(program);
		std::string metadata = "//RESOURCE_USAGE\n";
		for (const auto &entry : inspector.GetEntryPoints()) {
			for (const auto &binding : inspector.GetResourceBindings(entry.name)) {
				metadata += "//RESOURCE_USED:" + std::to_string(binding.bind_group) + "," + std::to_string(binding.binding) + "\n";
			}
			for (const auto &pair : inspector.GetSamplerTextureUses(entry.name)) {
				metadata += "//TEXTURE_SAMPLED:" + std::to_string(pair.texture_binding_point.group) + "," + std::to_string(pair.texture_binding_point.binding) + "\n";
			}
		}
		if (inspector.has_error()) {
			return tint::Failure{inspector.error()};
		}
		TINT_CHECK_RESULT_UNWRAP(output, tint::wgsl::writer::Generate(program, wgsl_options));
		return metadata + output.wgsl;
	};
	auto result = convert();
	if (result != tint::Success) {
		if (r_error) {
			const std::string &reason = result.Failure().reason;
			char *err = (char *)malloc(reason.size() + 1);
			if (err) {
				memcpy(err, reason.c_str(), reason.size() + 1);
			}
			*r_error = err;
		}
		return nullptr;
	}

	const std::string &wgsl = result.Get();
	char *out = (char *)malloc(wgsl.size() + 1);
	if (!out) {
		return nullptr;
	}
	memcpy(out, wgsl.c_str(), wgsl.size() + 1);
	return out;
}
