# WebGPU rendering driver

This opt-in backend ports `dwalter/godotwebgpu` at
`f329e39ce8db7acaa5c9d6628a530fb769969228` to the 2dog Godot 4.7.2 fork.
It supports browser WebGPU and optional native Dawn on Windows, Linux and macOS.
Existing rendering defaults and other backends remain available.

`2dog-4.7.2-webgpu-mobile` supports the Mobile renderer. The later
`2dog-4.7.2-webgpu` branch adds Forward+ support and its compiler regressions.
Both branches permit headless resource imports and exports for WebGPU projects.

## Dependencies and builds

SCons prepares [checksum-pinned upstream sources](dependencies.md) when
`webgpu=yes`. Local Tint, SPIRV-Tools and Emscripten compatibility changes are
kept as patches. Compiler source selection is shared with the host Tint CLI.

For the .NET browser host, keep Emscripten **3.1.56** and build from the outer
2dog checkout with the Godot worktree substituted below:

```text
uv run scons -C <godot-checkout> platform=web target=template_debug library_type=static_library module_mono_enabled=yes disable_crash_handler=yes disable_path_overrides=no threads=no lto=none webgpu=yes webgpu_precompile=no
```

`webgpu_port=auto` uses the prepared Emdawnwebgpu port. Its legacy JavaScript
uses the Emscripten 3.x stack dependency names; newer SDKs use the original
JavaScript. Tint also retains a locale-independent float parser for older libc++.
An explicit local `.py` port or `webgpu_port=emdawnwebgpu` may be supplied for
other toolchains; the remote port requires Emscripten 4.0.10 or later.

On Windows, keep the Emscripten cache on the SDK's drive to avoid the 3.1.56
system-library builder's cross-drive path limitation. Use separate native and
.NET link caches and a writable `WasmCachePath` for the browser host.

`webgpu_precompile=no` uses runtime shader translation. Optional build-time
precompilation requires the POSIX host Tint tool and glslang. It is disabled by
default for Windows and native builds.

For desktop builds, use `webgpu=yes webgpu_native_path=<boundary-sdk>`.
[Native setup](native/README.md) describes the separately pinned Dawn SDK,
shared C API boundary, sidecar libraries and platform commands. The boundary
isolates Dawn's compiler symbols from Godot's patched Tint.

## Runtime selection

Select `--rendering-driver webgpu --rendering-method mobile` for the Mobile
branch. The Forward+ branch also supports `--rendering-method forward_plus`.
Use the OpenGL driver when selecting `gl_compatibility`.

A custom browser shell must set `renderingDriver: 'webgpu'` before Engine
initialization. The final emcc link must use the prepared Dawn port and its
Closure externs with Emscripten 3.1.56.

The driver keeps shader transformations and renderer workarounds behind WebGPU
capabilities. Browser readback is asynchronous; native readback waits for GPU
completion. Native HWND, Xlib/Wayland and normal/embedded CAMetalLayer surfaces
use the same driver-selection mechanism as the other backends.

## Verification

[Port checks](../../webgpu_tests/README.md) cover real compiler transformations,
resource reflection, SDK configuration, JavaScript engine startup and native
render/readback. Optional upstream diagnostics and demos are fetched separately.

The port retains the four-argument `RenderingDevice.draw_list_draw` binding;
the first-instance overload is internal, preserving existing GodotSharp method
hashes. WebGPU has no ray tracing, and unsupported multiview paths are skipped.
Physical-device macOS and Linux validation depends on available hardware.
