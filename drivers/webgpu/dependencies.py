"""Prepare the checksum-pinned WebGPU sources without vendoring them in Git."""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[2]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def safe_path(name):
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts or "\\" in name or ":" in name:
        raise ValueError(f"Invalid dependency path: {name}")
    return path


def download(spec, cache, offline):
    archive = cache / (spec["sha256"] + (".zip" if spec["url"].endswith(".zip") else ".tar.gz"))
    if archive.is_file() and digest(archive.read_bytes()) == spec["sha256"]:
        return archive
    if offline:
        raise RuntimeError(f"Missing or corrupt cached WebGPU archive: {archive}. Run preparation online first.")
    print(f"Downloading WebGPU dependency: {spec['url']}", flush=True)
    with tempfile.NamedTemporaryFile(dir=cache, delete=False) as partial:
        partial_path = Path(partial.name)
        try:
            with urllib.request.urlopen(spec["url"], timeout=60) as source:
                shutil.copyfileobj(source, partial)
        except BaseException:
            partial.close()
            partial_path.unlink(missing_ok=True)
            raise
    try:
        if digest(partial_path.read_bytes()) != spec["sha256"]:
            raise ValueError(f"Checksum mismatch for {spec['url']}")
        partial_path.replace(archive)
    finally:
        partial_path.unlink(missing_ok=True)
    return archive


def extract(archive, destination, files=(), prefixes=()):
    """Copy regular files only, stripping the archive's single top directory."""
    wanted = set(files)
    found = set()

    def copy(name, read):
        safe_path(name)
        _, separator, relative = name.partition("/")
        if not separator or not relative:
            return
        if relative not in wanted and not any(relative.startswith(prefix) for prefix in prefixes):
            return
        output = destination / safe_path(relative)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(read().replace(b"\r\n", b"\n"))
        found.add(relative)

    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as source:
            for member in source.infolist():
                if not member.is_dir():
                    copy(member.filename, lambda: source.read(member))
    else:
        with tarfile.open(archive) as source:
            for member in source:
                if member.isfile():
                    copy(member.name, lambda: source.extractfile(member).read())
    if missing := wanted - found:
        raise ValueError(f"Files missing from {archive.name}: {sorted(missing)}")


def generate_spirv_tables(stage, archives, specs):
    tools = stage / "generators/spirv-tools"
    headers = stage / "generators/spirv-headers"
    extract(archives["spirv-tools"], tools, files=["CHANGES"], prefixes=["utils/"])
    extract(archives["spirv-headers-webgpu"], headers, prefixes=["include/"])
    grammar = headers / "include/spirv/unified1"
    output = stage / "thirdparty/spirv-tools/generated"
    output.mkdir(parents=True)
    args = [
        sys.executable,
        str(tools / "utils/ggt.py"),
        f"--core-tables-body-output={output}/core_tables_body.inc",
        f"--core-tables-header-output={output}/core_tables_header.inc",
        f"--spirv-core-grammar={grammar}/spirv.core.grammar.json",
    ]
    # Keep this list in sync with the pinned SPIRV-Tools source/CMakeLists.txt.
    for prefix, name in (
        ("", "glsl.std.450"),
        ("", "opencl.std.100"),
        ("CLDEBUG100_", "opencl.debuginfo.100"),
        ("SHDEBUG100_", "nonsemantic.shader.debuginfo"),
        ("", "spv-amd-shader-explicit-vertex-parameter"),
        ("", "spv-amd-shader-trinary-minmax"),
        ("", "spv-amd-gcn-shader"),
        ("", "spv-amd-shader-ballot"),
        ("", "debuginfo"),
        ("", "nonsemantic.clspvreflection"),
        ("", "nonsemantic.vkspreflection"),
        ("TOSA_", "tosa.001000.1"),
        ("", "arm.motion-engine.100"),
    ):
        args.append(f"--extinst={prefix},{grammar}/extinst.{name}.grammar.json")
    subprocess.run(args, check=True)
    subprocess.run(
        [
            sys.executable,
            str(tools / "utils/generate_registry_tables.py"),
            f"--xml={headers}/include/spirv/spir-v.xml",
            f"--generator-output={output}/generators.inc",
        ],
        check=True,
    )
    subprocess.run(
        [
            sys.executable,
            str(tools / "utils/update_build_version.py"),
            str(tools / "CHANGES"),
            str(output / "build-version.inc"),
        ],
        check=True,
        env={**os.environ, "FORCED_BUILD_VERSION_DESCRIPTION": specs["spirv-tools"]["revision"]},
    )


def prepare(root=ROOT, cache=None, offline=False, verify=False):
    root = Path(root).resolve()
    specs = json.loads((root / "drivers/webgpu/dependencies.json").read_text(encoding="utf-8"))
    cache = Path(cache or os.environ.get("WEBGPU_DEPS_CACHE", root / "thirdparty/.webgpu-cache")).resolve()
    pending = []
    fingerprints = {}
    states = {}
    for name, spec in specs.items():
        patches = sorted((root / "thirdparty" / name / "patches").glob("*.patch"))
        fingerprints[name] = digest(
            Path(__file__).read_bytes().replace(b"\r\n", b"\n")
            + json.dumps(spec, sort_keys=True).encode()
            + b"".join(p.name.encode() + p.read_bytes().replace(b"\r\n", b"\n") for p in patches)
        )
        directory = root / "thirdparty" / name
        stamp = directory / ".webgpu-source.json"
        state = json.loads(stamp.read_text(encoding="utf-8")) if stamp.is_file() else {}
        states[name] = state
        if state.get("fingerprint") != fingerprints[name] or any(
            not (directory / file).is_file() for file in spec["files"]
        ):
            pending.append(name)
        elif verify:
            for file, expected in state["files"].items():
                if digest((directory / file).read_bytes().replace(b"\r\n", b"\n")) != expected:
                    raise ValueError(f"Locally modified WebGPU dependency: {directory / file}")
    if verify and pending:
        raise RuntimeError(f"WebGPU sources need preparation: {', '.join(pending)}")
    if not pending:
        return
    cache.mkdir(parents=True, exist_ok=True)
    needed = set(pending)
    if "spirv-tools" in needed:
        needed.add("spirv-headers-webgpu")
    archives = {name: download(specs[name], cache, offline) for name in sorted(needed)}
    with tempfile.TemporaryDirectory(prefix="prepare-", dir=cache) as directory:
        stage = Path(directory)
        for name in pending:
            files = [
                file
                for file in specs[name]["files"]
                if not file.startswith("generated/") and file != "webgpu/src/library_webgpu_legacy.js"
            ]
            extract(archives[name], stage / "thirdparty" / name, files=files)
        if "spirv-tools" in pending:
            generate_spirv_tables(stage, archives, specs)
        if "emdawnwebgpu" in pending:
            source = stage / "thirdparty/emdawnwebgpu/webgpu/src/library_webgpu.js"
            source.with_name("library_webgpu_legacy.js").write_bytes(
                source.read_bytes().replace(b"'$stackSave', '$stackRestore'", b"'stackSave', 'stackRestore'")
            )
        for name in pending:
            for patch in sorted((root / "thirdparty" / name / "patches").glob("*.patch")):
                # Prevent Git from discovering the surrounding Godot/2dog worktree.
                subprocess.run(
                    ["git", "apply", "--whitespace=nowarn", "-"],
                    cwd=stage,
                    check=True,
                    input=patch.read_bytes().replace(b"\r\n", b"\n"),
                    env={**os.environ, "GIT_CEILING_DIRECTORIES": str(stage.parent)},
                )
        for name in pending:
            hashes = {}
            for file, previous in states[name].get("files", {}).items():
                if file in specs[name]["files"]:
                    continue
                obsolete = root / "thirdparty" / name / safe_path(file)
                if obsolete.is_file():
                    if digest(obsolete.read_bytes().replace(b"\r\n", b"\n")) != previous:
                        raise ValueError(f"Locally modified WebGPU dependency: {obsolete}; preserve changes first")
                    obsolete.unlink()
            for file in specs[name]["files"]:
                data = (stage / "thirdparty" / name / file).read_bytes().replace(b"\r\n", b"\n")
                output = root / "thirdparty" / name / safe_path(file)
                output.parent.mkdir(parents=True, exist_ok=True)
                existing = output.read_bytes().replace(b"\r\n", b"\n") if output.is_file() else None
                previous = states[name].get("files", {}).get(file)
                if existing is not None and previous and digest(existing) != previous and existing != data:
                    raise ValueError(f"Locally modified WebGPU dependency: {output}; preserve changes as a patch first")
                if existing != data:
                    output.write_bytes(data)
                hashes[file] = digest(data)
            stamp = root / "thirdparty" / name / ".webgpu-source.json"
            stamp.write_text(
                json.dumps({"fingerprint": fingerprints[name], "files": hashes}, indent=2) + "\n", encoding="utf-8"
            )
            print(f"Prepared WebGPU dependency: {name} ({specs[name]['revision']})", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, help="Archive cache (or WEBGPU_DEPS_CACHE)")
    parser.add_argument("--offline", action="store_true", help="Use cached archives only")
    parser.add_argument("--verify", action="store_true", help="Check prepared files without writing or downloading")
    args = parser.parse_args()
    prepare(cache=args.cache, offline=args.offline, verify=args.verify)
