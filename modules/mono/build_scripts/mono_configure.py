def is_desktop(platform):
    return platform in ["windows", "macos", "linuxbsd"]


def is_unix_like(platform):
    return platform in ["macos", "linuxbsd", "android", "ios"]


def module_supports_tools_on(platform):
    return is_desktop(platform)


def configure(env, env_mono):
    # is_android = env["platform"] == "android"
    # is_web = env["platform"] == "web"
    # is_ios = env["platform"] == "ios"
    # is_ios_sim = is_ios and env["arch"] in ["x86_32", "x86_64"]

    if env.editor_build:
        if not module_supports_tools_on(env["platform"]):
            raise RuntimeError("This module does not currently support building for this platform for editor builds.")
        env_mono.Append(CPPDEFINES=["GD_MONO_HOT_RELOAD"])

# Enable system hostfxr discovery for shared library builds (libgodot).
    # This allows libgodot to use the host application's .NET runtime via hostfxr
    # instead of trying to load a bundled coreclr (which would conflict).
    if env.get("library_type", "") == "shared_library":
        env_mono.Append(CPPDEFINES=["LIBGODOT_HOSTFXR"])

    # Any library build (shared or static) may have the host register the
    # GodotPlugins initialize function pointer directly (set_load_from_executable_fn)
    # instead of loading GodotPlugins.dll from disk. Required on web, optional elsewhere.
    if env.get("library_type", "executable") != "executable":
        env_mono.AppendUnique(CPPDEFINES=["GD_MONO_LIBGODOT_ENABLED"])
