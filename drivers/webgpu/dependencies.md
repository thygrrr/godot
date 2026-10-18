# WebGPU dependency references

Tint (from Dawn), SPIRV-Tools, SPIRV-Headers and Emdawnwebgpu are pinned by
upstream revision, archive URL, SHA-256 and imported file list in
[`dependencies.json`](dependencies.json). Their source files and generated tables
are ignored by Git. Local compatibility patches remain in `thirdparty/*/patches`.
Native Dawn already uses the separately pinned SDK described in
[`native/README.md`](native/README.md).

SCons prepares the sources when `webgpu=yes`, before configuring the platform.
The host Tint CLI prepares them before compiling. Builds with WebGPU disabled
require no dependency download. Preparation can also run explicitly from this
Godot checkout:

```text
uv run --no-project python drivers/webgpu/dependencies.py
uv run --no-project python drivers/webgpu/dependencies.py --verify
```

Preparation verifies each archive, extracts only the locked files, regenerates
SPIRV-Tools' tables using its upstream Python generators, and applies the sorted
patch stack with `git apply`. The legacy Emscripten JavaScript is derived from
the original release. The initial conversion reproduces all 1,234 previously
tracked dependency files, including generated tables and local modifications.

`thirdparty/.webgpu-cache` caches archives by digest. Set `WEBGPU_DEPS_CACHE` or
pass `--cache <directory>` to share a cache between worktrees or prepopulate a
CI machine. Once the four archives have been cached, an offline checkout can use:

```text
uv run --no-project python drivers/webgpu/dependencies.py --offline --cache <directory>
```

Prepared checkouts build without network access. `--verify` checks fingerprints
and every prepared file's hash without downloading or changing files. Normal
preparation preserves timestamps for unchanged sources and refuses to replace
locally modified files recorded by an earlier preparation. Save changes as patch
files before changing the pins or switching between different patch stacks.

To update a dependency, change its revision, URL, checksum and file list in the
lock file, rebase its patches, prepare, and verify. Tint's SPIRV-Tools and header
revisions should follow Dawn's `DEPS`. Keep the table-generator grammar list in
`dependencies.py` aligned with the pinned SPIRV-Tools `source/CMakeLists.txt`.
Update `thirdparty/README.md` and rerun the WebGPU compiler and renderer checks
when changing dependency behavior.

This uses archive references instead of submodules so the patched, trimmed Tint
sources do not require a separately maintained Dawn fork. Existing Git history
still contains the earlier vendored snapshots; neither WebGPU branch is rewritten.
