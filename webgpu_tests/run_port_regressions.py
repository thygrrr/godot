"""Build and run the 2dog port regressions against an existing web archive."""

import argparse
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--emscripten", type=Path, required=True)
parser.add_argument("--archive", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
subprocess.run([sys.executable, str(root / "drivers/webgpu/dependencies.py")], check=True)
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
compiler = args.emscripten.resolve() / "em++.py"
archive = args.archive.resolve()
driver = root / "drivers/webgpu"
common = [
    f"-I{driver / 'tint_cli'}",
    f"-I{driver}",
    f"-I{root / 'thirdparty/tint'}",
    f"-I{root / 'thirdparty/spirv-tools/include'}",
    "-std=c++20",
    "-sSTACK_SIZE=5242880",
    "-sALLOW_MEMORY_GROWTH=1",
    "-sENVIRONMENT=node",
    "-sASSERTIONS=1",
]
for name in (
    "storage_access",
    "binding_array_literals",
    "tint_resource_usage",
    "tint_descriptor_arrays",
    "tint_float_classification",
    "tint_legacy_parse",
):
    sources = [str(root / f"webgpu_tests/{name}_test.cpp")]
    if name in ("storage_access", "binding_array_literals"):
        sources.append(str(driver / "spirv_preprocess.cpp"))
    target = output / f"{name}.js"
    subprocess.run(
        [sys.executable, str(compiler), *sources, str(archive), *common, "-o", str(target)],
        check=True,
        env=os.environ.copy(),
    )
    subprocess.run(["node", str(target)], check=True)
