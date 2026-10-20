#!/usr/bin/env bash
# Focused port checks; requires a built bin/tint_convert_cli for compiler suites.
set -euo pipefail
cd "$(dirname "$0")/.."

for tool in uv node; do
    command -v "$tool" >/dev/null || { echo "Missing required tool: $tool" >&2; exit 1; }
done

uv run --no-project python -m unittest discover -s webgpu_tests -p 'test_*.py' -v
node --test webgpu_tests/engine_boot.test.mjs

# The runners skip when the converter is missing, so require it here instead of passing vacuously.
if [[ ! -f bin/tint_convert_cli && ! -f bin/tint_convert_cli.exe ]]; then
    echo 'Build the host converter first: ./drivers/webgpu/tint_cli/build.sh' >&2
    exit 1
fi
node webgpu_tests/preprocessing_tests/run_tests.mjs
node webgpu_tests/shader_corpus/run_tests.mjs
