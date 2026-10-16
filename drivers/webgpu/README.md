# WebGPU Rendering Driver

## 2dog 4.7.2 integration spike

This worktree ports `dwalter/godotwebgpu` at
`f329e39ce8db7acaa5c9d6628a530fb769969228` onto 2dog's
`2165a59e1b5fbd3574403b56e6218e7713f116b4`, including the existing web
threading and side-module work. The current 2dog submodule checkout and native
packages are separate from this spike.

WebGPU is opt-in (`webgpu=yes`). OpenGL/WebGL remains enabled and
`gl_compatibility` remains the web project default. Existing renderer shader
code stays behind the non-WebGPU path; the new driver traits default to zero
for other drivers. The Godot version remains 4.7.2.

The spike keeps .NET's Emscripten **3.1.56** pin. A checksum-verified local
Emdawnwebgpu port is vendored in `thirdparty/emdawnwebgpu`; its legacy JS copy
uses the 3.x stack dependency names. Tint has a tested, locale-independent
float-parsing fallback for the older libc++. Newer SDKs use the original Dawn
JS library and `std::from_chars` path. WebGPU's newer SPIR-V headers are kept
in a separate directory, leaving the engine's existing headers unchanged.

The Godot 4.7 port adds the new barrier argument and explicit unsupported
ray-tracing/HDR interfaces. It also retains the existing 2dog error-callback
export in non-SCU MSVC shared-library builds.

The original four-argument `RenderingDevice.draw_list_draw` binding and C++
overload are preserved; WebGPU's first-instance argument uses an internal
five-argument overload. Existing GodotSharp packages therefore keep their
method hashes. Buffer upload unmapping is a WebGPU driver trait, not a change
to Vulkan's staging-buffer lifetime.

Runtime fixes found with the 4.7 Mobile renderer include storage-buffer access
normalization, opaque texture-helper inlining before Tint, and Tint call-graph
reflection for per-stage resource visibility. Binding-array flattening uses
SPIRV-Tools' operand grammar so numeric ID collisions cannot rewrite literal
buffer offsets or binding numbers. Raw shadow-depth reads use explicit bilinear
texel loads on WebGPU; other renderers retain their original shader code.
Optional float32 features are queried by JS feature name across Dawn versions.

### Upstream PRs considered

- [#3](https://github.com/dwalter/godotwebgpu/pull/3): applied callback overloads
  for both Emdawnwebgpu callback generations.
- [#4](https://github.com/dwalter/godotwebgpu/pull/4): applied the matching Tint
  total-struct-size exemption; the patch is retained with Tint's other patches.
- [#8](https://github.com/dwalter/godotwebgpu/pull/8): removed the unused texture
  conflict scan from the draw hot path.
- [#10](https://github.com/dwalter/godotwebgpu/pull/10): applied the shell's
  storage-buffer limit fix. The Engine device helper already requests adapter
  limits; direct `Engine.init()` now uses that helper too.
- [#5](https://github.com/dwalter/godotwebgpu/pull/5) and
  [#7](https://github.com/dwalter/godotwebgpu/pull/7): WebGPU XR and repeated-eye
  shadow handling are deferred. Existing WebGL XR code is unchanged.
- [#6](https://github.com/dwalter/godotwebgpu/pull/6): optional boot-status UI is
  deferred.
- [#9](https://github.com/dwalter/godotwebgpu/pull/9): dynamic linking is optional;
  the .NET host still uses its existing static libgodot integration and custom
  GDExtension side-module loader.

### Building the spike

From the 2dog repository root, replace `<godot-checkout>` with this worktree:

```text
uv run scons -C <godot-checkout> platform=web target=template_debug library_type=static_library module_mono_enabled=yes disable_crash_handler=yes disable_path_overrides=no threads=no lto=none webgpu=yes webgpu_precompile=no
```

`webgpu_port=auto` selects the vendored Dawn port. An explicit local `.py` port
or `webgpu_port=emdawnwebgpu` can be supplied for other toolchains. The remote
port requires Emscripten 4.0.10 or later; do not change the .NET SDK pin to use
it. On Windows, use an Emscripten cache on the SDK's drive: this avoids the
3.1.56 system-library builder's cross-drive `os.path.relpath` failure.
Keep native SCons and .NET link caches separate. The .NET Emscripten workload's
default frozen cache cannot build the vendored port; set a writable
`WasmCachePath` for the host project.

`webgpu_precompile=no` leaves the optional build-time shader cache empty; the
linked Tint runtime still translates shaders. It is the default on Windows,
where the upstream cache-building helper requires POSIX tools. Other hosts can
enable the cache with host Tint and glslang installed.

A custom .NET browser shell must set `renderingDriver: 'webgpu'` on its Engine
configuration before `init()` or `startGame()`, and select the Mobile renderer.
Its final emcc link must also use the vendored Dawn port (and the bundled
Closure externs on Emscripten 3.1.56). The ordinary 2dog native packages/build
script do not enable or package this experimental payload automatically.

### Verification and remaining scope

- Windows x64 shared libgodot builds with the existing Vulkan and OpenGL paths.
- A separate .NET 10 host passes two lifetimes each in headless, Vulkan Mobile,
  Vulkan Forward+, and OpenGL Compatibility. Native errors/warnings fail the
  checks; all three graphical modes also read back distinct lit-cube pixels.
- Existing GodotSharp RenderingDevice initialization and GLSL-to-SPIR-V
  compilation pass against the Windows build without regenerating bindings.
- Imported driver tests: 327 passed. WGSL cache tests: 124 passed.
- New Engine boot tests: 4 passed. Emscripten 3.1.56 Tint parsing checks:
  20 passed. Existing 2dog web/Blazor JavaScript tests: 21 passed.
- A Dawn program compiles, links, and initializes with the pinned 3.1.56 SDK.
- Focused wasm regressions exercise literal-offset collisions, reachable
  resource reflection, nested opaque helpers, and storage access qualifiers.

The full single-threaded Mono WebGPU archive builds and links into a .NET 10
browser host using the existing 3.1.56 workload. A headless Chrome host renders
a lit cube in WebGPU Mobile and WebGL Compatibility, with no console errors or
GPU validation errors. Screenshots confirm distinct object/background pixels. No
cross-browser, WebGPU threading, WebGPU XR, Android, macOS, double-precision,
or release-package compatibility claim is made by these local checks. Promotion
to the main 2dog checkout requires a native revision bump, portable package
staging/link integration, repacking, and the normal platform CI checks.

For shader diagnostics, explicitly set `globalThis.webgpuDump = true` in the
browser shell to capture the last preprocessed shader in
`globalThis.webgpuLastSpirv`. This is disabled unless requested by the shell.

A `RenderingDeviceDriver` / `RenderingContextDriver` implementation targeting
WebGPU via Emscripten's **emdawnwebgpu** port (Dawn). This enables Godot's
Forward+ and Mobile renderers to run in the browser.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│  Godot RenderingDevice (servers/rendering/)             │
│    ↕ RenderingDeviceDriver interface                    │
├─────────────────────────────────────────────────────────┤
│  RenderingDeviceDriverWebGPU   (this driver)            │
│    • Buffers, Textures, Samplers, Pipelines, Draw calls │
│    • Push constant ring buffer emulation (group 3)      │
│    • Subpass flattening (each subpass → render pass)     │
│    • SPIR-V → WGSL translation (Tint, linked in)        │
├─────────────────────────────────────────────────────────┤
│  RenderingContextDriverWebGPU                           │
│    • Device import from JS pre-initialized GPUDevice    │
│    • Surface creation from HTML canvas (#canvas)        │
│    • Swap chain management via WGPUSurfaceTexture       │
├─────────────────────────────────────────────────────────┤
│  emdawnwebgpu (Emscripten port)                         │
│    • Dawn WebGPU C API → browser WebGPU JS API          │
└─────────────────────────────────────────────────────────┘
```

### Files

| File | Lines | Purpose |
|------|-------|---------|
| `rendering_device_driver_webgpu.cpp/h` | ~5250 | Main driver: buffers, textures, pipelines, draw, compute |
| `rendering_context_driver_webgpu.cpp/h` | ~290 | Device bootstrap, surface/swap chain management |
| `rendering_shader_container_webgpu.cpp/h` | ~210 | Shader container format (SPIR-V storage + Tint WGSL conversion) |
| `webgpu_objects.h` | ~320 | GPU object wrappers (WGBuffer, WGTexture, WGShader, etc.) |
| `spirv_preprocess.cpp/h` | ~1700 | SPIR-V preprocessing passes before Tint conversion |
| `tint_wrapper.cpp/h` | ~55 | C++20 isolation wrapper for Tint API |
| `pixel_formats_webgpu.h` | ~710 | Godot DataFormat → WGPUTextureFormat mapping table |

## Key Design Decisions

### Push Constant Emulation
WebGPU has no push constants. Emulated via a **256 KB ring buffer** (read-only
storage, binding 120 in group 3) with 256-byte aligned slots and dynamic
offsets. Each draw advances the ring offset; dirty-state tracking skips
unchanged data. Bind group created once and reused with dynamic offsets.

### Subpass Flattening
WebGPU has no subpasses. Each Godot subpass becomes a separate
`WGPURenderPassEncoder`. Attachment load/store ops are set per-pass based on
Godot's subpass configuration.

### Shader Translation
GLSL → SPIR-V (glslang, at build time) → WGSL (Tint, linked in as C++).
SPIR-V is preprocessed in C++ before Tint conversion: combined image-samplers
are split into separate texture + sampler bindings, push constant blocks are
rewritten to storage buffer references at binding 120, and various other
fixups (depth image flags, position Y negation, point size stripping) are
applied. Tint is compiled as a thirdparty C++20 library via a thin wrapper
(`tint_wrapper.cpp`) that isolates its C++20 headers from the Godot build.

### Barrier No-ops
WebGPU tracks resource hazards automatically. All barrier/sync commands are
no-ops.

### Buffer Mapping
WebGPU buffer mapping is asynchronous. Driver uses a **shadow buffer** pattern:
maintains a CPU-side copy, flushes to GPU via `wgpuQueueWriteBuffer()` on
unmap. Buffer reads use `wgpuBufferMapAsync` with callbacks.

### Bind Group Layout (BGL) Rebinding
When a shader's expected BGL doesn't match the uniform set's BGL (e.g., due to
specialization constant variants or merged push constant layouts), the driver
creates **adapted bind groups** on-the-fly using the shader's layout. A cache
prevents redundant re-creation.

## Known Limitations

- **Max 4 bind groups** (WebGPU spec) — Godot uses sets 0–3, with set 3 shared
  between material uniforms and push constant ring buffer.
- **No 3-component texture formats** — RGB8, RGB16F, RGB32F are unsupported as
  texture formats in WebGPU. The driver maps these to RGBA equivalents.
- **No multi-draw-indirect** — Each indirect draw is dispatched individually.
- **No subgroup operations** — `LIMIT_SUBGROUP_IN_SHADERS` reports 0.
- **Mobile renderer auto-selected** — Forward+ requires ≥48 sampled textures
  per stage; most WebGPU implementations report 16, so the mobile renderer is
  used automatically.
- **Timestamp queries** — Optional; depend on the `timestamp-query` device
  feature. Graceful fallback to dummy results when unavailable.
- **Synchronous readback** — Not available in WebGPU. Timestamp and buffer
  readbacks use async callbacks with shadow buffers.

## Build Instructions

```bash
# Prerequisites: Emscripten 5.x with emdawnwebgpu port
source /path/to/emsdk/emsdk_env.sh

# Build web template (debug, no threads, WebGPU only)
scons platform=web target=template_debug dlink_enabled=yes webgpu=yes opengl3=no threads=no -j$(nproc)

# Build web template (release)
scons platform=web target=template_release dlink_enabled=yes webgpu=yes opengl3=no threads=no -j$(nproc)

# Build with both WebGPU and WebGL2 support
scons platform=web target=template_debug dlink_enabled=yes webgpu=yes opengl3=yes threads=no -j$(nproc)

# Build macOS editor (does not include WebGPU driver, for reference)
scons platform=macos target=editor -j$(nproc)
```

The build flag `webgpu=yes` enables `WEBGPU_ENABLED` and adds
`--use-port=emdawnwebgpu` to both compile and link flags.

## Project Settings

The rendering driver is selected via project settings:

- `rendering/renderer/rendering_method.web` — `forward_plus`, `mobile`, or
  `gl_compatibility` (default)
- `rendering/rendering_device/driver.web` — `webgpu` (used when rendering
  method is `forward_plus` or `mobile`)

When `gl_compatibility` is selected, the existing WebGL 2.0 / GLES3 path is
used instead.

## Browser Compatibility

| Platform | Browser | Status |
|----------|---------|--------|
| macOS | Chrome 113+ | 100% — all demos and benchmarks pass |
| macOS | Safari 18+ | 100% — all demos and benchmarks pass |
| macOS | Firefox | 100% — all demos and benchmarks pass |
| Android | Chrome | 99% — minor edge cases |
| iOS | Safari | Mostly — some limitations |

The HTML export shell automatically detects WebGPU availability and shows a
clear error message if the browser doesn't support it.
