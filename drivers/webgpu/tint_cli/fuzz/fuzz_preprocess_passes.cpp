// Fuzz spirv_preprocess::run_pipeline() without Tint.
//
// Catches crashes in the raw SPIR-V manipulation code (out-of-bounds, integer overflow, etc.).

#include "../../spirv_preprocess.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	Vector<uint8_t> spv;
	spv.resize((int64_t)size);
	if (size > 0) {
		memcpy(spv.ptrw(), data, size);
	}
	spirv_preprocess::run_pipeline(spv);
	return 0;
}
