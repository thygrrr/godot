def can_build(env, platform):
    if platform == "web" and env["proxy_to_pthread"]:
        # WebXR is incompatible with proxy_to_pthread.
        return False

    # 2dog: .NET's Emscripten 3.1.56 lacks WebXR's $MainLoop dependency.
    # Remove this when .NET updates Emscripten.
    if platform == "web" and (env["module_mono_enabled"] or env["library_type"] != "executable"):
        return False

    return env["opengl3"] and not env["disable_xr"]


def configure(env):
    pass


def get_doc_classes():
    return ["WebXRInterface"]


def get_doc_path():
    return "doc_classes"
