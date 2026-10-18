#!/usr/bin/env bash
# Focused port checks; requires a built bin/tint_convert_cli for compiler suites.
set -euo pipefail
cd "$(dirname "$0")/.."

uv run --no-project python -m unittest discover -s webgpu_tests -p 'test_*.py' -v
node --test webgpu_tests/engine_boot.test.mjs
uv run --no-project python webgpu_tests/wgsl_cache/test_wgsl_precompile.py
node webgpu_tests/wgsl_cache/test_wgsl_cache.mjs

if [[ ! -x bin/tint_convert_cli ]]; then
    echo 'Build the host converter first: ./drivers/webgpu/tint_cli/build.sh' >&2
    exit 1
fi
node webgpu_tests/preprocessing_tests/run_tests.mjs
node webgpu_tests/shader_corpus/run_tests.mjs
