# WebGPU checks

The tracked checks exercise the actual compiler, build configuration and engine
JavaScript. Run them from the Godot checkout:

```text
uv run --no-project python -m unittest discover -s webgpu_tests -p "test_*.py" -v
node --test webgpu_tests/engine_boot.test.mjs
uv run --no-project python webgpu_tests/wgsl_cache/test_wgsl_precompile.py
node webgpu_tests/wgsl_cache/test_wgsl_cache.mjs
```

`drivers/webgpu/tint_cli/build.sh` builds a host converter on Linux/macOS using
the same compiler sources as SCons. The shader corpus and preprocessing suite
exercise that converter, rather than reimplementing the transformations:

```text
./drivers/webgpu/tint_cli/build.sh
node webgpu_tests/preprocessing_tests/run_tests.mjs
node webgpu_tests/shader_corpus/run_tests.mjs
```

The GLSL and SPIR-V fixtures in `shader_corpus/fixtures` are retained for these
checks. To regenerate them, use `shader_corpus/compile_fixtures.sh` with glslang.

For a built web archive, the port regressions compile and execute C++ checks
through the pinned .NET Emscripten SDK:

```text
uv run python webgpu_tests/run_port_regressions.py --emscripten <emsdk>/upstream/emscripten --archive <built-libgodot.a> --output <scratch-directory>
```

These cover storage access, resource IDs colliding with literal operands,
resource reflection and old-libc++ float parsing. Forward+ adds descriptor-array
and float-classification regressions. `native_smoke/` provides a real-engine
render/readback check; see [the driver notes](../drivers/webgpu/README.md) for
build and renderer selection.

## Optional upstream diagnostics and demos

The original WebGPU fork's unchanged benchmarks, browser demos, resource stress
tests, screenshot tools and JavaScript algorithm models are available by pinned
reference. They are optional companion tools; the JavaScript models do not test
the C++ driver's implementation. Restore them explicitly:

```text
uv run --no-project python webgpu_tests/prepare_upstream.py
```

This verifies the SHA-256 of `dwalter/godotwebgpu` at
`f329e39ce8db7acaa5c9d6628a530fb769969228` and restores 95 ignored files into
their original locations. `--cache <directory> --offline` uses a prefilled archive
cache. Existing local modifications are preserved by refusing to overwrite them.

After preparation, the upstream commands remain available, for example:

```text
node webgpu_tests/driver_unit_tests/run_tests.mjs
node webgpu_tests/resource_lifecycle/run_tests.mjs
node webgpu_tests/scene_smoketest/run_scenes.mjs --browser chrome
```

The browser tools require Playwright and exported projects. Their original
instructions are included in the restored source. They do not run as part of
the focused port checks above.
