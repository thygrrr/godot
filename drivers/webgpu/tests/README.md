# WebGPU checks

The tracked checks exercise the actual compiler, build configuration and engine
JavaScript. Run them from the Godot checkout:

```text
uv run --no-project python -m unittest discover -s drivers/webgpu/tests -p "test_*.py" -v
node --test drivers/webgpu/tests/engine_boot.test.mjs
```

`drivers/webgpu/tint_cli/build.sh` builds a host converter that runs the driver's
SPIR-V pipeline and Tint from the same compiler sources as SCons. It runs on Linux,
macOS and Git Bash with MinGW (`CXX=g++`). The preprocessing suite exercises that
converter on the `shader_corpus` fixtures and constructed SPIR-V, and fails when the
converter is missing:

```text
./drivers/webgpu/tint_cli/build.sh
node drivers/webgpu/tests/preprocessing_tests/run_tests.mjs
```

[`shader_corpus`](shader_corpus/README.md) describes the fixtures, how to regenerate
them, and how to convert a `GODOT_DUMP_SPIRV` dump of engine shaders.

For a built web archive, the port regressions compile and execute C++ checks
through the pinned .NET Emscripten SDK:

```text
uv run python drivers/webgpu/tests/run_port_regressions.py --emscripten <emsdk>/upstream/emscripten --archive <built-libgodot.a> --output <scratch-directory>
```

These cover storage access and read-only inference, resource IDs colliding with
literal operands, typed resource reflection, varying compaction, malformed
metadata and old-libc++ float parsing. The metadata checks compile the current
Tint wrapper against the archive's compiler dependencies.
`native_smoke/` checks material color, ambient and directional light by readback.
Its `binding_arrays.gd` checks dynamic indexing of four differently colored textures
through the actual shader, layout and uniform-set paths:

```text
<godot> --rendering-driver webgpu --path drivers/webgpu/tests/native_smoke --script res://binding_arrays.gd
```

See [the driver notes](../README.md) for build and renderer selection.

The outer `tests/webgpu-smoke` test reloads a baked shader container and checks
specialization constants and push constants by compute readback. Native and
browser rendering checks also cover two user varyings and a textured lightmapped
material with a shadow-mask atlas. The browser `limited-bindings` query constrains
sampled textures and inter-stage variables to 16 so these paths exercise the
adapter budget rather than the development GPU's higher limits.

## Optional upstream diagnostics and demos

The original WebGPU fork's unchanged benchmarks, browser demos, resource stress
tests, screenshot tools and JavaScript algorithm models are available by pinned
reference. They are optional companion tools; the JavaScript models do not test
the C++ driver's implementation. Restore them explicitly:

```text
uv run --no-project python drivers/webgpu/tests/prepare_upstream.py
```

This verifies the SHA-256 of `dwalter/godotwebgpu` at
`f329e39ce8db7acaa5c9d6628a530fb769969228` and restores 95 ignored files into
their original upstream locations under the repository-root `webgpu_tests/`.
`--cache <directory> --offline` uses a prefilled archive cache. Existing local
modifications are preserved by refusing to overwrite them.

After preparation, the upstream commands remain available, for example:

```text
node webgpu_tests/driver_unit_tests/run_tests.mjs
node webgpu_tests/resource_lifecycle/run_tests.mjs
node webgpu_tests/scene_smoketest/run_scenes.mjs --browser chrome
```

The browser tools require Playwright and exported projects. Their original
instructions are included in the restored source. They do not run as part of
the focused port checks above.
