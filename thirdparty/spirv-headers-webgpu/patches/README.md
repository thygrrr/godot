# WebGPU header compatibility

`0001-preserve-webgpu-header-compatibility.patch` records the differences between
the pinned SPIRV-Headers revision and the existing WebGPU snapshot. That snapshot
combined older C/C++ headers with a newer C++11 header and additional extension
enumerants. The patch preserves those inputs during the move to upstream archive
references; it introduces no new compiler behavior. It also retains the snapshot's
trimmed license text.

The unpatched upstream grammar files generate SPIRV-Tools' tables. They are
independent of these header compatibility changes. The engine's existing
`thirdparty/spirv-headers` remains separate.
