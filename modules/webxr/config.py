def can_build(env, platform):
    if platform == "web" and env["proxy_to_pthread"]:
        # WebXR is incompatible with proxy_to_pthread.
        return False

    # 2dog: library builds (any flavor) link into the .NET runtime pack, whose
    # emscripten (3.1.56) lacks the $MainLoop JS library webxr needs.
    # NOTE: Remove when the .NET emscripten updates from 3.1.56.
    if platform == "web" and (env["module_mono_enabled"] or env["library_type"] != "executable"):
        return False

    return env["opengl3"] and not env["disable_xr"]


def configure(env):
    pass


def get_doc_classes():
    return ["WebXRInterface"]


def get_doc_path():
    return "doc_classes"
